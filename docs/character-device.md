# MorseMouse diagnostic ABI v1

The authoritative definitions are in `include/morse_hid_uapi.h`.
The driver registers `/dev/morse_hid` as a misc device with mode 0600.
One open file is permitted at a time (EBUSY for another open). The
daemon owns this stream; HUD clients consume its snapshots instead.

## Record and ioctl contract

- Event: 104 bytes; info: 32 bytes; stats: 104 bytes. Fixed-width fields
  and aligned 64-bit values preserve layouts across compatible userspace.
- Reads accept zero bytes or whole multiples of the event size. Other
  sizes return EINVAL. Multiple available records can be returned together.
- Empty O_NONBLOCK reads return EAGAIN; blocking reads wait interruptibly.
- Failed copies return EFAULT without consuming the affected record.
  If earlier records were copied, read returns their byte count.
- poll reports POLLIN/POLLRDNORM when records are queued.
- GET_INFO (`_IOR(0xB9, 0, info)`) returns version, sizes and features.
- GET_STATS (`_IOR(0xB9, 1, stats)`) returns counters, queue depths and
  input-connected flag. Counters are individually atomic; the snapshot
  is not a transaction across all fields.
- Unknown commands, wrong directions and wrong encoded sizes return ENOTTY.
  Invalid output pointers return EFAULT. Compat ioctl uses compat_ptr_ioctl.
- No write/control/power ioctl exists. Reserved output fields are zero.

The ring holds 128 records and drops the oldest on overflow. Loss counters
increase; newly inserted records carry STREAM_GAP when displacement occurs.
Consumers must also check event IDs and statistics rather than reconstruct
decoder state across lost records. Sequence strings are NUL-terminated,
bounded to 63 symbols. Module reload resets IDs/counters.

Stop the reader before unloading. Open descriptors pin the module; normal
rmmod fails while it is in use. Physical disconnect generates a diagnostic
record and leaves the interface available for reconnection.

See `evidence/input/2026-11-17-qemu-character-device.md` for acceptance.

[November 18 follow-up](../evidence/input/2026-11-18-qemu-character-device.md) explicitly exercised the queued-event EFAULT/retention check skipped in the owner review.
