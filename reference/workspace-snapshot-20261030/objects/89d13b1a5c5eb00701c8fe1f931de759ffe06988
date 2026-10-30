#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""gate_sigwrap.py: record who stops the gate (check 5 of rules v4, rule 2: no pkill -f).

  gate_sigwrap.py SIGFILE COMMAND [ARG ...]

tools/gate.sh (x0x37_gate.sh) runs itself through this wrapper with exec, so the wrapper keeps the PID of the gate.
The wrapper starts COMMAND (the gate) as its child and waits. On SIGTERM, SIGINT or SIGHUP it reads the sender of the
signal (si_pid of the signal) and writes one line to SIGFILE:
    sig=SIGTERM pid=<sender PID> cwd=<sender working folder> cmd=<sender command line>
(cwd and cmd are "?" if the sender process has stopped). Then it sends the same signal to the child. The gate trap
(on_signal in gate.sh) reads SIGFILE and writes the sender to the gate.log KILLED line and to the KILLED output line.
Exit code: the exit code of the child (128 + N if a signal N stopped it). The wrapper removes SIGFILE at the end.
"""
import os
import signal
import subprocess
import sys

STOP = {signal.SIGTERM, signal.SIGINT, signal.SIGHUP}


def proc(pid, what):
    try:
        if what == "cwd":
            return os.readlink(f"/proc/{pid}/cwd")
        with open(f"/proc/{pid}/cmdline", "rb") as f:
            return f.read().replace(b"\0", b" ").decode("utf-8", "replace").strip()[:200] or "?"
    except OSError:
        return "?"


def main(argv):
    if len(argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    sigfile, cmd = argv[1], argv[2:]
    waitset = STOP | {signal.SIGCHLD}
    signal.pthread_sigmask(signal.SIG_BLOCK, waitset)
    child = subprocess.Popen(cmd, preexec_fn=lambda: signal.pthread_sigmask(signal.SIG_UNBLOCK, waitset))
    try:
        while True:
            info = signal.sigwaitinfo(waitset)
            if info.si_signo == signal.SIGCHLD:
                if child.poll() is not None:
                    break
                continue
            pid = info.si_pid
            line = (f"sig={signal.Signals(info.si_signo).name} pid={pid} cwd={proc(pid, 'cwd')} "
                    f"cmd={proc(pid, 'cmd')}\n")
            tmp = f"{sigfile}.tmp"
            with open(tmp, "w") as f:
                f.write(line)
            os.replace(tmp, sigfile)
            try:
                os.kill(child.pid, info.si_signo)
            except ProcessLookupError:
                pass
        rc = child.returncode
    finally:
        try:
            os.unlink(sigfile)
        except OSError:
            pass
    return 128 - rc if rc < 0 else rc


if __name__ == "__main__":
    sys.exit(main(sys.argv))
