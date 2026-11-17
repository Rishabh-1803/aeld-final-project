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
2. Borrow Raspberry Pi 4 after QEMU acceptance; build its image, inspect the
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
