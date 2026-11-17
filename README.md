# MorseMouse

**A Yocto-based Embedded Linux project that turns a three-button mouse into a Morse-code keyboard.**

[![Build System](https://img.shields.io/badge/Build%20System-Yocto%20%2F%20Poky-blue)](https://www.yoctoproject.org/) [![Development Target](https://img.shields.io/badge/Development-QEMU%20ARM64-green)](https://www.qemu.org/) [![Sprint 1](https://img.shields.io/badge/Coursera-Sprint%201%20submitted-success)](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1) [![Project Issues](https://img.shields.io/badge/Tracking-GitHub%20Issues-purple)](https://github.com/Rishabh-1803/aeld-final-project/issues)

**Student:** Rishabh Shah · **Course:** Advanced Embedded Linux Development · **Project type:** Individual

| Documentation | Link |
|---|---|
| Project Overview (course wiki) | [Open Project Overview](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Overview) |
| Project Schedule (course wiki) | [Open Project Schedule](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Schedule) |
| GitHub Issues | [Open issue tracker](https://github.com/Rishabh-1803/aeld-final-project/issues) |
| Architecture and project status | [`docs/architecture.md`](docs/architecture.md) · [`docs/project-status.md`](docs/project-status.md) |
| Build instructions | [`docs/build-qemuarm64.md`](docs/build-qemuarm64.md) |
| Source pins | [`manifest/source-pins.md`](manifest/source-pins.md) |

---

## 1. Project Overview

MorseMouse uses a three-button mouse as a discrete Morse input device:

| Mouse input | Morse action |
|---|---|
| Left button press | DOT (`.`) |
| Right button press | DASH (`-`) |
| Middle button press | COMMIT current character |
| Button release | Rearm; does not append a symbol |
| Left + right held together | Cancel current sequence and lock out until both are released |
| Scroll wheel | Disabled in v1 |

The authoritative Morse map covers all **95 printable ASCII characters** (decimal 32–126), with a declared US-QWERTY output mapping.

## 2. Architecture at a Glance

```mermaid
flowchart LR
    M["Three-button mouse"] -->|"2.4 GHz radio"| R["USB receiver"]
    R -->|"USB HID"| P["Raspberry Pi 4<br/>(planned physical target)"]
    Q["QEMU USB Tablet<br/>(development input)"] --> L["Linux input subsystem"]
    P --> L
    L --> D["MorseMouse input handler<br/>+ FSM + bounded event worker"]
    D --> V["Kernel virtual keyboard<br/>input_dev"]
    V --> O["Linux input consumers<br/>evtest / applications"]
    H["WSL2 Ubuntu + Yocto/BitBake"] -->|"builds QEMU image"| QEMU["QEMU ARM64<br/>qemuarm64"]
    QEMU --> Q
```

QEMU is the current development and validation target. Raspberry Pi 4 and the physical receiver are the planned final hardware target and remain gated by QEMU integration acceptance.

## 3. Target Build System and Hardware

- **Build system:** Yocto Project / Poky, Scarthgap 5.0.21.
- **QEMU development target:** `MACHINE = "qemuarm64"`.
- **Planned physical target:** Raspberry Pi 4 64-bit, `MACHINE = "raspberrypi4-64"`, supported through the [`meta-raspberrypi` BSP layer](https://github.com/agherzan/meta-raspberrypi).
- **Physical input peripheral:** Zebronics PULSE mouse with 2.4 GHz USB receiver.
- **Current status:** QEMU image/build and input-handler/FSM validation have evidence. Raspberry Pi physical build/boot and receiver validation are deferred until the QEMU gate passes.

## 4. Current Progress

| Workstream | Status |
|---|---|
| Yocto QEMU baseline and development image | Complete |
| Approved v1 grammar and 95-character map | Complete |
| Userspace reference decoder and `uinput` proof of concept | Complete |
| QEMU kernel input handler and reversible grab lifecycle | Complete |
| Kernel Morse FSM, bounded queue and teardown stress testing | Complete |
| Kernel virtual keyboard (`input_dev`) | Complete; QEMU evidence recorded |
| `/dev/morse_hid` UAPI | Complete; QEMU acceptance recorded |
| `morsed` diagnostic daemon | QEMU validated; power deferred |
| Read-only telemetry/HUD | QEMU validated |
| Final QEMU integration and physical Pi/receiver validation | Planned |

Sprint 1 snapshot: [`coursera-sprint-1`](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1). The tag is a frozen submission boundary, not a claim that later Sprint 2 work is complete.

## 5. Repository Layout

```text
meta-morse-mouse/   Project Yocto layer, recipes and image configuration
driver/             Kernel input handler and generated kernel Morse map
tools/              Reference decoder, shared FSM, keymap and host tests
config/             Authoritative Morse mapping
tests/              Shared FSM vectors and validators
docs/               Architecture, grammar, build and project status
manifest/           Pinned upstream layer/source revisions
evidence/            Build, boot and QEMU test reports
```

## 6. Build and Test

The reproducible QEMU build instructions are in [`docs/build-qemuarm64.md`](docs/build-qemuarm64.md).

Typical host regression command:

```bash
make -C tools clean all test
```

Yocto image build (from the repository root, after sourcing the build environment):

```bash
bitbake core-image-morse-dev
```

## 7. Scope and Validation Boundaries

- The current Morse grammar is explicit-commit and button-identity based; it is not a timed Morse-gap decoder.
- QEMU validation is not evidence of physical receiver compatibility.
- The final Raspberry Pi 4 and Zebronics hardware stage starts only after the QEMU integration gate.
- Visible text insertion into a graphical application, the daemon, HUD and power policy are not claimed as complete until implemented and tested.

## 8. Course Submission Links

- [Project Overview Wiki Page](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Overview)
- [Project Schedule Wiki Page](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Schedule)
- [Sprint 1 submission tag](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1)
- [Sprint 2 issues](https://github.com/Rishabh-1803/aeld-final-project/issues?q=is%3Aissue+label%3Asprint-2)
- [All project issues](https://github.com/Rishabh-1803/aeld-final-project/issues)


## Verified continuation checkpoint — 17 November 2026

Kernel virtual keyboard (issue #7) is complete with November 7 QEMU
evidence. The character-device implementation (issue #8) has passed the
November 17 QEMU runtime checks; see
[character-device contract](https://github.com/Rishabh-1803/aeld-final-project/blob/main/docs/character-device.md)
and [acceptance evidence](https://github.com/Rishabh-1803/aeld-final-project/blob/main/evidence/input/2026-11-17-qemu-character-device.md).
Earlier dated tables above are historical snapshots.

The owner confirmed on November 17 that power execution and shutdown
gestures are deferred until after the final submission. The daemon must
keep execution disabled. The Zebronics mouse is available; Raspberry Pi 4
will be borrowed for a limited test session after QEMU acceptance.
Daemon and HUD now have November 17 QEMU acceptance evidence. Final integration, physical validation and final demonstration remain pending.

See [daemon/HUD usage and policy](https://github.com/Rishabh-1803/aeld-final-project/blob/main/docs/daemon-hud.md) and November 17 daemon/HUD evidence. Power execution is deferred until after submission.
