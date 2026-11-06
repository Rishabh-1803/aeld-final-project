# QEMU Bounded Queue Overflow and Recovery Test

## Test environment

- Target: qemuarm64
- Kernel: 6.6.151-yocto-standard
- Kernel module: hid_morse_kbd 0.2
- Queue capacity: 64 events
- Test interface: Linux uinput
- Virtual device: Morse Overflow Test Tablet

## Test procedure

1. Created a virtual input tablet with EV_KEY and EV_ABS capabilities.
2. Verified that hid_morse_kbd attached to the device.
3. Pressed and held BTN_LEFT.
4. Pinned the event producer to CPU 0.
5. Temporarily enabled SCHED_FIFO priority 1.
6. Submitted 250,000 BTN_LEFT repeat events in one write.
7. Restored normal scheduling.
8. Allowed worker-side overflow recovery and state synchronization.
9. Released BTN_LEFT.
10. Submitted a fresh DOT, DASH, COMMIT sequence.
11. Destroyed the virtual device.

## Results

| Measurement | Result |
|---|---:|
| Queue capacity | 64 |
| Events received | 250,008 |
| Events processed | 8 |
| Queue overflows | 3,846 |
| Events dropped | 250,000 |
| Valid commits | 1 |
| Empty commits | 0 |
| Invalid commits | 0 |
| Sequence overflows | 0 |

## Kernel evidence

Connection:

    hid_morse_kbd: connected to "Morse Overflow Test Tablet";
    grab=enabled queue_capacity=64

Overflow:

    hid_morse_kbd: queue overflow; sequence reset;
    dropped=250000 left=1 right=0 middle=0 lockout=1

Recovery:

    hid_morse_kbd: FSM action=DOT sequence=.
    hid_morse_kbd: FSM action=DASH sequence=.-
    hid_morse_kbd: FSM COMMIT valid sequence=.- ascii=65 hex=0x41

Disconnect statistics:

    received=250008 processed=8 queue_overflows=3846
    dropped=250000 sequence_overflows=0
    commits_valid=1 commits_empty=0 commits_invalid=0

## Conclusion

PASS.

The production 64-entry queue overflowed under high-volume input.
The driver dropped queued history, recorded overflow accounting,
reset the Morse sequence, and resynchronized the held LEFT button.

After the button was released, a new DOT-DASH-COMMIT sequence
was successfully decoded as ASCII 65 ('A').

The virtual input device disconnected successfully.

This test validates the queue overflow and recovery path in QEMU.
It does not independently establish teardown while a worker is
actively executing.
