# QEMU device matching and receiver preparation

Date: 17 November 2026. Module 0.4; QEMU ARM64.

The input handler now accepts either absolute X/Y or relative X/Y devices,
provided they expose left/right/middle buttons and pass the configured
identity selector. A name selector remains the QEMU default. Optional vendor
and product filters require a complete nonzero pair; a blank name is permitted
only with that exact pair. An empty unrestricted selector is rejected.

QEMU validation selected vendor 0x1209/product 0x0008 with a blank name.
The ordinary QEMU tablet was excluded. A synthetic relative-motion mouse
matched, completed a 250000-event overflow/recovery run and emitted A.
Relative-device teardown under another 250000-event burst also completed.
Partial vendor-only and empty unrestricted selectors failed module loading.
The default QEMU selector was restored; DOT/DASH/COMMIT again yielded A
in daemon/HUD telemetry. No BUG/Oops/WARNING/hung-task matches were returned.
The final QEMU production/development image build completed 4,242 tasks.

## Windows receiver observation — not physical Pi acceptance

The owner connected the Zebronics PULSE receiver during the session. A new
USB composite/input mouse appeared with VID 3151 and PID 1020, including
mouse interface MI_01. These IDs are used in the Pi-only modprobe configuration,
with grabbing disabled by default. Receiver serial identifiers are omitted.
Linux descriptors, name, event capabilities, real button behavior and actual
Pi integration remain to be captured and validated on the borrowed board.

Source runner: tests/qemu_device_matching.py. Full transcript is adjacent.
