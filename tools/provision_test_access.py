#!/usr/bin/env python3
"""Provision ignored local Wi-Fi settings and a dedicated SSH test key."""
import argparse
import getpass
import hashlib
import pathlib
import re
import shutil
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--check', action='store_true', help='validate existing local provisioning')
parser.add_argument('--qemu-test', action='store_true', help='enable the same public key in the QEMU test image')
args = parser.parse_args()
private = root / 'build-rpi/private-files'
key = root / 'build-rpi/test-access-key'
if args.check:
    public = subprocess.check_output(['ssh-keygen', '-y', '-f', str(key)], text=True).split()[:2]
    if (private / 'authorized_keys').read_text().split()[:2] != public:
        raise SystemExit('Public and private test keys do not match')
    if not re.search(r'^\s*psk=[0-9a-f]{64}$', (private / 'wpa_supplicant.conf').read_text(), re.M):
        raise SystemExit('Missing derived Wi-Fi PSK')
    for path in [key, private / 'authorized_keys', private / 'wpa_supplicant.conf']:
        if path.stat().st_mode & 0o077:
            raise SystemExit('Private provisioning file permissions must exclude other users')
    print('Local provisioning files and dedicated key validated; no secret values displayed.')
    raise SystemExit(0)

builds = ['build-rpi'] + (['build-qemu'] if args.qemu_test else [])
for build in builds:
    if not (root / build / 'conf/local.conf').exists():
        raise SystemExit('Initialize ' + build + ' with oe-init-build-env first')
ssid = input('Wi-Fi SSID: ').encode('utf-8')
password = getpass.getpass('Wi-Fi password (not saved in plaintext): ').encode('utf-8')
if not 1 <= len(ssid) <= 32 or not 8 <= len(password) <= 63:
    raise SystemExit('SSID must be 1–32 bytes and WPA passphrase 8–63 bytes')
private.mkdir(parents=True, exist_ok=True)
private.chmod(0o700)
if not key.exists():
    subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'morsemouse-test', '-f', str(key)], check=True)
key.chmod(0o600)
shutil.copy(pathlib.Path(str(key) + '.pub'), private / 'authorized_keys')
psk = hashlib.pbkdf2_hmac('sha1', password, ssid, 4096, 32).hex()
(private / 'wpa_supplicant.conf').write_text(
    'ctrl_interface=/run/wpa_supplicant\nupdate_config=0\nnetwork={\n'
    '    ssid=' + ssid.hex() + '\n    scan_ssid=1\n    key_mgmt=WPA-PSK\n    psk=' + psk + '\n}\n')
for path in private.iterdir():
    path.chmod(0o600)
for build in builds:
    config = root / build / 'conf/local.conf'
    lines = [line for line in config.read_text().splitlines()
             if not line.startswith(('MORSE_ENABLE_TEST_ACCESS =', 'MORSE_PRIVATE_FILES ='))]
    lines += ['MORSE_ENABLE_TEST_ACCESS = "1"', 'MORSE_PRIVATE_FILES = "' + str(private) + '"']
    if build == 'build-rpi':
        lines = [line for line in lines if not line.startswith('hostname:pn-base-files =')]
        lines.append('hostname:pn-base-files = "morsemouse-pi"')
    config.write_text('\n'.join(lines) + '\n')
print('Local provisioning complete. Keep ignored build directories and credential-bearing images private.')
