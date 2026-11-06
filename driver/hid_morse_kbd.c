/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/workqueue.h>

#include "morse_fsm_core.h"

#define DRIVER_NAME "hid_morse_kbd"

#define MORSE_EVENT_QUEUE_CAPACITY 64
#define MORSE_MAX_SEQUENCE_LEN 63

struct morse_map_entry {
    const char *sequence;
    u8 ascii;
};

#include "morse_map_kernel.h"

struct morse_queued_event {
    u64 timestamp_ns;
    u16 code;
    s32 value;
};

struct morse_input_state {
    struct input_handle handle;

    /*
     * The input callback only updates this bounded queue.
     * All FSM and sequence processing happens in event_work.
     */
    spinlock_t queue_lock;
    struct work_struct event_work;

    struct morse_queued_event queue[MORSE_EVENT_QUEUE_CAPACITY];
    unsigned int queue_head;
    unsigned int queue_tail;
    unsigned int queue_count;

    bool stopping;
    bool overflow_pending;

    /*
     * Physical state shadow, maintained by the input callback under
     * queue_lock. It allows deterministic recovery after queue overflow.
     */
    bool raw_left_pressed;
    bool raw_right_pressed;
    bool raw_middle_pressed;

    struct morse_fsm_core fsm;

    char sequence[MORSE_MAX_SEQUENCE_LEN + 1];
    unsigned int sequence_len;

    atomic64_t received_events;
    atomic64_t processed_events;
    atomic64_t queue_overflows;
    atomic64_t dropped_events;
    atomic64_t sequence_overflows;
    atomic64_t valid_commits;
    atomic64_t empty_commits;
    atomic64_t invalid_commits;

    bool grabbed;
};

/*
 * QEMU development selector.
 *
 * The name match is combined with the event capability table below.
 * Physical receiver matching will be refined during the final hardware stage.
 */
static char target_name[64] = "USB Tablet";

module_param_string(target_name, target_name,
                    sizeof(target_name), 0444);

MODULE_PARM_DESC(target_name,
                 "Required substring in the input device name");

/*
 * Exclusive grabbing is optional and disabled by default.
 */
static bool grab_on_bind;

module_param(grab_on_bind, bool, 0444);

MODULE_PARM_DESC(grab_on_bind,
                 "Grab the selected input device when the handler connects");

/*
 * Match:
 *   EV_KEY: BTN_LEFT, BTN_RIGHT, BTN_MIDDLE
 *   EV_ABS: ABS_X, ABS_Y
 *
 * This excludes the ordinary QEMU keyboard and the future virtual keyboard.
 */
static const struct input_device_id morse_input_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_KEYBIT |
                 INPUT_DEVICE_ID_MATCH_ABSBIT,

        .evbit = {
            [BIT_WORD(EV_KEY)] = BIT_MASK(EV_KEY),
            [BIT_WORD(EV_ABS)] = BIT_MASK(EV_ABS),
        },

        .keybit = {
            [BIT_WORD(BTN_LEFT)] =
                BIT_MASK(BTN_LEFT) |
                BIT_MASK(BTN_RIGHT) |
                BIT_MASK(BTN_MIDDLE),
        },

        .absbit = {
            [BIT_WORD(ABS_X)] =
                BIT_MASK(ABS_X) |
                BIT_MASK(ABS_Y),
        },
    },
    { }
};

MODULE_DEVICE_TABLE(input, morse_input_ids);


static bool morse_match(struct input_handler *handler,
                        struct input_dev *dev)
{
    (void)handler;

    if (!dev->name || !target_name[0])
        return false;

    return strstr(dev->name, target_name) != NULL;
}


static bool morse_is_button(unsigned int code)
{
    return code == BTN_LEFT ||
           code == BTN_RIGHT ||
           code == BTN_MIDDLE;
}


static const char *morse_button_name(unsigned int code)
{
    switch (code) {
    case BTN_LEFT:
        return "LEFT";

    case BTN_RIGHT:
        return "RIGHT";

    case BTN_MIDDLE:
        return "MIDDLE";

    default:
        return "UNKNOWN";
    }
}


static void morse_sequence_reset(struct morse_input_state *state)
{
    state->sequence_len = 0;
    state->sequence[0] = '\0';
}


static bool morse_map_lookup(const char *sequence, u8 *ascii)
{
    unsigned int i;

    for (i = 0; i < MORSE_MAP_KERNEL_COUNT; i++) {
        if (strcmp(morse_map_kernel[i].sequence, sequence) == 0) {
            *ascii = morse_map_kernel[i].ascii;
            return true;
        }
    }

    return false;
}


static void morse_append_symbol(struct morse_input_state *state,
                                char symbol,
                                const struct morse_queued_event *event)
{
    if (state->sequence_len >= MORSE_MAX_SEQUENCE_LEN) {
        morse_sequence_reset(state);
        atomic64_inc(&state->sequence_overflows);

        pr_warn_ratelimited(
            DRIVER_NAME ": sequence buffer full; current sequence reset\n");
        return;
    }

    state->sequence[state->sequence_len++] = symbol;
    state->sequence[state->sequence_len] = '\0';

    pr_info_ratelimited(
        DRIVER_NAME ": FSM action=%s sequence=%s event_ns=%llu\n",
        symbol == '.' ? "DOT" : "DASH",
        state->sequence,
        (unsigned long long)event->timestamp_ns);
}


static void morse_commit_sequence(struct morse_input_state *state,
                                  const struct morse_queued_event *event)
{
    u8 ascii;
    char committed[MORSE_MAX_SEQUENCE_LEN + 1];

    if (state->sequence_len == 0) {
        atomic64_inc(&state->empty_commits);

        pr_info(
            DRIVER_NAME ": FSM COMMIT empty; ignored event_ns=%llu\n",
            (unsigned long long)event->timestamp_ns);

        return;
    }

    strscpy(committed, state->sequence, sizeof(committed));

    /*
     * Every COMMIT consumes the sequence, whether valid or invalid.
     */
    morse_sequence_reset(state);

    if (morse_map_lookup(committed, &ascii)) {
        atomic64_inc(&state->valid_commits);

        pr_info(
            DRIVER_NAME
            ": FSM COMMIT valid sequence=%s ascii=%u hex=0x%02x\n",
            committed,
            (unsigned int)ascii,
            (unsigned int)ascii);
    } else {
        atomic64_inc(&state->invalid_commits);

        pr_info(
            DRIVER_NAME
            ": FSM COMMIT invalid sequence=%s; consumed\n",
            committed);
    }
}


static void morse_process_event(struct morse_input_state *state,
                                const struct morse_queued_event *event)
{
    enum morse_fsm_core_button button;
    enum morse_fsm_core_action action;

    switch (event->code) {
    case BTN_LEFT:
        button = MORSE_CORE_BUTTON_LEFT;
        break;

    case BTN_RIGHT:
        button = MORSE_CORE_BUTTON_RIGHT;
        break;

    case BTN_MIDDLE:
        button = MORSE_CORE_BUTTON_MIDDLE;
        break;

    default:
        return;
    }

    action = morse_fsm_core_handle(&state->fsm,
                                   button,
                                   event->value);

    switch (action) {
    case MORSE_CORE_ACTION_APPEND_DOT:
        morse_append_symbol(state, '.', event);
        break;

    case MORSE_CORE_ACTION_APPEND_DASH:
        morse_append_symbol(state, '-', event);
        break;

    case MORSE_CORE_ACTION_COMMIT:
        morse_commit_sequence(state, event);
        break;

    case MORSE_CORE_ACTION_CANCEL:
        morse_sequence_reset(state);

        pr_info(
            DRIVER_NAME
            ": FSM chord cancellation; sequence cleared; lockout=1\n");
        break;

    case MORSE_CORE_ACTION_NONE:
        break;

    case MORSE_CORE_ACTION_IGNORE:
    default:
        break;
    }

    atomic64_inc(&state->processed_events);
}


/*
 * Deferred event processor.
 *
 * The worker is the only context allowed to modify:
 *   - FSM state
 *   - Morse sequence buffer
 *   - decoded commit counters
 *
 * The input callback never sleeps and never performs Morse decoding.
 */
static void morse_event_worker(struct work_struct *work)
{
    struct morse_input_state *state =
        container_of(work, struct morse_input_state, event_work);

    for (;;) {
        struct morse_queued_event event;
        unsigned long flags;
        bool have_event = false;
        bool do_resync = false;
        bool left_pressed = false;
        bool right_pressed = false;
        bool middle_pressed = false;
        unsigned int discarded = 0;

        spin_lock_irqsave(&state->queue_lock, flags);

        if (state->stopping) {
            state->queue_head = 0;
            state->queue_tail = 0;
            state->queue_count = 0;
            spin_unlock_irqrestore(&state->queue_lock, flags);
            return;
        }

        if (state->overflow_pending) {
            /*
             * Discard events accumulated around the overflow.
             * The callback-maintained button shadow is an exact snapshot
             * of all relevant button transitions observed up to this lock.
             */
            discarded = state->queue_count;

            state->queue_head = 0;
            state->queue_tail = 0;
            state->queue_count = 0;
            state->overflow_pending = false;

            left_pressed = state->raw_left_pressed;
            right_pressed = state->raw_right_pressed;
            middle_pressed = state->raw_middle_pressed;

            do_resync = true;
        } else if (state->queue_count != 0) {
            event = state->queue[state->queue_tail];

            state->queue_tail++;
            if (state->queue_tail == MORSE_EVENT_QUEUE_CAPACITY)
                state->queue_tail = 0;

            state->queue_count--;
            have_event = true;
        }

        spin_unlock_irqrestore(&state->queue_lock, flags);

        if (do_resync) {
            if (discarded)
                atomic64_add(discarded, &state->dropped_events);

            /*
             * Overflow invalidates the sequence and previous transition
             * history. Re-enter the reference FSM's fail-safe resync path.
             */
            morse_sequence_reset(state);

            morse_fsm_core_resync_begin(&state->fsm);

            morse_fsm_core_sync_buttons(&state->fsm,
                                        left_pressed,
                                        right_pressed,
                                        middle_pressed);

            pr_warn_ratelimited(
                DRIVER_NAME
                ": queue overflow; sequence reset; dropped=%llu "
                "left=%d right=%d middle=%d lockout=%d\n",
                (unsigned long long)atomic64_read(
                    &state->dropped_events),
                left_pressed,
                right_pressed,
                middle_pressed,
                morse_fsm_core_is_locked_out(&state->fsm));

            continue;
        }

        if (!have_event)
            return;

        morse_process_event(state, &event);
    }
}


/*
 * Non-sleeping input callback.
 *
 * It only:
 *   1. filters the three supported button codes;
 *   2. updates the physical button shadow;
 *   3. appends a timestamped event to the bounded queue;
 *   4. schedules deferred processing.
 */
static void morse_event(struct input_handle *handle,
                        unsigned int type,
                        unsigned int code,
                        int value)
{
    struct morse_input_state *state = handle->private;
    struct morse_queued_event *event;
    unsigned long flags;

    if (type != EV_KEY || !morse_is_button(code))
        return;

    spin_lock_irqsave(&state->queue_lock, flags);

    if (state->stopping) {
        spin_unlock_irqrestore(&state->queue_lock, flags);
        return;
    }

    if (code == BTN_LEFT) {
        if (value == 1)
            state->raw_left_pressed = true;
        else if (value == 0)
            state->raw_left_pressed = false;
    } else if (code == BTN_RIGHT) {
        if (value == 1)
            state->raw_right_pressed = true;
        else if (value == 0)
            state->raw_right_pressed = false;
    } else if (code == BTN_MIDDLE) {
        if (value == 1)
            state->raw_middle_pressed = true;
        else if (value == 0)
            state->raw_middle_pressed = false;
    }

    atomic64_inc(&state->received_events);

    if (state->queue_count >= MORSE_EVENT_QUEUE_CAPACITY) {
        /*
         * Explicit overflow policy:
         *   - drop the entire queued history;
         *   - drop the event that encountered the full queue;
         *   - request worker-side FSM reset and state resynchronization.
         */
        atomic64_add(state->queue_count + 1,
                     &state->dropped_events);

        state->queue_head = 0;
        state->queue_tail = 0;
        state->queue_count = 0;
        state->overflow_pending = true;

        atomic64_inc(&state->queue_overflows);
    } else {
        event = &state->queue[state->queue_head];

        event->timestamp_ns = ktime_get_ns();
        event->code = code;
        event->value = value;

        state->queue_head++;
        if (state->queue_head == MORSE_EVENT_QUEUE_CAPACITY)
            state->queue_head = 0;

        state->queue_count++;
    }

    spin_unlock_irqrestore(&state->queue_lock, flags);

    schedule_work(&state->event_work);
}


static void morse_stop_queue(struct morse_input_state *state)
{
    unsigned long flags;

    spin_lock_irqsave(&state->queue_lock, flags);

    state->stopping = true;
    state->queue_head = 0;
    state->queue_tail = 0;
    state->queue_count = 0;
    state->overflow_pending = false;

    spin_unlock_irqrestore(&state->queue_lock, flags);
}


/*
 * Matching input device becomes available.
 */
static int morse_connect(struct input_handler *handler,
                         struct input_dev *dev,
                         const struct input_device_id *id)
{
    struct morse_input_state *state;
    unsigned long flags;
    int error;

    (void)id;

    state = kzalloc(sizeof(*state), GFP_KERNEL);
    if (!state)
        return -ENOMEM;

    state->handle.dev = input_get_device(dev);
    if (!state->handle.dev) {
        error = -ENODEV;
        goto err_free_state;
    }

    state->handle.handler = handler;
    state->handle.name = DRIVER_NAME;
    state->handle.private = state;

    spin_lock_init(&state->queue_lock);
    INIT_WORK(&state->event_work, morse_event_worker);

    morse_fsm_core_init(&state->fsm);
    morse_sequence_reset(state);

    atomic64_set(&state->received_events, 0);
    atomic64_set(&state->processed_events, 0);
    atomic64_set(&state->queue_overflows, 0);
    atomic64_set(&state->dropped_events, 0);
    atomic64_set(&state->sequence_overflows, 0);
    atomic64_set(&state->valid_commits, 0);
    atomic64_set(&state->empty_commits, 0);
    atomic64_set(&state->invalid_commits, 0);

    state->grabbed = false;
    state->stopping = false;

    /*
     * Seed the callback's physical-state shadow from the input device's
     * current key bitmap before opening the handler.
     */
    spin_lock_irqsave(&dev->event_lock, flags);
    state->raw_left_pressed = test_bit(BTN_LEFT, dev->key);
    state->raw_right_pressed = test_bit(BTN_RIGHT, dev->key);
    state->raw_middle_pressed = test_bit(BTN_MIDDLE, dev->key);
    spin_unlock_irqrestore(&dev->event_lock, flags);

    error = input_register_handle(&state->handle);
    if (error) {
        pr_err(DRIVER_NAME ": input_register_handle failed: %d\n",
               error);
        goto err_put_device;
    }

    error = input_open_device(&state->handle);
    if (error) {
        pr_err(DRIVER_NAME ": input_open_device failed: %d\n",
               error);
        goto err_unregister_handle;
    }

    if (grab_on_bind) {
        error = input_grab_device(&state->handle);
        if (error) {
            pr_err(DRIVER_NAME ": input_grab_device failed: %d\n",
                   error);
            goto err_close_device;
        }

        state->grabbed = true;
    }

    pr_info(
        DRIVER_NAME
        ": connected to \"%s\"; grab=%s queue_capacity=%u\n",
        dev->name ? dev->name : "unknown",
        state->grabbed ? "enabled" : "disabled",
        MORSE_EVENT_QUEUE_CAPACITY);

    return 0;

err_close_device:
    morse_stop_queue(state);
    input_close_device(&state->handle);

err_unregister_handle:
    morse_stop_queue(state);
    input_unregister_handle(&state->handle);
    cancel_work_sync(&state->event_work);

err_put_device:
    input_put_device(state->handle.dev);

err_free_state:
    kfree(state);
    return error;
}


/*
 * Disconnect, USB removal or module unload.
 */
static void morse_disconnect(struct input_handle *handle)
{
    struct morse_input_state *state = handle->private;
    const char *name = handle->dev->name ?
                       handle->dev->name : "unknown";

    morse_stop_queue(state);

    if (state->grabbed) {
        input_release_device(handle);
        state->grabbed = false;
    }

    input_close_device(handle);
    input_unregister_handle(handle);

    /*
     * Wait for deferred processing to finish before freeing state.
     */
    cancel_work_sync(&state->event_work);

    pr_info(
        DRIVER_NAME
        ": disconnected from \"%s\""
        " received=%lld processed=%lld"
        " queue_overflows=%lld dropped=%lld"
        " sequence_overflows=%lld"
        " commits_valid=%lld commits_empty=%lld commits_invalid=%lld\n",
        name,
        (long long)atomic64_read(&state->received_events),
        (long long)atomic64_read(&state->processed_events),
        (long long)atomic64_read(&state->queue_overflows),
        (long long)atomic64_read(&state->dropped_events),
        (long long)atomic64_read(&state->sequence_overflows),
        (long long)atomic64_read(&state->valid_commits),
        (long long)atomic64_read(&state->empty_commits),
        (long long)atomic64_read(&state->invalid_commits));

    input_put_device(handle->dev);
    kfree(state);
}


static struct input_handler morse_handler = {
    .event      = morse_event,
    .match      = morse_match,
    .connect    = morse_connect,
    .disconnect = morse_disconnect,
    .name       = DRIVER_NAME,
    .id_table   = morse_input_ids,
};


static int __init morse_init(void)
{
    int error;

    error = input_register_handler(&morse_handler);
    if (error) {
        pr_err(DRIVER_NAME ": handler registration failed: %d\n",
               error);
        return error;
    }

    pr_info(
        DRIVER_NAME
        ": registered; target_name=\"%s\", grab_on_bind=%s "
        "queue_capacity=%u sequence_capacity=%u map_entries=%u\n",
        target_name,
        grab_on_bind ? "true" : "false",
        MORSE_EVENT_QUEUE_CAPACITY,
        MORSE_MAX_SEQUENCE_LEN,
        (unsigned int)MORSE_MAP_KERNEL_COUNT);

    return 0;
}


static void __exit morse_exit(void)
{
    input_unregister_handler(&morse_handler);
    pr_info(DRIVER_NAME ": unloaded\n");
}


module_init(morse_init);
module_exit(morse_exit);

MODULE_AUTHOR("MorseMouse Project");
MODULE_DESCRIPTION("MorseMouse QEMU input handler and kernel FSM");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
