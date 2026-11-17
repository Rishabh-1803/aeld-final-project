# Diagnostic daemon and terminal HUD

`morsed` owns the exclusive diagnostic stream and publishes bounded atomic
snapshots to `/run/morsemouse/status`. `morse_hud_client` reads that file;
it has no command, socket-write, driver-control or power-control path.

## Build and service

Host: `make -C userspace`. Yocto: `bitbake morsed core-image-morse-dev`.
The development image starts the daemon through SysV init. Run
`/etc/init.d/morsed start`, `stop`, or `restart`. Stop it before running
`morse-hid-test` or unloading the module because the stream is exclusive.

Run `morse_hud_client` for one snapshot or `morse_hud_client --watch`
for a one-second display loop. SIGINT/SIGTERM exits watch mode. The client
reports missing, invalid or stale telemetry explicitly. Stale means older
than three configured publication intervals, using CLOCK_MONOTONIC.

## Configuration

`/etc/morsemouse/morsed.conf` contains exact key=value lines:

```
device=/dev/morse_hid
interval_ms=1000
power_mode=disabled
```

Blank lines and whole-line comments beginning with # are accepted. Unknown
keys, malformed/overlong lines, relative device paths, and intervals outside
100–10000 ms are rejected. `morsed PATH --check-config` validates without
opening devices. `dry-run` is accepted as a disabled alias; execution modes
are rejected. No shell commands, power execution or remote controls exist.

Device opens are retried at the configured interval. An unavailable, busy or
incompatible driver is represented in the snapshot. A root-owned lock prevents
two daemons. SIGTERM publishes stopped status and closes the driver.

## Snapshot version 1

The newline-delimited key=value snapshot includes its version, monotonic
publication time, publication interval, daemon/driver status, power status,
input-connected flag, received/processed/dropped counts, overflow counters,
queue depths, and the most recent event ID/type/ASCII/sequence. Event types
are defined by the UAPI header. Event type 0 means no event observed since
the daemon opened the driver. The sequence describes the most recent event,
not a guaranteed reconstruction of current held-button or lockout state.
Loss notifications count discontinuous event IDs or STREAM_GAP records.
Statistics are individually sampled, not a transaction. During driver loss,
last-known counters remain visible beside the unavailable driver status.
Snapshots use atomic rename so readers see one complete publication.

Events are processed in bounded batches of 16; idle waits use poll/nanosleep.
HUD file reads are capped at 4095 bytes. The daemon publishes no credentials
or configuration secrets. See the QEMU daemon/HUD acceptance report for tests.

Power-control gestures and execution are deferred until after final submission.
