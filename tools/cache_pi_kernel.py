#!/usr/bin/env python3
"""Cache the pinned Pi kernel as a Yocto depth-one source tarball."""
import argparse
import pathlib
import re
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--downloads', type=pathlib.Path,
                    default=pathlib.Path.home() / 'yocto-cache/downloads')
args = parser.parse_args()
recipe = (root / 'meta-raspberrypi/recipes-kernel/linux/linux-raspberrypi_6.6.bb').read_text()
revision = re.search(r'SRCREV_machine = "([0-9a-f]{40})"', recipe).group(1)
branch = re.search(r'LINUX_RPI_BRANCH \?= "([^"]+)"', recipe).group(1)
directory = root / 'build-rpi/shallow-kernel'
directory.mkdir(parents=True, exist_ok=True)
url = 'https://github.com/raspberrypi/linux.git'
def git(*arguments):
    return subprocess.check_output(['git', '-C', str(directory), *arguments], text=True).strip()
if not (directory / '.git').exists():
    git('init')
    git('remote', 'add', 'origin', url)
if git('remote', 'get-url', 'origin') != url:
    raise SystemExit('Refusing to change a source directory with a different origin')
try:
    present = git('rev-parse', '--verify', revision + '^{commit}') == revision
except subprocess.CalledProcessError:
    present = False
if not present:
    git('fetch', '--depth=1', 'origin', revision + ':refs/remotes/origin/' + branch)
git('update-ref', 'refs/remotes/origin/' + branch, revision)
git('update-ref', 'refs/heads/' + branch, revision)
git('symbolic-ref', 'HEAD', 'refs/heads/' + branch)
assert git('rev-parse', 'HEAD') == revision
args.downloads.mkdir(parents=True, exist_ok=True)
archive = args.downloads / ('gitshallow_github.com.raspberrypi.linux.git_' +
                          revision[:7] + '-1_' + branch.replace('/', '.') + '.tar.gz')
if not archive.exists():
    temporary = archive.with_suffix(archive.suffix + '.tmp')
    subprocess.run(['tar', '-czf', str(temporary), '-C', str(directory), '.git'], check=True)
    temporary.replace(archive)
print('Pinned commit:', revision)
print('Yocto shallow source cache:', archive)
