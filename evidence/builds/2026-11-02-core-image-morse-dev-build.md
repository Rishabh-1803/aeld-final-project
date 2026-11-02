# QEMU ARM64 core-image-morse-dev build and evtest discovery

**Date:** 2 November 2026  
**Machine:** `qemuarm64`  
**Yocto:** Poky Scarthgap 5.0.21  
**Purpose:** Add input diagnostics without changing the minimal `core-image-morse` image.

## Layer and recipe

The OpenEmbedded `meta-openembedded` checkout uses the Scarthgap branch at:

```text
2db6aece32c7f8ebf6015b468e1b4e21de072e3b
```

The `meta-oe` layer is registered in `build-qemu` as `openembedded-layer`. BitBake resolves `evtest` version `1.36` from that layer.

The development image recipe is `meta-morse-mouse/recipes-core/images/core-image-morse-dev.bb`. It requires `core-image-morse.bb` and appends only `evtest` to the image installation list. The base `core-image-morse` recipe is unchanged.

The evaluated package installation list was:

```
IMAGE_INSTALL="packagegroup-core-boot  packagegroup-morse-mouse evtest"
```

## Build result

Command:

```bash
bitbake core-image-morse-dev
```

Result: **4,121 tasks attempted; 4,087 did not need to be rerun; all tasks succeeded.** One host-kernel configuration warning was reported; there were no failed tasks.

The generated root filesystem was:

```text
core-image-morse-dev-qemuarm64.rootfs-20261102161417.ext4
```

## QEMU boot and evtest

The generated QEMU boot configuration successfully launched the development root filesystem. The guest reached the Poky 5.0.21 `qemuarm64` login prompt and root login succeeded.

Inside the guest:

```text
root@qemuarm64:~# evtest --version
evtest 1.36
```

The `evtest` scan found:

- `/dev/input/event0`: QEMU QEMU USB Tablet
- `/dev/input/event1`: QEMU QEMU USB Keyboard

The tablet's reported supported events included:

- `BTN_LEFT` (272), `BTN_RIGHT` (273), `BTN_MIDDLE` (274)
- `REL_WHEEL` (8), `REL_WHEEL_HI_RES` (11)
- `ABS_X` (0), `ABS_Y` (1)
- `MSC_SCAN` (4)

## What this proves—and does not prove

**Verified:** the development image builds and boots; `evtest` 1.36 is installed and can enumerate the QEMU tablet and keyboard; the tablet advertises the required three mouse buttons and vertical wheel capabilities.

**Still pending:** an actual click/scroll event capture was not recorded in this session; `/dev/uinput` virtual-device creation and event injection have not been tested; physical Zebronics PULSE receiver capabilities remain unverified. QEMU tablet capability results are not a substitute for physical-device validation.

The minimal production-oriented `core-image-morse` composition remains separate and unchanged.
