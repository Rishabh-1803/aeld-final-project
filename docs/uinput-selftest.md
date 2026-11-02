# MorseMouse `uinput` self-test

This utility validates the userspace-to-kernel virtual-input path independently of the MorseMouse driver.

## What it tests

1. Opens `/dev/uinput`.
2. Registers a virtual keyboard supporting `KEY_A`.
3. Creates the virtual device and locates its evdev node by the exact device name in sysfs (it does not assume a fixed `eventN`).
4. Opens that evdev node.
5. Injects `KEY_A` press and release events, each followed by `SYN_REPORT`.
6. Reads the generated events back through evdev and checks for both values.
7. Destroys the virtual device, including on test failure after successful creation.

The program should be run as root in the current development image because `/dev/uinput` is mode `0600` and owned by root.

## Build and run

After pulling the project changes and initializing the build environment:

```bash
cd ~/projects/morsemouse
source poky/oe-init-build-env build-qemu
bitbake-layers show-recipes uinput-selftest
bitbake core-image-morse-dev
```

Boot the development image:

```bash
QB=tmp/deploy/images/qemuarm64/core-image-morse-dev-qemuarm64.rootfs.qemuboot.conf
../poky/scripts/runqemu "$QB" nographic slirp
```

Inside the guest, log in as root and execute:

```bash
uinput-selftest
```

Expected success output includes:

```text
MorseMouse uinput self-test
Creating virtual keyboard: MorseMouse-uinput-selftest
Virtual keyboard registered as /dev/input/eventN
Injecting KEY_A press, sync, release, sync...
Verifying events through evdev:
  EV_KEY / KEY_A (30), value 1
  EV_SYN / SYN_REPORT
  EV_KEY / KEY_A (30), value 0
  EV_SYN / SYN_REPORT
PASS: virtual keyboard creation and KEY_A press/release readback verified.
```

The actual `eventN` number can vary. A successful result proves that this kernel exposes a functioning `uinput` interface and that a synthetic keyboard's key transitions can be read back through evdev. It does not yet test the MorseMouse driver, keymap, or physical mouse.

If the command fails, record the complete output and `dmesg | tail -n 60`; do not mark the test complete until the cause is resolved.


## Recorded QEMU result — 2 November 2026

The updated development image built successfully and booted on QEMU ARM64. Running `uinput-selftest` as root created `MorseMouse-uinput-selftest`, registered it as `/dev/input/event2`, and read back `KEY_A` press (value 1) and release (value 0), each followed by `SYN_REPORT`. The program printed:

```text
PASS: virtual keyboard creation and KEY_A press/release readback verified.
```

See [the test evidence](../evidence/input/2026-11-02-uinput-virtual-keyboard-selftest.md). This is QEMU input-stack evidence only; it is not MorseMouse driver or physical hardware acceptance.
