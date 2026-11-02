# Build and run on QEMU ARM64

## Host and source versions

The verified development host is WSL 2 with Ubuntu 24.04.5 LTS. The workspace used during Sprint 1 is:

```text
/home/rishabh_shah/projects/morsemouse
```

Pinned sources are documented in [Source pins](../manifest/source-pins.md). Keep source and build directories inside the WSL Linux filesystem rather than under `/mnt/c` or `/mnt/d`.

## Initialize the build environment

From the workspace root:

```bash
cd ~/projects/morsemouse
source poky/oe-init-build-env build-qemu
```

Sourcing this script changes the current directory to `build-qemu`.

## Machine and caches

The verified QEMU build uses:

```bitbake
MACHINE = "qemuarm64"
DL_DIR = "/home/rishabh_shah/yocto-cache/downloads"
SSTATE_DIR = "/home/rishabh_shah/yocto-cache/sstate-cache"
BB_NUMBER_THREADS = "10"
PARALLEL_MAKE = "-j 6"
```

These are the values observed in the project workspace on 2 November 2026. Adapt absolute cache paths to the host when reproducing the build. The Make parallelism value of 6 is intentionally retained from the working user configuration; monitor memory pressure during builds.

## Add the project layer

The project layer is `../meta-morse-mouse` relative to `build-qemu`. Add it once:

```bash
bitbake-layers add-layer ../meta-morse-mouse
bitbake-layers show-layers
```

If the layer is already listed, do not add it a second time.

## Build targets

The baseline target was successfully built:

```bash
bitbake core-image-minimal
```

The project-owned image recipe is:

```bash
bitbake core-image-morse
```

**Status note:** the baseline `core-image-minimal` build and QEMU boot are verified. The `core-image-morse` recipe and package group are discovered by BitBake and pass metadata parsing, but the project image build/boot must still be run and evidenced.

## Inspect image output

```bash
ls -lh tmp/deploy/images/qemuarm64/
```

Inspect the generated kernel, root filesystem, manifest, and QEMU boot configuration before launching. Do not assume a timestamped artifact name.

## Run QEMU

Check the available syntax first:

```bash
runqemu --help
```

The previously verified baseline command was:

```bash
runqemu qemuarm64 nographic slirp
```

After the project image is built, confirm that `runqemu` selects the `core-image-morse` artifacts (or use the generated boot configuration as supported by the installed `runqemu`). Capture the actual command and boot transcript as evidence.

## Baseline validation facts

The recorded baseline guest reported:

- Architecture: `aarch64`
- Kernel: `6.6.151-yocto-standard`
- Root login successful
- DHCP address: `10.0.2.15`

The minimal image did not contain `/etc/os-release` or the `tree` utility; these observations did not indicate a failed build or boot.
