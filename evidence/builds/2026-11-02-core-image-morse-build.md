# QEMU ARM64 project image build evidence

**Date:** 2 November 2026  
**Target:** `core-image-morse`  
**Machine:** `qemuarm64`  
**Yocto/Poky:** Scarthgap 5.0.21  
**Poky commit:** `4fb7016a07507683359ebd620f1951fd5ea44bfa`  
**MorseMouse layer revision used by the build:** `be68005676611065954f57bdacf9c277fcd354c0`

## Build configuration

- `MACHINE = "qemuarm64"`
- `BB_NUMBER_THREADS = "10"`
- `PARALLEL_MAKE = "-j 6"`

Make parallelism at 6 was retained because the user reported that the previous build succeeded with this setting.

## Initial image attempt and recipe refinement

The first project image recipe inherited `core-image` defaults and appended the MorseMouse package group. Its resulting manifest contained a broader set of packages, including BlueZ, ALSA, Avahi, D-Bus and X11 libraries. This was more than the project needed at the initial stage.

The recipe was refined to follow the upstream `core-image-minimal` package composition:

```bitbake
IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"
IMAGE_LINGUAS = " "
inherit core-image
IMAGE_INSTALL:append = " packagegroup-morse-mouse"
IMAGE_ROOTFS_SIZE ?= "8192"
IMAGE_ROOTFS_EXTRA_SPACE:append = "${@bb.utils.contains("DISTRO_FEATURES", "systemd", " + 4096", "", d)}"
```

## Final minimal-composition build result

Command:

```bash
set -o pipefail
bitbake core-image-morse 2>&1 | tee ../core-image-morse-minimal-build.log
```

Result from the user's terminal:

- 4,083 tasks attempted.
- 4,072 tasks did not need to be rerun.
- All tasks succeeded; no failed tasks were reported.
- One warning: the build host kernel does not provide process I/O statistics because `CONFIG_TASK_IO_ACCOUNTING` is unset.

The earlier broader-image build had 5,332 successful tasks and warnings; it was superseded by the minimal-composition recipe and final build above.

## Manifest comparison

The user compared sorted package names between the baseline `core-image-minimal` manifest and the final `core-image-morse` manifest:

```bash
comm -3 \
  <(awk '{print $1}' "$BASE" | sort) \
  <(awk '{print $1}' "$MORSE" | sort)
```

The only additional package name was:

```text
packagegroup-morse-mouse
```

This verifies that the final project's rootfs package set matches the baseline, plus the intentionally empty MorseMouse package group.

## Generated artifacts

Timestamp: `20261102154400`

| Artifact | Observed size |
|---|---:|
| `core-image-morse-qemuarm64.rootfs-20261102154400.ext4` | 23 MB |
| Manifest | 944 bytes |
| QEMU boot config | 1.9 KB |
| SPDX archive | 122 KB |
| Compressed rootfs tarball | 5.4 MB |
| Testdata JSON | 264 KB |

The deploy directory also contains the standard unversioned symlinks to the timestamped rootfs, manifest, QEMU config and auxiliary artifacts.

## QEMU boot

The image was launched by passing its generated QEMU configuration explicitly:

```bash
QB=tmp/deploy/images/qemuarm64/core-image-morse-qemuarm64.rootfs.qemuboot.conf
../poky/scripts/runqemu "$QB" nographic slirp
```

The boot transcript confirms that the selected rootfs was:

`core-image-morse-qemuarm64.rootfs-20261102154400.ext4`

The guest reached the Poky login prompt, root login succeeded, `uname -m` returned `aarch64`, and `uname -r` returned `6.6.151-yocto-standard`. DHCP assigned `10.0.2.15`. See [QEMU boot evidence](../boots/2026-11-02-core-image-morse-qemu-boot.md).

## Validation boundary

The minimal project image build and QEMU boot are verified. This does not mean MorseMouse input decoding or application functionality is implemented. The guest showed the QEMU USB tablet/keyboard enumerating through HID and a `/dev/uinput` node, but synthetic uinput event injection has not yet been exercised.
