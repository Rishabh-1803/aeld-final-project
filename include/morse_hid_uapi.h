/* SPDX-License-Identifier: MIT */

#ifndef MORSE_HID_UAPI_H
#define MORSE_HID_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

/*
 * MorseMouse character-device userspace ABI.
 *
 * ABI v1:
 *   - Fixed-width fields.
 *   - Fixed-size diagnostic event records.
 *   - Read-only diagnostic ioctls.
 *   - No arbitrary command execution.
 */

#define MORSE_HID_DEVICE_NAME            "morse_hid"

#define MORSE_HID_ABI_VERSION            1U

#define MORSE_HID_DIAG_QUEUE_CAPACITY    128U

#define MORSE_HID_MAX_SEQUENCE_LEN       63U
#define MORSE_HID_SEQUENCE_SIZE          (MORSE_HID_MAX_SEQUENCE_LEN + 1U)

/* Supported interface features. */
#define MORSE_HID_FEATURE_READ           (1U << 0)
#define MORSE_HID_FEATURE_POLL           (1U << 1)
#define MORSE_HID_FEATURE_GET_INFO       (1U << 2)
#define MORSE_HID_FEATURE_GET_STATS      (1U << 3)
#define MORSE_HID_FEATURE_EXCLUSIVE_READ (1U << 4)

/* Event types returned by read(). */
enum morse_hid_event_type {
    MORSE_HID_EVENT_DOT = 1,
    MORSE_HID_EVENT_DASH,
    MORSE_HID_EVENT_COMMIT_VALID,
    MORSE_HID_EVENT_COMMIT_INVALID,
    MORSE_HID_EVENT_COMMIT_EMPTY,
    MORSE_HID_EVENT_CHORD_CANCEL,
    MORSE_HID_EVENT_INPUT_QUEUE_OVERFLOW,
    MORSE_HID_EVENT_SEQUENCE_OVERFLOW,
    MORSE_HID_EVENT_DEVICE_CONNECTED,
    MORSE_HID_EVENT_DEVICE_DISCONNECTED,
};

/* Event flags. */
#define MORSE_HID_EVENT_F_STREAM_GAP     (1U << 0)

/* GET_STATS flags. */
#define MORSE_HID_STATS_F_INPUT_CONNECTED (1U << 0)

/*
 * Fixed-size diagnostic record.
 *
 * value:
 *   DOT/DASH: '.' or '-'
 *   Other events: 0 unless documented otherwise.
 *
 * ascii:
 *   Valid COMMIT: decoded printable ASCII value.
 *   Other events: 0.
 *
 * sequence:
 *   NUL-terminated Morse sequence snapshot.
 *   sequence_len excludes the NUL terminator.
 */
struct morse_hid_event {
    __aligned_u64 event_id;
    __aligned_u64 timestamp_ns;

    __u32 type;
    __u32 flags;

    __s32 value;
    __u32 ascii;

    __u32 sequence_len;
    __u8 sequence[MORSE_HID_SEQUENCE_SIZE];

    __u32 reserved;
};

/* ABI and device capability information. */
struct morse_hid_info {
    __u32 struct_size;
    __u32 abi_version;

    __u32 event_size;
    __u32 diagnostic_queue_capacity;

    __u32 max_sequence_len;
    __u32 feature_flags;

    __u32 reserved[2];
};

/* Driver and diagnostic queue statistics. */
struct morse_hid_stats {
    __u32 struct_size;
    __u32 abi_version;

    __aligned_u64 input_events_received;
    __aligned_u64 input_events_processed;

    __aligned_u64 input_queue_overflows;
    __aligned_u64 input_events_dropped;

    __aligned_u64 sequence_overflows;

    __aligned_u64 valid_commits;
    __aligned_u64 empty_commits;
    __aligned_u64 invalid_commits;

    __aligned_u64 diagnostic_queue_overflows;
    __aligned_u64 diagnostic_events_dropped;

    __u32 input_queue_depth;
    __u32 diagnostic_queue_depth;

    __u32 flags;
    __u32 reserved;
};

/*
 * Project-private ioctl type 0xB9.
 *
 * Keep existing command numbers and structures stable after ABI v1
 * is released. Add new command numbers for future extensions.
 */
#define MORSE_HID_IOC_TYPE              0xB9

#define MORSE_HID_IOC_GET_INFO \
    _IOR(MORSE_HID_IOC_TYPE, 0x00, struct morse_hid_info)

#define MORSE_HID_IOC_GET_STATS \
    _IOR(MORSE_HID_IOC_TYPE, 0x01, struct morse_hid_stats)

#endif /* MORSE_HID_UAPI_H */
