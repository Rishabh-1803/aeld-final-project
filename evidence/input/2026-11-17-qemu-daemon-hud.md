# QEMU daemon and HUD acceptance

Date: 17 November 2026. Target: QEMU ARM64 / Poky 5.0.21,
kernel 6.6.151-yocto-standard. No physical hardware results are claimed.

`bitbake morsed core-image-morse-dev` completed successfully (4,225 tasks).
The C programs also compiled on the host with Wall/Wextra/Wpedantic/Werror.

QEMU acceptance:

- SysV startup created a current snapshot: daemon running, driver ready,
  power disabled, input connected.
- Paced QMP DOT/DASH/COMMIT produced ASCII 65 in driver and HUD telemetry.
- Service restart produced a fresh ready snapshot with no fabricated event.
- Service stop published stopped status; after four seconds HUD reported STALE.
- Temporarily moving the generated snapshot produced telemetry unavailable.
- Starting without the module produced driver unavailable; loading the module
  recovered to ready without restarting the daemon.
- An unsafe power_mode=execute configuration was rejected with exit 2.
- interval_ms=0 was rejected with exit 2.
- A second daemon was rejected by the single-instance lock with exit 1.
- After stopping the service, character-device self-tests passed. The invalid
  read-pointer subtest was explicitly skipped in this run because the daemon
  had already consumed the connection record; its passing fresh-load coverage
  is recorded in the separate character-device acceptance transcript.
- Final dmesg scan returned no BUG/Oops/WARNING/hung-task matches.
- Guest shutdown stopped the daemon cleanly.

The attached raw transcript contains the exact commands and outputs. The
test runner is `tests/qemu_daemon_hud.py` (host Python pexpect required).

Power execution/gestures are deferred until after submission by the owner's
November 17 decision. No allowlist executor or physical confirmation gesture
is implemented or claimed. The submitted daemon cannot execute power actions.
