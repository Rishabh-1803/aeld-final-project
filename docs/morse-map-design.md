# MorseMouse 95-character map — draft v0.1

**Status:** Candidate mapping for local validation; not yet frozen for the kernel FSM.
**Date:** 2 November 2026
**Scope:** Printable 7-bit ASCII, decimal 32–126, declared US QWERTY layout.

## Goals

- Map every printable ASCII value from 32 through 126 exactly once.
- Keep standard International Morse sequences for A–Z (case-insensitive letter code represented by uppercase ASCII), digits, and the selected ITU punctuation marks.
- Make uppercase and lowercase ASCII distinct.
- Keep all extra symbols explicit and collision-free.
- Store the actual mapping in `config/morse_map.csv`; that CSV is the authoritative table.

## Mapping conventions

### Standard characters

A–Z and 0–9 use International Morse as specified in ITU-R Recommendation M.1677-1, Annex 1, sections 1.1.1 and 1.1.2. For the US QWERTY printable-ASCII acceptance set, the following 13 punctuation symbols use the ITU table in section 1.1.3:

`!` is not in this standard punctuation table; the listed symbols are:

| Character | Morse |
|---|---|
| `.` | `.-.-.-` |
| `,` | `--..--` |
| `:` | `---...` |
| `?` | `..--..` |
| `'` | `.----.` |
| `-` | `-....-` |
| `/` | `-..-.` |
| `(` | `-.--.` |
| `)` | `-.--.-` |
| `"` | `.-..-.` |
| `=` | `-...-` |
| `+` | `.-.-.` |
| `@` | `.--.-.` |

The official recommendation's letters are printed lowercase; this project assigns the same standard sequences to uppercase ASCII and creates separate lowercase extension sequences below.

### Lowercase extension

- Prefix: `--.-`
- Append the standard Morse sequence for the corresponding uppercase letter.
- Example: `A` = `.-`; `a` = `--.-.-`.
- Example: `E` = `.`; `e` = `--.-.`.

This is a MorseMouse-specific convention, not International Morse. It is designed to retain the familiar A–Z patterns while making lowercase ASCII unique.

### Space

- ASCII 32 (`SPACE`) is assigned the MorseMouse-specific sequence `------` in this draft.
- This is a direct sequence mapping for the table. The proposed middle-button-plus-wheel word-space gesture has not been confirmed and is not assumed by this map.

### Remaining punctuation and symbols

The 19 printable ASCII punctuation/symbol characters not listed in the selected ITU punctuation set use a project extension:

- Prefix: `..-.`
- Append the five-symbol suffix assigned in ascending ASCII order in the CSV.
- The suffixes are unique 5-bit dot/dash patterns, so all extended symbols are unique.
- This extension is project-defined and is not represented as standard International Morse.

## Character boundaries are separate from mapping

The table maps one complete Morse sequence to one ASCII byte. It does not define how the user signals the end of a character.

International Morse relies on timed spacing between elements and characters. MorseMouse currently proposes an explicit middle-button commit action, but that action and its precedence with other gestures are still pending confirmation. The reference decoder must therefore be tested with explicit `COMMIT` events; it must not infer a character boundary from an arbitrary idle timeout until timing behavior is separately specified.

The mapping is unique as an exact lookup table, but it is not claimed to be prefix-free. Prefix-free coding is not required when the input protocol has an unambiguous explicit character-commit event.

## Validation

Run from the repository root:

```bash
python3 tools/validate_morse_map.py
```

The validator checks:

1. Exactly 95 rows cover ASCII 32–126.
2. Each row's decimal value, hexadecimal value, and character agree.
3. Every Morse sequence contains only `.` and `-`.
4. Every ASCII value has a non-empty label and encoding description.
5. All 95 Morse sequences are unique.
6. Standard mappings in the agreed base set are unchanged.
7. Uppercase and lowercase entries use the documented extension and remain distinct.
8. Forward and reverse lookup round-trip for all 95 values.

## Freeze criteria

Do not call this mapping final until:

- Rishabh reviews the uppercase/lowercase convention and maximum sequence length;
- all extra punctuation extensions are accepted;
- the space entry is reconciled with the final word-space gesture decision;
- the character commit gesture is frozen;
- reference-decoder tests pass against the CSV.

## Reference

ITU-R Recommendation M.1677-1 (10/2009), *International Morse code*, Annex 1, Part I, section 1.1. Official ITU page: https://www.itu.int/rec/R-REC-M.1677-1-200910-I/ .
