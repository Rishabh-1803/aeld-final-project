# MorseMouse architecture

## Purpose

MorseMouse is an Embedded Linux input system designed to turn a supported mouse into a compact Morse text-entry device. The approved v1 grammar uses the left button for DOT, the right button for DASH, and the middle button for explicit COMMIT. Scroll-wheel gestures are disabled in v1. QEMU ARM64 is the development and integration target; Raspberry Pi 4 64-bit is reserved for final physical validation.

## Planned data path

```text
Physical mouse + USB receiver
            |
        Linux HID
            |
     Linux input core
            |
  targeted input_handler
     (hid_morse_kbd.ko)
            |
    bounded event queue
            |
 ordered workqueue / Morse FSM
       /             \
      v               v
virtual input_dev   /dev/morse_hid
 keyboard                |
      |                 morsed
      v                   |
Linux applications   read-only telemetry
                          |
                    morse_hud_client
```

This diagram describes the intended architecture. Components must not be treated as implemented until source code and validation evidence exist.

## Implemented userspace reference path

The current userspace reference prototype implements this data path:

```text
QEMU USB Tablet / Linux evdev
             |
             v
      morse_ref_decoder
             |
             v
          morse_fsm
             |
       DOT / DASH / COMMIT
             |
             v
      Morse sequence buffer
             |
             v
       95-character map
             |
             v
       US QWERTY keymap
             |
             v
           /dev/uinput
             |
             v
    Virtual Linux keyboard
```

Implemented behavior:

- Left-button press appends one DOT; right-button press appends one DASH.
- Releases update button state without adding symbols.
- Repeated press and key-repeat events are ignored.
- Middle-button press requests explicit COMMIT; empty COMMIT is ignored.
- A committed sequence is consumed whether valid or invalid.
- An overlapping left/right press clears the sequence and activates chord lockout.
- Lockout ends only after both primary buttons have been released.
- Decoded printable ASCII characters can be emitted through uinput.

The independent FSM has 37 passing unit checks. The integrated decoder and virtual keyboard were verified in QEMU. This is a userspace reference implementation, not the planned kernel input handler.

## Kernel input-handler module — implemented baseline

The `hid_morse_kbd.ko` module is implemented and committed. It registers a targeted Linux `input_handler`, matches the QEMU USB Tablet by name substring and event capabilities, logs button transitions, supports the `target_name` and `grab_on_bind` module parameters, and handles connect/disconnect and module lifecycle. The QEMU acceptance evidence verifies automatic binding, keyboard exclusion, left/right/middle press/release events, exclusive grab, grab release on unload, HID unbind/rebind, and actual QEMU USB hot-unplug/hot-replug.

Source: `driver/hid_morse_kbd_main.c`; Yocto recipe: `meta-morse-mouse/recipes-kernel/hid-morse-kbd/hid-morse-kbd_0.3.bb`. See [QEMU kernel input-handler runtime evidence](../evidence/input/2026-11-05-qemu-kernel-input-handler.md).

The kernel path now includes the bounded queue, shared FSM, virtual keyboard
and versioned diagnostic character device. Issue #7 evidence is dated
November 7; Issue #8 acceptance is dated November 17. The input callback
queues transitions without sleeping; per-device work processes them in order.

Input/timer callbacks must not perform filesystem or socket I/O, user-copy, arbitrary command execution, or unbounded processing. Timer/workqueue teardown must be synchronized before associated objects are freed.

## Userspace components

| Component | Planned responsibility |
|---|---|
| `morsed` | Read driver events, coordinate service lifecycle, diagnostics, guarded local power-intent validation, and telemetry |
| `morsectl` | Local administrative/query/test interface |
| `morse_hud_client` | Read-only status display for mode, decoder, layer, queue, counters, and service state |

The daemon must not interpret client-provided strings as shell commands. The HUD is read-only and cannot initiate power actions.

## Input mapping and state machine

The approved v1 grammar is documented in [`docs/input-grammar.md`](input-grammar.md).

| Physical input | v1 behavior |
|---|---|
| Left-button press | Append DOT (`.`) |
| Right-button press | Append DASH (`-`) |
| Left/right release | Update button state; no symbol |
| Middle-button press | Explicit COMMIT |
| Empty COMMIT | Ignore |
| Left/right overlap | Clear sequence and enter chord lockout |
| During chord lockout | Ignore input until both primary buttons are released |
| Scroll wheel | Disabled in v1 |

Button identity determines DOT and DASH. Press duration has no Morse meaning. There is no automatic character timeout or arbitrary debounce interval in v1.

The authoritative mapping is `config/morse_map.csv`. It contains all 95 printable ASCII characters (decimal 32–126), with unique Morse sequences. The validator checks completeness, uniqueness, symbol validity and mapping conventions.

The userspace reference decoder uses this mapping and a separate US QWERTY Linux keymap. The standalone FSM is implemented in `tools/morse_fsm.c` and tested in `tools/morse_fsm_test.c`.

The kernel handler uses the shared FSM core and approved grammar. Host tests cover 52 FSM checks and 58 shared vectors.

## Targets and validation boundary

- **QEMU ARM64 (`qemuarm64`):** first development and integration target. Linux `uinput` will inject deterministic synthetic mouse events.
- **Raspberry Pi 4 64-bit (`raspberrypi4-64`):** physical target using the Zebronics PULSE mouse and 2.4 GHz USB receiver.

QEMU `uinput` tests validate driver/FSM behavior. They do not validate the RF receiver, physical button behavior, or Pi hardware. Physical tests must be recorded separately.

Build kernel modules independently for each target. Never copy a QEMU-built `.ko` to the Pi; check module `vermagic` against the target kernel.

## Guarded power behavior

Power control is separate from ordinary text entry. The development default is dry-run or disabled. Any future power intent requires a dedicated two-stage local physical confirmation and independent userspace policy validation. This is not cryptographic authentication. Network telemetry/HUD input must never cause reboot or shutdown.

## Current implementation status — 17 November 2026

Implemented and QEMU-validated: Yocto development image, userspace reference,
95-character map/keymap, kernel input-handler lifecycle, bounded queue/FSM,
kernel virtual keyboard and diagnostic UAPI. See the evidence directory
and [character-device contract](character-device.md).

Pending: daemon, read-only HUD, final integration and physical Pi/receiver
validation. Power execution is deferred until after final submission by the
owner's November 17 decision; it must remain disabled in the submitted system.
