# MorseMouse QEMU FSM and uinput Integration Evidence

**Date:** 4 November 2026  
**Target:** Yocto QEMU ARM64 (`qemuarm64`)  
**Kernel:** Linux 6.6.151-yocto-standard  
**Yocto:** Poky Scarthgap 5.0.21  
**Image:** `core-image-morse-dev-qemuarm64`  
**Root filesystem:** `core-image-morse-dev-qemuarm64.rootfs-20261104074706.ext4`

## 1. Build verification

The userspace reference decoder was extended with an independent Morse FSM.

Host build and tests:

```bash
make -C tools clean all test
```

Results:

- Reference decoder: all 95 printable ASCII mappings decoded and committed successfully.
- Keymap test: all 95 printable ASCII characters mapped to Linux US QWERTY events.
- FSM test: all 37 checks passed.
- Morse map validator: 95 printable ASCII entries, 95 unique Morse sequences, no collisions, and round-trip lookups passed.

Yocto results:

- `bitbake morse-ref-decoder`: 843 tasks attempted; all succeeded.
- `bitbake core-image-morse-dev`: 4,155 tasks attempted; all succeeded.
- Package data confirmed `/usr/bin/morse-ref-decoder` and `/usr/share/morsemouse/morse_map.csv`.

One non-blocking host warning concerned missing `CONFIG_TASK_IO_ACCOUNTING`.

## 2. QEMU environment

The QEMU guest booted successfully.

Detected input devices:

| Device | Event node |
|---|---|
| QEMU QEMU USB Tablet | `/dev/input/event0` |
| QEMU QEMU USB Keyboard | `/dev/input/event1` |
| MorseMouse Reference Keyboard | `/dev/input/event2` |

The installed decoder passed its self-test inside the guest.

The integrated process was started with:

```bash
morse-ref-decoder \
    --map /usr/share/morsemouse/morse_map.csv \
    --evdev /dev/input/event0 \
    --uinput
```

It created the virtual keyboard successfully.

## 3. Empty COMMIT and normal decoding

An empty middle-button press was tested.

Observed:

```text
INPUT: BTN_MIDDLE down -> COMMIT; ignored because sequence is empty.
```

The sequence `.-` was then entered and committed.

Observed:

```text
BUFFER: .
BUFFER: .-
DECODED: ascii=65 hex=0x41 char='A'
UINPUT: emitted ASCII 65 successfully.
```

## 4. Chord cancellation and lockout

The following QEMU HMP button masks were entered:

| HMP command | Button state |
|---|---|
| `mouse_button 1` | Left held |
| `mouse_button 3` | Left and right held |
| `mouse_button 2` | Right held |
| `mouse_button 3` | Left and right held |
| `mouse_button 2` | Right held |
| `mouse_button 0` | Both released |

Observed:

```text
BUFFER: .
INPUT: overlapping BTN_LEFT/BTN_RIGHT presses ->
       sequence cleared; chord lockout active.
```

A subsequent left press while the right button remained held did not append a symbol. Lockout ended after both primary buttons were released.

## 5. Recovery after chord lockout

After both primary buttons were released, the sequence `.-` was entered and committed again.

Observed:

```text
BUFFER: .
BUFFER: .-
DECODED: ascii=65 hex=0x41 char='A'
UINPUT: emitted ASCII 65 successfully.
```

This confirms that normal input resumed after chord lockout.

## 6. Virtual keyboard verification

The virtual keyboard was monitored using `evtest`.

Two complete uppercase-A key sequences were observed. Each contained:

1. `KEY_LEFTSHIFT` press.
2. `KEY_A` press.
3. `SYN_REPORT`.
4. `KEY_A` release.
5. `KEY_LEFTSHIFT` release.
6. `SYN_REPORT`.

The decoder remained running after the tests.

## 7. Acceptance summary

| Requirement | Result |
|---|---|
| Empty COMMIT ignored | PASS |
| DOT/DASH input | PASS |
| Character COMMIT | PASS |
| Left/right chord cancellation | PASS |
| Chord lockout | PASS |
| Input ignored during lockout | PASS |
| Recovery after both releases | PASS |
| Character decoding after recovery | PASS |
| uinput uppercase key emission | PASS |

## 8. Limitations

- Testing used the QEMU-emulated USB tablet, not the physical Zebronics PULSE mouse.
- Raspberry Pi 4 hardware testing has not been performed.
- Keyboard events were verified with `evtest`; visible text insertion into a graphical application was not tested.
- Runtime `SYN_DROPPED` recovery was not exercised in this QEMU integration session.
- Physical input capabilities and event delivery remain to be validated during the final hardware stage.

This evidence establishes successful QEMU integration of the v1 Morse FSM, reference decoder, and uinput virtual keyboard. It does not establish completion of the Linux kernel input handler or physical hardware acceptance.