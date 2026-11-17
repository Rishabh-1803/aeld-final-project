import json, socket, time, pathlib, pexpect, os, re, subprocess

root = pathlib.Path(os.environ.get('MORSE_PROJECT_ROOT', str(pathlib.Path(__file__).resolve().parents[1])))
sock = '/tmp/morse-acceptance-qmp.sock'
pathlib.Path(sock).unlink(missing_ok=True)
log = open(str(root / 'morse-ssh-acceptance.log'), 'w')
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

key_output=shell('dropbearkey -y -f /etc/dropbear/dropbear_rsa_host_key; cat /etc/default/dropbear')
hostkey=re.search(r'(ssh-rsa [A-Za-z0-9+/=]+)',key_output).group(1)
known=root/'build-qemu/ssh-known-hosts'
known.write_text('[127.0.0.1]:2222 '+hostkey+'\n')
sequence('.-'); shell('sleep 2')
base=['ssh','-T','-p','2222','-i',str(root/'build-rpi/test-access-key'),'-o','StrictHostKeyChecking=yes','-o','UserKnownHostsFile='+str(known),'-o','BatchMode=yes','-o','ConnectTimeout=10']
result=subprocess.run(base+['root@127.0.0.1','uname -m; morse_hud_client'],text=True,capture_output=True,timeout=30)
log.write('\nSSH_KEY_LOGIN_RC='+str(result.returncode)+'\n'+result.stdout+result.stderr);log.flush()
assert result.returncode==0 and 'aarch64' in result.stdout and 'ascii=65' in result.stdout,result
negative=subprocess.run(base+['-v','-o','PubkeyAuthentication=no','-o','PreferredAuthentications=password','root@127.0.0.1','true'],text=True,capture_output=True,timeout=30)
methods=re.findall(r'Authentications that can continue: ([^\r\n]+)',negative.stderr)
log.write('\nSSH_PASSWORD_LOGIN_RC='+str(negative.returncode)+'\nSSH_SERVER_METHODS='+repr(methods)+'\n');log.flush()
assert negative.returncode==255 and methods and all(s=='publickey' for s in methods),negative.stderr
shell('sync')
p.sendline('poweroff'); p.expect(pexpect.EOF)
log.close();print('Verified-host key login passed; password authentication rejected')
