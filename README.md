# MorseMouse

**MorseMouse** is an Embedded Linux AELD final project that turns a supported three-button mouse and scroll wheel into a chorded Morse-code text-entry device.

- **Course:** Advanced Embedded Linux Development (AELD), ECEN 5013
- **Owner:** Rishabh Shah (solo project)
- **Repository:** `Rishabh-1803/aeld-final-project`
- **Primary build target:** QEMU ARM64 (`qemuarm64`)
- **Physical demonstration target:** Raspberry Pi 4, 64-bit (`raspberrypi4-64`)
- **Build system:** Yocto Project / Poky, Scarthgap 5.0.21 LTS

## Current status — 4 November 2026

The QEMU-side userspace reference implementation and the first kernel input-handler lifecycle implementation have passed their current QEMU runtime tests.

### Implemented and verified

- Yocto Scarthgap `qemuarm64` development image builds and boots.
- QEMU exposes an emulated USB tablet with left, right and middle button events.
- Approved v1 grammar: left = DOT, right = DASH, middle = explicit COMMIT; scroll-wheel gestures are disabled.
- Overlapping left/right input clears the current sequence and locks input until both primary buttons are released.
- `config/morse_map.csv` contains 95 unique printable ASCII mappings.
- The mapping validator and US QWERTY Linux keymap tests pass.
- The C reference decoder reads evdev input and decodes committed Morse sequences.
- The independent userspace FSM passes all 37 unit checks.
- QEMU tests verify empty COMMIT, normal decoding, chord cancellation, lockout and recovery.
- The decoder creates a virtual keyboard through `/dev/uinput`.
- `evtest` confirmed two complete uppercase-A key sequences, including modifier and key press/release events.
- The updated Yocto decoder recipe and `core-image-morse-dev` image build successfully with FSM integration.
- The `hid_morse_kbd` kernel input-handler module builds, packages and autoloads in the QEMU development image.
- QEMU runtime tests verify tablet matching, keyboard exclusion, left/right/middle press/release receipt, exclusive grab, and restoration of ordinary input after module unload.
- Linux HID unbind/rebind and genuine QEMU USB hot-unplug/hot-replug trigger clean disconnect and automatic reconnect callbacks.

### Not yet implemented or validated

- In-kernel Morse FSM, bounded event queue and deferred event processing.
- Kernel virtual `input_dev` keyboard and `/dev/morse_hid` UAPI.
- `morsed`, `morsectl` and the read-only HUD.
- Kernel virtual keyboard, `/dev/morse_hid` UAPI, daemon and HUD integration in QEMU.
- Raspberry Pi 4 and physical Zebronics PULSE receiver validation.
- Visible text insertion into a graphical application.
- Runtime `SYN_DROPPED` recovery integration testing.

The QEMU integration establishes the userspace reference behavior. It does not establish completion of the kernel driver or physical hardware acceptance.

See [QEMU FSM integration evidence](evidence/input/2026-11-04-qemu-fsm-integration.md), [kernel input-handler evidence](evidence/input/2026-11-05-qemu-kernel-input-handler.md), and [Project status](docs/project-status.md).

## Project goal

The approved v1 grammar uses the left and right mouse buttons for DOT/DASH input and the middle button for explicit COMMIT. Scroll-wheel gestures are disabled in v1. The userspace reference decoder and FSM have been validated in QEMU. The planned kernel input handler will eventually provide the kernel-side implementation. A userspace daemon and read-only HUD remain planned.

The core target is **95 printable ASCII characters (decimal 32–126)** under a declared US QWERTY acceptance layout. All mappings and custom extensions must be documented and tested.

## Planned architecture

```text
Three-button mouse + USB receiver
              |
          Linux HID
              |
       Linux input core
              |
   targeted input_handler
      hid_morse_kbd.ko
              |
   bounded queue / ordered work
       Morse FSM + keymap
          /         \
         v           v
 virtual input_dev  /dev/morse_hid
    keyboard              |
         |              morsed
         v                |
 Linux applications   read-only telemetry
                          |
                    morse_hud_client
```

Kernel callbacks must remain bounded and non-sleeping. Work requiring sleepable context is deferred to an ordered workqueue. The driver must recover safely from invalid sequences, timeouts, disconnects, daemon failure, and module unload.

Power-control behavior is separate from text entry. It is dry-run/disabled by default and requires a distinct two-stage **local physical confirmation**. This is not cryptographic authentication. The HUD is read-only and must not initiate reboot or shutdown.

## Hardware and targets

| Component | Role | Status |
|---|---|---|
| QEMU ARM64, `qemuarm64` | Reproducible development and integration tests | Minimal and development images built/booted; QEMU evdev input verified; userspace Morse FSM, chord lockout, recovery and `uinput` keyboard integration verified; kernel driver and physical mouse validation pending |
| Raspberry Pi 4, `raspberrypi4-64` | Physical demonstration target | Selected; project image not built |
| Zebronics PULSE mouse + 2.4 GHz USB receiver | Physical input device | Available; Linux event capabilities not yet captured |

The QEMU integration tests validate the userspace reference decoder/FSM and the first kernel input-handler lifecycle implementation. They do not replace physical receiver and Raspberry Pi acceptance tests.

## Repository layout

```text
.
├── meta-morse-mouse/       # Project Yocto layer
├── docs/                   # Architecture, build and status documentation
├── manifest/               # Pinned external layer/source revisions
├── config/                 # Authoritative Morse map and defaults
├── driver/                 # Kernel input handler (planned)
├── include/                # Versioned userspace/kernel UAPI (planned)
├── userspace/              # Daemon, CLI and HUD (planned)
├── tools/                  # Reference decoder, keymap and validation tools
├── tests/                  # Unit, QEMU and hardware tests (planned)
├── scripts/                # Build/test helpers (planned)
└── evidence/               # Build and validation evidence
```

Upstream Poky and BSP source trees, BitBake build output, downloads, and shared-state caches are not committed in this repository. See [Build: QEMU ARM64](docs/build-qemuarm64.md) and [Source pins](manifest/source-pins.md).

## Sprint schedule

| Sprint | Dates | Goal |
|---|---|---|
| Sprint 1 | 2–15 Nov 2026 | Build proof, mouse discovery, grammar/keymap, reference decoder |
| Sprint 2 | 16–29 Nov 2026 | Kernel input handler, FSM, virtual keyboard, character-device ABI |
| Sprint 3 | 30 Nov–13 Dec 2026 | Daemon, guarded dry-run power policy, HUD, integration, validation and demo |

Implementation status and acceptance evidence are tracked in [GitHub Issues](https://github.com/Rishabh-1803/aeld-final-project/issues) and the project board.

## Safety and engineering constraints

- Do not hard-code `/dev/input/eventN`; select the intended device by identity/capabilities.
- Exclude the virtual keyboard from physical-mouse matching.
- Release all held keys/modifiers on reset, mode exit, unplug, error, and unload.
- Use a bounded event queue and account for overflow.
- Do not sleep, perform user-copy, filesystem/socket I/O, or execute commands in input/timer callbacks.
- Never route arbitrary text or unauthenticated network messages to shell commands.
- Do not claim zero latency, cryptographic authentication, or timing precision without an implemented and measured basis.
- Do not copy a QEMU-built kernel module to the Raspberry Pi; build separately for each target kernel.

## License

A repository license has not yet been selected. Do not assume that the Yocto recipe's `LICENSE = "MIT"` declaration settles the licensing of every project source file.

## Latest evidence

- [2 Nov 2026 — QEMU ARM64 `core-image-morse` build](evidence/builds/2026-11-02-core-image-morse-build.md)
- [2 Nov 2026 — QEMU ARM64 `core-image-morse` boot](evidence/boots/2026-11-02-core-image-morse-qemu-boot.md)
- [2 Nov 2026 — QEMU ARM64 development image and `evtest`](evidence/builds/2026-11-02-core-image-morse-dev-build.md)
- [2 Nov 2026 — QEMU evdev runtime mouse-event test](evidence/input/2026-11-02-qemu-evdev-runtime-events.md)
- [2 Nov 2026 — QEMU `uinput` virtual-keyboard self-test](evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md)
- [3 Nov 2026 — QEMU Morse-to-uinput reference decoder validation](evidence/input/2026-11-03-qemu-uinput-keyboard.md)

- [5 Nov 2026 — QEMU kernel input-handler build and lifecycle evidence](evidence/input/2026-11-05-qemu-kernel-input-handler.md)
