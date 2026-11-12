#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
"""Bluetooth start test of an SMK-37 over USB, with no hands on the unit.

Each round:
  1. ED_BT_TEST op 1: clear the start block (as BLUETOOTH OFF). The BLUETOOTH setting must be ON.
  2. ED_BT_TEST op 2: the unit restarts (a chip reset through P33, as fm1_reboot). It never goes
     to the update loader.
  3. Wait until the USB MIDI port goes and comes back. After a crash the unit shows the crash
     screen for 4 s and restarts once more: wait until DIAG answers and the state is stable.
  4. Read DIAG (editor command 33) and print STATUS, the start record and the crash record.
     --diag-only also prints the inbound recorder and the pad LED output (editor command 38 INLOG).
     --padcal-export FILE reads the pad calibration (CLP-79, INLOG args 2-5) for apc_pad_colors.py --pad-override.
     --pad-inputs reads one sensor snapshot (INLOG arg 6), without any Bluetooth reset/test commands.
     For READY, read DIAG again after --hold seconds: the controller must stay up (polls grow).

A software restart is not a power cycle: .noinit RAM (the start guard, the crash record) stays,
and the radio's analog and power (P33) state may stay too. Confirm a result with a real power
cycle at the end.

Usage: tools/ble_autotest.py [-n ROUNDS] [--hold S] [--log FILE] [--diag-only]
Needs mido with python-rtmidi. Exit 0 = every round READY and stayed up.
"""
import argparse
import json
import sys
import time

import mido

sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
import inlog_fmt  # noqa: E402  (a76: editor command 38 INLOG)

HDR = [0x7D, 0x46, 0x4C]
ED_DIAG, ED_BT_TEST = 33, 34
PORT_KEYS = ("X0X", "SMK", "FM-1", "Felucca")

CRASH = ["magic", "count", "vec", "pc", "rets", "emu", "dbg", "sp", "psr", "icfg", "up_ms",
         "etm0", "etm1", "etm2", "etm3", "early", "core"]
BOOT = ["bg_failed", "bg_pending"]
GUARD = ["g_magic", "trying", "blocked", "why", "fails", "start_ms", "init_ms", "mb_state", "mb_err",
         "dbg_stage", "dbg_ms", "dbg_delays", "dbg_ra", "dbg_init_ms", "dbg_stall", "reset_prev",
         "sink_n", "sink_at", "sink_val", "wait_end", "xosc_pre", "xosc_post", "wait_ms",
         "seen_stage", "seen_ms", "seen_delays", "seen_ra", "code_bad", "code_sum"]
LIVE = ["img_boot_up_dead", "want_host_blocked_c1on", "c1_gave_up", "plat_state", "mb_state_now",
        "mb_err_now", "polls", "poll_max", "h_state", "c1_trace", "reset_now",
        "h2c", "c2h", "c2h_drops", "h_step", "ctl_took", "ctl_gave", "rx0", "rx1", "alive_ms", "alive_polls", "rails0", "rails1"]
STATE = ["NO IMAGE", "OFF", "RESTART TO TURN ON", "STARTING", "READY", "CONNECTED", "FAILED", "BAD IMAGE"]
WHY = ["-", "TIMEOUT", "WRONG JOB", "NOT READY", "RESET", "NO TICK"]
MB = ["OFF", "INIT", "READY", "FAILED"]
ERR = ["NONE", "BOOT", "HEAP", "TASK", "ASSERT", "RESET"]


def ports():
    i = [n for n in mido.get_input_names() if any(k in n for k in PORT_KEYS)]
    o = [n for n in mido.get_output_names() if any(k in n for k in PORT_KEYS)]
    return (i[0], o[0]) if i and o else None


def ask(cmd, args=(), timeout=3.0, tries=2):
    """one request; again once if no reply (after idle the first reply can be lost: the host
    does not read the USB IN endpoint at once, and the unit gives a SysEx reply 200 ms)"""
    for _ in range(tries):
        r = _ask(cmd, args, timeout)
        if r is not None:
            return r
    return None


def _ask(cmd, args=(), timeout=3.0):
    p = ports()
    if not p:
        return None
    with mido.open_input(p[0]) as i, mido.open_output(p[1]) as o:
        time.sleep(0.3)                            # macOS drops a message sent at once after the port opens
        for _ in i.iter_pending():
            pass
        o.send(mido.Message("sysex", data=HDR + [cmd] + list(args)))
        t = time.time()
        while time.time() - t < timeout:
            for m in i.iter_pending():
                if m.type == "sysex" and list(m.data[:4]) == HDR + [cmd]:
                    return list(m.data[4:])
            time.sleep(0.01)
    return None


def diag():
    r = ask(ED_DIAG)
    if not r:
        return None
    n, w = r[0], []
    for k in range(n):
        b = r[1 + 5 * k:6 + 5 * k]
        w.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
    trace = {}
    if len(w) >= 6 and w[-6] == 0x534D4301:
        trace = dict(zip(["smc_trace", "smc_handles", "smc_acl0", "smc_acl1", "smc_reject", "smc_att"], w[-6:]))
        w = w[:-6]
    head = CRASH + BOOT
    d = {k: v for k, v in zip(head, w)}
    rest = w[len(head):]
    if len(rest) >= len(LIVE):                     # the live words are always the last len(LIVE)
        d.update(zip(LIVE, rest[-len(LIVE):]))
        rest = rest[:-len(LIVE)]
    d.update(trace)
    d.update(zip(GUARD, rest))                     # an older firmware sends fewer guard words
    for k in GUARD + LIVE:
        d.setdefault(k, 0)
    return d


def wait_port(present, timeout):
    t = time.time()
    while time.time() - t < timeout:
        if bool(ports()) == present:
            return True
        time.sleep(0.2)
    return False


def settle(timeout=30.0):
    """DIAG after the restart(s): wait until the state is not STARTING and the port stays."""
    t, last, any_d = time.time(), None, None
    while time.time() - t < timeout:
        if not ports():
            time.sleep(0.3)
            continue
        d = diag()
        any_d = d or any_d
        if d and STATE[d.get("plat_state", 0) % 8] != "STARTING":
            if last and last.get("reset_now") == d.get("reset_now") and time.time() - t > 6:
                return d
            last = d
        time.sleep(1.0)
    return last or any_d                           # still STARTING after the timeout: that state


T5W = ["magic", "ms", "ticks", "tag", "dbg", "c0n", "c1n", "prpn", "prpid", "hits", "h_ms", "h_tag", "h_slot",
       "h_old", "h_new", "h_sp", "h_dbg", "h_prpid", "h_c1n", "h_prpn", "icfg0_3", "icfg0_5", "hits_now"]
C1R = ["d_magic", "d_n", "d_t4_ms", "d_c0_ms", "d_dbg", "d_emu"] + [f"d_etm{i}_{j}" for i in range(4) for j in range(4)] + ["c_armed", "c_n", "c_scans"] + [f"c_{i}_{f}" for i in range(8) for f in ("at", "val", "ms")]
TAG = ["begin", "after input", "before Bluetooth", "after Bluetooth", "end"]
SLOT = ["r3", "r2", "r1", "r0", "psr", "rets", "reti", "word above"]


def bt_words(op):
    r = ask(ED_BT_TEST, [op])
    if not r or r[0] != op:
        return None
    w = []
    for k in range(r[1]):
        b = r[2 + 5 * k:7 + 5 * k]
        if len(b) < 5:
            break
        w.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
    return w


def t5w():
    """ED_BT_TEST op 3: the timer 5 frame watch of the last boot (fm1_guard.h); op 4: core 1's records"""
    w = bt_words(3)
    if w is None:
        return None
    t = dict(zip(T5W, w))
    w4 = bt_words(4)
    if w4:
        t.update(zip(C1R, w4))
    return t


def t5w_text(t):
    if not t:
        return "no reply (older firmware)"
    if t["magic"] != 0x54355731:
        return f"no record of the last boot; changes now {t['hits_now']}"
    s = (f"last boot: last tick at {t['ms']} ms (tick {t['ticks']}, checkpoint {TAG[t['tag'] % 5]}); "
         f"DBG_MSG 0x{t['dbg']:08X}, write limit errors core0 {t['c0n']} core1 {t['c1n']} DMA {t['prpn']} "
         f"(DMA id 0x{t['prpid']:X}); frame changes {t['hits']}")
    if t["hits"]:
        s += (f"; first at {t['h_ms']} ms, found at {TAG[t['h_tag'] % 5]}, word {SLOT[t['h_slot'] % 8]} "
              f"at 0x{t['h_sp'] + 4 * t['h_slot']:08X}: 0x{t['h_old']:08X} -> 0x{t['h_new']:08X}, "
              f"DBG_MSG 0x{t['h_dbg']:08X}, core1 errors {t['h_c1n']}, DMA errors {t['h_prpn']} id 0x{t['h_prpid']:X}")
    if "icfg0_3" in t:
        s += f"; core 0 ICFG 24-31 0x{t['icfg0_3']:08X} 40-47 0x{t['icfg0_5']:08X}"
    s += f"; changes this boot {t.get('hits_now', 0)}"
    if t.get("d_magic") == 0xC0DEAD01:
        s += (f"\n          core 1 saw core 0 stop: clock stuck at {t['d_c0_ms']} ms (TIMER4 {t['d_t4_ms']} ms), "
              f"DBG_MSG 0x{t['d_dbg']:08X} EMU_MSG 0x{t['d_emu']:08X}; core 0 ETM:")
        for i in range(min(t["d_n"], 4)):
            s += " [" + " ".join(f"{t[f'd_etm{i}_{j}']:08X}" for j in range(4)) + "]"
    elif "d_magic" in t:
        s += "; core 1 did not see core 0 stop"
    if t.get("c_armed") == 0xCA9A11E5:
        s += f"\n          RAM canary 0x01C72000-0x01C74000: {t['c_n']} changed words in {t['c_scans']} full scans"
        for i in range(min(t["c_n"], 8)):
            s += f"; 0x{t[f'c_{i}_at']:08X}=0x{t[f'c_{i}_val']:08X} at {t[f'c_{i}_ms']} ms"
    return s


def h(v):
    return f"0x{v:08X}"


def summary(d, before):
    s = STATE[d["plat_state"] % 8] if "plat_state" in d else "?"
    crash_new = before is None or any(d.get(k) != before.get(k) for k in ("count", "pc", "up_ms", "rets"))
    out = {
        "status": s,
        "guard": f"blocked {d['blocked']} why {WHY[d['why'] % 6]} fails {d['fails']}",
        "start": f"mailbox {MB[d['mb_state'] % 4]} err {ERR[d['mb_err'] % 6]} stage {d['dbg_stage']} "
                 f"delays {d['dbg_delays']} stage time {d['dbg_ms']} ms image init {d['dbg_init_ms']} ms "
                 f"core0 wait {h(d['wait_end'])} last look {d['wait_ms']} ms",
        "xosc": f"{h(d['xosc_pre'])} -> {h(d['xosc_post'])}",
        "core0": f"last look {d['wait_ms']} ms; image then: stage {d['seen_stage']} at {d['seen_ms']} ms, "
                 f"delays {d['seen_delays']}, last delay caller {h(d['seen_ra'])}; RAM code "
                 + (f"CHANGED at {d['code_bad'] & 0xFFFFF} ms (sum {h(d['code_sum'])})" if d['code_bad'] else "unchanged"),
        "hci": f"host state {d['h_state']} step {d['h_step'] & 0xFF} wait {d['h_step'] >> 8 & 0xFF} "
               f"credits {d['h_step'] >> 16 & 0xFF}; h2c head/tail {d['h2c'] & 0xFFFF}/{d['h2c'] >> 16} "
               f"c2h {d['c2h'] & 0xFFFF}/{d['c2h'] >> 16} drops {d['c2h_drops']}; controller took "
               f"{d['ctl_took']}, gave {d['ctl_gave']}; polls {d['polls']}",
        "link": (f"SMC receive (counters modulo 256): ACL {d['rx0'] & 255}, notifications {d['rx0'] >> 8 & 255}, "
                 f"MIDI {d['rx0'] >> 16 & 255}, rejected {d['rx0'] >> 24}; last receive rejection "
                 f"{d['h_step'] >> 24 & 127}; queue enqueued {d['rx1'] & 255}, dropped {d['rx1'] >> 8 & 255}, "
                 f"dispatched {d['rx1'] >> 16 & 255}, dispatch reason {d['rx1'] >> 24}"
                 if d['h_step'] & 0x80000000 else f"interval {(d['rx0'] & 0xFFFF) * 1.25:g} ms, latency {d['rx1'] >> 21 & 7}; last end reason 0x{d['rx0'] >> 16 & 0xFF:02X}; "
                 f"pairing refused {d['rx0'] >> 24}; asked MTU {d['rx1'] >> 8 & 0x3FF}; L2CAP requests {d['rx1'] >> 24}; MIDI in drops {d['rx1'] & 0xFF}; "
                 f"parameter request {['not sent', 'sent'][d['rx1'] >> 18 & 1]}, {['no answer', 'accepted', 'rejected', '?'][d['rx1'] >> 19 & 3]}"),
        "alive": f"core 0 last Bluetooth tick at {d['alive_ms']} ms after boot, core 1 polls then {d['alive_polls']}, now {d['polls']}",
        "rails": (lambda a, b: f"before/after the stock supply setup: SYSVDD sel {(a >> 16) & 15} -> {(b >> 16) & 15}, VDC14 sel {(a >> 8) & 7} -> {(b >> 8) & 7}, VDDIOM sel {a & 7} -> {b & 7}, CON0 0x{(a >> 24) & 255:02X} -> 0x{(b >> 24) & 255:02X} (DCDC13 {'on' if (b >> 24) & 8 else 'off'}, LDO13 {'on' if (b >> 24) & 4 else 'off'}); setup read-back {'ok' if ((b >> 24) & 0x0C) == 0x08 and (b & 0x37) == 0x34 and ((b >> 8) & 7) == 7 and ((b >> 16) & 15) == 15 else 'NOT ok'} (raw 0x{a:08X} -> 0x{b:08X})")(d.get('rails0', 0), d.get('rails1', 0)),
        "reset": f"this boot {h(d['reset_now'])} last start {h(d['reset_prev'])}",
        "crash": (f"NEW core {d['core']} vec {d['vec']} PC {h(d['pc'])} RETS {h(d['rets'])} DBG {h(d['dbg'])} "
                  f"up {d['up_ms']} ms etm {h(d['etm3'])} {h(d['etm2'])} {h(d['etm1'])} {h(d['etm0'])}")
                 if crash_new and d.get("magic") == 0x43525348 else "none new",
    }
    if d.get("smc_trace") == 0x534D4301 and d["h_step"] & 0x80000000:
        hdr = d["smc_acl0"].to_bytes(4, "little") + d["smc_acl1"].to_bytes(4, "little")
        out["smc_trace"] = (f"value handle {d['smc_handles'] & 65535}, CCCD handle {d['smc_handles'] >> 16}; "
                            f"ATT response pending {d['smc_att'] >> 16}; "
                            f"last ATT opcode 0x{d['smc_reject'] >> 24:02X}, notify handle {d['smc_att'] & 65535}; "
                            f"last rejected ACL: reason {d['smc_reject'] >> 16 & 255}, bytes {d['smc_reject'] & 65535}, "
                            f"header {hdr.hex(' ')}, CID 0x{d['smc_acl1'] >> 16:04X} (CID valid for start packets of at least 8 bytes)")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-n", type=int, default=1, help="rounds (default 1)")
    ap.add_argument("--hold", type=float, default=20.0, help="seconds READY must stay up (default 20)")
    ap.add_argument("--log", help="append each round as one JSON line")
    ap.add_argument("--diag-only", action="store_true", help="read and print DIAG only")
    ap.add_argument("--pad-inputs", action="store_true", help="read one pad sensor snapshot only; never reset")
    ap.add_argument("--padcal-export", metavar="FILE",
                    help="read the pad calibration (CLP-79, INLOG args 2-5) into FILE for apc_pad_colors.py --pad-override")
    a = ap.parse_args()
    if not ports():
        sys.exit("ble_autotest: no X0X USB MIDI port")
    if a.pad_inputs:
        d = inlog_fmt.parse_pad_inputs(ask(inlog_fmt.ED_INLOG, [inlog_fmt.PAD_INPUT_ARG]))
        print("\n".join(inlog_fmt.pad_inputs_text(d)))
        if d and d["status"] == "snapshot":
            print(json.dumps(d, sort_keys=True))
            return 0
        return 1
    if a.padcal_export:
        ents, tl = [], None
        for arg in inlog_fmt.PADCAL_ARGS:
            rr = ask(inlog_fmt.ED_INLOG, [arg])
            e = inlog_fmt.parse_padcal(rr, arg)
            tl = inlog_fmt.parse_padcal_tl(rr)
            if e is None:
                sys.exit(f"ble_autotest: no pad calibration reply (arg {arg}; firmware before CLP-79?)")
            ents += e
        open(a.padcal_export, "w").write("\n".join(inlog_fmt.padcal_lines(ents, tl)) + "\n")
        print(f"{a.padcal_export}: {sum(e[4] for e in ents)} of 128 entries marked ok")
        return 0
    if a.diag_only:
        d = diag()
        if not d:
            sys.exit("ble_autotest: no DIAG reply")
        out = summary(d, d)
        out["t5w"] = t5w_text(t5w())
        print(json.dumps(out, indent=1))
        p = inlog_fmt.parse(ask(inlog_fmt.ED_INLOG))      # a76: the inbound recorder and the pad output
        pal = inlog_fmt.palette()
        print("\n".join(inlog_fmt.text(p, pal)))
        print("\n".join(inlog_fmt.pads_text(inlog_fmt.parse_pads(ask(inlog_fmt.ED_INLOG, [1])), p[2] if p else None)))
        return 0
    ok = 0
    for rnd in range(1, a.n + 1):
        before = diag()
        r = ask(ED_BT_TEST, [1])
        if not r:
            sys.exit("ble_autotest: no ED_BT_TEST reply (firmware without command 34?)")
        if not r[1]:
            sys.exit("ble_autotest: BLUETOOTH is OFF in the settings: set it ON once on the unit")
        ask(ED_BT_TEST, [2], timeout=1.0)
        t0 = time.time()
        if not wait_port(False, 10):
            print(f"round {rnd}: the port did not go away (no restart?)")
        if not wait_port(True, 40):
            sys.exit(f"round {rnd}: the unit did not come back on USB in 40 s: check it")
        d = settle()
        if not d:
            sys.exit(f"round {rnd}: no DIAG after the restart")
        s = summary(d, before)
        up = False
        if s["status"] in ("READY", "CONNECTED"):
            p0 = d["polls"]
            time.sleep(a.hold)
            d2 = diag()
            up = bool(d2) and STATE[d2["plat_state"] % 8] in ("READY", "CONNECTED") and d2["polls"] > p0 \
                and d2["alive_ms"] >= d["alive_ms"] + 900 * a.hold      # no reset: core 0 ticked all along
            s["hold"] = f"{a.hold:.0f} s: {'stayed up' if up else 'NOT up'}" + \
                (f", polls {p0} -> {d2['polls']}" if d2 else ", no DIAG")
        ok += up
        t = t5w()
        s["t5w"] = t5w_text(t)
        s["round"], s["seconds"] = rnd, round(time.time() - t0, 1)
        print(f"--- round {rnd} ({s['seconds']} s)")
        for k in ("status", "hold", "guard", "start", "hci", "alive", "rails", "reset", "crash", "t5w"):
            if k in s:
                print(f"  {k:7} {s[k]}")
        if a.log:
            with open(a.log, "a") as f:
                f.write(json.dumps({**s, "diag": d, "t5w_raw": t}) + "\n")
    print(f"=== {ok} of {a.n} rounds READY and stayed up")
    return 0 if ok == a.n else 1


if __name__ == "__main__":
    sys.exit(main())
