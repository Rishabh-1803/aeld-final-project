# QEMU uinput virtual-keyboard self-test

**Date:** 2 November 2026  
**Target:** QEMU ARM64 `core-image-morse-dev`  
**Kernel:** `6.6.151-yocto-standard`  
**Utility:** `uinput-selftest`, built by the Yocto recipe in `meta-morse-mouse`

## Test performed

Inside the booted development image, logged in as root and confirmed:

```text
root@qemuarm64:~# ls -l /dev/uinput
crw------- 1 root root 10, 223 Nov 2 16:54 /dev/uinput
```

Then ran:

```bash
uinput-selftest
```

The utility created the virtual keyboard and the kernel registered it:

```text
input: MorseMouse-uinput-selftest as /devices/virtual/input/input2
Virtual keyboard registered as /dev/input/event2
```

The utility injected a `KEY_A` press and release, and read them back from the new evdev node:

```text
Verifying events through evdev:
  EV_KEY / KEY_A (30), value 1
  EV_SYN / SYN_REPORT
  EV_KEY / KEY_A (30), value 0
  EV_SYN / SYN_REPORT
PASS: virtual keyboard creation and KEY_A press/release readback verified.
```

## Result

**PASS.** The guest kernel accepted the `/dev/uinput` request, registered the virtual keyboard, exposed it through evdev, and delivered the injected `KEY_A` press/release transitions to the test reader.

This verifies the userspace `uinput` → Linux input core → virtual keyboard → evdev readback path in QEMU.

## Scope and limitations

- The test was run as root because `/dev/uinput` is root-owned with mode `0600` in this image.
- It verifies only the `KEY_A` press/release path for the test virtual keyboard.
- It does not yet verify arbitrary keymaps, modifiers, multiple simultaneous keys, repeat handling, application-level text entry, or MorseMouse driver behavior.
- It does not validate the physical Zebronics PULSE mouse or Raspberry Pi target.

The QEMU input foundation is now verified in both directions: mouse events can be read from evdev, and a userspace-created virtual keyboard can emit key events that are read back through evdev.
