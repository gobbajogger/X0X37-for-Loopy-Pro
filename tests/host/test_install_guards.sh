#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/fm1_install.py start guards for a real device (no device is opened: MidoBackend is replaced by a stub).
#   check 3 (rule 7): stdin is not a TTY and STY and TMUX are empty -> "REFUSED", exit 10. Cases: tests/host/fixtures/install_guards/foreground.txt.
#   check 14 (rule 3): the hardware lock file (X0X_HW_LOCK) is held by another process -> "REFUSED", exit 10, for an
#     install and for --info. A free lock -> the run continues (the stub backend then says "not found", exit 3).
#   The gate guard stays the first step: no --commit -> exit 9.
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd)"
T=$(mktemp -d "${TMPDIR:-/tmp}/instguard.XXXXXX"); trap 'rm -rf "$T"' EXIT
printf 'pkg\n' > "$T/p.fwsc"
X0X_HW_LOCK="$T/hw.lock" TOP="$TOP" T="$T" python3 - <<'PY'
import io, os, subprocess, sys, time
sys.path.insert(0, os.path.join(os.environ["TOP"], "tools"))
import fm1_install as m
T = os.environ["T"]; fails = n = 0
def ok(c, what):
    global fails, n
    n += 1
    print(("ok   " if c else "FAIL ") + what)
    if not c: fails += 1
class Stdin:
    def __init__(s, t): s.t = t
    def isatty(s): return s.t
fx = os.path.join(os.environ["TOP"], "tests/host/fixtures/install_guards/foreground.txt")
fg = getattr(m, "foreground_guard", None)
for line in open(fx):
    if line.startswith("#") or not line.strip(): continue
    name, tty, sty, tmux, want = line.rstrip("\n").split("|")
    env = {k: v for k, v in (("STY", sty), ("TMUX", tmux)) if v}
    rc = fg(Stdin(tty == "1"), env) if fg else None
    ok(rc == (0 if want == "OK" else 10), f"foreground guard, {name}: rc {rc}, want {want}")
# main(): gate guard first, then the foreground guard, then the lock
class NoDevice(Exception): pass
def stub(): raise m.InstallError("notfound", "stub backend: no device")
m.MidoBackend = stub
real_gg = m.gate_guard
ok(m.main([os.path.join(T, "p.fwsc")]) == 9, "main: no --commit -> gate guard exit 9 (first step)")
m.gate_guard = lambda a: 0
sys.stdin = Stdin(False); os.environ.pop("STY", None); os.environ.pop("TMUX", None)
ok(m.main([os.path.join(T, "p.fwsc"), "--commit", "abc1234"]) == 10, "main: no TTY, no screen -> exit 10")
os.environ["STY"] = "1.x0x"
ok(m.main([os.path.join(T, "p.fwsc"), "--commit", "abc1234"]) == 3, "main: screen session, free lock -> continues (stub: exit 3)")
def release():   # this process keeps the lock after a run (as a real run does): drop it for the next case
    if m._HW_LOCK: m._HW_LOCK.close(); m._HW_LOCK = None
release()
# a second process holds the lock (it waits at most 20 s for it)
holder = subprocess.Popen([sys.executable, "-c", "import fcntl, os, sys, time; f = open(os.environ['X0X_HW_LOCK'], 'a'); "
                           "t = time.time()\nwhile True:\n try: fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB); break\n except OSError:\n  if time.time() - t > 20: sys.exit(1)\n  time.sleep(0.1)\n"
                           "print('held', flush=True); time.sleep(30)"], stdout=subprocess.PIPE, text=True)
holder.stdout.readline()
ok(m.main([os.path.join(T, "p.fwsc"), "--commit", "abc1234"]) == 10, "main: lock held by another process -> install exit 10")
ok(m.main(["--info"]) == 10, "main: lock held by another process -> --info exit 10")
holder.kill(); holder.wait(); release()
ok(m.main(["--info"]) == 3, "main: lock free again -> --info continues (stub: exit 3)")
print(f"install guards: {n - fails} of {n} ok")
sys.exit(1 if fails else 0)
PY
