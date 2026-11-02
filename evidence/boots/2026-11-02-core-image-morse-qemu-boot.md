# QEMU ARM64 boot evidence — core-image-morse

**Date:** 2 November 2026  
**Image:** `core-image-morse`  
**Machine:** `qemuarm64`  
**Rootfs:** `core-image-morse-qemuarm64.rootfs-20261102154400.ext4`  
**QEMU config:** `core-image-morse-qemuarm64.rootfs.qemuboot.conf`

## Launch procedure

The automatic shorthand launch initially failed because `IMAGE_LINK_NAME` was not available to the image-discovery path in the pinned `runqemu` script. The image's generated QEMU configuration was therefore supplied explicitly:

```bash
cd ~/projects/morsemouse/build-qemu
QB=tmp/deploy/images/qemuarm64/core-image-morse-qemuarm64.rootfs.qemuboot.conf
../poky/scripts/runqemu "$QB" nographic slirp
```

The runqemu startup output explicitly resolved:

- Machine: `qemuarm64`
- Filesystem type: `ext4`
- Rootfs: `core-image-morse-qemuarm64.rootfs-20261102154400.ext4`
- Kernel image: `tmp/deploy/images/qemuarm64/Image`
- QEMU machine: `virt`
- CPU: Cortex-A57
- CPUs: 4
- Guest RAM: 256 MiB
- Networking: SLIRP with DHCP
- Host forwarding: 127.0.0.1:2222 to guest port 22; 127.0.0.1:2323 to guest port 23

This confirms that QEMU booted the project rootfs, not the earlier `core-image-minimal` rootfs.

## Boot and login result

The kernel booted and mounted the EXT4 rootfs successfully. The guest reached:

```text
Poky (Yocto Project Reference Distro) 5.0.21 qemuarm64 /dev/ttyAMA0
qemuarm64 login:
```

Root login succeeded.

Commands and observed output:

| Command | Result |
|---|---|
| `uname -a` | Linux `qemuarm64`, kernel `6.6.151-yocto-standard`, AArch64 GNU/Linux |
| `uname -m` | `aarch64` |
| `uname -r` | `6.6.151-yocto-standard` |
| `cat /proc/version` | GCC 13.5.0; GNU Binutils 2.42.0.20240723; kernel 6.6.151 |
| `df -h /` | 19.3M total, 12.9M used, 4.9M available, 72% used |

## Network

The guest obtained DHCP successfully:

- Guest IP: `10.0.2.15`
- Gateway: `10.0.2.2`
- DNS: `10.0.2.3`

The startup sequence printed `ip: RTNETLINK answers: File exists` while configuring network interfaces, but DHCP succeeded. This is an observation, not a boot failure.

## Input subsystem observations

The boot log showed:

- USB HID core driver registered.
- QEMU USB tablet enumerated as `QEMU QEMU USB Tablet` and bound to `hid-generic`.
- QEMU USB keyboard enumerated as `QEMU QEMU USB Keyboard` and bound to `hid-generic`.
- The guest's `/dev` listing included `/dev/input` and `/dev/uinput`.

The existence of `/dev/uinput` is preliminary evidence only. The guest's input capabilities, kernel configuration symbols, and successful creation/injection of a synthetic device have not yet been tested. `evtest` has not yet been added to the image.

## Conclusion

**The minimal-composition `core-image-morse` QEMU image build and boot are verified.** This validates the Yocto image integration and basic ARM64 boot path. It does not demonstrate Morse decoding, kernel module behavior, virtual keyboard output, daemon, HUD, or physical Raspberry Pi operation.
