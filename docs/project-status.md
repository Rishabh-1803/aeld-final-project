# Project status

**Snapshot:** 4 November 2026  
**Owner:** Rishabh Shah (solo)  
**Current sprint:** Sprint 1 — QEMU reference implementation; Sprint 2 begins 16 November 2026

## Verified

| Item | Status | Evidence |
|---|---|---|
| WSL 2 / Ubuntu 24.04.5 | Verified | WSL distribution used for builds |
| Poky Scarthgap | Verified | Commit `4fb7016a07507683359ebd620f1951fd5ea44bfa` |
| meta-raspberrypi Scarthgap | Pinned | Commit `018d44467c4f09eb63f8ea5e86e85d755250cf89` |
| meta-openembedded (`meta-oe`) | Pinned | Commit `2db6aece32c7f8ebf6015b468e1b4e21de072e3b` |
| QEMU baseline | Verified | `core-image-minimal` built and booted |
| QEMU guest | Verified | `aarch64`; kernel `6.6.151-yocto-standard` |
| Custom layer | Verified | `meta-morse-mouse` registered with BitBake |
| Project image | Verified | `core-image-morse` built and booted in QEMU |
| Development image | Verified | `core-image-morse-dev` built and booted |
| QEMU evdev | Verified | QEMU tablet left/right/middle and wheel events captured |
| uinput self-test | Verified | Virtual keyboard creation and KEY_A press/release readback |
| Morse map | Verified | 95 printable ASCII entries, 95 unique Morse sequences |
| Mapping validator | Verified | Completeness, uniqueness, symbol and round-trip checks pass |
| Userspace reference decoder | Verified | All 95 mapped characters decode and commit |
| US QWERTY keymap | Verified | All 95 printable ASCII key mappings pass |
| Independent Morse FSM | Verified | 37 FSM checks pass |
| Chord cancellation and lockout | Verified in QEMU | Overlap clears sequence; lockout persists until both primary buttons release |
| Lockout recovery | Verified in QEMU | New Morse input decodes after both buttons are released |
| uinput keyboard output | Verified in QEMU | Two uppercase-A key sequences observed with `evtest` |
| Updated Yocto package | Verified | `morse-ref-decoder` built; binary and CSV present in pkgdata |
| Updated development image | Verified | `core-image-morse-dev` rebuilt on 4 November 2026 |

## Current build configuration

- `MACHINE = "qemuarm64"`
- `DL_DIR = "/home/rishabh_shah/yocto-cache/downloads"`
- `SSTATE_DIR = "/home/rishabh_shah/yocto-cache/sstate-cache"`
- `BB_NUMBER_THREADS = "10"`
- `PARALLEL_MAKE = "-j 6"`
- Poky Scarthgap 5.0.21.
- QEMU guest kernel: `6.6.151-yocto-standard`.
- QEMU guest DHCP address observed: `10.0.2.15`.

## Build observations

- `bitbake morse-ref-decoder`: 843 tasks attempted; all succeeded.
- `bitbake core-image-morse-dev`: 4,155 tasks attempted; all succeeded.
- A non-blocking host warning reported that `CONFIG_TASK_IO_ACCOUNTING` is unset.
- QEMU networking printed `ip: RTNETLINK answers: File exists` during startup, but DHCP succeeded.
- The latest QEMU development image was `core-image-morse-dev-qemuarm64.rootfs-20261104074706.ext4`.

## Not yet complete

### QEMU-side implementation still required

- Kernel `input_handler` and reversible selected-device grab lifecycle.
- Kernel-side bounded event queue and in-kernel v1 FSM.
- Kernel virtual keyboard using `input_dev`.
- Versioned `/dev/morse_hid` userspace ABI (`read`, `poll`, `ioctl`).
- `morsed` daemon and guarded dry-run power policy.
- Read-only telemetry/HUD client.
- QEMU integration and recovery testing for the kernel driver, UAPI, daemon and HUD.
- Runtime `SYN_DROPPED` recovery test for the current reference path.

### Final physical hardware stage — deliberately deferred

Do not start actual Raspberry Pi/Zebronics implementation or acceptance until all QEMU-side kernel and userspace implementation and integration work is complete.

The final stage will cover:

- Build the `raspberrypi4-64` image.
- Boot the physical Raspberry Pi 4.
- Capture Zebronics PULSE receiver USB VID/PID and HID descriptors.
- Verify real left/right/middle button and other relevant input events.
- Validate device matching, grab/release, unplug/replug and recovery on hardware.
- Record hardware evidence separately from QEMU evidence.

## Delivery sequence

| Order | Work | Tracking |
|---|---|---|
| 1 | QEMU Yocto baseline and development image | Issue #1 — complete |
| 2 | Approved v1 grammar, map and userspace reference decoder/FSM | Issues #3 and #4 — complete |
| 3 | Kernel input handler and reversible device lifecycle | Issue #5 |
| 4 | Kernel v1 FSM and bounded event processing | Issue #6 |
| 5 | Kernel virtual keyboard synthesis | Issue #7 |
| 6 | Character-device ABI and diagnostics | Issue #8 |
| 7 | Daemon and guarded power policy | Issue #9 |
| 8 | Read-only telemetry and HUD | Issue #10 |
| 9 | Complete QEMU integration, recovery and security acceptance | Issue #11 — QEMU phase |
| 10 | **Final only:** Raspberry Pi 4 and physical Zebronics implementation/acceptance | Issues #2 and #11 — hardware phase |
| 11 | Final course documentation, demonstration and submission | Issue #12 |

## Sprint plan

| Sprint | Dates | Focus |
|---|---|---|
| Sprint 1 | 2–15 Nov 2026 | Platform/build proof, approved grammar and userspace reference |
| Sprint 2 | 16–29 Nov 2026 | QEMU kernel input handler, FSM, virtual keyboard and UAPI |
| Sprint 3 | 30 Nov–13 Dec 2026 | QEMU daemon, HUD, integration, recovery/security; physical hardware only after QEMU acceptance |

Do not close an issue or mark a deliverable complete without implementation and validation evidence. QEMU and physical-target results must remain clearly separated.

## Evidence

- [QEMU ARM64 core-image-morse build](../evidence/builds/2026-11-02-core-image-morse-build.md)
- [QEMU ARM64 core-image-morse boot](../evidence/boots/2026-11-02-core-image-morse-qemu-boot.md)
- [QEMU development image and evtest](../evidence/builds/2026-11-02-core-image-morse-dev-build.md)
- [QEMU evdev runtime mouse-event test](../evidence/input/2026-11-02-qemu-evdev-runtime-events.md)
- [QEMU uinput virtual-keyboard self-test](../evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md)
- [QEMU Morse-to-uinput reference decoder validation](../evidence/input/2026-11-03-qemu-uinput-keyboard.md)
- [QEMU FSM, chord lockout, recovery and uinput integration](../evidence/input/2026-11-04-qemu-fsm-integration.md)
