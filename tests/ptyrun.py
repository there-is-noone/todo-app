# usage: python3 -I ptyrun.py <mode all|out|err> <outfile> <errfile> <cmd...>
# Runs cmd with stdout and/or stderr attached to a pseudo terminal; stdin is /dev/null.
# The pty side is written to outfile (modes all/out) or errfile (mode err).
import os
import subprocess
import sys

mode, outf, errf = sys.argv[1:4]
cmd = sys.argv[4:]
m, s = os.openpty()
o = open(outf, "wb")
e = open(errf, "wb")
stdout = s if mode in ("all", "out") else o
stderr = s if mode in ("all", "err") else e
p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr)
os.close(s)
buf = b""
while True:
    try:
        d = os.read(m, 4096)
    except OSError:
        break
    if not d:
        break
    buf += d
try:
    rc = p.wait(timeout=10)
except subprocess.TimeoutExpired:
    p.kill()
    rc = 124
(o if mode in ("all", "out") else e).write(buf)
o.close()
e.close()
sys.exit(rc)
