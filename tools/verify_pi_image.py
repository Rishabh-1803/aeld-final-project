from pathlib import Path
import bz2, hashlib, json, re, shutil, struct, subprocess, lzma

r=Path(__file__).resolve().parents[1]
deploy=r/'build-rpi/tmp/deploy/images/raspberrypi4-64'
archive=(deploy/'core-image-morse-dev-raspberrypi4-64.rootfs.wic.bz2').resolve()
work=r/'build-rpi/image-verification';work.mkdir(exist_ok=True)
raw=work/'morsemouse-dev.wic'
digest=hashlib.sha256()
with bz2.open(archive,'rb') as source,raw.open('wb') as target:
    while block:=source.read(1024*1024):
        digest.update(block);target.write(block)
with raw.open('rb') as f: mbr=f.read(512)
assert mbr[510:512]==b'\x55\xaa'
parts=[]
for index in range(4):
    entry=mbr[446+index*16:462+index*16]
    kind=entry[4];start,size=struct.unpack_from('<II',entry,8)
    if kind: parts.append(dict(index=index+1,type=hex(kind),offset=start*512,size=size*512))
fat=next(x for x in parts if x['type'] in ['0xb','0xc','0xe','0x6'])
root=next(x for x in parts if x['type']=='0x83')
filesystem=work/'rootfs.img'
with raw.open('rb') as source,filesystem.open('wb') as target:
    source.seek(root['offset']);remaining=root['size']
    while remaining:
        block=source.read(min(1024*1024,remaining))
        assert block
        target.write(block);remaining-=len(block)
debugfs=shutil.which('debugfs');assert debugfs
def debug(command):
    return subprocess.run([debugfs,'-R',command,str(filesystem)],capture_output=True,check=True).stdout
checks={}
for path,mode in [('/etc/wpa_supplicant/wpa_supplicant-wlan0.conf','0600'),('/home/root/.ssh/authorized_keys','0600'),('/home/root/.ssh','0700')]:
    text=debug('stat '+path).decode()
    assert 'Mode:  '+mode in text or re.search(r'Mode:\s+0?'+mode.lstrip('0')+r'\b',text),text
    assert re.search(r'User:\s+0\s+Group:\s+0',text),text
    checks[path]='root-owned, mode '+mode
assert debug('cat /etc/wpa_supplicant/wpa_supplicant-wlan0.conf')==(r/'build-rpi/private-files/wpa_supplicant.conf').read_bytes()
assert debug('cat /home/root/.ssh/authorized_keys')==(r/'build-rpi/private-files/authorized_keys').read_bytes()
assert b'-s -g' in debug('cat /etc/default/dropbear')
assert b'wpa-driver nl80211' in debug('cat /etc/network/interfaces')
assert debug('cat /etc/hostname').strip()==b'morsemouse-pi'
selector=debug('cat /etc/modprobe.d/morse-mouse.conf')
assert b'target_vendor=0x3151' in selector and b'target_product=0x1020' in selector
staging=r/'build-rpi/tmp/work/raspberrypi4_64-poky-linux/core-image-morse-dev/1.0/rootfs'
module=next(staging.rglob('hid_morse_kbd.ko*'))
path='/'+str(module.relative_to(staging))
data=debug('cat '+path)
if module.suffix=='.xz': data=lzma.decompress(data)
assert data[:4]==b'\x7fELF' and struct.unpack_from('<H',data,18)[0]==183
vermagic=re.search(rb'vermagic=([^\x00]+)',data).group(1).decode()
assert vermagic.startswith('6.6.63-v8 '),vermagic
mdir=r/'build-rpi/tmp/work/raspberrypi4_64-poky-linux/core-image-morse-dev/1.0/recipe-sysroot-native/usr/bin/mdir'
listing=subprocess.check_output([str(mdir),'-b','-i',str(raw)+'@@'+str(fat['offset']),'::'],text=True)
for name in ['kernel8.img','config.txt','bcm2711-rpi-4-b.dtb','start4.elf']:
    assert name in listing,(name,listing)
mtype=mdir.with_name('mtype')
def boot_file(name):
    return subprocess.check_output([str(mtype),'-i',str(raw)+'@@'+str(fat['offset']),'::'+name])
boot_config=boot_file('config.txt').decode()
cmdline=boot_file('cmdline.txt').decode().strip()
assert not re.search(r'^\s*arm_64bit\s*=\s*0\s*$',boot_config,re.M)
assert 'enable_uart=1' in boot_config
assert b'/sbin/getty 38400 tty1' in debug('cat /etc/inittab')
assert '/dev/vda' not in cmdline
assert 'root=/dev/mmcblk0p2' in cmdline or re.search(r'root=PARTUUID=[0-9a-fA-F]+-02',cmdline),cmdline
partuuid=re.search(r'root=PARTUUID=([0-9a-fA-F]+)-02',cmdline)
if partuuid: assert int(partuuid.group(1),16)==struct.unpack_from('<I',mbr,440)[0]
assert boot_file('kernel8.img')[56:60]==b'ARM\x64'
manifest=(deploy/'core-image-morse-dev-raspberrypi4-64.rootfs.manifest').read_text()
for package in ['hid-morse-kbd','morsed','morse-test-access','dropbear','wpa-supplicant','avahi-daemon','usbutils']:
    assert re.search(r'^'+re.escape(package)+r'\s',manifest,re.M),package
report=dict(artifact=archive.name,compressed_bytes=archive.stat().st_size,
            sha256=hashlib.file_digest(archive.open('rb'),'sha256').hexdigest(),
            image_bytes=raw.stat().st_size,image_sha256=digest.hexdigest(),
            partitions=parts,checks=checks,module_arch='AArch64',module_vermagic=vermagic,
            boot_files=['kernel8.img','config.txt','bcm2711-rpi-4-b.dtb','start4.elf'],
            boot_arm64=True,boot_root_partition_consistent=True,hdmi_login_console=True,
            private_profile_matches=True,key_only_ssh_config=True,physical_boot_tested=False)
(work/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ['artifact','compressed_bytes','image_bytes','module_arch','module_vermagic','physical_boot_tested']},indent=2))
print('Image integrity, partitions, boot files and private provisioning checks passed.')
