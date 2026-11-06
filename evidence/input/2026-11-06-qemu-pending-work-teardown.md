# QEMU Pending-Work Teardown Stress Test

## Environment

- Target: qemuarm64
- Kernel: 6.6.151-yocto-standard
- Kernel module: hid_morse_kbd 0.2
- Queue capacity: 64
- Test interface: Linux uinput
- Virtual device: Morse Overflow Test Tablet

## Test procedure

1. Configured the input handler to match Morse Overflow Test Tablet.
2. Created a virtual input device with EV_KEY and EV_ABS capabilities.
3. Verified that hid_morse_kbd attached successfully.
4. Pressed BTN_LEFT.
5. Pinned the event producer to CPU 0.
6. Enabled SCHED_FIFO priority 1.
7. Submitted 250,000 BTN_LEFT repeat events.
8. Destroyed the virtual input device before restoring normal scheduling.
9. Allowed the input handler disconnect callback to complete.
10. Restored normal scheduling.
11. Verified successful return from the teardown utility.
12. Reloaded the handler against the normal QEMU USB Tablet.

## Results

| Measurement | Result |
|---|---:|
| Events submitted | 250,000 |
| Events received | 250,002 |
| Events processed | 1 |
| Queue capacity | 64 |
| Queue overflows | 3,846 |
| Events dropped | 250,001 |
| UI_DEV_DESTROY | Successful |
| Handler disconnect | Successful |
| Teardown utility | SUCCESS |

## Kernel evidence

Connection:

    hid_morse_kbd: connected to "Morse Overflow Test Tablet";
    grab=enabled queue_capacity=64

Overflow:

    hid_morse_kbd: queue overflow; sequence reset;
    dropped=250001 left=0 right=0 middle=0 lockout=0

Disconnect:

    hid_morse_kbd: disconnected from "Morse Overflow Test Tablet"
    received=250002 processed=1 queue_overflows=3846
    dropped=250001 sequence_overflows=0

Utility result:

    PASS: UI_DEV_DESTROY returned successfully.
    Normal scheduling restored.
    PASS: teardown test completed.
    Result: SUCCESS

## Normal device restoration

After the test, the module was reloaded with:

    target_name="USB Tablet"
    grab_on_bind=1

The kernel confirmed:

    hid_morse_kbd: connected to "QEMU QEMU USB Tablet";
    grab=enabled queue_capacity=64

## Conclusion

PASS: Device destruction and handler teardown completed
successfully under high-volume event and queue pressure.

The implementation invokes cancel_work_sync() before freeing
the per-device state, ensuring that pending or running deferred
work is synchronized during teardown.

This stress test does not independently prove that the worker
was actively executing at the exact instant of device destruction.
