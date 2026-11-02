# QEMU evdev runtime mouse-event test

**Date:** 2 November 2026  
**Target:** QEMU ARM64 `core-image-morse-dev`  
**Guest device:** `/dev/input/event0`, QEMU QEMU USB Tablet  
**Tool:** `evtest 1.36`, QEMU 8.2.7 monitor

## Test method

Ran `evtest /dev/input/event0` in the guest and used the QEMU monitor to inject button-state and wheel events. QEMU reported one active device:

```text
* Mouse #1: QEMU HID Tablet (absolute)
```

Commands and observed Linux event mapping:

| QEMU monitor command | Observed evdev event | Result |
|---|---|---|
| `mouse_button 1` | `EV_KEY BTN_LEFT (272), value 1` | Pass |
| `mouse_button 0` after left | `EV_KEY BTN_LEFT (272), value 0` | Pass |
| `mouse_button 2` | `EV_KEY BTN_RIGHT (273), value 1` | Pass |
| `mouse_button 0` after right | `EV_KEY BTN_RIGHT (273), value 0` | Pass |
| `mouse_button 4` | `EV_KEY BTN_MIDDLE (274), value 1` | Pass |
| `mouse_button 0` after middle | `EV_KEY BTN_MIDDLE (274), value 0` | Pass |
| `mouse_move 0 0 1` | `REL_WHEEL (8), value 1`; `REL_WHEEL_HI_RES (11), value 120` | Pass |
| `mouse_move 0 0 -1` | `REL_WHEEL (8), value -1`; `REL_WHEEL_HI_RES (11), value -120` | Pass |

Each observed input event was followed by `SYN_REPORT`. Button events also included `MSC_SCAN` values (0x90001 left, 0x90002 right, 0x90003 middle).

## Result

**Runtime event delivery is verified for the QEMU-emulated tablet:** left/right/middle press and release transitions and vertical wheel up/down events reached the guest evdev interface with the expected Linux event codes and values.

## Limits and next tests

- This test uses QEMU HMP-generated events and the QEMU absolute tablet, not the physical Zebronics PULSE USB receiver.
- It does not validate physical mouse timing, debounce, input latency, or real hardware characteristics.
- The guest-side `/dev/uinput` create/inject/readback test is still pending.
- A kernel log warning, `hrtimer: interrupt took 120723744 ns`, appeared after the test. Its cause has not been established; do not interpret this event test as a timing/latency benchmark.

The next milestone is a small reproducible `uinput` self-test that creates a virtual keyboard, emits a key press/release, and verifies the resulting events through evdev.
