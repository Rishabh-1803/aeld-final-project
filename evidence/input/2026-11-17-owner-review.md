# Owner laptop review — 17 November 2026

Source: terminal transcript supplied by Rishabh Shah after following the review
walkthrough. Reviewed against the implementation and expected results.
Source checkout: d095efd19f2a455176edba9303ce72ac0fa98ba9; main matched
origin/main and the working tree was clean before and after the session.

| Check | Observed result |
|---|---|
| Host decoder/map | All 95 characters; empty/unmapped commit and reset checks passed |
| US-QWERTY keymap | All 95 printable characters and invalid argument checks passed |
| FSM | All 52 checks and 58 shared vectors passed; generated map matched |
| Interactive decoder | A (ASCII 65), empty commit, reset, ten-dot rejection and recovery passed |
| Userspace build | Up to date; exit 0 (not a fresh compilation in this session) |
| Config validation | Valid configuration exit 0; negative, negative-wrap, 99 ms, unknown key and execute mode each rejected with exit 2 |
| QEMU boot | Snapshot mode; ARM64, Linux 6.6.151-yocto-standard; rootfs timestamp 20261117083010 |
| Driver | Loaded; /dev/morse_hid root:root mode 0600; tablet connected |
| Character device | ABI 1, event size 104, capacity 128; all executed tests passed; exit 0 |
| Invalid read-pointer case | SKIPPED: no queued event; no pass claimed for this case in this session |
| Daemon lifecycle | Stop/start succeeded; HUD current, daemon running, driver ready |
| Emulated button path | DOT/DASH/COMMIT decoded .- as ASCII 65; driver logged virtual-keyboard emission |
| Telemetry | 6 received, 6 processed, no drops/overflows/loss notifications; watch timestamps advanced |
| Shutdown | Normal poweroff; QEMU cleaned up; final Git status clean |

The initial event_type=9 record represents startup synchronization. After the
daemon reopened with no new event, event_id/type=0 was expected. The subsequent
commit appeared as event_type=3, ascii=65, sequence=.-. The actual driver status
string is `ready`, correcting the walkthrough's illustrative `available`.

Boot messages about the out-of-tree module and the already configured network
route did not prevent this boot or these tests. No test failure was observed.

This review does not repeat the full stress/hotplug suite, capture new evtest
keyboard events, prove actual application text insertion, or validate physical
Pi, Wi-Fi or receiver behavior. Earlier dedicated QEMU acceptance reports remain
separate evidence. Power execution remains disabled.
