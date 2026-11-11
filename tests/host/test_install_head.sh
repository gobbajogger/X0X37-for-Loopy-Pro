#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/fm1_install.py bootloader guard (check 4 of rules v4, Orchestrator decision 02:55): a real-device install
# refuses (exit 11) when the bytes [0, 0x4000) of the package flash.bin do not have the sha256 of a reference read-back.
# The test builds a small package (header, file list, flash.bin with a head and the loader marker) and a reference
# for each case of tests/host/fixtures/install_guards/head_cases.txt. It checks head_guard(), --head-check, and main()
# with a stub device: the guard runs after the gate guard and the foreground guard, and a refusal opens no device.
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd)"
T=$(mktemp -d "${TMPDIR:-/tmp}/insthead.XXXXXX"); trap 'rm -rf "$T"' EXIT
X0X_HW_LOCK="$T/hw.lock" HOME="$T/home" TOP="$TOP" T="$T" python3 - <<'PY'
import io, os, sys
sys.path.insert(0, os.path.join(os.environ["TOP"], "tools"))
import fm1_install as m
T = os.environ["T"]; fails = n = 0
os.environ.pop("X0X_HEAD_REF", None)
def ok(c, what):
    global fails, n
    n += 1; print(("ok   " if c else "FAIL ") + what)
    if not c: fails += 1
HEAD_END = getattr(m, "HEAD_END", 0x4000)
# the flash.bin: a head [0, 0x4000) with a pattern, then an app area with the loader marker
flash = bytes((i * 7 + 3) & 0xFF for i in range(HEAD_END)) + b"\x11" * 0x100 + m.LOADER_MARK + b"\x22" * 0x100
off = 0x40 + 0x50
e = bytearray(0x50); e[0:2] = (0).to_bytes(2, "little"); e[4:6] = m._crc16(flash).to_bytes(2, "little")
e[8:12] = off.to_bytes(4, "little"); e[12:16] = len(flash).to_bytes(4, "little"); e[0x40:0x49] = b"flash.bin"
lst = m._dec(bytes(e))
h = bytearray(0x40); h[2:4] = m._crc16(lst).to_bytes(2, "little"); h[8:10] = (1).to_bytes(2, "little")
h[0:2] = m._crc16(h[2:0x40]).to_bytes(2, "little")
image = m._dec(bytes(h)) + lst + flash
prod = "X0X_900"
raw = bytearray()
for i in range(m.BLOCKS):
    ch = prod[i] if i < len(prod) else None
    raw += image[i * m.KEEP:(i + 1) * m.KEEP] + bytes([(ord(ch) + i + 1) & 0xFF if ch else 0x7D])
raw += image[m.BLOCKS * m.KEEP:]
pkg = os.path.join(T, "p.fwsc"); open(pkg, "wb").write(raw)
ok(m.package_damage(m.deinterleave(bytes(raw))[0]) is None, "the test package is a valid package")
class A: pass
def args(ref):
    a = A(); a.package = pkg; a.force = False; a.head_ref = ref; return a
fx = os.path.join(os.environ["TOP"], "tests/host/fixtures/install_guards/head_cases.txt")
hg = getattr(m, "head_guard", None)
for line in open(fx):
    if line.startswith("#") or not line.strip(): continue
    name, kind, want = line.rstrip("\n").split("|")
    ref = bytearray(flash)
    if kind.startswith("flip:"): ref[int(kind[5:], 16)] ^= 0x01
    elif kind.startswith("app:"): ref[int(kind[4:], 16)] ^= 0x01
    elif kind == "short": ref = ref[:HEAD_END - 1]
    rp = os.path.join(T, "ref.bin"); open(rp, "wb").write(ref)
    err = io.StringIO()
    rc = hg(args(rp), out=err) if hg else None
    w = 0 if want == "OK" else 11
    ok(rc == w and (want == "OK" or "REFUSED" in err.getvalue()), f"head guard, {name}: rc {rc}, want {w} ({want}): {err.getvalue().strip()[:110]}")
# no reference: a note, no refusal (no install in flight is blocked)
err = io.StringIO(); rc = hg(args(None), out=err) if hg else None
ok(rc == 0 and "no reference" in err.getvalue(), f"no reference: continue with a note (rc {rc})")
# X0X_HEAD_REF is used when --head-ref is not given
bad = bytearray(flash); bad[0x10] ^= 0xFF; open(os.path.join(T, "bad.bin"), "wb").write(bad)
os.environ["X0X_HEAD_REF"] = os.path.join(T, "bad.bin")
err = io.StringIO(); rc = hg(args(None), out=err) if hg else None
ok(rc == 11, f"X0X_HEAD_REF with a changed byte: rc {rc}, want 11")
# --head-check: compare only
out = io.StringIO(); rc = m.main([pkg, "--head-check"], out=out)
ok(rc == 11 and "HEAD MISMATCH" in out.getvalue(), f"--head-check, changed byte: rc {rc}, {out.getvalue().strip()[-30:]}")
open(os.path.join(T, "good.bin"), "wb").write(flash)
out = io.StringIO(); rc = m.main([pkg, "--head-check", "--head-ref", os.path.join(T, "good.bin")], out=out)
ok(rc == 0 and "HEAD OK" in out.getvalue(), f"--head-check, same head: rc {rc}")
# main(): gate guard, foreground guard, then the bootloader guard; a refusal opens no device
opened = []
def stub():
    opened.append(1); raise m.InstallError("notfound", "stub backend: no device")
m.MidoBackend = stub
m.gate_guard = lambda a: 0
m.foreground_guard = lambda *x, **k: 0
sys.stderr = io.StringIO()
rc = m.main([pkg, "--commit", "abc1234"])
ok(rc == 11 and not opened, f"main, changed head: exit {rc}, device opened: {bool(opened)}")
if m._HW_LOCK: m._HW_LOCK.close(); m._HW_LOCK = None   # this process keeps the lock after a run: drop it
rc = m.main([pkg, "--commit", "abc1234", "--head-ref", os.path.join(T, "good.bin")])
ok(rc == 3 and opened, f"main, same head: the install goes on to the device (stub: exit {rc})")
sys.stderr = sys.__stderr__
print(f"install head: {n - fails} of {n} ok")
sys.exit(1 if fails else 0)
PY
