# QEMU ARM64 project image build evidence

**Date:** 2 November 2026  
**Target:** `core-image-morse`  
**Machine:** `qemuarm64`  
**Yocto/Poky:** Scarthgap 5.0.21  
**Poky commit:** `4fb7016a07507683359ebd620f1951fd5ea44bfa`  
**MorseMouse layer revision used:** `be68005676611065954f57bdacf9c277fcd354c0`

## Build configuration observed

- `BB_NUMBER_THREADS = "10"`
- `PARALLEL_MAKE = "-j 6"`
- `MACHINE = "qemuarm64"`

The user elected to retain Make parallelism at 6 because the previous build had worked with that value.

## Build result

The first `bitbake core-image-morse` command completed successfully:

- 5,332 tasks attempted.
- All tasks succeeded; no task failures were reported.
- Seven fetch warnings were emitted while fetching source archives/Git sources and attempting configured mirrors.
- The terminal transcript did not show a failed task.

A second invocation was captured using `tee`:

```bash
bitbake core-image-morse 2>&1 | tee ../core-image-morse-build.log
```

BitBake reported 5,332 tasks, all up to date/successful, and one warning about the build host kernel not providing process I/O statistics because `CONFIG_TASK_IO_ACCOUNTING` is unset.

## Current validation boundary

This evidence establishes successful BitBake completion for the project-owned image recipe. It does **not** establish that the resulting deployed artifacts have been inspected or that the project image has booted in QEMU. Those remain pending.

## Next validation steps

1. Inspect `tmp/deploy/images/qemuarm64/` for the generated image, manifest, kernel and QEMU boot configuration.
2. Confirm the manifest/image names correspond to `core-image-morse`.
3. Launch QEMU using the generated project image, not the earlier `core-image-minimal` root filesystem.
4. Record root login, `uname -m`, `uname -r`, and available image diagnostics.
5. Preserve the complete terminal log as a build artifact if useful; avoid committing large rootfs/kernel binaries.
