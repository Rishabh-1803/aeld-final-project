# MorseMouse

**MorseMouse** is an Embedded Linux AELD final project that turns a supported three-button mouse and scroll wheel into a chorded Morse-code text-entry device.

- **Course:** Advanced Embedded Linux Development (AELD), ECEN 5013
- **Owner:** Rishabh Shah (solo project)
- **Repository:** `Rishabh-1803/aeld-final-project`
- **Primary build target:** QEMU ARM64 (`qemuarm64`)
- **Physical demonstration target:** Raspberry Pi 4, 64-bit (`raspberrypi4-64`)
- **Build system:** Yocto Project / Poky, Scarthgap 5.0.21 LTS

## Current status — 2 November 2026

The project is in **Sprint 1 (2–15 November 2026)**.

### Verified

- Poky Scarthgap 5.0.21 is checked out at `4fb7016a07507683359ebd620f1951fd5ea44bfa`.
- `meta-raspberrypi` Scarthgap is checked out at `018d44467c4f09eb63f8ea5e86e85d755250cf89`.
- The `qemuarm64` `core-image-minimal` baseline was built and booted successfully.
- The QEMU guest reported `aarch64` and kernel `6.6.151-yocto-standard`.
- The custom `meta-morse-mouse` layer has been created and registered in the QEMU build configuration.
- BitBake discovers `core-image-morse` and `packagegroup-morse-mouse`; `bitbake -e core-image-morse` parses successfully and resolves `MACHINE = "qemuarm64"`.

### Not yet implemented or validated

- The `core-image-morse` image has **not yet been built or booted**.
- No MorseMouse kernel input handler/module, UAPI, daemon, CLI, HUD, or reference decoder has been implemented.
- QEMU `uinput` mouse injection and physical Zebronics PULSE receiver event discovery remain pending.
- The Morse grammar and complete printable-ASCII keymap remain to be frozen and tested.
- The Raspberry Pi image and physical acceptance tests remain pending.

The successful baseline is a platform milestone, not evidence that the MorseMouse application is implemented.

## Project goal

Use the left and right mouse buttons as Morse dit/dah inputs, with the middle button and scroll wheel providing documented layer, commit, navigation, and guarded local-control gestures. A kernel input handler will process the selected mouse and synthesize keyboard events through a virtual Linux `input_dev`. A userspace daemon will consume structured driver events for diagnostics, service coordination, and telemetry. A read-only HUD will display status.

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
| QEMU ARM64, `qemuarm64` | Reproducible development and integration tests | Baseline boot verified |
| Raspberry Pi 4, `raspberrypi4-64` | Physical demonstration target | Selected; project image not built |
| Zebronics PULSE mouse + 2.4 GHz USB receiver | Physical input device | Available; Linux event capabilities not yet captured |

QEMU `uinput` tests validate driver/FSM behavior with synthetic events. They do not replace physical receiver and Raspberry Pi acceptance tests.

## Repository layout

```text
.
├── meta-morse-mouse/       # Project Yocto layer
├── docs/                   # Architecture, build and status documentation
├── manifest/               # Pinned external layer/source revisions
├── config/                 # Authoritative Morse keymap and defaults (planned)
├── driver/                 # Kernel input handler (planned)
├── include/                # Versioned userspace/kernel UAPI (planned)
├── userspace/              # Daemon, CLI and HUD (planned)
├── tools/                  # Keymap generator/validator (planned)
├── tests/                  # Unit, QEMU and hardware tests (planned)
├── scripts/                # Build/test helpers (planned)
└── evidence/               # Build and validation evidence (planned)
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
