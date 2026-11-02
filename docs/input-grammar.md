# MorseMouse input grammar

**Status:** Partial decision recorded — 2 November 2026  
**Decision owner:** Rishabh Shah  
**Scope:** Initial discrete Morse input mode

## 1. Confirmed encoding decision

MorseMouse will use **button identity**, not press duration, to distinguish Morse elements.

| Physical control | Linux input event | Morse element | Meaning |
|---|---|---|---|
| Left mouse button | `BTN_LEFT` (expected code 272) | `.` | Dit |
| Right mouse button | `BTN_RIGHT` (expected code 273) | `-` | Dah |

The event codes above are the standard Linux input codes. They still need to be confirmed on the physical Zebronics PULSE receiver; current runtime evidence is from the QEMU emulated tablet only.

### Event interpretation

- A button-down transition (event value `1`) contributes exactly one Morse element.
- The corresponding button-up transition (event value `0`) ends the physical press; it does not add another element.
- Press duration has no Morse meaning. A short and a long left-button press both mean one dit; a short and a long right-button press both mean one dah.
- A held button must not generate repeated Morse elements. Only a new down transition after a release can contribute another element.
- Kernel repeat events, if ever observed, must not be interpreted as additional Morse elements.

This is the selected baseline. Do not implement duration thresholds for dit-versus-dah classification.

## 2. Remaining grammar decisions

The button-identity choice does **not** yet freeze the complete interaction grammar. The following controls remain candidates and must be confirmed before the FSM is considered frozen:

| Candidate control | Candidate action | Status |
|---|---|---|
| Middle-button click | Commit the current Morse sequence as one character | Proposed; not confirmed |
| Wheel up/down | Select previous/next character layer | Proposed; not confirmed |
| Middle button held + wheel | Insert a word space | Proposed; not confirmed |
| Left and right held simultaneously | Treat as an invalid/reserved chord and discard the overlapping input | Proposed safety rule; not confirmed |

The final grammar must define precedence for middle-button and wheel combinations, chord handling, invalid sequences, reset/cancel behavior, and layer count.

## 3. Timing and filtering

The following are deliberately **not frozen**:

- Debounce/filter interval.
- Chord-detection interval and exact overlap semantics.
- Whether a character can be committed only explicitly or also by an inactivity timeout.
- Any long-press gesture used for controls other than Morse element entry.

The earlier duration-based dit/dah timing suggestions are not applicable to the selected button-identity encoding. Timing may still be used for noise filtering or gesture recognition, but never to decide whether a left/right press means dit or dah.

## 4. Initial mode and scope

- Start with discrete dit/dah entry; do not add Iambic keying to the first implementation.
- Target printable ASCII decimal 32–126 (95 characters) using a declared US QWERTY acceptance layout.
- Standard Morse letters and digits can be used directly. Any extensions needed for punctuation, symbols, or other printable characters must be explicit, deterministic, documented, and collision-free.
- The mapping must be stored in one authoritative table and validated for duplicate codes, missing characters, and invalid sequences.

## 5. Required tests

Once the remaining grammar is agreed and implemented, the reference decoder and driver tests must cover at least:

1. Left down/up produces one dit.
2. Right down/up produces one dah.
3. Short and long holds of the same button produce the same single element.
4. A continuous hold does not produce repeated elements.
5. A new press after release produces a new element.
6. Release events do not append elements.
7. Simultaneous/chord behavior follows the final documented rule.
8. Invalid/unmapped sequences are handled deterministically.
9. All 95 printable ASCII characters map uniquely under the declared layout.
10. QEMU-generated tests and physical Zebronics receiver tests are recorded separately.

## 6. Hardware-validation boundary

QEMU has verified delivery of left/right/middle button and wheel events from its emulated USB tablet. QEMU has also verified the independent `uinput` virtual-keyboard path. Neither result proves that the Zebronics PULSE mouse and receiver expose identical events on Raspberry Pi 4. Physical capabilities and event delivery remain a separate acceptance item.

See [QEMU evdev runtime-event evidence](../evidence/input/2026-11-02-qemu-evdev-runtime-events.md) and [uinput virtual-keyboard self-test](../evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md).
