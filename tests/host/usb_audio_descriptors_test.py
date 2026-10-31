#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Parse the real usb.c descriptors (compiled on the host) and check them against what an
iPhone / iPad (CoreAudio, class-compliant USB Audio 1.0) and macOS need:
  - the descriptor chain is well formed, wTotalLength and bNumInterfaces are exact
  - bus powered, bMaxPower <= 100 mA (an iPhone refuses more)
  - one Audio Control interface whose header lists every streaming interface (audio + MIDI)
  - terminals: unique IDs, sources that exist, every stream linked to a USB streaming terminal
  - an audio INPUT stream (device -> host) AND an audio OUTPUT stream (host -> device): iOS only
    offers a USB device as its audio output when it has a playback stream
  - each audio streaming interface: alt 0 with no endpoint, alt 1 with one isochronous endpoint in
    the right direction, Type I PCM, 2 ch, 16 bit, 44.1 or 48 kHz only, a packet big enough for the
    rate, bInterval 1, IN asynchronous, OUT adaptive (or synchronous) -- no feedback endpoint needed
  - the interface association (CDC builds) covers the audio function
Run for the release configuration (no CDC) and the CDC console configuration."""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CC = os.environ.get("CC", "cc")
fails = []


def check(cond, msg):
    if not cond:
        fails.append(msg)
    return cond


def build_and_dump(defs, tmp):
    exe = Path(tmp) / ("dump" + "".join(d.replace("=", "") for d in defs))
    cmd = [CC, "-O2", "-w", "-I" + str(ROOT / "firmware/src"), "-I" + str(ROOT / "firmware/hal"),
           *["-D" + d for d in defs], "-o", str(exe), str(ROOT / "tests/host/usb_desc_dump.c")]
    subprocess.run(cmd, check=True)
    out = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout
    res = {}
    for line in out.splitlines():
        k, v = line.split(" ", 1)
        res[k] = bytes.fromhex(v) if v != "-" else None
    return res


def u16(b, i):
    return b[i] | b[i + 1] << 8


def validate(name, d, expect_cdc):
    dev, cfg = d["device"], d["config"]
    p = f"[{name}] "
    check(len(dev) == 18 and dev[0] == 18 and dev[1] == 1, p + "device descriptor: 18 bytes, type 1")
    check(dev[7] in (8, 16, 32, 64), p + "bMaxPacketSize0")
    check(u16(dev, 2) in (0x0110, 0x0200), p + "bcdUSB 1.10 or 2.00")
    if dev[4] == 0xEF:
        check(dev[5] == 2 and dev[6] == 1, p + "misc class needs subclass 2 / protocol 1 (IAD)")
    else:
        check(dev[4] == 0 and dev[5] == 0 and dev[6] == 0, p + "class defined at interface level")
    check(dev[15] != 0 and d["string2"] is not None, p + "a product string")
    check(dev[17] == 1, p + "one configuration")

    # walk the chain
    descs, i = [], 0
    while i < len(cfg):
        n = cfg[i]
        if not check(n >= 2 and i + n <= len(cfg), p + f"descriptor at {i}: bad length {n}"):
            return
        descs.append(cfg[i:i + n])
        i += n
    c = descs[0]
    check(c[1] == 2 and c[0] == 9, p + "configuration descriptor first")
    check(u16(c, 2) == len(cfg), p + f"wTotalLength {u16(c, 2)} != {len(cfg)}")
    check(c[7] & 0x80, p + "bmAttributes bit 7")
    check(not (c[7] & 0x40), p + "bus powered (an iPhone accessory)")
    check(c[8] <= 50, p + f"bMaxPower {c[8] * 2} mA > 100 mA: iOS refuses it")

    # group by interface (number, alt) with its class-specific and endpoint descriptors
    ifaces, cur, iads = [], None, []
    for x in descs[1:]:
        t = x[1]
        if t == 4:
            check(len(x) == 9, p + "interface descriptor 9 bytes")
            cur = {"num": x[2], "alt": x[3], "neps": x[4], "cls": x[5], "sub": x[6], "cs": [], "eps": []}
            ifaces.append(cur)
        elif t == 0x0B:
            iads.append(x)
        elif t == 5:
            check(cur is not None, p + "endpoint outside an interface")
            cur["eps"].append({"d": x, "cs": []})
        elif t == 0x24:
            cur["cs"].append(x)
        elif t == 0x25:
            check(cur["eps"], p + "CS endpoint before an endpoint")
            cur["eps"][-1]["cs"].append(x)
        else:
            check(False, p + f"unexpected descriptor type {t:#x}")
    nums = sorted({f["num"] for f in ifaces})
    check(nums == list(range(len(nums))), p + f"interface numbers contiguous from 0: {nums}")
    check(c[4] == len(nums), p + f"bNumInterfaces {c[4]} != {len(nums)}")
    for f in ifaces:
        check(len(f["eps"]) == f["neps"], p + f"IF{f['num']} alt {f['alt']}: bNumEndpoints {f['neps']} != {len(f['eps'])}")
    addrs = [e["d"][2] for f in ifaces for e in f["eps"] if f["alt"] != 0 or True]
    seen = {}
    for f in ifaces:
        for e in f["eps"]:
            a = e["d"][2]
            check(seen.get(a, f["num"]) == f["num"], p + f"endpoint {a:#04x} used by two interfaces")
            seen[a] = f["num"]

    ac = [f for f in ifaces if f["cls"] == 1 and f["sub"] == 1]
    if not check(len(ac) == 1, p + "exactly one Audio Control interface"):
        return
    ac = ac[0]
    check(ac["alt"] == 0 and ac["neps"] == 0, p + "AC: alt 0, no interrupt endpoint")
    hdr = ac["cs"][0]
    check(hdr[2] == 1 and u16(hdr, 3) == 0x0100, p + "AC header first, bcdADC 1.00")
    check(u16(hdr, 5) == sum(len(x) for x in ac["cs"]), p + f"AC wTotalLength {u16(hdr, 5)} != {sum(len(x) for x in ac['cs'])}")
    coll = list(hdr[8:8 + hdr[7]])
    check(hdr[0] == 8 + hdr[7], p + "AC header length matches bInCollection")
    streaming = sorted({f["num"] for f in ifaces if f["cls"] == 1 and f["sub"] in (2, 3)})
    check(sorted(coll) == streaming, p + f"AC collection {coll} != streaming interfaces {streaming}")

    units = {}
    for x in ac["cs"][1:]:
        sub = x[2]
        check(sub in (2, 3, 6), p + f"AC descriptor subtype {sub}")
        check(x[3] != 0 and x[3] not in units, p + f"terminal/unit id {x[3]} unique and non-zero")
        units[x[3]] = x
    for uid, x in units.items():
        if x[2] == 2:
            check(x[0] == 12, p + f"IT{uid} 12 bytes")
            check(x[7] >= 1 and x[7] <= 2, p + f"IT{uid} channels")
        elif x[2] == 3:
            check(x[0] == 9 and x[7] in units, p + f"OT{uid}: source {x[7]} exists")
            check(units.get(x[7], b"\0\0\0")[2] != 3, p + f"OT{uid}: source is not an output terminal")
        elif x[2] == 6:
            check(x[4] in units, p + f"FU{uid}: source exists")

    dirs = set()
    for f in [f for f in ifaces if f["cls"] == 1 and f["sub"] == 2]:
        alts = [g for g in ifaces if g["num"] == f["num"]]
        if f["alt"] != 0:
            continue
        P = p + f"AS IF{f['num']}: "
        check(f["neps"] == 0, P + "alt 0 has no endpoint (zero bandwidth)")
        for g in alts[1:]:
            check(g["neps"] == 1 and g["alt"] >= 1, P + f"alt {g['alt']}: one endpoint")
            gen = [x for x in g["cs"] if x[2] == 1]
            fmt = [x for x in g["cs"] if x[2] == 2]
            if not check(len(gen) == 1 and len(fmt) == 1 and g["eps"], P + "AS general + format type"):
                continue
            gen, fmt, ep = gen[0], fmt[0], g["eps"][0]
            check(gen[0] == 7 and u16(gen, 5) == 0x0001, P + "PCM")
            link = units.get(gen[3])
            if not check(link is not None, P + f"terminal link {gen[3]} exists"):
                continue
            check(u16(link, 4) == 0x0101, P + "linked to a USB streaming terminal")
            out = link[2] == 2                      # an input terminal fed by USB = host -> device
            addr, attr = ep["d"][2], ep["d"][3]
            check(ep["d"][0] == 9, P + "audio endpoint descriptor 9 bytes")
            check(bool(addr & 0x80) != out, P + f"endpoint {addr:#04x} direction")
            check(attr & 3 == 1, P + "isochronous")
            sync = (attr >> 2) & 3
            if out:
                check(sync in (2, 3), P + "OUT adaptive or synchronous (no feedback endpoint)")
            else:
                check(sync in (1, 3), P + "IN asynchronous or synchronous")
            check(ep["d"][6] == 1, P + "bInterval 1 (1 ms)")
            check(fmt[3] == 1 and fmt[0] == 8 + 3 * fmt[7], P + "Type I, length matches the rates")
            ch, sub, bits = fmt[4], fmt[5], fmt[6]
            check(ch == 2 and sub == 2 and bits == 16, P + f"2 ch 16 bit ({ch} ch, {sub} B, {bits} bit)")
            rates = [fmt[8 + 3 * k] | fmt[9 + 3 * k] << 8 | fmt[10 + 3 * k] << 16 for k in range(fmt[7])]
            check(fmt[7] >= 1 and all(r in (44100, 48000) for r in rates), P + f"rates {rates}: 44.1 / 48 kHz")
            need = -(-max(rates) // 1000) * ch * sub
            mps = u16(ep["d"], 4)
            check(need <= mps <= 1023, P + f"wMaxPacketSize {mps} >= {need}")
            check(ep["cs"] and ep["cs"][0][0] == 7 and ep["cs"][0][2] == 1, P + "CS general endpoint")
            dirs.add("out" if out else "in")
    check(dirs == {"in", "out"}, p + f"audio streams {sorted(dirs)}: need input AND output for iOS")

    midi = [f for f in ifaces if f["cls"] == 1 and f["sub"] == 3]
    check(len(midi) == 1 and midi[0]["neps"] == 2, p + "one MIDI streaming interface, two bulk endpoints")
    if expect_cdc:
        check(dev[4] == 0xEF and iads, p + "CDC: misc device class with IADs")
    if iads:
        a = iads[0]
        check(a[2] == 0 and a[3] == len(streaming) + 1 and a[4] == 1, p + f"audio IAD covers IF 0..{len(streaming)}")
    print(f"{p}{len(cfg)} bytes, {c[4]} interfaces, audio {sorted(dirs)}, bMaxPower {c[8] * 2} mA, "
          f"bcdDevice {u16(dev, 12):04x}")


with tempfile.TemporaryDirectory() as tmp:
    validate("release", build_and_dump(["FELUCCA_UAC=1"], tmp), False)
    validate("cdc", build_and_dump(["FELUCCA_UAC=1", "FELUCCA_CDC=1"], tmp), True)

if fails:
    for f in fails:
        print("FAIL", f)
    sys.exit(1)
print("usb-audio-descriptors: ok")
