import json, socket, time, pathlib, pexpect, os

root = pathlib.Path(os.environ.get('MORSE_PROJECT_ROOT', str(pathlib.Path(__file__).resolve().parents[1])))
sock = '/tmp/morse-acceptance-qmp.sock'
pathlib.Path(sock).unlink(missing_ok=True)
log = open(str(root / 'morse-acceptance.log'), 'w')
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
shell('morse-hid-test --self-test')
shell('morse-hid-test --wait > /tmp/wakeup.log 2>&1 &')
click(1)
shell('cat /tmp/wakeup.log')
# Discard the partial character using a chord, then drain before the matrix.
buttons(3); buttons(0)
shell('morse-hid-test --self-test')
shell('evtest /dev/input/event2 > /tmp/keyboard.log 2>&1 &')
sequence('.-'); click(4); sequence('..........')
click(1); buttons(3); buttons(0); sequence('.-')
shell('morse-hid-test --self-test')
shell('kill %1; tail -20 /tmp/keyboard.log')
# Overflow the diagnostic ring at a paced rate without starving the worker.
for i in range(140): click(1)
shell('morse-hid-test --self-test')
shell('rmmod hid_morse_kbd; test ! -e /dev/morse_hid && echo NODE_REMOVED; modprobe hid_morse_kbd; morse-hid-test --self-test')
shell("dmesg | grep -iE 'BUG:|Oops:|WARNING:|hung task' || true")
shell('sync')
p.sendline('poweroff'); p.expect(pexpect.EOF)
log.close()
print('QEMU run complete; transcript: /tmp/morse-acceptance.log')
