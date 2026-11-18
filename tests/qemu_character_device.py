import json, socket, time, pathlib, pexpect, os

root = pathlib.Path(os.environ.get('MORSE_PROJECT_ROOT', str(pathlib.Path(__file__).resolve().parents[1])))
sock = '/tmp/morse-acceptance-qmp-' + str(os.getpid()) + '.sock'
pathlib.Path(sock).unlink(missing_ok=True)
results = root / 'build-qemu/test-results'
results.mkdir(parents=True, exist_ok=True)
log = open(results / 'morse-acceptance.log', 'w')
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
def self_test():
    output = shell('morse-hid-test --self-test; echo SELF_TEST_RC=$?')
    assert 'SELF_TEST_RC=0' in output, output
    assert 'RESULT: ALL EXECUTED TESTS PASSED' in output, output
    assert '[FAIL]' not in output, output
    return output
shell('/etc/init.d/morsed stop 2>/dev/null || true')
self_test()
shell('morse-hid-test --wait > /tmp/wakeup.log 2>&1 &')
click(1)
shell('cat /tmp/wakeup.log')
# Discard the partial character using a chord, then drain before the matrix.
buttons(3); buttons(0)
output = self_test()
assert 'Invalid read pointer returns EFAULT without consuming event' in output, output
output = shell('for p in /sys/class/input/event*/device/name; do [ "$(cat "$p")" = "MorseMouse Virtual Keyboard" ] && echo KEYBOARD_NODE=/dev/input/$(basename "$(dirname "$(dirname "$p")")"); done')
import re
match = re.search(r'KEYBOARD_NODE=(/dev/input/event[0-9]+)', output)
assert match, output
shell('evtest ' + match.group(1) + ' > /tmp/keyboard.log 2>&1 &')
sequence('.-'); click(4); sequence('..........')
click(1); buttons(3); buttons(0); sequence('.-')
self_test()
shell('kill %1; tail -20 /tmp/keyboard.log')
# Overflow the diagnostic ring at a paced rate without starving the worker.
for i in range(140): click(1)
self_test()
output = shell('rmmod hid_morse_kbd; test ! -e /dev/morse_hid && echo NODE_REMOVED; modprobe hid_morse_kbd')
assert 'NODE_REMOVED' in output, output
self_test()
shell("dmesg | grep -iE 'BUG:|Oops:|WARNING:|hung task' || true")
shell('sync')
p.sendline('poweroff'); p.expect(pexpect.EOF)
log.close()
print('QEMU run complete; transcript:', log.name)
