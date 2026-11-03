# QEMU ARM64 MorseMouse keyboard validation

Date: 3 November 2026
Target: qemuarm64
Guest architecture: aarch64
Guest kernel: 6.6.151-yocto-standard
Image: core-image-morse-dev

## Build results

Host tests:
- All 95 printable ASCII Morse mappings passed.
- All 95 US QWERTY key mappings passed.
- Invalid ASCII and NULL output handling passed.
- Empty COMMIT, unmapped sequence and RESET tests passed.

Yocto:
- morse-ref-decoder package build: 843 tasks, all successful.
- core-image-morse-dev build: 4155 tasks, all successful.
- Development image booted successfully.

## Virtual keyboard

The guest exposed /dev/uinput with root-only permissions.

The program created:
MorseMouse Reference Keyboard

Its event node was discovered through:
/sys/class/input/event*/device/name

## Character tests

| Character | Morse | Result |
|---|---|---|
| A | .- | Passed |
| e | --.-. | Passed |
| 5 | ..... | Passed |
| ! | ..-...... | Passed |
| SPACE | ------ | Passed |

evtest confirmed press and release events.

Uppercase A:
- KEY_LEFTSHIFT press
- KEY_A press
- KEY_A release
- KEY_LEFTSHIFT release

Exclamation mark:
- KEY_LEFTSHIFT press
- KEY_1 press
- KEY_1 release
- KEY_LEFTSHIFT release

SPACE:
- KEY_SPACE press
- KEY_SPACE release

## Reliability tests

- Two consecutive A characters generated two separate key sequences.
- An invalid ten-symbol Morse sequence was rejected.
- Invalid input generated no keyboard events.
- A valid A was decoded successfully immediately after an invalid COMMIT.
- The virtual keyboard was destroyed during process termination.
- The device disappeared from sysfs after cleanup.

## Limitations

These tests validate the userspace evdev-to-Morse-to-uinput path in QEMU.

They do not establish:
- Physical Zebronics PULSE receiver compatibility.
- Raspberry Pi 4 operation.
- Kernel input-handler or kernel-module functionality.
- Final debounce, chord, timing, wheel or layer behavior.
- Visible text entry into a graphical application.

Middle-click COMMIT remains a provisional test gesture.
