# MorseMouse userspace evdev prototype — v0.2

**Status:** Experimental QEMU validation utility. This is not the planned kernel driver and does not freeze the full physical gesture grammar.

## Purpose

Extend the mapping-only C reference decoder so it can read Linux `evdev` input events directly. This validates the event-to-Morse-symbol mapping in userspace before implementing the kernel `input_handler`.

## Provisional controls used by this test

| Linux event | Prototype action |
|---|---|
| `EV_KEY / BTN_LEFT`, value `1` | Append one dot (`.`) |
| `EV_KEY / BTN_RIGHT`, value `1` | Append one dash (`-`) |
| `EV_KEY / BTN_MIDDLE`, value `1` | Commit the buffered character |
| Any button release, value `0` | Ignore |
| Key repeat, value `2` | Ignore |
| `EV_SYN / SYN_DROPPED` | Reset the buffered sequence and report queue overflow |

Middle-click commit is used only as a convenient provisional test input. The complete gesture grammar (including wheel/layer/space and chord precedence) remains undecided. The utility does not implement debounce, duration-based classification, chord recognition, mouse grabbing, or virtual-keyboard output.

## Build in the project layer

The recipe `morse-ref-decoder_0.1.bb` compiles the utility with the Yocto target compiler and installs:

- `/usr/bin/morse-ref-decoder`
- `/usr/share/morsemouse/morse_map.csv`

The development image recipe includes `morse-ref-decoder`; the production `core-image-morse` recipe is unchanged.

From the project root in WSL:

```bash
cd ~/projects/morsemouse
source poky/oe-init-build-env build-qemu
bitbake-layers show-recipes morse-ref-decoder
bitbake core-image-morse-dev
```

The `source` command changes the current directory to `build-qemu`.

## Run inside QEMU

Boot the newly built development image using the project's existing QEMU procedure. In the guest, identify the event device first:

```bash
cat /proc/bus/input/devices
ls -l /dev/input
```

The earlier QEMU test observed the emulated tablet at `/dev/input/event0`, but event numbers can change. Confirm the current node before running the prototype.

Run as root (the input event node may be restricted):

```bash
morse-ref-decoder --map /usr/share/morsemouse/morse_map.csv --evdev /dev/input/event0
```

The program validates that the selected device advertises `EV_KEY`, `BTN_LEFT`, `BTN_RIGHT`, and `BTN_MIDDLE`. It does not assume every event node is a mouse.

## QEMU HMP smoke test

With the program listening, open the QEMU monitor using the established `Ctrl+A`, then `C` sequence. For character `A` (`.-`), inject button press/release pairs:

```text
mouse_button 1
mouse_button 0
mouse_button 2
mouse_button 0
mouse_button 4
mouse_button 0
```

Expected guest output includes:

```text
INPUT: BTN_LEFT down -> DOT
BUFFER: .
INPUT: BTN_RIGHT down -> DASH
BUFFER: .-
INPUT: BTN_MIDDLE down -> COMMIT
DECODED: ascii=65 hex=0x41 char='A'
```

The QEMU monitor codes above were verified earlier against the emulated QEMU tablet. Verify the current device and runtime events rather than assuming physical Zebronics behavior.

## Important boundaries

- Use a new event-node path only after inspecting `/proc/bus/input/devices`; do not assume `event0` is stable.
- This userspace prototype intentionally does not call `EVIOCGRAB`; normal desktop pointer handling remains active.
- By default, decoded characters are printed to stdout. With the optional
`--uinput` flag, decoded characters are emitted through the Linux
virtual keyboard `MorseMouse Reference Keyboard`.

The QEMU prototype verified keyboard events for uppercase, lowercase,
digits, punctuation and SPACE. See
`evidence/input/2026-11-03-qemu-uinput-keyboard.md`.
- Run it as root in the test image if permissions prevent opening `/dev/input/eventN`.
- QEMU validates the emulated tablet and the code path, not the Zebronics PULSE receiver or Raspberry Pi hardware.
- The kernel `input_handler`, virtual keyboard, `/dev/morse_hid` ABI, daemon, and physical acceptance tests remain future work.
