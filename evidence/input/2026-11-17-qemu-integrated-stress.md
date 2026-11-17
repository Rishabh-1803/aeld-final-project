# Integrated QEMU overflow and teardown acceptance

Date: 17 November 2026. Poky 5.0.21, QEMU ARM64, kernel 6.6.151.
The diagnostic daemon was running while a uinput tablet generated pressure.

| Test | Received | Processed | Dropped | Result |
|---|---:|---:|---:|---|
| Overflow and recovery | 250008 | 8 | 250000 | PASS; fresh DOT/DASH/COMMIT emitted A |
| Teardown under pressure | 250002 | 1 | 250001 | PASS; device destruction completed |

Global telemetry after both runs: received 500010, processed 9, dropped
500001, input overflow incidents 2. The current counter represents coalesced
worker recovery incidents; it must not be compared directly with older
per-queue-full counts recorded by earlier module versions.

The module was restored to the normal QEMU USB Tablet selector. Fresh
DOT/DASH/COMMIT yielded event type 3, ASCII 65, sequence .- in the HUD.
The final dmesg scan returned no BUG/Oops/WARNING/hung-task matches.
The test does not establish that the worker was executing at the exact
instant of destruction. The teardown synchronizes deferred work before freeing
state; pressure and successful completion are the observed runtime evidence.

`tests/qemu_integrated_stress.py` reproduces the run. Its utility requires
root, uinput, CPU affinity and SCHED_FIFO support. It temporarily uses FIFO
priority 1 while submitting a bounded 250000-event burst, restores ordinary
scheduling and destroys the synthetic device. Use only in the development
image. No physical mouse or Raspberry Pi behavior is inferred from this test.
