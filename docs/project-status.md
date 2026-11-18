# Project status — 17 November 2026

Sprint 1 is complete and tagged. Issues #5–#8 are complete with QEMU evidence.
Daemon and read-only HUD implementation and QEMU acceptance are complete;
GitHub issue updates accompany this checkpoint. Final integration acceptance,
physical hardware and final demonstration remain outstanding.

## Verified implementation

- Yocto Scarthgap 5.0.21; pinned upstream revisions in manifest/source-pins.md.
- QEMU ARM64 images, reference decoder, 95-character Morse map and keymap.
- Kernel handler, bounded queue/shared FSM, lifecycle/grab and stress evidence.
- Kernel virtual keyboard; November 7 event-level evidence.
- ABI v1 character device; November 17 read/poll/ioctl and overflow evidence.
- Diagnostic daemon, SysV service and read-only terminal HUD; November 17
  lifecycle, missing-driver recovery, stale/missing telemetry and negative tests.
- Host regression: 95 mappings, 52 FSM checks, 58 shared vectors.

## Remaining gates

1. Issue #11 Phase A is accepted with the integrated service-image stress/recovery
   evidence. Preserve that software checkpoint while preparing hardware.
2. Use the built Pi 4 image for the borrowed-board session; write the card and inspect the
   owner's Zebronics PULSE receiver and validate actual hardware separately.
3. Complete final documentation, public-link checks, demonstration video and
   submission evidence. No video URL or hardware result has been invented.

Power execution and shutdown gestures are deferred until after submission by
the owner's November 17 decision. No executable power policy is included.

## Reproduction and evidence

- [Build instructions](build-qemuarm64.md)
- [Diagnostic ABI](character-device.md)
- [Daemon/HUD](daemon-hud.md)
- [Character-device acceptance](../evidence/input/2026-11-17-qemu-character-device.md)
- [Daemon/HUD acceptance](../evidence/input/2026-11-17-qemu-daemon-hud.md)

Build directories and external layers remain outside version control. Physical
tests must not reuse QEMU kernel modules; build against the Pi's actual kernel.


## Raspberry Pi build checkpoint

Production and development Pi 4 images built successfully on November 17
(4,864 tasks). Offline image checks passed; see the Pi build report. Optional
Wi-Fi/key-only SSH provisioning is enabled in the local image, with secrets
excluded from Git. Public/default builds remain credential-free and opt-in
access is disabled. Card writing and physical tests are deferred by the owner.

VM test runners now use QEMU snapshot disks and keep logs under build-qemu/test-results. The verified SSH regression left the base image SHA-256 unchanged. Both public wiki pages returned HTTP 200 with the expected page titles in an unauthenticated check.

Online Coursera enrollment is confirmed. Status and Sprint assignments are synchronized on the public project board. The saved Project-Schedule table is grouped by Sprint: Sprint 1 (3 items), Sprint 2 (4 items), and Sprint 3 (5 items). The grouping was saved through GitHub and verified after reloading the view and through the GitHub API.

Owner review passed: [review report](../evidence/input/2026-11-17-owner-review.md). See [submission readiness](submission-readiness.md) for the remaining gates and [physical results sheet](final-session-results.md) for the borrowed-board session.

November 18 pre-hardware audit: architecture and video navigation updated; queued-event EFAULT coverage confirmed in five passing diagnostic self-test runs. Physical gates and final recording remain pending. Displayed course dates are informational and will be rechecked at submission.
