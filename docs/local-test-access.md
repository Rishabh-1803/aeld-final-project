# Optional private Wi-Fi and SSH provisioning

Public builds default to MORSE_ENABLE_TEST_ACCESS=0. Credentials and client
private keys are never part of the source repository. The owner's local build
has this option enabled for the borrowed-board session.

## Reproduce local provisioning

Initialize build-rpi with the pinned BSP and project layers, then run:

```
python3 tools/provision_test_access.py --qemu-test
python3 tools/provision_test_access.py --check
```

The helper prompts for SSID and a hidden WPA passphrase, derives the PSK,
creates a dedicated Ed25519 client key, and updates ignored local.conf files.
The password is not saved in plaintext. The derived PSK remains a credential.
Do not publish ignored build directories, generated source archives/SBOM
artifacts containing private configuration, or the provisioned SD image.

Only the public login key is installed in /home/root/.ssh/authorized_keys.
The client private key remains in build-rpi/test-access-key with mode 0600.
The Wi-Fi configuration is installed with mode 0600 under
/etc/wpa_supplicant/wpa_supplicant-wlan0.conf on the Pi only.

The optional Pi access package includes Wi-Fi tools, USB diagnostics and mDNS.
The Pi hostname is morsemouse-pi. Try morsemouse-pi.local on the same Wi-Fi,
or use the IP shown on its display/router. SSH uses the dedicated key;
password authentication is disabled. Verify the actual server host key before
adding it to a known-hosts file. Do not disable host-key checking.

The existing development console policy is unchanged. This is a local
project/test image, not a claim of production hardening. No port forwarding
or public Internet exposure is configured.

Wi-Fi uses the provided local profile. No regulatory country was supplied;
no country code has been invented. Association, actual radio behavior and
Pi SSH reachability must be verified on the board. If a country/channel or
WPA mode adjustment is needed, confirm the actual network requirements.

See the QEMU SSH acceptance report for positive key login and rejected
password authentication. Stop morsed before using the exclusive diagnostic
self-test reader or unloading the module.

All VM runners are intended to run one at a time. They use snapshot disks, a per-process QMP socket and build-qemu/test-results logs, so tests do not modify the base image.
