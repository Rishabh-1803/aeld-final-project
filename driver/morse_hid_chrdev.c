/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/atomic.h>
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "morse_hid_chrdev.h"

#define DRIVER_NAME "hid_morse_kbd"

/* ------------------------------------------------------------------------- */
/* Diagnostic event ring                                                    */
/* ------------------------------------------------------------------------- */

static struct morse_hid_event diagnostic_ring[
    MORSE_HID_DIAG_QUEUE_CAPACITY
];

static DEFINE_MUTEX(diagnostic_lock);
static DEFINE_MUTEX(read_lock);

static DECLARE_WAIT_QUEUE_HEAD(diagnostic_wait_queue);

static u32 ring_head;
static u32 ring_tail;
static u32 ring_count;

static bool stream_gap_pending;
static bool device_stopping;
static bool device_registered;

static atomic_t reader_open = ATOMIC_INIT(0);

/* ------------------------------------------------------------------------- */
/* Event and diagnostic statistics                                           */
/* ------------------------------------------------------------------------- */

static atomic64_t next_event_id = ATOMIC64_INIT(0);

static atomic64_t input_events_received = ATOMIC64_INIT(0);
static atomic64_t input_events_processed = ATOMIC64_INIT(0);

static atomic64_t input_queue_overflows = ATOMIC64_INIT(0);
static atomic64_t input_events_dropped = ATOMIC64_INIT(0);

static atomic64_t sequence_overflows = ATOMIC64_INIT(0);

static atomic64_t valid_commits = ATOMIC64_INIT(0);
static atomic64_t empty_commits = ATOMIC64_INIT(0);
static atomic64_t invalid_commits = ATOMIC64_INIT(0);

static atomic64_t diagnostic_queue_overflows = ATOMIC64_INIT(0);
static atomic64_t diagnostic_events_dropped = ATOMIC64_INIT(0);

static atomic_t connected_devices = ATOMIC_INIT(0);
static atomic_t current_input_queue_depth = ATOMIC_INIT(0);

/* ------------------------------------------------------------------------- */
/* Device registration                                                      */
/* ------------------------------------------------------------------------- */

static const struct file_operations morse_hid_fops;

static struct miscdevice morse_hid_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MORSE_HID_DEVICE_NAME,
    .fops = &morse_hid_fops,
    .mode = 0600,
};

/* ------------------------------------------------------------------------- */
/* Diagnostic event publishing                                              */
/* ------------------------------------------------------------------------- */

void morse_hid_chrdev_push_event(
    u32 type,
    u64 timestamp_ns,
    s32 value,
    u32 ascii,
    const char *sequence,
    u32 sequence_len)
{
    struct morse_hid_event *event;

    if (!READ_ONCE(device_registered) ||
        READ_ONCE(device_stopping))
        return;

    mutex_lock(&diagnostic_lock);

    if (!READ_ONCE(device_registered) ||
        READ_ONCE(device_stopping))
        goto out_unlock;

    /*
     * Full ring policy:
     *   1. Discard the oldest record.
     *   2. Increment loss counters.
     *   3. Mark the next inserted record with STREAM_GAP.
     */
    if (ring_count == MORSE_HID_DIAG_QUEUE_CAPACITY) {
        ring_tail++;
        if (ring_tail == MORSE_HID_DIAG_QUEUE_CAPACITY)
            ring_tail = 0;

        WRITE_ONCE(ring_count, ring_count - 1);

        atomic64_inc(&diagnostic_queue_overflows);
        atomic64_inc(&diagnostic_events_dropped);

        stream_gap_pending = true;
    }

    event = &diagnostic_ring[ring_head];
    memset(event, 0, sizeof(*event));

    event->event_id = atomic64_inc_return(&next_event_id);
    event->timestamp_ns = timestamp_ns ?
                          timestamp_ns : ktime_get_ns();

    event->type = type;
    event->value = value;
    event->ascii = ascii;

    if (sequence && sequence_len) {
        event->sequence_len =
            min_t(u32, sequence_len, MORSE_HID_MAX_SEQUENCE_LEN);

        memcpy(event->sequence,
               sequence,
               event->sequence_len);

        event->sequence[event->sequence_len] = '\0';
    }

    if (stream_gap_pending) {
        event->flags |= MORSE_HID_EVENT_F_STREAM_GAP;
        stream_gap_pending = false;
    }

    ring_head++;
    if (ring_head == MORSE_HID_DIAG_QUEUE_CAPACITY)
        ring_head = 0;

    WRITE_ONCE(ring_count, ring_count + 1);

out_unlock:
    mutex_unlock(&diagnostic_lock);

    wake_up_interruptible(&diagnostic_wait_queue);
}

/* ------------------------------------------------------------------------- */
/* Atomic statistics API                                                     */
/* ------------------------------------------------------------------------- */

void morse_hid_chrdev_note_input_received(void)
{
    atomic64_inc(&input_events_received);
}

void morse_hid_chrdev_note_input_processed(void)
{
    atomic64_inc(&input_events_processed);
}

void morse_hid_chrdev_note_input_queue_overflow(u32 dropped)
{
    atomic64_inc(&input_queue_overflows);
    atomic64_add(dropped, &input_events_dropped);
}

void morse_hid_chrdev_note_input_dropped(u32 dropped)
{
    atomic64_add(dropped, &input_events_dropped);
}

void morse_hid_chrdev_note_sequence_overflow(void)
{
    atomic64_inc(&sequence_overflows);
}

void morse_hid_chrdev_note_commit(u32 event_type)
{
    switch (event_type) {
    case MORSE_HID_EVENT_COMMIT_VALID:
        atomic64_inc(&valid_commits);
        break;

    case MORSE_HID_EVENT_COMMIT_EMPTY:
        atomic64_inc(&empty_commits);
        break;

    case MORSE_HID_EVENT_COMMIT_INVALID:
        atomic64_inc(&invalid_commits);
        break;

    default:
        break;
    }
}

void morse_hid_chrdev_note_device_connected(void)
{
    atomic_inc(&connected_devices);
}

void morse_hid_chrdev_note_device_disconnected(void)
{
    atomic_dec_if_positive(&connected_devices);
}

void morse_hid_chrdev_set_input_queue_depth(u32 depth)
{
    atomic_set(&current_input_queue_depth, depth);
}

/* ------------------------------------------------------------------------- */
/* Exclusive open                                                            */
/* ------------------------------------------------------------------------- */

static int morse_hid_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;

    if (READ_ONCE(device_stopping) ||
        !READ_ONCE(device_registered))
        return -ENODEV;

    if (atomic_cmpxchg(&reader_open, 0, 1) != 0)
        return -EBUSY;

    return 0;
}

static int morse_hid_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;

    atomic_set(&reader_open, 0);

    return 0;
}

/* ------------------------------------------------------------------------- */
/* read()                                                                     */
/* ------------------------------------------------------------------------- */

static ssize_t morse_hid_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *position)
{
    size_t copied = 0;
    ssize_t result = 0;
    const size_t record_size = sizeof(struct morse_hid_event);

    (void)position;

    if (count == 0)
        return 0;

    if (count < record_size || count % record_size)
        return -EINVAL;

    result = mutex_lock_interruptible(&read_lock);
    if (result)
        return result;

    while (copied < count) {
        struct morse_hid_event event;

        if (READ_ONCE(ring_count) == 0) {
            if (copied)
                break;

            if (file->f_flags & O_NONBLOCK) {
                result = -EAGAIN;
                break;
            }

            result = wait_event_interruptible(
                diagnostic_wait_queue,
                READ_ONCE(ring_count) > 0 ||
                READ_ONCE(device_stopping));

            if (result)
                break;

            if (READ_ONCE(device_stopping) &&
                READ_ONCE(ring_count) == 0) {
                result = 0;
                break;
            }
        }

        mutex_lock(&diagnostic_lock);

        if (ring_count == 0) {
            mutex_unlock(&diagnostic_lock);
            continue;
        }

        /*
         * Keep the ring locked until copy_to_user finishes.
         * This prevents the producer from replacing the record
         * before the reader consumes it.
         */
        event = diagnostic_ring[ring_tail];

        if (copy_to_user(buffer + copied, &event, sizeof(event))) {
            mutex_unlock(&diagnostic_lock);
            result = -EFAULT;
            break;
        }

        ring_tail++;
        if (ring_tail == MORSE_HID_DIAG_QUEUE_CAPACITY)
            ring_tail = 0;

        WRITE_ONCE(ring_count, ring_count - 1);

        mutex_unlock(&diagnostic_lock);

        copied += record_size;
    }

    mutex_unlock(&read_lock);

    return copied ? (ssize_t)copied : result;
}

/* ------------------------------------------------------------------------- */
/* poll()                                                                     */
/* ------------------------------------------------------------------------- */

static __poll_t morse_hid_poll(
    struct file *file,
    struct poll_table_struct *wait)
{
    __poll_t mask = 0;

    (void)file;

    poll_wait(file, &diagnostic_wait_queue, wait);

    if (READ_ONCE(ring_count) > 0)
        mask |= EPOLLIN | EPOLLRDNORM;

    if (READ_ONCE(device_stopping))
        mask |= EPOLLHUP;

    return mask;
}

/* ------------------------------------------------------------------------- */
/* ioctl()                                                                    */
/* ------------------------------------------------------------------------- */

static long morse_hid_ioctl(
    struct file *file,
    unsigned int command,
    unsigned long argument)
{
    void __user *user_argument = (void __user *)argument;

    (void)file;

    switch (command) {
    case MORSE_HID_IOC_GET_INFO: {
        struct morse_hid_info info = {
            .struct_size = sizeof(info),
            .abi_version = MORSE_HID_ABI_VERSION,
            .event_size = sizeof(struct morse_hid_event),
            .diagnostic_queue_capacity =
                MORSE_HID_DIAG_QUEUE_CAPACITY,
            .max_sequence_len = MORSE_HID_MAX_SEQUENCE_LEN,
            .feature_flags =
                MORSE_HID_FEATURE_READ |
                MORSE_HID_FEATURE_POLL |
                MORSE_HID_FEATURE_GET_INFO |
                MORSE_HID_FEATURE_GET_STATS |
                MORSE_HID_FEATURE_EXCLUSIVE_READ,
        };

        if (copy_to_user(user_argument, &info, sizeof(info)))
            return -EFAULT;

        return 0;
    }

    case MORSE_HID_IOC_GET_STATS: {
        struct morse_hid_stats stats = {
            .struct_size = sizeof(stats),
            .abi_version = MORSE_HID_ABI_VERSION,

            .input_events_received =
                atomic64_read(&input_events_received),

            .input_events_processed =
                atomic64_read(&input_events_processed),

            .input_queue_overflows =
                atomic64_read(&input_queue_overflows),

            .input_events_dropped =
                atomic64_read(&input_events_dropped),

            .sequence_overflows =
                atomic64_read(&sequence_overflows),

            .valid_commits =
                atomic64_read(&valid_commits),

            .empty_commits =
                atomic64_read(&empty_commits),

            .invalid_commits =
                atomic64_read(&invalid_commits),

            .diagnostic_queue_overflows =
                atomic64_read(&diagnostic_queue_overflows),

            .diagnostic_events_dropped =
                atomic64_read(&diagnostic_events_dropped),

            .input_queue_depth =
                atomic_read(&current_input_queue_depth),

            .diagnostic_queue_depth =
                READ_ONCE(ring_count),

            .flags =
                atomic_read(&connected_devices) > 0 ?
                MORSE_HID_STATS_F_INPUT_CONNECTED : 0,
        };

        if (copy_to_user(user_argument, &stats, sizeof(stats)))
            return -EFAULT;

        return 0;
    }

    default:
        return -ENOTTY;
    }
}

/* ------------------------------------------------------------------------- */
/* File operations                                                           */
/* ------------------------------------------------------------------------- */

static const struct file_operations morse_hid_fops = {
    .owner          = THIS_MODULE,
    .open           = morse_hid_open,
    .release        = morse_hid_release,
    .read           = morse_hid_read,
    .poll           = morse_hid_poll,
    .unlocked_ioctl = morse_hid_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = compat_ptr_ioctl,
#endif
    .llseek         = no_llseek,
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* Initialization and cleanup                                                */
/* ------------------------------------------------------------------------- */

int morse_hid_chrdev_init(void)
{
    int error;

    mutex_lock(&diagnostic_lock);

    ring_head = 0;
    ring_tail = 0;
    WRITE_ONCE(ring_count, 0);

    stream_gap_pending = false;

    memset(diagnostic_ring, 0, sizeof(diagnostic_ring));

    mutex_unlock(&diagnostic_lock);

    atomic_set(&reader_open, 0);
    atomic_set(&connected_devices, 0);
    atomic_set(&current_input_queue_depth, 0);

    atomic64_set(&next_event_id, 0);

    atomic64_set(&input_events_received, 0);
    atomic64_set(&input_events_processed, 0);
    atomic64_set(&input_queue_overflows, 0);
    atomic64_set(&input_events_dropped, 0);
    atomic64_set(&sequence_overflows, 0);

    atomic64_set(&valid_commits, 0);
    atomic64_set(&empty_commits, 0);
    atomic64_set(&invalid_commits, 0);

    atomic64_set(&diagnostic_queue_overflows, 0);
    atomic64_set(&diagnostic_events_dropped, 0);

    WRITE_ONCE(device_registered, false);
    WRITE_ONCE(device_stopping, false);

    /*
     * Set the registration state before exposing the device node.
     */
    WRITE_ONCE(device_registered, true);

    error = misc_register(&morse_hid_misc_device);

    if (error) {
        WRITE_ONCE(device_registered, false);

        pr_err(DRIVER_NAME
               ": misc device registration failed: %d\n", error);

        return error;
    }

    pr_info(DRIVER_NAME
            ": character device registered; name=%s major=%d minor=%d\n",
            MORSE_HID_DEVICE_NAME,
            MISC_MAJOR,
            morse_hid_misc_device.minor);

    return 0;
}

void morse_hid_chrdev_exit(void)
{
    WRITE_ONCE(device_stopping, true);

    wake_up_interruptible(&diagnostic_wait_queue);

    if (READ_ONCE(device_registered)) {
        misc_deregister(&morse_hid_misc_device);
        WRITE_ONCE(device_registered, false);
    }

    pr_info(DRIVER_NAME ": character device unregistered\n");
}
