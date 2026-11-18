# QEMU diagnostic follow-up — 18 November 2026

Purpose: exercise the invalid read-pointer case skipped during the owner review,
and make the runtime runner fail when a self-test reports a failure.

Command: `python3 tests/qemu_character_device.py` from the repository root.
Uses the existing development image in snapshot mode (rootfs 20261117083010,
Linux 6.6.151-yocto-standard). No driver/image source changed for this follow-up.
The virtual keyboard is now discovered by sysfs name rather than event number.

Result: runner exited 0. Five self-test invocations printed
`RESULT: ALL EXECUTED TESTS PASSED` and `SELF_TEST_RC=0`; no [FAIL] lines.
Four invocations exercised `Invalid read pointer returns EFAULT without
consuming event`. The empty-queue invocation may skip that case; the explicit
queued-record assertion ensures it is exercised elsewhere in this run.
Keyboard output contains KEY_A events; module removal printed NODE_REMOVED,
and reloaded-driver self-test passed. No BUG/Oops/WARNING/hung-task output was
returned by the guest kernel-log check. QEMU powered off normally.

[Full transcript](2026-11-18-qemu-character-device.log).
This closes the review's coverage gap without changing its historical SKIP.
Physical hardware behavior remains pending. The runner now asserts every
self-test result; other captured observations are reviewed from its transcript.
