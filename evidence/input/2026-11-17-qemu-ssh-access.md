# Optional SSH access acceptance

Date: 17 November 2026. QEMU ARM64; optional local test access enabled.
The updated development image build completed 4,287 tasks successfully.

- Installed only the dedicated client's public key in the guest.
- Read the guest SSH server public key through its serial console and used
  that key in a dedicated known-hosts file. StrictHostKeyChecking remained on.
- Login with the dedicated local Ed25519 client key succeeded (exit 0).
- SSH returned aarch64 and current HUD telemetry with ASCII 65 after a real
  QMP DOT/DASH/COMMIT sequence.
- The server advertised only publickey authentication. A password-only client
  was rejected with exit 255. No password was transmitted in that negative test.
- Guest shutdown stopped Dropbear and morsed cleanly.

This validates SSH in QEMU, not physical Wi-Fi association or Pi reachability.
The Wi-Fi credentials are in ignored local provisioning files and will be
installed only in the private Pi image. No private client key is in the image
or repository. The public provisioning code is opt-in and contains no credentials.

The initial provisioning recipe had a license-checksum typo. Yocto rejected
it; the checksum was corrected against the actual common MIT license before
this passing build. No QA check was disabled.
