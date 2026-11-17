/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MORSE_HID_CHRDEV_H
#define MORSE_HID_CHRDEV_H

#include <linux/types.h>

#include "morse_hid_uapi.h"

int morse_hid_chrdev_init(void);
void morse_hid_chrdev_exit(void);

/*
 * Called only from sleepable contexts.
 */
void morse_hid_chrdev_push_event(
    u32 type,
    u64 timestamp_ns,
    s32 value,
    u32 ascii,
    const char *sequence,
    u32 sequence_len
);

/* Input statistics: safe for atomic context. */
void morse_hid_chrdev_note_input_received(void);
void morse_hid_chrdev_note_input_processed(void);
void morse_hid_chrdev_note_input_queue_overflow(u32 dropped);
void morse_hid_chrdev_note_input_dropped(u32 dropped);
void morse_hid_chrdev_note_sequence_overflow(void);
void morse_hid_chrdev_note_commit(u32 event_type);

/* Input-device lifecycle. */
void morse_hid_chrdev_note_device_connected(void);
void morse_hid_chrdev_note_device_disconnected(void);

void morse_hid_chrdev_set_input_queue_depth(u32 depth);

#endif /* MORSE_HID_CHRDEV_H */
