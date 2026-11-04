# MorseMouse input grammar

**Status:** v1 baseline approved — 4 November 2026  
**Decision owner:** Rishabh Shah  
**Scope:** Initial discrete Morse input mode

## 1. Confirmed encoding decision

MorseMouse uses **button identity**, not press duration, to distinguish Morse elements.

| Physical control | Linux input event | Morse element | Meaning |
|---|---|---|---|
| Left mouse button | `BTN_LEFT` (272) | `.` | DOT |
| Right mouse button | `BTN_RIGHT` (273) | `-` | DASH |

QEMU runtime tests confirm these event mappings for the emulated USB tablet. The physical Zebronics PULSE receiver remains to be inspected during the final hardware-validation stage.

### Event interpretation

- A button-down transition (value `1`) contributes one Morse element.
- The corresponding button-up transition (value `0`) adds no symbol and rearms the button.
- Press duration has no Morse meaning.
- A held button must not generate repeated Morse elements.
- Kernel key-repeat events are ignored.

## 2. Approved v1 input grammar

| Input | v1 action | Status |
|---|---|---|
| Left-button press | Append one DOT (`.`) | Approved |
| Right-button press | Append one DASH (`-`) | Approved |
| Left/right release | No symbol; rearm the button | Approved |
| Repeated press while held | Ignore | Approved |
| Middle-button press (value 1) | Commit current Morse sequence as one character | Approved |
| Middle-button release | No additional commit | Approved |
| Empty COMMIT | Ignore | Approved |
| Invalid/unmapped COMMIT | Reject and clear sequence | Approved |
| Left and right held simultaneously | Clear sequence and enter chord lockout | Approved |
| During chord lockout | Ignore input until both primary buttons are released | Approved |
| Wheel up/down | Ignore input | Disabled in v1 |
| Middle button held + wheel | Ignore input | Disabled in v1 |

### 2.1 Simultaneous left/right input

If the left and right buttons become held simultaneously:

1. Clear the current Morse sequence.
2. Enter chord lockout.
3. Ignore input while either primary button remains held.
4. Exit lockout only after both primary buttons are released.
5. Accept new input after lockout ends.

This behavior is implemented in the independent userspace FSM and verified in QEMU.

## 3. Timing and filtering

The following rules are fixed for v1:

- Press duration does not distinguish DOT from DASH.
- No automatic character timeout.
- No long-press interpretation.
- No arbitrary debounce interval until hardware measurements justify one.
- Explicit middle-button COMMIT is required to finish a character.
- Chord detection is based on overlapping button-held states, not a configurable time window.

Hardware measurements may justify future filtering changes, but must not silently change the approved v1 grammar.

## 4. Initial mode and mapping scope

- Use discrete DOT/DASH entry; Iambic keying is excluded from v1.
- Target printable ASCII decimal 32–126 (95 characters) under the declared US QWERTY acceptance layout.
- Standard Morse letters and digits are used directly.
- MorseMouse-specific extensions for lowercase, punctuation and SPACE are documented in the authoritative CSV.
- `config/morse_map.csv` is the single authoritative mapping source.
- The validator checks missing characters, duplicate sequences and invalid symbols.

The map contains 95 unique Morse sequences and 385 prefix relationships. Explicit COMMIT is therefore required; inactivity-based autocommit is not used.

## 5. Required tests

The reference decoder and FSM tests cover:

1. Left down/up produces one DOT.
2. Right down/up produces one DASH.
3. Short and long holds of the same button produce the same single element.
4. A continuous hold does not produce repeated elements.
5. A new press after release produces a new element.
6. Release events do not append symbols.
7. Empty middle-button COMMIT is ignored.
8. A valid sequence is consumed after COMMIT.
9. An invalid sequence is rejected and consumed.
10. A left/right overlap clears the current sequence.
11. Input remains ignored during chord lockout.
12. Input resumes only after both primary buttons are released.
13. All 95 printable ASCII characters map uniquely.
14. QEMU results and physical Zebronics results are recorded separately.

The independent FSM test executable reports 37 passing checks. The reference decoder and keymap test suites also pass.

## 6. Hardware-validation boundary

QEMU verified button events from its emulated USB tablet and the complete userspace evdev → Morse FSM → decoder → uinput keyboard path. QEMU results do not prove that the Zebronics PULSE receiver exposes identical capabilities or event behavior on Raspberry Pi 4.

Actual receiver VID/PID, HID descriptors, physical button events and Raspberry Pi behavior are reserved for the final hardware stage.

## 7. Evidence

- [QEMU evdev runtime-event evidence](../evidence/input/2026-11-02-qemu-evdev-runtime-events.md)
- [QEMU uinput virtual-keyboard self-test](../evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md)
- [QEMU Morse-to-uinput reference decoder validation](../evidence/input/2026-11-03-qemu-uinput-keyboard.md)
- [QEMU FSM, chord lockout, recovery and uinput integration — 4 November 2026](../evidence/input/2026-11-04-qemu-fsm-integration.md)
