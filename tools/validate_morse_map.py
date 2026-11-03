#!/usr/bin/env python3
"""Validate the MorseMouse printable-ASCII mapping CSV."""

from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path

EXPECTED_CODES = {
    32: "------",
    65: ".-", 66: "-...", 67: "-.-.", 68: "-..", 69: ".",
    70: "..-.", 71: "--.", 72: "....", 73: "..", 74: ".---",
    75: "-.-", 76: ".-..", 77: "--", 78: "-.", 79: "---",
    80: ".--.", 81: "--.-", 82: ".-.", 83: "...", 84: "-",
    85: "..-", 86: "...-", 87: ".--", 88: "-..-", 89: "-.--",
    90: "--..",
    48: "-----", 49: ".----", 50: "..---", 51: "...--", 52: "....-",
    53: ".....", 54: "-....", 55: "--...", 56: "---..", 57: "----.",
    46: ".-.-.-", 44: "--..--", 58: "---...", 63: "..--..",
    39: ".----.", 45: "-....-", 47: "-..-.", 40: "-.--.",
    41: "-.--.-", 34: ".-..-.", 61: "-...-", 43: ".-.-.", 64: ".--.-.",
}

REQUIRED_COLUMNS = ["ascii_decimal", "ascii_hex", "character", "label", "morse", "encoding"]


def fail(message: str, errors: list[str]) -> None:
    errors.append(message)


def validate(path: Path) -> int:
    errors: list[str] = []
    try:
        with path.open("r", newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            if reader.fieldnames != REQUIRED_COLUMNS:
                print("FAIL: unexpected CSV columns")
                print(f"Expected: {REQUIRED_COLUMNS}")
                print(f"Found:    {reader.fieldnames}")
                return 1
            rows = list(reader)
    except (OSError, UnicodeError, csv.Error) as exc:
        print(f"FAIL: cannot read mapping file: {exc}")
        return 1

    by_ascii: dict[int, dict[str, str]] = {}
    code_to_ascii: dict[str, list[int]] = {}

    for line_number, row in enumerate(rows, start=2):
        try:
            code = int(row["ascii_decimal"], 10)
        except (TypeError, ValueError):
            fail(f"line {line_number}: invalid ASCII decimal value {row.get('ascii_decimal')!r}", errors)
            continue

        if code in by_ascii:
            fail(f"line {line_number}: duplicate ASCII decimal value {code}", errors)
        else:
            by_ascii[code] = row

        if not 32 <= code <= 126:
            fail(f"line {line_number}: ASCII value {code} is outside printable range 32..126", errors)
            continue

        expected_char = chr(code)
        if row["character"] != expected_char:
            fail(
                f"line {line_number}: ASCII {code} should contain {expected_char!r}, "
                f"found {row['character']!r}", errors
            )

        expected_hex = f"0x{code:02X}"
        if row["ascii_hex"] != expected_hex:
            fail(f"line {line_number}: ASCII {code} should use hex {expected_hex}", errors)

        morse = row["morse"]
        if not morse:
            fail(f"line {line_number}: empty Morse sequence for ASCII {code}", errors)
        elif any(symbol not in ".-" for symbol in morse):
            fail(f"line {line_number}: invalid Morse sequence {morse!r}; only '.' and '-' are allowed", errors)
        else:
            code_to_ascii.setdefault(morse, []).append(code)

        if not row["label"].strip():
            fail(f"line {line_number}: empty label for ASCII {code}", errors)
        if not row["encoding"].strip():
            fail(f"line {line_number}: empty encoding/source description for ASCII {code}", errors)

    expected_ascii = set(range(32, 127))
    actual_ascii = set(by_ascii)
    missing = sorted(expected_ascii - actual_ascii)
    unexpected = sorted(actual_ascii - expected_ascii)
    if missing:
        fail(f"missing printable ASCII values: {missing}", errors)
    if unexpected:
        fail(f"unexpected ASCII values: {unexpected}", errors)

    for morse, codes in sorted(code_to_ascii.items()):
        if len(codes) > 1:
            fail(f"duplicate Morse sequence {morse!r} assigned to ASCII values {codes}", errors)

    for ascii_code, expected_morse in EXPECTED_CODES.items():
        row = by_ascii.get(ascii_code)
        if row is not None and row["morse"] != expected_morse:
            fail(
                f"standard mapping changed for ASCII {ascii_code}: expected {expected_morse!r}, "
                f"found {row['morse']!r}", errors
            )

    # Enforce the documented project-extension grammar so the table and design note agree.
    for upper in range(ord("A"), ord("Z") + 1):
        lower = upper + 32
        upper_row = by_ascii.get(upper)
        lower_row = by_ascii.get(lower)
        if upper_row and lower_row:
            expected_lower = "--.-" + upper_row["morse"]
            if lower_row["morse"] != expected_lower:
                fail(
                    f"lowercase extension mismatch for {chr(lower)!r}: "
                    f"expected {expected_lower!r}, found {lower_row['morse']!r}", errors
                )

    # A reverse lookup is safe only when every sequence is unique.
    if not errors and len(code_to_ascii) == 95:
        reverse = {morse: codes[0] for morse, codes in code_to_ascii.items()}
        for code, row in by_ascii.items():
            if reverse.get(row["morse"]) != code:
                fail(f"round-trip lookup failed for ASCII {code}", errors)
                break

    if errors:
        print(f"FAIL: {len(errors)} validation error(s) in {path}")
        for item in errors:
            print(f"  - {item}")
        return 1

    lengths = [len(row["morse"]) for row in by_ascii.values()]
    encoding_counts = Counter(
        "ITU" if row["encoding"].startswith("ITU-R") else "MorseMouse extension"
        for row in by_ascii.values()
    )
    print(f"PASS: {len(by_ascii)} printable ASCII characters mapped (32..126).")
    print(f"PASS: {len(code_to_ascii)} unique Morse sequences; no collisions.")
    print("PASS: all sequences contain only '.' and '-'.")
    print("PASS: standard letters, figures, and selected punctuation match the expected table.")
    print("PASS: uppercase/lowercase pairs are distinct and use the documented lowercase prefix.")
    print("PASS: forward/reverse table lookup round-trips for every printable ASCII character.")
    print(f"Sequence length: min={min(lengths)}, max={max(lengths)}, average={sum(lengths)/len(lengths):.2f} symbols")
    print(f"Mapping sources: {dict(encoding_counts)}")
    print(f"Validated file: {path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    default_path = Path(__file__).resolve().parents[1] / "config" / "morse_map.csv"
    parser.add_argument("csv_path", nargs="?", type=Path, default=default_path)
    args = parser.parse_args()
    return validate(args.csv_path)


if __name__ == "__main__":
    sys.exit(main())
