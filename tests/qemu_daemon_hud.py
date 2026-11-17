import json, socket, time, pathlib, pexpect, os

root = pathlib.Path(os.environ.get('MORSE_PROJECT_ROOT', str(pathlib.Path(__file__).resolve().parents[1])))
sock = '/tmp/morse-acceptance-qmp.sock'
pathlib.Path(sock).unlink(missing_ok=True)
log = open(str(root / 'morse-daemon-acceptance.log'), 'w')
cmd = 'source poky/oe-init-build-env build-qemu >/dev/null && runqemu tmp/deploy/images/qemuarm64/core-image-morse-dev-qemuarm64.rootfs.qemuboot.conf nographic slirp qemuparams="-qmp unix:'+sock+',server=on,wait=off"'
p = pexpect.spawn('/bin/bash', ['-lc', cmd], cwd=str(root), encoding='utf-8', timeout=120)
p.logfile = log
p.expect('login:'); p.sendline('root'); p.expect('root@qemuarm64:~#')
q = socket.socket(socket.AF_UNIX); q.connect(sock); f = q.makefile('rwb', buffering=0)
f.readline()
def qmp(execute, arguments=None):
    f.write((json.dumps(dict(execute=execute, **({'arguments': arguments} if arguments else {})))+'\n').encode())
    while True:
        result=json.loads(f.readline())
        if 'error' in result: raise RuntimeError(result)
        if 'return' in result: return result['return']
qmp('qmp_capabilities')
counter = 0
def shell(command):
    global counter
    counter += 1
    marker = 'MORSE_COMMAND_DONE_' + str(counter)
    p.sendline(command + (' echo ' if command.rstrip().endswith('&') else '; echo ') + marker)
    p.expect(r'\r+\n' + marker + r'\r+\n')
    output = p.before
    p.expect('root@qemuarm64:~#')
    return output
def buttons(mask):
    qmp('human-monitor-command', {'command-line':'mouse_button '+str(mask)})
    time.sleep(.03)
def click(mask): buttons(mask); buttons(0)
def sequence(s):
    for c in s: click(1 if c=='.' else 2)
    click(4)

shell('sleep 2; morse_hud_client')
sequence('.-')
shell('sleep 2; morse_hud_client')
shell('/etc/init.d/morsed restart; sleep 2; morse_hud_client')
shell('/etc/init.d/morsed stop; sleep 4; morse_hud_client')
shell('mv /run/morsemouse/status /tmp/saved-status; morse_hud_client; mv /tmp/saved-status /run/morsemouse/status')
shell('rmmod hid_morse_kbd; /etc/init.d/morsed start; sleep 2; morse_hud_client')
shell('modprobe hid_morse_kbd; sleep 2; morse_hud_client')
shell("printf 'power_mode=execute\\n' > /tmp/unsafe.conf; morsed /tmp/unsafe.conf --check-config; echo CONFIG_RC=$?")
shell("printf 'interval_ms=0\\n' > /tmp/invalid.conf; morsed /tmp/invalid.conf --check-config; echo INTERVAL_RC=$?")
shell('morsed; echo SECOND_DAEMON_RC=$?')
shell('/etc/init.d/morsed stop; morse-hid-test --self-test; /etc/init.d/morsed start')
shell("dmesg | grep -iE 'BUG:|Oops:|WARNING:|hung task' || true")
shell('sync')
p.sendline('poweroff'); p.expect(pexpect.EOF)
log.close()
print('Daemon and HUD QEMU transcript saved')
