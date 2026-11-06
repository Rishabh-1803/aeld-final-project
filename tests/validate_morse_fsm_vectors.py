#!/usr/bin/env python3

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VECTOR_FILE = ROOT / "tests" / "vectors" / "morse_fsm_v1.csv"
MAP_FILE = ROOT / "config" / "morse_map.csv"


def load_map():
    with MAP_FILE.open("r", encoding="utf-8", newline="") as source:
        rows = csv.DictReader(source)
        return {row["morse"]: row["character"] for row in rows}


class FSM:
    def __init__(self):
        self.left = False
        self.right = False
        self.middle = False
        self.lockout = False
        self.sequence = ""

    def reset(self):
        self.left = False
        self.right = False
        self.middle = False
        self.lockout = False
        self.sequence = ""

    def resync_begin(self):
        self.left = False
        self.right = False
        self.middle = False
        self.lockout = True
        self.sequence = ""

    def sync(self, left, right, middle):
        was_locked = self.lockout
        self.left = left
        self.right = right
        self.middle = middle

        if was_locked:
            self.lockout = left or right
        else:
            self.lockout = left and right

    def handle(self, button, value):
        if button == "MIDDLE":
            if value == 0:
                self.middle = False
                return "NONE"

            if value != 1:
                return "IGNORE"

            if self.middle:
                return "IGNORE"

            self.middle = True

            if self.lockout:
                return "IGNORE"

            return "COMMIT"

        if button == "LEFT":
            pressed = self.left
            opposite = self.right
            symbol = "."
        elif button == "RIGHT":
            pressed = self.right
            opposite = self.left
            symbol = "-"
        else:
            return "IGNORE"

        if value == 0:
            if button == "LEFT":
                self.left = False
            else:
                self.right = False

            if self.lockout and not self.left and not self.right:
                self.lockout = False

            return "NONE"

        if value == 2:
            return "IGNORE"

        if value != 1 or pressed:
            return "IGNORE"

        if button == "LEFT":
            self.left = True
        else:
            self.right = True

        if self.lockout:
            return "IGNORE"

        if opposite:
            self.lockout = True
            return "CANCEL"

        self.sequence += symbol
        return "DOT" if symbol == "." else "DASH"

def main():
    morse_map = load_map()
    fsm = FSM()
    checks = 0
    failures = []

    with VECTOR_FILE.open("r", encoding="utf-8", newline="") as source:
        vectors = list(csv.DictReader(source))

    for row in vectors:
        button = row["button"]
        value = row["value"]
        decoded = ""

        if button == "RESET":
            fsm.reset()
            action = "RESET"

        elif button == "RESYNC":
            if value != "BEGIN":
                failures.append(f"{row['case']}:{row['step']}: invalid RESYNC vector")
                continue

            fsm.resync_begin()
            action = "RESYNC_BEGIN"

        elif button == "SYNC":
            states = {
                "BOTH_UP": (False, False, False),
                "LEFT_ONLY": (True, False, False),
                "RIGHT_ONLY": (False, True, False),
                "BOTH_DOWN": (True, True, False),
                "MIDDLE_ONLY": (False, False, True),
                "LEFT_MIDDLE": (True, False, True),
                "RIGHT_MIDDLE": (False, True, True),
                "ALL_DOWN": (True, True, True),
            }

            if value not in states:
                failures.append(f"{row['case']}:{row['step']}: invalid SYNC vector")
                continue

            fsm.sync(*states[value])
            action = "SYNC"

        else:
            values = {
                "RELEASE": 0,
                "PRESS": 1,
                "REPEAT": 2,
                "INVALID": 7,
            }

            if value not in values:
                failures.append(f"{row['case']}:{row['step']}: invalid value")
                continue

            action = fsm.handle(button, values[value])

            if action == "CANCEL":
                fsm.sequence = ""

            elif action == "COMMIT":
                if not fsm.sequence:
                    decoded = "EMPTY"
                elif fsm.sequence in morse_map:
                    decoded = morse_map[fsm.sequence]
                else:
                    decoded = "INVALID"

                fsm.sequence = ""

        checks += 1

        actual = {
            "expected_action": action,
            "expected_decode": decoded,
            "left_pressed": str(int(fsm.left)),
            "right_pressed": str(int(fsm.right)),
            "middle_pressed": str(int(fsm.middle)),
            "chord_lockout": str(int(fsm.lockout)),
            "sequence": fsm.sequence,
        }

        for field, value in actual.items():
            if row[field] != value:
                failures.append(
                    f"{row['case']}:{row['step']} {field}: "
                    f"expected vector={row[field]!r}, actual={value!r}"
                )

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        print(f"FAILED: {len(failures)} mismatches across {checks} vectors.")
        return 1

    print(f"PASS: all {checks} shared Morse FSM vectors passed.")
    print("PASS: DOT/DASH, duplicate/repeat, duplicate middle COMMIT, chord lockout and recovery.")
    print("PASS: reset and synchronization recovery.")
    print("PASS: the valid '.-' vector decodes to uppercase A.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
