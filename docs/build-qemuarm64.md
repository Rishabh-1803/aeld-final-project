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

## Image composition

The project image inherits `core-image` but explicitly uses the same base package group as upstream `core-image-minimal`, then appends the MorseMouse package group:

```bitbake
IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"
IMAGE_LINGUAS = " "
inherit core-image
IMAGE_INSTALL:append = " packagegroup-morse-mouse"
```

This keeps the initial project image minimal. Development tools such as `evtest` should be added deliberately when needed, rather than pulling in unrelated desktop/audio/Bluetooth packages.

## Development image and input diagnostics

The minimal `core-image-morse` recipe remains unchanged. Input diagnostics are kept in a separate image:

```bitbake
# meta-morse-mouse/recipes-core/images/core-image-morse-dev.bb
require core-image-morse.bb
IMAGE_INSTALL:append = " evtest uinput-selftest"
```

The `evtest` recipe (version 1.36) is provided by the OpenEmbedded `meta-oe` layer. The `uinput-selftest` recipe and C program are maintained in this project layer. The external `meta-openembedded` checkout is pinned in [Source pins](../manifest/source-pins.md). Register it once:

```bash
git clone --branch scarthgap --single-branch https://github.com/openembedded/meta-openembedded.git meta-openembedded
source poky/oe-init-build-env build-qemu
bitbake-layers add-layer ../meta-openembedded/meta-oe
bitbake-layers show-recipes evtest uinput-selftest
```

Build and boot the diagnostic image:

```bash
bitbake core-image-morse-dev
QB=tmp/deploy/images/qemuarm64/core-image-morse-dev-qemuarm64.rootfs.qemuboot.conf
../poky/scripts/runqemu "$QB" nographic slirp
```

In the guest, run `evtest` and select the QEMU USB Tablet. The 2 Nov 2026 tests verified its advertised `BTN_LEFT`, `BTN_RIGHT`, `BTN_MIDDLE`, `REL_WHEEL`, and `REL_WHEEL_HI_RES` capabilities, and captured actual button press/release and wheel up/down events. See [QEMU evdev runtime evidence](../evidence/input/2026-11-02-qemu-evdev-runtime-events.md).

The development image also includes `uinput-selftest`. Run it as root to create a temporary virtual keyboard, inject `KEY_A` press/release events, and verify those events through evdev. See [uinput self-test instructions](uinput-selftest.md). The updated image must be rebuilt after pulling the test utility changes. QEMU results do not validate the physical Zebronics receiver.

## Build targets

The baseline target was successfully built:

```bash
bitbake core-image-minimal
```

Build the project image:

```bash
bitbake core-image-morse
```

The final minimal-composition build on 2 Nov 2026 attempted 4,083 tasks; all succeeded. The final manifest's package-name set was identical to the baseline plus `packagegroup-morse-mouse`.

## Inspect image output

```bash
ls -lh tmp/deploy/images/qemuarm64/ | grep core-image-morse
```

Inspect the generated kernel, root filesystem, manifest and QEMU boot configuration. The verified final rootfs artifact was:

```text
core-image-morse-qemuarm64.rootfs-20261102154400.ext4
```

## Run QEMU

The `runqemu` script is a file at `poky/scripts/runqemu`, not a directory. If the command is not in PATH, invoke it through its relative path.

The automatic shorthand invocation `runqemu qemuarm64 core-image-morse ext4 nographic slirp` failed in this environment because `IMAGE_LINK_NAME` was not set for its automatic QEMU config lookup. The generated QEMU config was then provided explicitly and booted successfully:

```bash
QB=tmp/deploy/images/qemuarm64/core-image-morse-qemuarm64.rootfs.qemuboot.conf
../poky/scripts/runqemu "$QB" nographic slirp
```

This selected the timestamped `core-image-morse` rootfs, not the `core-image-minimal` rootfs.

## Verified project-image boot

The 2 Nov 2026 guest validation reported:

- Root login succeeded at the Poky 5.0.21 `qemuarm64` login prompt.
- `uname -m` returned `aarch64`.
- `uname -r` returned `6.6.151-yocto-standard`.
- DHCP assigned `10.0.2.15`.
- QEMU used 4 CPUs and 256 MiB guest RAM.
- The guest root filesystem reported 19.3 MiB total, 12.9 MiB used, 4.9 MiB available.
- QEMU USB tablet and keyboard were enumerated by the USB HID/input stack.
- `/dev/uinput` was available with root-only permissions.
- The MorseMouse reference decoder created a virtual keyboard and emitted verified keyboard events for printable characters. See [QEMU uinput keyboard evidence](../evidence/input/2026-11-03-qemu-uinput-keyboard.md).

The network startup printed `ip: RTNETLINK answers: File exists`, but DHCP completed successfully. See [boot evidence](../evidence/boots/2026-11-02-core-image-morse-qemu-boot.md).

## Baseline notes

The earlier `core-image-minimal` guest also reported `aarch64` and kernel `6.6.151-yocto-standard`. Its lack of `/etc/os-release` and `tree` was expected for the minimal image and did not indicate a boot failure.

## Latest QEMU MorseMouse FSM integration

The 4 November 2026 development image includes the v1 userspace FSM in `morse-ref-decoder`.

After boot, identify the tablet by its sysfs device name (QEMU reports `QEMU QEMU USB Tablet`; do not assume an event number):

```bash
MOUSE_NODE=""
for p in /sys/class/input/event*/device/name; do
    [ -r "$p" ] || continue
    case "$(cat "$p")" in
        *"USB Tablet"*)
            EVENT_SYS="${p%/device/name}"
            MOUSE_NODE="/dev/input/${EVENT_SYS##*/}"
            break
            ;;
    esac
done
```

Start the reference decoder and uinput keyboard:

```bash
morse-ref-decoder \
    --map /usr/share/morsemouse/morse_map.csv \
    --evdev "$MOUSE_NODE" \
    --uinput
```

In the QEMU HMP monitor, `mouse_button 1` holds left, `mouse_button 2` holds right, `mouse_button 4` holds middle, `mouse_button 3` holds left+right, and `mouse_button 0` releases all buttons.

The 4 November 2026 integration run verified empty COMMIT, `.-` decoding as ASCII `A`, left/right overlap cancellation, lockout until both buttons were released, successful recovery, and virtual-keyboard Shift+A press/release events.

See [QEMU FSM integration evidence](../evidence/input/2026-11-04-qemu-fsm-integration.md). This remains QEMU emulated-tablet evidence; physical Zebronics/Pi validation is deferred to the final hardware stage.
