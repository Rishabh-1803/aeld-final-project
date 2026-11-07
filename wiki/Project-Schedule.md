# MorseMouse — Project Schedule

> **Project:** MorseMouse — Advanced Embedded Linux Development  
> **Student:** Rishabh Shah (individual project)  
> **Build system:** Yocto Project / Poky Scarthgap 5.0.21  
> **Repository:** https://github.com/Rishabh-1803/aeld-final-project

[Project Overview](https://github.com/Rishabh-1803/aeld-final-project/wiki/Project-Overview) ·
[Source repository](https://github.com/Rishabh-1803/aeld-final-project) ·
[GitHub Issues](https://github.com/Rishabh-1803/aeld-final-project/issues)

---

## Sprint Breakdown

| Sprint | Dates | Goals | Status |
|---|---|---|---|
| Sprint 1 | 2–15 November 2026 | Yocto/QEMU baseline, approved Morse grammar, reference decoder and userspace keyboard proof of concept | **Complete** |
| Sprint 2 | 16–29 November 2026 | Kernel input handler, kernel FSM, virtual keyboard and versioned character-device interface | **In progress / implementation ahead of sprint start** |
| Sprint 3 | 30 November–13 December 2026 | Daemon, guarded power policy, telemetry/HUD, full integration, recovery, documentation and final demonstration | **Planned** |

Sprint dates follow the project plan. Issue status below reflects the current GitHub issue tracker; completed Sprint 2 work was accepted ahead of the formal Sprint 2 date.

## GitHub Project Board Layout

Create a GitHub Projects V2 project named **MorseMouse** and add a table view named **Project-Schedule**. Configure the view to match the course example, with the following columns and sprint grouping:

| Board field | Type / configuration |
|---|---|
| Title | Built-in item title (GitHub issue title) |
| Repository | Built-in repository field |
| Assignees | Built-in assignees field |
| Status | Built-in status field: Todo, In Progress, Done |
| Sprint | Single-select custom field: Sprint 1, Sprint 2, Sprint 3 |
| Grouping | Group rows by the Sprint field |

### Issue-to-board mapping

| Sprint group | Issue | Assignee | Status |
|---|---|---|---|
| Sprint 1 | [#1](https://github.com/Rishabh-1803/aeld-final-project/issues/1), [#3](https://github.com/Rishabh-1803/aeld-final-project/issues/3), [#4](https://github.com/Rishabh-1803/aeld-final-project/issues/4) | Rishabh-1803 | Done |
| Sprint 2 | [#5](https://github.com/Rishabh-1803/aeld-final-project/issues/5), [#6](https://github.com/Rishabh-1803/aeld-final-project/issues/6) | Rishabh-1803 | Done |
| Sprint 2 | [#7](https://github.com/Rishabh-1803/aeld-final-project/issues/7) | Rishabh-1803 | In Progress |
| Sprint 2 | [#8](https://github.com/Rishabh-1803/aeld-final-project/issues/8) | Rishabh-1803 | Todo |
| Sprint 3 | [#2](https://github.com/Rishabh-1803/aeld-final-project/issues/2), [#9](https://github.com/Rishabh-1803/aeld-final-project/issues/9), [#10](https://github.com/Rishabh-1803/aeld-final-project/issues/10), [#11](https://github.com/Rishabh-1803/aeld-final-project/issues/11), [#12](https://github.com/Rishabh-1803/aeld-final-project/issues/12) | Rishabh-1803 | Todo (issue #2 remains hardware-gated) |

All twelve project issues are assigned to Rishabh-1803. The issue-to-sprint mapping above is the intended board configuration; it does not claim that the Projects V2 board has already been created.

## Sprint 1 — Platform and Reference Implementation

**Dates:** 2–15 November 2026  
**Status:** Complete

| Issue | Deliverable | Status |
|---|---|---|
| [#1](https://github.com/Rishabh-1803/aeld-final-project/issues/1) | Reproducible Yocto Scarthgap QEMU build baseline | Closed |
| [#3](https://github.com/Rishabh-1803/aeld-final-project/issues/3) | Approved v1 Morse grammar, reference FSM and 95-character keymap | Closed |
| [#4](https://github.com/Rishabh-1803/aeld-final-project/issues/4) | Userspace Morse decoder and `/dev/uinput` keyboard proof of concept | Closed |

### Sprint 1 evidence

- QEMU `qemuarm64` baseline and development images built and booted.
- The v1 grammar and the 95-character Morse map were validated.
- Host decoder, keymap and FSM tests passed.
- QEMU evdev input drove the userspace decoder.
- `evtest` verified userspace virtual-keyboard key press/release sequences, including Shift ordering.
- Git tag [`coursera-sprint-1`](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1) records the Sprint 1 submission snapshot.

## Sprint 2 — Kernel Input Path

**Dates:** 16–29 November 2026  
**Status:** In progress

| Issue | Student issue | Status |
|---|---|---|
| [#5](https://github.com/Rishabh-1803/aeld-final-project/issues/5) | QEMU-tested Linux input handler and reversible mouse-grab lifecycle | Closed |
| [#6](https://github.com/Rishabh-1803/aeld-final-project/issues/6) | Kernel Morse FSM and bounded event processing | Closed |
| [#7](https://github.com/Rishabh-1803/aeld-final-project/issues/7) | Kernel virtual keyboard event synthesis | Open — assigned |
| [#8](https://github.com/Rishabh-1803/aeld-final-project/issues/8) | `/dev/morse_hid` character device and versioned UAPI | Open — assigned |

### Student Issues

The following upcoming-sprint issues are assigned to **Rishabh-1803** and have explicit deliverables and Definitions of Done.

#### Issue #7 — Implement virtual keyboard event synthesis

**Deliverable**
- Register a kernel-owned virtual keyboard using `input_dev`.
- Use the shared US-QWERTY mapping for all 95 printable ASCII characters.
- Emit key-down/key-up and `EV_SYN` events in the correct order.
- Handle Shift and punctuation.
- Release any tracked key and modifier on reset, error, disconnect and module unload.
- Keep the virtual keyboard out of physical mouse-handler matching.

**Definition of Done**
1. The virtual keyboard registers successfully and is discoverable through `/proc/bus/input/devices`, sysfs and QEMU input tools.
2. Decoded test characters generate the expected Linux keycodes.
3. Uppercase and shifted punctuation produce modifier-down, key-down, synchronization, key-up and modifier-up in the correct order.
4. No key or modifier remains held after normal output, reset, device disconnect or module unload.
5. The ordinary QEMU keyboard and unrelated input devices continue to work.
6. Host keymap tests, Yocto module/image build and QEMU `evtest` output are committed as evidence.

#### Issue #8 — Add `/dev/morse_hid` character device and versioned UAPI

**Deliverable**
- Define fixed-width, versioned UAPI structures.
- Register and remove `/dev/morse_hid` with the driver lifecycle.
- Implement fixed-format diagnostic reads and `poll()` readiness.
- Implement a small documented `ioctl()` set.
- Expose safe queue/error diagnostics and validate unsupported requests.

**Definition of Done**
1. The device node appears when the module is active and is removed on unload.
2. The UAPI defines a version, fixed-width fields, ioctl magic, direction and structure sizes.
3. Blocking and non-blocking `read()` behavior is documented and tested.
4. `poll()` correctly reports no-data and data-ready conditions.
5. Every ioctl validates arguments and rejects unsupported commands safely.
6. Queue-overflow and error counters are visible through the documented interface.
7. A QEMU userspace test client covers success, invalid arguments, unsupported commands, read, poll and module teardown.

## Sprint 3 — System Integration and Final Demonstration

**Dates:** 30 November–13 December 2026  
**Status:** Planned

| Issue | Student issue | Status |
|---|---|---|
| [#2](https://github.com/Rishabh-1803/aeld-final-project/issues/2) | Physical Zebronics PULSE receiver and Raspberry Pi 4 input validation (Phase B; gated by issue #11 Phase A) | Open — intentionally blocked until QEMU gate |
| [#9](https://github.com/Rishabh-1803/aeld-final-project/issues/9) | `morsed` daemon and guarded power-control policy | Open |
| [#10](https://github.com/Rishabh-1803/aeld-final-project/issues/10) | Read-only telemetry and live HUD client | Open |
| [#11](https://github.com/Rishabh-1803/aeld-final-project/issues/11) | QEMU integration gate, then Raspberry Pi 4 and Zebronics hardware validation | Open |
| [#12](https://github.com/Rishabh-1803/aeld-final-project/issues/12) | Final documentation, demonstration and submission evidence | Open |

### Issue #9 — `morsed` daemon and guarded power policy

**Definition of Done**
1. Daemon starts, detects the driver interface and exits cleanly in QEMU.
2. Configuration is parsed and validated; errors produce useful diagnostics.
3. Power actions are disabled/dry-run by default.
4. No arbitrary shell command execution or remote-triggered power control exists.
5. Any future power action is fixed-allowlist only and requires a separate local physical confirmation.
6. Driver-unavailable, daemon restart and shutdown behavior is tested and documented.

### Issue #10 — Read-only telemetry and HUD

**Definition of Done**
1. HUD displays input/decoder state, recent events, queue/overflow counters and daemon status.
2. Refresh and data handling are bounded; the client does not busy-loop.
3. HUD is read-only and cannot issue power or driver-control commands.
4. Missing and stale telemetry are clearly represented.
5. QEMU evidence covers normal operation, no-data, daemon restart and driver-unavailable cases.

### Issue #11 — Integration gate and physical hardware

**Phase A — QEMU Definition of Done**
1. Complete Morse-to-keyboard path passes in QEMU.
2. Kernel/reference FSM vectors agree for the shared test set.
3. Empty/invalid COMMIT, malformed input, chord lockout, queue overflow, device removal and module reload are tested.
4. UAPI read/poll/ioctl, daemon service and HUD behavior pass their QEMU checks.
5. Key-release cleanup, permissions, safe defaults and no-arbitrary-command behavior are reviewed.
6. Logs and evidence are committed.

**Phase B — physical hardware Definition of Done (only after Phase A)**
1. Yocto `raspberrypi4-64` image builds and boots on the actual Raspberry Pi 4.
2. Zebronics receiver VID/PID, interfaces, HID descriptors and actual button events are captured.
3. Device identity/capability matching and reversible grab/release work on the physical receiver.
4. Unplug/replug, recovery and complete physical Morse-to-keyboard path pass.
5. Physical evidence is kept separate from QEMU evidence.

### Issue #12 — Final documentation and submission

**Definition of Done**
1. Project Overview and Schedule wiki pages are complete and cross-linked.
2. README links to Project Overview.
3. Build, boot, test, recovery and troubleshooting instructions are reproducible.
4. Final video outline and public video URL are provided.
5. Documentation clearly distinguishes implemented, QEMU-tested, physically tested and planned features.
6. The final repository and public links are reviewed for course submission.

## Blockers Identified

Dependencies are linked below. Closed dependencies are historical prerequisites; open dependencies are active blockers.

| Student issue / phase | Blocked by | Current blocker status | Reason |
|---|---|---|---|
| [#7 Virtual keyboard](https://github.com/Rishabh-1803/aeld-final-project/issues/7) | [#3 grammar/keymap](https://github.com/Rishabh-1803/aeld-final-project/issues/3), [#5 input handler](https://github.com/Rishabh-1803/aeld-final-project/issues/5), [#6 kernel FSM](https://github.com/Rishabh-1803/aeld-final-project/issues/6) | All closed | Requires approved mapping, device selection and kernel event-processing foundation. |
| [#8 Character-device UAPI](https://github.com/Rishabh-1803/aeld-final-project/issues/8) | [#6 kernel FSM/queue](https://github.com/Rishabh-1803/aeld-final-project/issues/6) | Closed dependency | UAPI exposes diagnostics from the established event/queue model. |
| [#9 Daemon](https://github.com/Rishabh-1803/aeld-final-project/issues/9) | [#8 UAPI](https://github.com/Rishabh-1803/aeld-final-project/issues/8) | Open dependency | Daemon consumes the versioned driver interface. |
| [#10 HUD](https://github.com/Rishabh-1803/aeld-final-project/issues/10) | [#8 UAPI](https://github.com/Rishabh-1803/aeld-final-project/issues/8), [#9 daemon](https://github.com/Rishabh-1803/aeld-final-project/issues/9) | Open dependencies | HUD consumes the agreed telemetry exposed by the daemon/driver. |
| [#11 Phase A — QEMU gate](https://github.com/Rishabh-1803/aeld-final-project/issues/11) | Issues [#5](https://github.com/Rishabh-1803/aeld-final-project/issues/5) through [#10](https://github.com/Rishabh-1803/aeld-final-project/issues/10) | #5/#6 closed; #7–#10 open | Full QEMU integration requires every kernel/userspace component. |
| [#2 Physical receiver validation](https://github.com/Rishabh-1803/aeld-final-project/issues/2) | [#11 Phase A](https://github.com/Rishabh-1803/aeld-final-project/issues/11) | Open, intentionally gated | Do not begin physical receiver acceptance until the QEMU integration gate passes. |
| [#11 Phase B — Raspberry Pi/hardware](https://github.com/Rishabh-1803/aeld-final-project/issues/11) | #11 Phase A and [#2](https://github.com/Rishabh-1803/aeld-final-project/issues/2) | Open, intentionally gated | Requires accepted QEMU implementation and actual physical hardware evidence. |
| [#12 Final submission](https://github.com/Rishabh-1803/aeld-final-project/issues/12) | Issues #1–#11, including the final hardware gate | Several complete; several open | Final docs and demo must describe the actual completed system and evidence. |

## Final Hardware Sequencing Rule

Raspberry Pi image build, physical receiver inspection and hardware acceptance are intentionally deferred until **all QEMU-side issues #5–#10 are complete and issue #11 Phase A passes**. QEMU output must never be described as evidence of Zebronics or Raspberry Pi hardware behavior.

## Submission Snapshots

| Submission | Git tag | Status |
|---|---|---|
| Sprint 1 | [`coursera-sprint-1`](https://github.com/Rishabh-1803/aeld-final-project/tree/coursera-sprint-1) | Created and pushed |
| Sprint 2 | `coursera-sprint-2` | Create after issues #5–#8 are implemented, built and validated |
| Sprint 3 | `coursera-sprint-3` | Create after final integration, hardware stage and documentation are complete |

A tag is a frozen source snapshot. Do not move a previously submitted tag to a later commit.