# QEMU character-device acceptance

Date: 17 November 2026. Target: QEMU ARM64, Poky 5.0.21,
Linux 6.6.151-yocto-standard. This is emulated-tablet evidence;
no Raspberry Pi or physical receiver test is claimed.

## Results

- Booted the existing November 7 development image.
- `/dev/morse_hid` appeared with root-only mode 0600.
- ABI v1: event size 104 bytes, diagnostic capacity 128, features 0x1f.
- GET_INFO, GET_STATS, exclusive open, unsupported ioctl (ENOTTY),
  invalid ioctl pointer (EFAULT), invalid read size (EINVAL), and invalid
  read pointer with event retention passed.
- Empty nonblocking read returned EAGAIN; empty poll timed out.
- A blocking reader woke after a QEMU left-button press.
- DOT, DASH, valid A, empty COMMIT, invalid ten-DOT COMMIT and chord
  cancellation were observed as diagnostic records.
- evtest captured Shift down, A down, synchronization, A up, Shift up,
  synchronization for a valid A commit.
- 140 paced DOT presses filled the diagnostic ring. Retained records
  carried STREAM_GAP (flags 0x1). Sequence overflow also reset the buffer.
- The paced run processed 328 of 328 input transitions without input loss.
  Input-queue stress acceptance remains separately documented in the
  November 5/6 evidence; it is not inferred from this paced run.
- Unload removed the node; reload recreated it and reset statistics.
  The character-device self-test passed after reload.
- No BUG/Oops/WARNING/hung-task matches were returned by the final dmesg check.

Host regression: all 95 decoder/keymap entries, 52 FSM checks and 58
shared vectors passed; generated kernel map matched the CSV.

## Reproduction

Run `make -C tools test`, build `hid-morse-kbd morse-hid-test
core-image-morse-dev`, and boot the explicit development qemuboot.conf.
Run `morse-hid-test --self-test` in the guest. Use `--wait` for a
blocking single-event read and `--watch` for continuous diagnostics.
QEMU HMP `mouse_button` masks are 1=left, 2=right, 4=middle, 0=release.
The automated runner requires host Python pexpect and uses QMP for paced
input; its raw transcript is included alongside this report.

Module unload with an open file is intentionally prevented by the module
reference held by file_operations.owner. Stop readers before unloading;
forced unload is not supported or tested.
