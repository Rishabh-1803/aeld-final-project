# MorseMouse C reference decoder — draft v0.1

**Status:** Local prototype; not a kernel driver and not yet a frozen input protocol.

## Purpose

This small C program reads the authoritative `config/morse_map.csv` and provides a userspace reference for exact Morse-sequence lookup. It is intended to establish mapping and character-boundary behavior before implementing the Linux input handler or kernel state machine.

It does not read `/dev/input/event*`, capture mouse buttons, emit keyboard events, or implement debounce/chord timing. It accepts abstract `DOT`, `DASH`, `COMMIT`, and `RESET` tokens so that these concerns remain separate.

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

## Design boundary

The mapping contains prefix relationships, so this decoder deliberately requires explicit `COMMIT`. It must not decide a character boundary from a guessed idle timeout. The middle-button commit gesture remains a candidate in `docs/input-grammar.md`; the CLI's `COMMIT` token is only a test abstraction and does not freeze that physical gesture.

The current CSV includes a MorseMouse-specific direct code for ASCII SPACE. Whether space is entered with that code or via a separate word-space gesture remains a grammar decision.

## Next validation stages

1. Review and freeze the 95-character mapping and its extensions.
2. Agree the complete physical gesture grammar and filtering behavior.
3. Add unit tests around the event-to-symbol state machine.
4. Port the same defined behavior into the kernel implementation, with separate QEMU and physical mouse evidence.
