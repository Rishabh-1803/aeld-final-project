# Physical session results sheet

Prepared checklist only. No physical result is recorded yet. Complete this
sheet from actual observations and save sanitized logs under evidence/hardware/.
Never include Wi-Fi passwords, private SSH keys or device serial numbers.

Record: session date, source commit, image SHA-256, board model/RAM, kernel,
module version/vermagic, receiver VID/PID and input-device names. Identify the
card before writing; retain a separate keyboard for recovery.

| Test | Result (PENDING/PASS/FAIL/SKIP) | Actual observation / log |
|---|---|---|
| Image written and verified on identified microSD | PENDING | |
| Pi boot, model, kernel and module match image | PENDING | |
| Wi-Fi association and SSH connection | PENDING | |
| Receiver Linux USB/HID identity and capabilities | PENDING | |
| Left/right/middle press and release events | PENDING | |
| Exact receiver selection; unrelated devices unaffected | PENDING | |
| Character-device self-test; record every skip | PENDING | |
| Virtual keyboard events and real text entry | PENDING | |
| Empty/invalid commit, chord lockout and recovery | PENDING | |
| Grab enabled only on confirmed receiver; released on unload | PENDING | |
| Unplug/replug and service/driver recovery | PENDING | |
| HUD current/missing/stale behavior; no stuck modifiers | PENDING | |
| Demonstration recorded and public link checked | PENDING | |
| Logs copied; service stopped; normal shutdown | PENDING | |

## Demonstration character card

Left = dot; right = dash; release each button between presses. Middle commits.
These codes come from config/morse_map.csv, including the project's extensions;
they are not all standard international Morse codes.

| Character | Sequence before middle-button COMMIT |
|---|---|
| A | .- |
| a | --.-.- |
| 1 | .---- |
| ! | ..-...... |

Use an actual local Linux text-input consumer on the Pi. A serial/SSH terminal
will not automatically display virtual keyboard input generated on another
console. Record both input events and visible local text entry. If the minimal
image cannot support the chosen display/input path, resolve that during setup
before recording the final demonstration.

For detailed procedure see [physical-test-session.md](physical-test-session.md).
Power gestures/execution are excluded until after submission.
