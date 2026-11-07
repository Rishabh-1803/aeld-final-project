# Project Status

**Snapshot:** 6 November 2026  
**Owner:** Rishabh Shah (individual project)  
**Build system:** Yocto Project / Poky Scarthgap 5.0.21  
**Current work:** Sprint 2 kernel input path; Sprint 1 accepted and tagged

## Verified

| Item | Status | Evidence |
|---|---|---|
| WSL2 / Ubuntu build host | Verified | WSL2 Ubuntu 24.04 build environment |
| Poky Scarthgap | Pinned and verified | Commit `4fb7016a07507683359ebd620f1951fd5ea44bfa` |
| meta-raspberrypi Scarthgap | Pinned | Commit `018d44467c4f09eb63f8ea5e86e85d755250cf89` |
| meta-openembedded (`meta-oe`) | Pinned | Commit `2db6aece32c7f8ebf6015b468e1b4e21de072e3b` |
| QEMU ARM64 baseline | Verified | `core-image-minimal` built and booted |
| QEMU guest | Verified | `aarch64`; kernel `6.6.151-yocto-standard` |
| Project layer | Verified | `meta-morse-mouse` registered with BitBake |
| Project/development images | Verified | `core-image-morse` and `core-image-morse-dev` built and booted |
| QEMU evdev input | Verified | QEMU USB Tablet left/right/middle input events captured |
| Userspace uinput self-test | Verified | Virtual keyboard and KEY_A press/release readback |
| Morse map | Verified | 95 printable ASCII entries and 95 unique Morse sequences |
| Userspace reference decoder | Verified | All 95 mapped characters decode and commit |
| US QWERTY keymap | Verified | All 95 printable ASCII key mappings pass |
| Shared Morse FSM | Verified | C FSM suite: 52 checks; shared vector validator: 58 vectors |
| Kernel Morse map generation | Verified | Generated kernel map agrees with all 95 CSV entries |
| Kernel input handler | Verified in QEMU | Tablet matching, keyboard exclusion, left/right/middle events, reversible grab and unload |
| Linux HID unbind/rebind | Verified in QEMU | Disconnect and automatic reconnect callbacks |
| QEMU USB hot-unplug/hot-replug | Verified in QEMU | Clean disconnect and reconnect |
| Kernel bounded event queue | Verified in QEMU | Capacity 64; callback-side queueing, overflow accounting and deferred worker |
| Overflow recovery | Verified in QEMU | 250,008 received, 3,846 overflows, 250,000 dropped; fresh `.-` decoded as A after recovery |
| Disconnect teardown under pressure | Verified in QEMU | 250,002 received, 1 processed, 3,846 overflows and 250,001 dropped; disconnect completed without observed BUG/Oops/warning/hung-task |
| Sprint 1 Coursera tag | Published | `coursera-sprint-1` points to commit `3a0bc5a8bc74e7933b49e2a52a4da29cfced7a59` |

## Current Build Configuration

- `MACHINE = "qemuarm64"`
- `DL_DIR = "/home/rishabh_shah/yocto-cache/downloads"`
- `SSTATE_DIR = "/home/rishabh_shah/yocto-cache/sstate-cache"`
- `BB_NUMBER_THREADS = "10"`
- `PARALLEL_MAKE = "-j 6"`
- Poky Scarthgap 5.0.21.
- QEMU guest kernel: `6.6.151-yocto-standard`.
- QEMU guest DHCP address observed: `10.0.2.15`.

## Build Observations

- `bitbake morse-ref-decoder`: 843 tasks attempted; all succeeded in the earlier package build.
- `bitbake core-image-morse-dev`: successful QEMU development image builds recorded for the previous milestones.
- The kernel input-handler/FSM build and QEMU acceptance are documented in the evidence files below.
- A non-blocking host warning previously reported `CONFIG_TASK_IO_ACCOUNTING` unset.
- QEMU networking previously printed `ip: RTNETLINK answers: File exists` during startup, but DHCP succeeded.

## Not Yet Complete

### QEMU-side implementation still required

- Kernel virtual keyboard registered through `input_dev` and event-level Morse-to-keyboard verification.
- Versioned `/dev/morse_hid` UAPI (`read`, `poll`, `ioctl`).
- `morsed` daemon and guarded dry-run power policy.
- Read-only telemetry/HUD client.
- QEMU integration/recovery/security testing for issues #7–#10.
- Runtime `SYN_DROPPED` recovery integration test.

### Final Physical Hardware Stage — Deliberately Deferred

Do not begin physical Raspberry Pi/Zebronics acceptance until the QEMU-side implementation and issue #11 Phase A gate are complete.

The final stage will cover:

- Build the `raspberrypi4-64` image.
- Boot the physical Raspberry Pi 4.
- Capture Zebronics PULSE receiver USB VID/PID and HID descriptors.
- Verify actual left/right/middle button events.
- Validate device matching, grab/release, unplug/replug and recovery.
- Record physical evidence separately from QEMU evidence.

## Delivery Sequence

| Order | Work | Tracking | Status |
|---|---|---|---|
| 1 | QEMU Yocto baseline and development image | Issue #1 | Complete |
| 2 | Approved v1 grammar, map and userspace reference decoder/FSM | Issues #3 and #4 | Complete |
| 3 | Kernel input handler and reversible device lifecycle | Issue #5 | Complete |
| 4 | Kernel v1 FSM and bounded event processing | Issue #6 | Complete |
| 5 | Kernel virtual keyboard synthesis | Issue #7 | In progress |
| 6 | Character-device ABI and diagnostics | Issue #8 | Planned |
| 7 | Daemon and guarded power policy | Issue #9 | Planned |
| 8 | Read-only telemetry and HUD | Issue #10 | Planned |
| 9 | Complete QEMU integration, recovery and security acceptance | Issue #11 Phase A | Pending |
| 10 | **Final only:** Raspberry Pi 4 and physical Zebronics implementation/acceptance | Issues #2 and #11 Phase B | Hardware-gated |
| 11 | Final course documentation, demonstration and submission | Issue #12 | Pending |

## Sprint Plan

| Sprint | Dates | Focus | Status |
|---|---|---|---|
| Sprint 1 | 2–15 Nov 2026 | Platform/build proof, grammar, userspace reference | Complete |
| Sprint 2 | 16–29 Nov 2026 | QEMU kernel input handler, FSM, virtual keyboard and UAPI | In progress |
| Sprint 3 | 30 Nov–13 Dec 2026 | QEMU daemon, HUD, integration/recovery/security; physical stage only after QEMU acceptance | Planned |

## Evidence

- [QEMU ARM64 core-image-morse build](../evidence/builds/2026-11-02-core-image-morse-build.md)
- [QEMU ARM64 core-image-morse boot](../evidence/boots/2026-11-02-core-image-morse-qemu-boot.md)
- [QEMU development image and evtest](../evidence/builds/2026-11-02-core-image-morse-dev-build.md)
- [QEMU evdev runtime mouse-event test](../evidence/input/2026-11-02-qemu-evdev-runtime-events.md)
- [QEMU uinput virtual-keyboard self-test](../evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md)
- [QEMU Morse-to-uinput reference decoder validation](../evidence/input/2026-11-03-qemu-uinput-keyboard.md)
- [QEMU FSM, chord lockout, recovery and uinput integration](../evidence/input/2026-11-04-qemu-fsm-integration.md)
- [QEMU kernel input-handler build, binding, grab/recovery and USB hotplug evidence](../evidence/input/2026-11-05-qemu-kernel-input-handler.md)
- [QEMU bounded-queue overflow stress](../evidence/input/2026-11-05-qemu-bounded-queue-overflow.md)
- [QEMU pending-work teardown stress](../evidence/input/2026-11-06-qemu-pending-work-teardown.md)

QEMU and physical-target results must remain clearly separated. Do not mark a feature complete without implementation and corresponding validation evidence.
