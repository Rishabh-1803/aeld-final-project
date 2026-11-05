// SPDX-License-Identifier: GPL-2.0-only

#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/input.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DRIVER_NAME "hid_morse_kbd"

struct morse_input_state {
    struct input_handle handle;
    atomic64_t button_events;
    bool grabbed;
};

/*
 * QEMU development selector.
 *
 * The capability table below ensures that this name match is not sufficient
 * by itself: the device must also expose the required mouse buttons and
 * absolute pointer axes.
 *
 * Physical receiver matching will be refined during the final hardware stage.
 */
static char target_name[64] = "USB Tablet";

module_param_string(target_name, target_name, sizeof(target_name), 0444);
MODULE_PARM_DESC(target_name,
                 "Required substring in the input device name");

/*
 * Optional exclusive grab.
 *
 * Disabled by default. When enabled, this handler receives events exclusively
 * from the selected device. Disconnect or module unload releases the grab.
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
 * This capability filter excludes the ordinary QEMU keyboard and the
 * MorseMouse virtual keyboard.
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

/*
 * The name check is an additional QEMU-specific selector.
 * Capability matching is performed first by the input subsystem.
 */
static bool morse_match(struct input_handler *handler,
                        struct input_dev *dev)
{
    (void)handler;

    if (!dev->name || !target_name[0])
        return false;

    return strstr(dev->name, target_name) != NULL;
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
        return NULL;
    }
}

/*
 * Sprint 2 / Issue #5:
 * Receive and report raw button events.
 *
 * Morse decoding and bounded event processing are deliberately implemented
 * in the next stage, under issue #6.
 */
static void morse_event(struct input_handle *handle,
                        unsigned int type,
                        unsigned int code,
                        int value)
{
    struct morse_input_state *state = handle->private;
    const char *button;
    const char *action;
    s64 event_count;

    if (type != EV_KEY)
        return;

    button = morse_button_name(code);
    if (!button)
        return;

    switch (value) {
    case 0:
        action = "RELEASE";
        break;

    case 1:
        action = "PRESS";
        break;

    case 2:
        action = "REPEAT";
        break;

    default:
        action = "UNKNOWN";
        break;
    }

    event_count = atomic64_inc_return(&state->button_events);

    pr_info_ratelimited(
        DRIVER_NAME ": device=\"%s\" button=%s action=%s value=%d count=%lld\n",
        handle->dev->name ? handle->dev->name : "unknown",
        button,
        action,
        value,
        (long long)event_count);
}

/*
 * Called when a matching input device becomes available.
 *
 * Registration, opening and optional grabbing are performed in this order.
 * Every failure path reverses the operations already completed.
 */
static int morse_connect(struct input_handler *handler,
                         struct input_dev *dev,
                         const struct input_device_id *id)
{
    struct morse_input_state *state;
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

    atomic64_set(&state->button_events, 0);
    state->grabbed = false;

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
        DRIVER_NAME ": connected to \"%s\"; grab=%s\n",
        dev->name ? dev->name : "unknown",
        state->grabbed ? "enabled" : "disabled");

    return 0;

err_close_device:
    input_close_device(&state->handle);

err_unregister_handle:
    input_unregister_handle(&state->handle);

err_put_device:
    input_put_device(state->handle.dev);

err_free_state:
    kfree(state);
    return error;
}

/*
 * Called when the selected input device disconnects or the handler unloads.
 *
 * Release the exclusive grab before closing the handle. Unregister the handle
 * before dropping the retained input-device reference.
 */
static void morse_disconnect(struct input_handle *handle)
{
    struct morse_input_state *state = handle->private;
    const char *name = handle->dev->name ?
                       handle->dev->name : "unknown";
    s64 total_events = atomic64_read(&state->button_events);

    if (state->grabbed) {
        input_release_device(handle);
        state->grabbed = false;
    }

    input_close_device(handle);
    input_unregister_handle(handle);

    pr_info(
        DRIVER_NAME ": disconnected from \"%s\"; received_events=%lld\n",
        name,
        (long long)total_events);

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
        DRIVER_NAME ": registered; target_name=\"%s\", grab_on_bind=%s\n",
        target_name,
        grab_on_bind ? "true" : "false");

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
MODULE_DESCRIPTION("MorseMouse QEMU input handler");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
