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

## Kernel module responsibilities

The planned `hid_morse_kbd.ko` module will:

- Register an `input_handler` that matches the intended physical mouse by identity and capabilities, not by a hard-coded `/dev/input/eventN` name.
- Exclude the virtual keyboard from matching.
- Start in ordinary pointer mode and use reversible grab/release behavior when Morse mode is enabled.
- Handle connect, disconnect, open, close, hot-unplug, and module-unload lifecycles.
- Record input events and monotonic timestamps in a bounded queue with synchronization and overflow accounting.
- Keep input and timer callbacks bounded and non-sleeping.
- Defer state processing that may sleep to an ordered workqueue.
- Register a virtual `input_dev` keyboard and emit balanced key-down/key-up events.
- Release held keys and modifiers on every reset, mode exit, disconnect, error, and unload path.
- Expose a planned `/dev/morse_hid` character device with a versioned userspace ABI.

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

The kernel-side input handler and in-kernel FSM remain future work. They must follow the approved grammar and reproduce the verified reference behavior.

## Targets and validation boundary

- **QEMU ARM64 (`qemuarm64`):** first development and integration target. Linux `uinput` will inject deterministic synthetic mouse events.
- **Raspberry Pi 4 64-bit (`raspberrypi4-64`):** physical target using the Zebronics PULSE mouse and 2.4 GHz USB receiver.

QEMU `uinput` tests validate driver/FSM behavior. They do not validate the RF receiver, physical button behavior, or Pi hardware. Physical tests must be recorded separately.

Build kernel modules independently for each target. Never copy a QEMU-built `.ko` to the Pi; check module `vermagic` against the target kernel.

## Guarded power behavior

Power control is separate from ordinary text entry. The development default is dry-run or disabled. Any future power intent requires a dedicated two-stage local physical confirmation and independent userspace policy validation. This is not cryptographic authentication. Network telemetry/HUD input must never cause reboot or shutdown.

## Current implementation status

As of 4 November 2026, the following QEMU-side userspace functionality is implemented and validated:

- Yocto Scarthgap QEMU ARM64 development image.
- Approved v1 input grammar and 95-character Morse mapping.
- Morse mapping validator and US QWERTY keymap.
- C userspace reference decoder with evdev input.
- Independent Morse FSM with 37 passing unit checks.
- Chord cancellation, lockout and recovery.
- Optional `/dev/uinput` virtual keyboard output.
- QEMU integration tests for normal decoding, empty COMMIT, chord handling, recovery and uppercase keyboard events.

See [QEMU FSM integration evidence](../evidence/input/2026-11-04-qemu-fsm-integration.md).

The following components are not yet implemented:

- Kernel `input_handler` module and in-kernel FSM.
- Kernel-side event queue and virtual `input_dev` keyboard.
- Versioned `/dev/morse_hid` userspace ABI.
- `morsed`, `morsectl` and the read-only HUD.
- QEMU integration of these future kernel/userspace components.
- Raspberry Pi 4 and physical Zebronics validation, deliberately deferred to the final hardware stage.
