# Project status

**Snapshot:** 2 November 2026  
**Owner:** Rishabh Shah (solo)  
**Current sprint:** Sprint 1, 2–15 November 2026

## Verified

| Item | Status | Evidence |
|---|---|---|
| WSL 2 / Ubuntu 24.04.5 | Verified | WSL distribution installed and used for build |
| Poky Scarthgap | Verified | Commit `4fb7016a07507683359ebd620f1951fd5ea44bfa` |
| meta-raspberrypi Scarthgap | Verified | Commit `018d44467c4f09eb63f8ea5e86e85d755250cf89` |
| QEMU baseline | Verified | `core-image-minimal` built and booted |
| QEMU guest baseline | Verified | Root login; `aarch64`; kernel `6.6.151-yocto-standard` |
| Custom layer | Verified | `meta-morse-mouse` registered; BitBake lists collection `morse` |
| Image metadata | Verified | Both project recipes discovered; `bitbake -e core-image-morse` succeeds |
| Minimal project image build | Verified | 4,083 tasks attempted; all succeeded; no task failures |
| Package composition | Verified | Final manifest differs from baseline only by `packagegroup-morse-mouse` |
| Project image QEMU boot | Verified | Explicit QEMU config selected project rootfs; root login succeeded |
| Guest architecture/kernel | Verified | `aarch64`; `6.6.151-yocto-standard` |
| Guest network | Verified | DHCP address `10.0.2.15` |
| Initial input visibility | Partially verified | QEMU USB tablet and keyboard enumerated through HID; `/dev/uinput` node exists; event injection not tested |

## Current configuration observed on 2 November 2026

- `MACHINE = "qemuarm64"`
- `DL_DIR = "/home/rishabh_shah/yocto-cache/downloads"`
- `SSTATE_DIR = "/home/rishabh_shah/yocto-cache/sstate-cache"`
- `BB_NUMBER_THREADS = "10"`
- `PARALLEL_MAKE = "-j 6"`
- WSL reported 20 processors, approximately 9.7 GiB memory, and 4 GiB swap.

The parallel Make setting is retained because the user reports that it worked for the baseline build. Observe resource use and change it only if measurements or build failures justify doing so.

## Build and boot warnings/observations

- The final minimal-composition build reported one warning: host kernel process I/O accounting is unavailable because `CONFIG_TASK_IO_ACCOUNTING` is unset. No build task failed.
- QEMU boot printed `ip: RTNETLINK answers: File exists` during network interface setup, but DHCP succeeded and the guest received `10.0.2.15`.
- Rootfs reported `19.3M` total, `12.9M` used and `4.9M` available (72% used) inside the guest.
- `/dev/uinput` was present. This alone does not prove that creating and injecting a virtual device works.
- `evtest` and the guest-side synthetic mouse injector have not yet been installed/implemented or tested.

## Not yet complete

- Inspect kernel config values for `CONFIG_INPUT`, `CONFIG_INPUT_EVDEV`, `CONFIG_INPUT_UINPUT`, `CONFIG_USB_HID` and `CONFIG_HID_GENERIC`.
- Verify the guest's existing input devices and capabilities.
- Add diagnostics such as `evtest` to a development image or test package set.
- Implement and test a guest-side `uinput` synthetic mouse injector.
- Capture actual Zebronics PULSE receiver capabilities on the Raspberry Pi.
- Freeze gesture grammar and timing rules.
- Create the authoritative 95-character mapping and validator.
- Build and test a userspace reference decoder.
- Implement the kernel input handler, FSM, virtual keyboard and character-device ABI.
- Implement `morsed`, `morsectl` and the read-only HUD.
- Build and validate the Raspberry Pi image and record physical acceptance evidence.

## Sprint plan

| Sprint | Dates | Focus |
|---|---|---|
| Sprint 1 | 2–15 Nov 2026 | Platform/build proof, mouse discovery, grammar and reference prototype |
| Sprint 2 | 16–29 Nov 2026 | Kernel input handler, FSM/timing, virtual keyboard, UAPI |
| Sprint 3 | 30 Nov–13 Dec 2026 | Daemon, guarded dry-run power policy, HUD, integration, security, physical validation and final demo |

Do not close an issue or mark a deliverable complete without implementation and validation evidence. Keep this document, issue descriptions/comments, project board, and course wiki synchronized.

## Evidence

- [QEMU ARM64 core-image-morse build](../evidence/builds/2026-11-02-core-image-morse-build.md)
- [QEMU ARM64 core-image-morse boot](../evidence/boots/2026-11-02-core-image-morse-qemu-boot.md)
