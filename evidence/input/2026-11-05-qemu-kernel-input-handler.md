# QEMU kernel input-handler runtime evidence

**Date:** 5 November 2026  
**Target:** Yocto Scarthgap 5.0.21, `qemuarm64`  
**Kernel:** `6.6.151-yocto-standard`  
**Driver:** `hid_morse_kbd.ko` version `0.1`  
**Source commit:** `ed88aa9` — Add QEMU-tested Linux input handler

## Build and image integration

The out-of-tree module recipe `hid-morse-kbd_0.1.bb` built successfully. The module was packaged as `kernel-module-hid-morse-kbd-6.6.151-yocto-standard` and installed through the `hid-morse-kbd` recipe dependency into the QEMU development image.

The development image `core-image-morse-dev-qemuarm64.rootfs-20261104100611.ext4` booted successfully. The manifest contained both the main recipe package and the versioned kernel-module package.

## Automatic load and device matching

The module was automatically loaded from `/etc/modules-load.d/hid_morse_kbd.conf`.

Observed messages:

```text
hid_morse_kbd: connected to "QEMU QEMU USB Tablet"; grab=disabled
hid_morse_kbd: registered; target_name="USB Tablet", grab_on_bind=false
```

The tablet exposed `event0 hid_morse_kbd`. The QEMU USB Keyboard remained separate with `sysrq kbd event1`. This verified the targeted handler did not bind to the keyboard.

## Button event receipt

QEMU HMP generated left, right and middle press/release events. `evtest` on the tablet and the kernel log both showed all six transitions:

```text
button=LEFT action=PRESS value=1 count=1
button=LEFT action=RELEASE value=0 count=2
button=RIGHT action=PRESS value=1 count=3
button=RIGHT action=RELEASE value=0 count=4
button=MIDDLE action=PRESS value=1 count=5
button=MIDDLE action=RELEASE value=0 count=6
```

The event codes observed by evtest were `BTN_LEFT=272`, `BTN_RIGHT=273`, and `BTN_MIDDLE=274`, with matching press values `1` and release values `0`.

## Exclusive grab and restoration

The module was unloaded and reloaded using:

```sh
modprobe -r hid_morse_kbd
modprobe hid_morse_kbd grab_on_bind=1
```

The parameter read back as `Y`, and the driver logged `grab=enabled`. While grabbed, QEMU HMP generated all three button press/release pairs. The MorseMouse driver logged all six transitions, while a simultaneous `evtest` process on `/dev/input/event0` received no input events.

The module was then unloaded. The tablet returned to `Handlers=event0`; a new evtest capture received all six subsequent button transitions. This verified release of the exclusive grab and restoration of ordinary input delivery.

## Linux HID unbind/rebind lifecycle

With the module loaded, the tablet HID interface `1-1:1.0` was unbound from `usbhid`:

```sh
echo 1-1:1.0 > /sys/bus/usb/drivers/usbhid/unbind
```

The handler logged a disconnect; the tablet disappeared from `/proc/bus/input/devices`; the kernel module remained loaded. Rebinding with:

```sh
echo 1-1:1.0 > /sys/bus/usb/drivers/usbhid/bind
```

created a new input device (`input2`) and the loaded handler automatically connected to it. The keyboard stayed attached throughout.

## Genuine QEMU USB hot-unplug/hot-replug

For actual QEMU bus removal, a separate QEMU boot configuration assigned the tablet the ID `morse-tablet`. HMP `device_del morse-tablet` removed USB port 1. Linux logged:

```text
usb 1-1: USB disconnect, device number 2
hid_morse_kbd: disconnected from "QEMU QEMU USB Tablet"; received_events=0
```

The QEMU USB keyboard on port 2 remained attached, and `hid_morse_kbd` remained loaded.

The tablet was re-added to the same port using:

```text
device_add usb-tablet,bus=usb-bus.0,port=1,id=morse-tablet-replug1
```

Linux enumerated a new USB device (device number 4), registered a new input device (`input2`), and the handler automatically reconnected:

```text
usb 1-1: new high-speed USB device number 4 using xhci_hcd
input: QEMU QEMU USB Tablet ... input2
hid_morse_kbd: connected to "QEMU QEMU USB Tablet"; grab=disabled
```

The final input-device inventory showed `Handlers=event0 hid_morse_kbd` for the reconnected tablet and `Handlers=sysrq kbd event1` for the keyboard.

## Kernel warning check

The guest check:

```sh
dmesg | grep -Ei 'BUG:|WARNING:|Oops:|hung task'
```

returned no matching messages during the captured test session.

## Scope and limitations

This evidence validates the QEMU development target only. It does not validate the physical Zebronics PULSE receiver, Raspberry Pi 4, GUI text insertion, or the later kernel FSM, bounded event queue, virtual input keyboard, character-device UAPI, daemon, or HUD. Those remain separate work items. The physical stage remains deliberately deferred until QEMU-side implementation and integration are complete.
