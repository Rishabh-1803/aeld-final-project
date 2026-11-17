# Raspberry Pi physical test session

## Confirmed arrangements

Borrowed Raspberry Pi 4, 8 GB, available for a few hours. The owner has the
Zebronics PULSE mouse/receiver, a microSD card, power supply, display/cable and
keyboard. The owner authorized erasing the microSD when ready; the exact
card/disk must still be identified before any write. No disk has been erased.

Windows observed USB VID 3151/PID 1020 after receiver connection. This is
preparation evidence only. Capture Linux descriptors and actual input events
on the Pi before declaring receiver acceptance. Do not publish serial numbers.

## Image preparation

An independent `build-rpi` directory uses MACHINE=raspberrypi4-64 and the pinned
meta-raspberrypi layer. It shares downloads/sstate with the QEMU build. UART is
enabled for recovery. Initialize with:

```
source poky/oe-init-build-env build-rpi
bitbake core-image-morse-dev
```

The production package group includes the driver and diagnostic service.
Development adds evtest, reference/self-test utilities and morsed-tests.
Inspect `tmp/deploy/images/raspberrypi4-64/` for the generated image and manifest.
Verify its checksum before writing the positively identified microSD card.
The Pi-only modprobe file uses exact vendor/product matching, an empty name
filter, and grab_on_bind=0. Reconfirm the actual Linux IDs before changing it.

## One borrowed-board session

1. Write and verify the image only after confirming the target card. Boot with
   display and keyboard; retain serial recovery access if available.
2. Capture uname, machine/board model, boot output, image manifest reference,
   module version/vermagic, device permissions and service status.
3. Capture receiver USB vendor/product IDs, descriptors, input names and
   capabilities. Stop morsed before diagnostic self-tests or module reload.
4. Use evtest on the receiver to verify left/right/middle transitions. Select
   nodes by sysfs names/identity, never a presumed event number.
5. Start the driver/service. Capture DOT/DASH, A, lowercase, digit, punctuation,
   empty and invalid commits, chord cancellation and recovery. Capture virtual
   keyboard press/release events with evtest and demonstrate actual text entry.
6. Check grab_on_bind=1 only on the confirmed receiver, then verify release on
   unload; preserve the separate ordinary keyboard for recovery.
7. Test unplug/replug, service restart, driver-unavailable recovery, module
   reload, no stuck modifiers and HUD missing/stale behavior.
8. Keep physical logs under evidence/hardware with image and commit IDs. Mark
   each acceptance item PASS/FAIL with actual observations. Do not copy QEMU
   results into this evidence.
9. Record the five-minute demonstration, capture remaining submission links,
   sync logs, stop the service and shut down the board normally before return.

Power gestures/execution are excluded from this session and deferred until
after submission. Plain operating-system shutdown for safe board removal is
not a claim that the Morse power-control feature exists.
