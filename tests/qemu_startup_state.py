import json, socket, time, pathlib, pexpect, os

root = pathlib.Path(os.environ.get('MORSE_PROJECT_ROOT', str(pathlib.Path(__file__).resolve().parents[1])))
sock = '/tmp/morse-acceptance-qmp-' + str(os.getpid()) + '.sock'
pathlib.Path(sock).unlink(missing_ok=True)
results = root / 'build-qemu/test-results'
results.mkdir(parents=True, exist_ok=True)
log = open(results / 'morse-startup-acceptance.log', 'w')
cmd = 'source poky/oe-init-build-env build-qemu >/dev/null && runqemu tmp/deploy/images/qemuarm64/core-image-morse-dev-qemuarm64.rootfs.qemuboot.conf nographic slirp qemuparams="-snapshot -qmp unix:'+sock+',server=on,wait=off"'
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

shell('mkdir -p /etc/modprobe.d; echo blacklist hid_morse_kbd > /etc/modprobe.d/startup-test.conf; /etc/init.d/morsed stop; rmmod hid_morse_kbd')
shell('morse-input-stress --held-start > /tmp/held.log 2>&1 &')
shell('sleep 1; cat /tmp/held.log; modprobe hid_morse_kbd target_name="Stress Tablet"; /etc/init.d/morsed start')
out=shell('sleep 1; morse_hud_client')
assert 'event_type=9' in out and 'ascii=0' in out,out
out=shell('sleep 5; cat /tmp/held.log; dmesg | tail -18; morse_hud_client')
assert 'PASS: held-start input sequence completed' in out and 'ascii=65' in out,out
out=shell("printf 'interval_ms=-18446744073709550616\\n' > /tmp/invalid.conf; morsed /tmp/invalid.conf --check-config; echo NEGATIVE_RC=$?")
assert 'NEGATIVE_RC=2' in out,out
shell('rm /etc/modprobe.d/startup-test.conf; sync')
p.sendline('poweroff'); p.expect(pexpect.EOF)
log.close()
print('Held-at-bind and negative interval regression checks passed')
