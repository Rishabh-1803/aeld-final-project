# MorseMouse C reference decoder — v0.2

**Status:** Userspace reference prototype with optional Linux virtual-keyboard output. This is not the planned kernel driver.

## Purpose

This small C program reads the authoritative `config/morse_map.csv` and provides a userspace reference for exact Morse-sequence lookup. It is intended to establish mapping and character-boundary behavior before implementing the Linux input handler or kernel state machine.

It supports interactive DOT/DASH/COMMIT/RESET testing and Linux evdev mouse input. Its optional `--uinput` mode emits decoded characters through a Linux virtual keyboard. The approved v1 userspace FSM implements button-state tracking, ignored repeats, explicit COMMIT, chord cancellation and release-based lockout/recovery. This remains a userspace reference, not the kernel driver.

## Build

From the repository root:

```bash
make -C tools
```

The build uses `-std=c11 -Wall -Wextra -Wpedantic -Werror` by default.

## Automated self-test

```bash
make -C tools test
```

Or run directly:

```bash
./tools/morse-ref-decoder --map config/morse_map.csv --self-test
```

The self-test loads all 95 rows, verifies CSV/header/ASCII fields, validates dot/dash symbols, rejects duplicate codes and missing ASCII values, round-trips every mapping through append + explicit commit, rejects an empty commit, rejects a sequence longer than any mapped sequence, and verifies reset behavior.

## Interactive test

```bash
./tools/morse-ref-decoder --map config/morse_map.csv --interactive
```

Enter one token per line. Example for `A` (`.-`):

```text
DOT
DASH
COMMIT
```

Other commands:

- `DOT` or `.`: append one dot.
- `DASH` or `-`: append one dash.
- `COMMIT`: decode the complete current sequence and clear the buffer.
- `RESET`: discard the current sequence.
- `QUIT`: exit.

An invalid sequence is reported and cleared. An empty commit is ignored and reported. No character is emitted before `COMMIT`.

## v1 input grammar and design boundary

The approved v1 grammar is documented in [input-grammar.md](input-grammar.md): left-button press adds DOT, right-button press adds DASH, and middle-button press commits. Releases add no symbol; repeated press events are ignored. A left/right overlap clears the sequence and locks input until both buttons are released. Scroll-wheel gestures are disabled.

The mapping contains prefix relationships, so explicit COMMIT is required; no inactivity-based autocommit is used. Press duration does not distinguish DOT from DASH, and v1 has no automatic character timeout or long-press behavior.

The current CSV includes a MorseMouse-specific direct code for ASCII SPACE. That mapping is part of the authoritative 95-character table.

## Validation status and next stages

The 95-character map, US QWERTY keymap and independent v1 FSM have passed their host tests. The FSM has 37 passing checks. QEMU integration verified the reference path through evdev and uinput, including chord cancellation and recovery; see [QEMU FSM integration evidence](../evidence/input/2026-11-04-qemu-fsm-integration.md).

Next, port the approved behavior into the kernel input handler and kernel virtual keyboard, and validate those components in QEMU. Physical Zebronics/Raspberry Pi work is deliberately the final hardware stage.
## Evdev and virtual keyboard mode

The decoder can read mouse button events from a Linux evdev device.

Run the decoder using:

```bash
./tools/morse-ref-decoder --map config/morse_map.csv --evdev DEVICE
