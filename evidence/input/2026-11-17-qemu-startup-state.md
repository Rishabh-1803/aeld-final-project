# Held-state startup and configuration regression

Date: 17 November 2026; QEMU ARM64, kernel module 0.4.

Code review found that the input callback's held-button shadow was seeded at
bind time but the shared FSM was not synchronized with it. The handler now
enters the shared resynchronization path using the input core's known button
state before registering/opening the handle. This does not claim knowledge
of hardware state that the input core has never observed.

The regression creates a synthetic uinput mouse and establishes LEFT-held
state before loading the selected handler. Automatic udev module loading is
suppressed only in this disposable guest test, then restored afterward.
After bind, the HUD showed only DEVICE_CONNECTED (event 9), ASCII 0. A RIGHT
press during the initial held state was ignored. After both releases, a fresh
DOT/DASH/COMMIT emitted ASCII 65. Device destruction completed normally.

Configuration parsing was also hardened to require unsigned decimal interval
text; a negative string that could wrap strtoul to an in-range unsigned value
was rejected with exit 2. Host compilation and the 4,244-task QEMU image build
passed. The raw transcript and tests/qemu_startup_state.py reproduce the run.

Earlier HMP-only attempts did not establish a held state in the input core,
and an initial synthetic-device setup was affected by automatic udev loading.
Those setup attempts are not counted as passing driver tests. The final run
uses a controlled pre-bind uinput state and asserts the actual observations.
