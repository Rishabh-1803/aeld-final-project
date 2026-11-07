# QEMU Kernel Virtual Keyboard Validation

**Date:** 2026-11-07  
**Issue:** #7 — Implement virtual keyboard event synthesis  
**Implementation commit:** `4a21f49` — Add kernel virtual keyboard output  
**Target:** Yocto Scarthgap / QEMU ARM64 (`qemuarm64`)  
**Kernel:** `6.6.151-yocto-standard`  
**Kernel module:** `hid_morse_kbd` version `0.3`

## Scope and test environment

The kernel input handler registers a separate Linux `input_dev` named `MorseMouse Virtual Keyboard`. It exposes keyboard events only and is distinct from the physical QEMU USB Tablet input device used by the MorseMouse handler.

The shared US-QWERTY keymap covers all 95 printable ASCII characters (ASCII 32–126). Morse characters are emitted only after an explicit middle-button COMMIT.

QEMU monitor input was used to inject left-button (DOT), right-button (DASH), and middle-button (COMMIT) transitions into the emulated USB Tablet. Keyboard output was captured from the virtual keyboard event node using `evtest`.

## Build and host regression results

The detached Yocto build ran:

```sh
bitbake hid-morse-kbd morse-ref-decoder core-image-morse-dev
```

**Result:** successful; 4,193 tasks attempted and all tasks succeeded. The build reported a non-fatal host warning that `CONFIG_TASK_IO_ACCOUNTING` was unset.

Host validation ran:

```sh
make -C tools clean test
```

Results:

- Decoder: all 95 printable ASCII mappings decoded and committed.
- Empty COMMIT, invalid sequence consumption, and RESET behavior: passed.
- Keymap: all 95 printable ASCII characters mapped correctly.
- Invalid ASCII values and NULL output pointer handling: passed.
- C FSM unit tests: 52 checks passed.
- Generated kernel map: all 95 CSV entries matched.
- Shared FSM vectors: all 58 vectors passed, including DOT/DASH, duplicate/repeat events, duplicate middle COMMIT, chord cancellation, lockout/recovery, reset/synchronization recovery, and the valid `.-` to uppercase `A` vector.

## QEMU runtime event tests

The virtual keyboard was registered and visible in `/proc/bus/input/devices`:

```text
N: Name="MorseMouse Virtual Keyboard"
P: Phys=morse-mouse/virtual-keyboard0
H: Handlers=kbd event2
B: EV=3
```

The virtual keyboard's event node was `/dev/input/event2` during these tests. The event number is assigned dynamically and can change on a later boot or registration.

### Character matrix

| Character | Morse sequence | ASCII | Observed key events | Result |
|---|---|---:|---|---|
| `A` | `.-` | 65 | `KEY_LEFTSHIFT` + `KEY_A` | PASS |
| `a` | `--.-.-` | 97 | `KEY_A`, no Shift | PASS |
| `1` | `.----` | 49 | `KEY_1`, no Shift | PASS |
| `!` | `..-......` | 33 | `KEY_LEFTSHIFT` + `KEY_1` | PASS |
| `?` | `..--..` | 63 | `KEY_LEFTSHIFT` + `KEY_SLASH` | PASS |

### Representative evtest traces

Uppercase `A`:

```text
EV_KEY KEY_LEFTSHIFT value 1
EV_KEY KEY_A        value 1
SYN_REPORT
EV_KEY KEY_A        value 0
EV_KEY KEY_LEFTSHIFT value 0
SYN_REPORT
```

Lowercase `a`:

```text
EV_KEY KEY_A value 1
SYN_REPORT
EV_KEY KEY_A value 0
SYN_REPORT
```

Digit `1`:

```text
EV_KEY KEY_1 value 1
SYN_REPORT
EV_KEY KEY_1 value 0
SYN_REPORT
```

Exclamation mark `!`:

```text
EV_KEY KEY_LEFTSHIFT value 1
EV_KEY KEY_1        value 1
SYN_REPORT
EV_KEY KEY_1        value 0
EV_KEY KEY_LEFTSHIFT value 0
SYN_REPORT
```

Question mark `?`:

```text
EV_KEY KEY_LEFTSHIFT value 1
EV_KEY KEY_SLASH     value 1
SYN_REPORT
EV_KEY KEY_SLASH     value 0
EV_KEY KEY_LEFTSHIFT value 0
SYN_REPORT
```

A further uppercase `A` test passed after unloading and reloading the module, confirming that keyboard output worked after re-registration.

## Module lifecycle and isolation

The module unload/reload test confirmed:

1. The module unloaded successfully.
2. `MorseMouse Virtual Keyboard` disappeared from `/proc/bus/input/devices`.
3. The QEMU USB Tablet and QEMU USB Keyboard remained enumerated.
4. Reloading the module re-registered the virtual keyboard and reconnected the handler to `QEMU QEMU USB Tablet`.
5. The re-registered virtual keyboard was visible again and emitted uppercase `A` correctly.

The physical QEMU keyboard remained separately enumerated; the MorseMouse input handler connected to the QEMU USB Tablet. This validation checked device separation and preservation of device registration, not a separate physical-keyboard keystroke demonstration.

At unload, the driver reported:

```text
received=96
processed=96
queue_overflows=0
dropped=0
sequence_overflows=0
commits_valid=5
commits_empty=0
commits_invalid=1
```

The one invalid COMMIT was produced by an earlier deliberately incomplete/mis-sequenced punctuation attempt during test setup. The driver consumed the invalid sequence; the corrected `!` sequence subsequently passed. There were no queue overflows, dropped events, or sequence overflows.

The post-test kernel log search for `BUG:`, `Oops:`, kernel panic, use-after-free, and general protection fault returned no matches.

## Result and limitations

**Issue #7 QEMU acceptance: PASS.** The kernel virtual keyboard builds, registers, maps the tested printable characters to the expected Linux key events, handles Shift ordering, releases keys, and survives module unload/reload.

This is QEMU validation only. No physical Raspberry Pi 4 or Zebronics mouse test is claimed here; physical hardware validation remains deferred to the integration/hardware work tracked by issue #11 (and the dedicated hardware validation issue).
