#!/usr/bin/env python3

import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SOURCE = ROOT / "config" / "morse_map.csv"
OUTPUT = ROOT / "driver" / "morse_map_kernel.h"

REQUIRED_FIELDS = [
    "ascii_decimal",
    "ascii_hex",
    "character",
    "label",
    "morse",
    "encoding",
]

ASCII_MIN = 32
ASCII_MAX = 126
MAX_MORSE_LEN = 63


def generate():
    rows = []
    seen_ascii = set()
    seen_codes = set()

    with SOURCE.open("r", encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)

        if reader.fieldnames != REQUIRED_FIELDS:
            raise ValueError("Unexpected Morse CSV header.")

        for line_number, row in enumerate(reader, start=2):
            ascii_value = int(row["ascii_decimal"])
            character = row["character"]
            code = row["morse"]
            label = row["label"]

            if not ASCII_MIN <= ascii_value <= ASCII_MAX:
                raise ValueError(f"Invalid ASCII value at line {line_number}")

            if len(character) != 1 or ord(character) != ascii_value:
                raise ValueError(f"Character mismatch at line {line_number}")

            if not code or len(code) > MAX_MORSE_LEN:
                raise ValueError(f"Invalid code length at line {line_number}")

            if any(symbol not in ".-" for symbol in code):
                raise ValueError(f"Invalid Morse symbols at line {line_number}")

            if ascii_value in seen_ascii:
                raise ValueError(f"Duplicate ASCII value: {ascii_value}")

            if code in seen_codes:
                raise ValueError(f"Duplicate Morse sequence: {code}")

            seen_ascii.add(ascii_value)
            seen_codes.add(code)
            rows.append((ascii_value, code, label))

    expected = set(range(ASCII_MIN, ASCII_MAX + 1))

    if seen_ascii != expected:
        raise ValueError("The map must contain all 95 printable ASCII values.")

    rows.sort()

    lines = [
        "/* SPDX-License-Identifier: GPL-2.0-only */",
        "/* AUTO-GENERATED from config/morse_map.csv. DO NOT EDIT. */",
        "",
        "static const struct morse_map_entry morse_map_kernel[] = {",
    ]

    for ascii_value, code, label in rows:
        lines.append(
            f'    {{ "{code}", {ascii_value} }}, /* {label} */'
        )

    lines.extend([
        "};",
        "",
        "#define MORSE_MAP_KERNEL_COUNT \\",
        "    (sizeof(morse_map_kernel) / sizeof(morse_map_kernel[0]))",
        "",
    ])

    return "\n".join(lines), rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    try:
        content, rows = generate()
    except (OSError, ValueError, KeyError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != content:
            print("FAIL: Kernel Morse map is out of date.")
            return 1

        print(f"PASS: Kernel Morse map matches all {len(rows)} CSV entries.")
        return 0

    OUTPUT.write_text(content, encoding="utf-8")
    maximum = max(len(code) for _, code, _ in rows)

    print(f"Generated {OUTPUT}")
    print(f"Entries: {len(rows)}")
    print(f"Maximum sequence length: {maximum}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
