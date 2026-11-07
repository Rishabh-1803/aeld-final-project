# MorseMouse

**A Yocto-based Embedded Linux project that turns a three-button mouse into a Morse-code keyboard.**

[![Build System](https://img.shields.io/badge/Build%20System-Yocto%20%2F%20Poky-blue)](https://www.yoctoproject.org/)
[![Development Target](https://img.shields.io/badge/Development-QEMU%20ARM64-green)](https://www.qemu.org/)
[![Sprint 1](https://img.shields.io/badge/Coursera-Sprint%201%20submitted-success)](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1)

**Course:** Advanced Embedded Linux Development (ECEN 5013)  
**Owner:** Rishabh Shah (individual project)  
**Repository:** `Rishabh-1803/aeld-final-project`

## Course Project Pages

| Page | Purpose |
|---|---|
| [Project Overview Wiki](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Overview) | Project proposal, Yocto build system, hardware, connectivity, open-source dependencies, previous/new/shared content and source organization |
| [Project Schedule Wiki](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Schedule) | Sprint milestones, assigned issues, blockers, dependencies and Definitions of Done |
| [GitHub Issues](https://github.com/Rishabh-1803/aeld-final-project/issues) | Implementation tasks and acceptance evidence |
| [Sprint 1 submission snapshot](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1) | Frozen Sprint 1 source snapshot |

---

## Current Status — 6 November 2026

The QEMU-side reference implementation, Linux input handler and kernel Morse FSM/bounded event processing have been implemented and validated. The kernel virtual keyboard, UAPI, daemon and HUD remain in progress or planned and are not represented as QEMU-accepted yet.

### Implemented and verified

- Yocto Project / Poky Scarthgap 5.0.21 `qemuarm64` development image builds and boots.
- QEMU exposes an emulated USB Tablet with left, right and middle mouse-button events.
- Approved v1 grammar: left = DOT, right = DASH, middle = explicit COMMIT; scroll-wheel gestures are disabled.
- Left/right overlap clears the current sequence and locks input until both primary buttons are released.
- `config/morse_map.csv` contains all 95 printable ASCII mappings (decimal 32–126).
- The userspace reference decoder and US-QWERTY keymap pass host regression tests.
- The shared FSM core is used by both the userspace reference and kernel input handler.
- Kernel FSM and bounded event processing are implemented with a 64-event queue, spinlock-protected event capture, deferred worker processing, explicit overflow accounting and synchronized teardown.
- Host validation: 52 C FSM checks, 58 shared FSM vectors, all 95 keymap entries and all 95 generated kernel Morse-map entries passed.
- The kernel input-handler module builds, packages and autoloads in the QEMU development image.
- QEMU verifies device matching, physical keyboard exclusion, button event receipt, reversible exclusive grab, module unload, HID unbind/rebind and USB hot-unplug/hot-replug.
- Queue overflow stress recorded 250,008 received events, 3,846 queue overflows and 250,000 dropped events; a fresh `.-` decoded to uppercase A after recovery.
- Disconnect teardown under queue pressure completed; no kernel BUG, Oops, warning or hung-task report was observed in the collected logs.

### Not yet implemented or validated

- Kernel virtual keyboard registered through `input_dev`, including end-to-end key event traces from the kernel Morse commit path.
- Versioned `/dev/morse_hid` UAPI (`read`, `poll`, `ioctl`).
- `morsed` daemon, guarded dry-run power policy and `morsectl`.
- Read-only telemetry/HUD client.
- Complete QEMU integration/recovery/security acceptance for issues #7–#10.
- Raspberry Pi 4 image build/boot and physical Zebronics PULSE receiver validation.
- Visible text insertion into a graphical application.
- Runtime `SYN_DROPPED` recovery integration test.

QEMU validation is not presented as proof of physical Raspberry Pi or Zebronics hardware acceptance.

See [Project status](docs/project-status.md), [QEMU kernel input-handler evidence](evidence/input/2026-11-05-qemu-kernel-input-handler.md), [queue overflow evidence](evidence/input/2026-11-05-qemu-bounded-queue-overflow.md) and [pending-work teardown evidence](evidence/input/2026-11-06-qemu-pending-work-teardown.md).

## Project Goal

MorseMouse converts a supported three-button mouse into a discrete Morse-code text-entry device. Button identity determines the Morse symbol: left press = DOT, right press = DASH and middle press = COMMIT. Button release rearms the button and appends no symbol. There is no automatic character timeout or duration-based DOT/DASH classification in v1.

The core target is **95 printable ASCII characters (decimal 32–126)** using the documented Morse map and US-QWERTY output mapping.

## Architecture

```text
Development host
Windows PC -> WSL2 Ubuntu -> Yocto / BitBake
                              |
                              v
                         QEMU ARM64
                        qemuarm64
                              |
                       QEMU USB Tablet
                              |
                       Linux input core
                              |
                    hid_morse_kbd.ko
                  input_handler + FSM
                   bounded event queue
                              |
                  [kernel virtual input_dev]
                              |
                  Linux input consumers

Planned physical target
Zebronics PULSE mouse
        |
   2.4 GHz link
        |
   USB receiver
        |
   USB HID
        |
Raspberry Pi 4 / 64-bit Yocto
        |
   Linux input core
        |
   MorseMouse kernel path
```

The kernel input callback remains bounded and non-sleeping. Morse processing runs in deferred worker context. The virtual keyboard is intended to be a separate keyboard-only input device and must not match the physical Morse input handler.

Power-control behavior is separate from text entry. It must remain dry-run/disabled by default, use a fixed allowlist and require local physical confirmation. It is not cryptographic authentication. The HUD is read-only and must not initiate reboot or shutdown.

## Target Build System and Hardware

**Build system:** Yocto Project / Poky, Scarthgap 5.0.21. Buildroot is not used.

| Component | Role | Status |
|---|---|---|
| QEMU ARM64, `qemuarm64` | Reproducible development and integration | Development image built and booted; kernel input handler and kernel FSM/queue validated |
| Raspberry Pi 4, `raspberrypi4-64` | Final physical demonstration target | Planned; project image not yet built or booted |
| Zebronics PULSE mouse + 2.4 GHz USB receiver | Physical input peripheral | Intended for final stage; physical VID/PID and input capabilities not yet captured |

The physical build is planned through the `meta-raspberrypi` Yocto BSP layer. Do not copy a QEMU-built kernel module to the Raspberry Pi; build separately for each target kernel.

## Repository Layout

```text
.
├── config/                 # Authoritative Morse map
├── docs/                   # Architecture, build, grammar and project status
├── driver/                 # Kernel input handler and generated kernel map
├── evidence/               # Build, boot, QEMU runtime and stress-test evidence
├── manifest/               # Pinned upstream Yocto/BSP revisions
├── meta-morse-mouse/       # Project Yocto layer, recipes and image definitions
├── tests/                  # Shared FSM vectors and validation scripts
└── tools/                  # Reference decoder, shared FSM, keymap and host tests
```

Upstream Poky/BSP checkouts, BitBake build output, downloads and shared-state caches are not committed. See [Build: QEMU ARM64](docs/build-qemuarm64.md) and [Source pins](manifest/source-pins.md).

## Sprint Schedule

| Sprint | Dates | Goal |
|---|---|---|
| Sprint 1 | 2–15 Nov 2026 | Yocto/QEMU baseline, approved grammar, reference decoder and userspace keyboard proof of concept |
| Sprint 2 | 16–29 Nov 2026 | Kernel input handler, FSM, virtual keyboard and character-device ABI |
| Sprint 3 | 30 Nov–13 Dec 2026 | Daemon, guarded power policy, HUD, integration, recovery, physical stage and demo |

Implementation status and acceptance evidence are tracked in [GitHub Issues](https://github.com/Rishabh-1803/aeld-final-project/issues) and the [Project Schedule Wiki](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Schedule).

## Safety and Engineering Constraints

- Do not hard-code `/dev/input/eventN`; select the intended device by identity/capabilities.
- Exclude the virtual keyboard from physical-mouse matching.
- Release all held keys/modifiers on reset, unplug, error and unload.
- Use a bounded event queue and account for overflow.
- Do not sleep, perform user-copy, filesystem/socket I/O or execute commands in input callbacks.
- Never route arbitrary text or unauthenticated network messages to shell commands.
- Do not claim zero latency, cryptographic authentication or timing precision without implemented measurements.
- Keep physical hardware results separate from QEMU results.

## License

A repository-wide license has not yet been selected. Do not assume that an individual Yocto recipe's `LICENSE` declaration settles the licensing of every project source file.

## Latest Evidence

- [2 Nov 2026 — QEMU ARM64 core-image-morse build](evidence/builds/2026-11-02-core-image-morse-build.md)
- [2 Nov 2026 — QEMU ARM64 core-image-morse boot](evidence/boots/2026-11-02-core-image-morse-qemu-boot.md)
- [2 Nov 2026 — QEMU development image and evtest](evidence/builds/2026-11-02-core-image-morse-dev-build.md)
- [2 Nov 2026 — QEMU evdev runtime mouse-event test](evidence/input/2026-11-02-qemu-evdev-runtime-events.md)
- [3 Nov 2026 — QEMU Morse-to-uinput reference decoder validation](evidence/input/2026-11-03-qemu-uinput-keyboard.md)
- [4 Nov 2026 — QEMU FSM, chord lockout, recovery and uinput integration](evidence/input/2026-11-04-qemu-fsm-integration.md)
- [5 Nov 2026 — QEMU kernel input-handler build and lifecycle](evidence/input/2026-11-05-qemu-kernel-input-handler.md)
- [5 Nov 2026 — QEMU bounded-queue overflow stress](evidence/input/2026-11-05-qemu-bounded-queue-overflow.md)
- [6 Nov 2026 — QEMU pending-work teardown stress](evidence/input/2026-11-06-qemu-pending-work-teardown.md)
