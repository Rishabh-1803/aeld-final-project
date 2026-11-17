# Rishabh Shah Final Project Video

## Video status

Recording pending physical Raspberry Pi 4/Zebronics acceptance. No public video
URL is available yet. This page is a prepared outline, not a completed demo.
The final video will be at most five minutes and accessible to anyone with its link.

## Contribution and outline

Individual project: Yocto layer/image integration, Morse map/shared FSM, kernel
input handler and virtual keyboard, diagnostic ABI, daemon/HUD, tests and documentation.

| Time | Topic |
|---|---|
| 0:00–0:30 | Goal, three-button grammar and system architecture |
| 0:30–1:15 | Actual board/receiver, boot, module, diagnostic device and HUD |
| 1:15–2:30 | Actual text entry: uppercase, lowercase, digit and punctuation |
| 2:30–3:30 | Invalid/chord cancellation, recovery and physical unplug/replug |
| 3:30–4:15 | ABI diagnostics and clearly labelled QEMU stress evidence |
| 4:15–5:00 | Challenges, lessons, limitations and source links |

## Technical challenges and lessons supported by current evidence

- A non-prefix-free 95-character map needs explicit COMMIT to delimit characters.
- Linux input callbacks require bounded deferred processing; overflow must be
  observable and decoder state recovered rather than silently reused.
- Held-button state at binding and reconnect requires explicit synchronization.
- Diagnostic stream ownership and atomic HUD snapshots separate the daemon from
  read-only clients; restarting the daemon must recover cleanly.
- Snapshot-based QEMU tests preserve the base disk. QEMU cannot establish real
  radio, receiver or Pi compatibility; those results must be captured separately.

Personal narration and physical-session lessons will be finalized after the
actual session. Power gestures/execution are deferred until after submission.

## Evidence and source

- [Source repository](https://github.com/Rishabh-1803/aeld-final-project)
- [Project Overview](Project-Overview)
- [Owner laptop review](https://github.com/Rishabh-1803/aeld-final-project/blob/main/evidence/input/2026-11-17-owner-review.md)
- [Recording plan](https://github.com/Rishabh-1803/aeld-final-project/blob/main/docs/final-demo-plan.md)
