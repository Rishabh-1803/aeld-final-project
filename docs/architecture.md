# MorseMouse architecture

## Purpose

MorseMouse is an Embedded Linux input system. A supported three-button mouse and scroll wheel become a compact chorded Morse text-entry device. The core implementation targets QEMU ARM64 first and Raspberry Pi 4 64-bit for physical demonstration.

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

The selected initial encoding is button-identity based: left-button down means dot and right-button down means dash; press duration does not distinguish the two. Middle-button and wheel gestures remain proposals for commit, layer, and word-space functions. Exact gesture precedence, chord handling, and filtering/timing remain to be frozen in [`docs/input-grammar.md`](input-grammar.md) before implementing the kernel FSM.

The intended printable range is ASCII decimal 32–126 (95 values) under a declared US QWERTY acceptance layout. `config/morse_map.csv` is intended to be the single authoritative mapping source. Any non-standard Morse extensions must be explicitly labelled.

A sentinel-bit representation is planned for bounded direct table lookup. Sequence length must be checked before shifting; invalid or unmapped sequences must have deterministic behavior.

## Targets and validation boundary

- **QEMU ARM64 (`qemuarm64`):** first development and integration target. Linux `uinput` will inject deterministic synthetic mouse events.
- **Raspberry Pi 4 64-bit (`raspberrypi4-64`):** physical target using the Zebronics PULSE mouse and 2.4 GHz USB receiver.

QEMU `uinput` tests validate driver/FSM behavior. They do not validate the RF receiver, physical button behavior, or Pi hardware. Physical tests must be recorded separately.

Build kernel modules independently for each target. Never copy a QEMU-built `.ko` to the Pi; check module `vermagic` against the target kernel.

## Guarded power behavior

Power control is separate from ordinary text entry. The development default is dry-run or disabled. Any future power intent requires a dedicated two-stage local physical confirmation and independent userspace policy validation. This is not cryptographic authentication. Network telemetry/HUD input must never cause reboot or shutdown.

## Current implementation status

As of 2 November 2026, only the Yocto baseline and initial custom image metadata have been validated. The project-specific kernel driver, ABI, daemon, CLI, HUD, decoder, and input tests are not implemented yet. See [Project status](project-status.md).
