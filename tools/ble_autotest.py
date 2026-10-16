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
     For READY, read DIAG again after --hold seconds: the controller must stay up (polls grow).

A software restart is not a power cycle: .noinit RAM (the start guard, the crash record) stays,
and the radio's analog and power (P33) state may stay too. Confirm a result with a real power
cycle at the end.

Attempt 52: every round also reads core 1's full fault record (ED_BT_TEST op 5: the whole frame
with usp/ssp/reti/rete/retx/psr/icfg, core 1's EMU_MSG, 16 words from usp, the live stack-limit
window) and, with --syms (a JSON made by --make-syms from ble/ctl/out/symbols.txt), peeks the
image's RAM (op 6): cur, idle_ctx, both task records with their saved frames and stack words, the
switch ring, the timers, the queue/semaphore counters and the mailbox (the HCI rings).

Usage: tools/ble_autotest.py [-n ROUNDS] [--hold S] [--log FILE] [--diag-only] [--syms FILE]
       tools/ble_autotest.py --make-syms ble/ctl/out/symbols.txt OUT.json
Needs mido with python-rtmidi. Exit 0 = every round READY and stayed up.
"""
import argparse
import json
import sys
import time

try:
    import mido
except ImportError:                                # --make-syms needs no MIDI
    mido = None

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
        "h2c", "c2h", "c2h_drops", "h_step", "ctl_took", "ctl_gave", "rx0", "rx1", "alive_ms", "alive_polls", "rails0", "rails1",
        "hx_state", "hx_int_mtu", "hx_links", "hx_pairs", "hx_heap", "hx_stacks", "hx_nv", "hx_rx", "hx_tx", "hx_tx_drops",
        "d_state_fr", "d_send_sx", "d_app_flags", "d_mirror_n"]
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
    w = []
    while True:                                    # attempt 48: up to 70 words a reply, the rest from an offset
        r = ask(ED_DIAG, [len(w)] if w else [])
        if not r:
            if not w:
                return None
            break
        n = r[0]
        for k in range(n):
            b = r[1 + 5 * k:6 + 5 * k]
            w.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
        if n < 70 or len(w) > 127:
            break
    head = CRASH + BOOT
    d = {k: v for k, v in zip(head, w)}
    rest = w[len(head):]
    if len(rest) >= len(LIVE):                     # the live words are always the last len(LIVE)
        d.update(zip(LIVE, rest[-len(LIVE):]))
        rest = rest[:-len(LIVE)]
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


# ---- attempt 52: core 1's full fault record and the image's RAM ----
FRAME = [f"r{i}" for i in range(16)] + ["reti", "rete", "retx", "stub", "psr", "icfg", "usp", "ssp", "sp", "rets"]
EMU_BITS = {0: "misaligned", 1: "illegal instruction", 2: "divide by zero", 3: "STACK LIMIT (sp outside the window)",
            4: "privilege", 5: "bus/address"}
SYMS = ["cur", "idle_ctx", "in_run_once", "ctl_sw_n", "ctl_sw_ring", "tasks", "timers", "ctl_q_full",
        "ctl_d_post", "ctl_d_pend", "ctl_d_ran", "heap_first", "ctl_mb", "ctl_bp", "ctl_d_timers",
        # attempt 52b: the controller's advertising state (btctrler.a ll_adv_*) and the host's view
        "adv_hdl", "ll_white_list_used", "adv_data", "scan_resp_data", "ll_error_code", "hx_started", "hx_radio",
        "hx_conn", "hx_adv.adv", "ctl_d_ev_n", "ctl_d_ev_last"]


def make_syms(src, out):
    """objdump -t text (ble/ctl/out/symbols.txt) -> {name: [address, size]} for --syms"""
    d = {}
    for line in open(src):
        f = line.split()
        if len(f) >= 5 and len(f[0]) == 8:
            try:
                d[f[-1]] = [int(f[0], 16), int(f[-2], 16)]
            except ValueError:
                pass
    json.dump({k: d[k] for k in SYMS + ["__ctl_isr_stack_lo", "__ctl_isr_stack_hi", "__ctl_heap_start",
                                        "__ctl_heap_end", "ctl_port_ctx_switch", "ctl_asm_ctx_switch"] if k in d},
              open(out, "w"), indent=1)


def peek(addr, n):
    w = []
    while n > 0:
        k = min(n, 64)
        a = addr + 4 * len(w)
        r = ask(ED_BT_TEST, [6] + [(a >> (7 * i)) & 0x7F for i in range(5)] + [k])
        if not r or r[0] != 6:
            return w or None
        for j in range(r[1]):
            b = r[2 + 5 * j:7 + 5 * j]
            w.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
        n -= k
    return w


def c1x():
    w = bt_words(5)
    if not w or len(w) < 50:
        return None
    return {"magic": w[0], "emu": w[1], "frame": dict(zip(FRAME, w[2:28])), "usp_words": w[28:44],
            "live": dict(zip(["emu_con", "emu_msg", "lim0_hi", "lim0_lo", "lim1_hi", "lim1_lo"], w[44:50]))}


def c1x_text(x, syms=None):
    if not x:
        return "no reply (older firmware)"
    lv = x["live"]
    win = f"live limit window {h(lv['lim0_lo'])}..{h(lv['lim0_hi'])} / {h(lv['lim1_lo'])}..{h(lv['lim1_hi'])}, EMU_CON {h(lv['emu_con'])}"
    if x["magic"] != 0xC1F4A052:
        return "no core 1 fault record; " + win
    f, e = x["frame"], x["emu"]
    bits = ", ".join(v for b, v in EMU_BITS.items() if e >> b & 1) or "none"
    out = (f"core 1 frame: reti {h(f['reti'])} rete {h(f['rete'])} retx {h(f['retx'])} rets {h(f['rets'])} "
           f"psr {h(f['psr'])} icfg {h(f['icfg'])} usp {h(f['usp'])} ssp {h(f['ssp'])} sp {h(f['sp'])}; "
           f"EMU_MSG {h(e)} ({bits}); " + win)
    lo, hi = min(lv["lim0_lo"], lv["lim1_lo"]), max(lv["lim0_hi"], lv["lim1_hi"])
    if lv["lim0_hi"] and not (lv["lim0_lo"] <= f["usp"] <= lv["lim0_hi"]):
        out += f"; usp OUTSIDE the window"
    out += "; r0-r15 " + " ".join(f"{f[f'r{i}']:08X}" for i in range(16))
    out += "; [usp] " + " ".join(f"{v:08X}" for v in x["usp_words"])
    return out


def ram_dump(syms):
    """the image's scheduler state by address (op 6)"""
    out = {}
    g = lambda n, k=None: peek(syms[n][0], k or max(1, (syms[n][1] + 3) // 4)) if n in syms else None
    for n in ("cur", "idle_ctx", "in_run_once", "ctl_sw_n", "ctl_q_full", "ctl_d_post", "ctl_d_pend", "ctl_d_ran",
              "heap_first", "ctl_mb", "ctl_d_timers"):
        v = g(n)
        if v:
            out[n] = v[0]
    ring = g("ctl_sw_ring")
    if ring and "ctl_sw_n" in out:
        n = out["ctl_sw_n"]
        ent = [(ring[i], ring[i + 1]) for i in range(0, 16, 2)]
        order = [(n - 2 - 2 * j) % 16 // 2 for j in range(min(n // 2, 8))]
        out["switches"] = [f"task {ent[i][0] >> 28} poll {ent[i][0] & 0xFFFFFFF} sp {h(ent[i][1])}" for i in order]
    if "idle_ctx" in out and 0x01C00000 <= out["idle_ctx"] < 0x01C80000:
        out["idle_frame"] = [h(v) for v in peek(out["idle_ctx"], 16) or []]
    t = g("tasks")
    if t:
        sz = len(t) // 2
        out["tasks"] = []
        for i in range(2):
            r = t[i * sz:(i + 1) * sz]
            k = {"name_p": h(r[0]), "fn": h(r[1]), "sp": h(r[3]), "stack": h(r[4]), "wake_ms": r[5],
                 "used": r[6] & 0xFF, "waiting": r[6] >> 8 & 0xFF, "qhead": r[6] >> 16, "qtail": r[7] & 0xFFFF,
                 "q": r[8:24]}
            if 0x01C00000 <= r[3] < 0x01C80000:
                k["saved"] = [h(v) for v in peek(r[3], 24) or []]   # rets/pc, r4..r15 order as pushed, then callers
            out["tasks"].append(k)
    tm = g("timers")
    if tm:
        out["timers_raw"] = [h(v) for v in tm]
    if out.get("ctl_mb") and 0x01C00000 <= out["ctl_mb"] < 0x01C80000:
        out["mailbox_raw"] = [h(v) for v in peek(out["ctl_mb"], 192) or []]   # rings h2c/c2h, dbg words, hx
    return out


def adv_dump(syms):
    """attempt 52b: is the controller advertising? (from the btctrler.a disassembly)
    adv_hdl -> 40 B ll_adv struct: byte 0 = 1 once ll_adv_open (HCI LE Set Advertising Enable 1) succeeded,
    word at +4 = the le_hw advertising instance (24 B, nonzero while open). ll_adv_set_params copies the
    15 B parameters to ll_white_list_used (0x..A4) only while byte 0 is 0. adv_data / scan_resp_data: length byte first."""
    pk = lambda n, k=None: peek(syms[n][0] & ~3, k or max(1, (syms[n][1] + (syms[n][0] & 3) + 3) // 4)) if n in syms else None
    def by(n, ln=None):
        w = pk(n, ((ln or syms[n][1]) + (syms[n][0] & 3) + 3) // 4) if n in syms else None
        if not w:
            return None
        b = b"".join(v.to_bytes(4, "little") for v in w)
        o = syms[n][0] & 3
        return b[o:o + (ln or syms[n][1])]
    out = {}
    hp = pk("adv_hdl")
    if hp:
        out["adv_hdl"] = h(hp[0])
        if 0x01C00000 <= hp[0] < 0x01C80000:
            st = peek(hp[0], 10) or []
            out["ll_adv_raw"] = [h(v) for v in st]
            if st:
                out["adv_enabled"] = bool(st[0] & 0xFF)
                out["hw_instance"] = h(st[1])
                if 0x01C00000 <= st[1] < 0x01C80000:
                    out["hw_raw"] = [h(v) for v in peek(st[1], 6) or []]
        else:
            out["adv_enabled"] = False          # ll_adv_init never ran: nothing ever asked for advertising
    for n, ln in (("ll_white_list_used", 16), ("adv_data", 32), ("scan_resp_data", 32), ("hx_adv.adv", 31)):
        b = by(n, ln)
        if b is not None:
            out[n if n != "ll_white_list_used" else "adv_param"] = b.hex()
    for n in ("ll_error_code", "hx_started", "hx_radio", "hx_conn", "ctl_d_ev_n", "ctl_d_ev_last"):
        b = by(n)
        if b is not None:
            out[n] = int.from_bytes(b, "little")
    if "adv_param" in out:
        p = bytes.fromhex(out["adv_param"])[1:]   # ll_white_list_used byte, then the 15 B parameters
        out["adv_param_text"] = (f"interval {int.from_bytes(p[0:2], 'little') * 0.625:g}..{int.from_bytes(p[2:4], 'little') * 0.625:g} ms, "
                                 f"type {p[4]}, own addr type {p[5]}, channels 0x{p[13]:02X}, filter {p[14]}")
    return out


def live_words(took, gave):
    """attempt 45 DIAG words after READY (ctl_main.c d_live), also kept in FAILED and after a reset.
    attempt 53: ctl_d_post/pend saturate at 15 in ctl_os.c d_inc, so the 4-bit fields never spill into
    each other (a52's 0x3399FF02 is 15, 15, 9, 9, not an overlap); 15 means "15 or more"."""
    over = (gave >> 8 & 0xFFFF) and any((gave >> s & 15) == 15 for s in (8, 12, 16, 20))
    return (f"took {took & 0xFF} frames, {took >> 8 & 0xFF} host events (last 0x{took >> 16 & 0xFF:02X}), "
            f"{took >> 24} timer runs; gave {gave & 0xFF} frames; controller task posts {gave >> 8 & 15} "
            f"pends {gave >> 12 & 15}, host task posts {gave >> 16 & 15} pends {gave >> 20 & 15}; "
            f"waiting {gave >> 24 & 3:02b}, task_create fails {gave >> 26 & 3}, ran {gave >> 28 & 3:02b}"
            + ("; a post/pend count reached 15 (saturated: 15 or more)" if over else "") + f" [raw took {h(took)} gave {h(gave)}]")


def stage_word(d):
    """attempt 51: the image's stop record (ctl_os.c d_stop) and its last task switch (dbg_stall)"""
    st, v, ra, sw = d["dbg_stage"], d["dbg_delays"], d["dbg_ra"], d["dbg_stall"]
    out = ""
    if st == 0x5110:
        out = f"STOP: a wait inside an interrupt, caller {h(ra)}; "
    elif st & ~1 == 0x5120:
        out = f"STOP: task {st & 1} saved sp {h(v)} outside its stack (word {h(ra)}); "
    if sw >> 24 == 0x51:
        out += f"last switch into task {sw >> 20 & 15} at poll {sw & 0xFFFFF}"
    return out or "-"


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
               f"c2h {d['c2h'] & 0xFFFF}/{d['c2h'] >> 16} drops {d['c2h_drops']}; "
               + live_words(d['ctl_took'], d['ctl_gave']) + f"; polls {d['polls']}",
        "link": (f"interval {(d['rx0'] & 0xFFFF) * 1.25:g} ms, latency {d['rx1'] >> 21 & 7}; last end reason 0x{d['rx0'] >> 16 & 0xFF:02X}; "
                 f"pairing refused {d['rx0'] >> 24}; asked MTU {d['rx1'] >> 8 & 0x3FF}; L2CAP requests {d['rx1'] >> 24}; MIDI in drops {d['rx1'] & 0xFF}; "
                 f"parameter request {['not sent', 'sent'][d['rx1'] >> 18 & 1]}, {['no answer', 'accepted', 'rejected', '?'][d['rx1'] >> 19 & 3]}"),
        "alive": f"core 0 last Bluetooth tick at {d['alive_ms']} ms after boot, core 1 polls then {d['alive_polls']}, now {d['polls']}",
        "rails": (lambda a, b: f"before/after the stock supply setup: SYSVDD sel {(a >> 16) & 15} -> {(b >> 16) & 15}, VDC14 sel {(a >> 8) & 7} -> {(b >> 8) & 7}, VDDIOM sel {a & 7} -> {b & 7}, CON0 0x{(a >> 24) & 255:02X} -> 0x{(b >> 24) & 255:02X} (DCDC13 {'on' if (b >> 24) & 8 else 'off'}, LDO13 {'on' if (b >> 24) & 4 else 'off'}); setup read-back {'ok' if ((b >> 24) & 0x0C) == 0x08 and (b & 0x37) == 0x34 and ((b >> 8) & 7) == 7 and ((b >> 16) & 15) == 15 else 'NOT ok'} (raw 0x{a:08X} -> 0x{b:08X})")(d.get('rails0', 0), d.get('rails1', 0)),
        "heap": (lambda x, t: f"peak {x & 0xFFFF} B, failed allocations {x >> 16 & 0xFF}, last failed size ~{(x >> 24) * 16} B; "
                 f"stack free: tasks {(t & 0xFF) * 16} B, run {(t >> 8 & 0xFF) * 16} B, irq {(t >> 16 & 0xFF) * 16} B; queue full {t >> 24}")(d['hx_heap'], d['hx_stacks']),
        "reset": f"this boot {h(d['reset_now'])} last start {h(d['reset_prev'])}",
        # attempt 51: after READY (and after a reset: the guard keeps the last start's mailbox) the
        # mailbox's MS / INIT_MS words are d_live's took/gave, not times; the stop record and switch
        "last_start": (("before the reset that blocked it: " if d['why'] == 4 else "")
                       + (live_words(d['dbg_init_ms'], d['dbg_ms']) if d['dbg_stage'] >= 7 else "init not done")
                       + f"; polls {d['polls']}; {stage_word(d)}"),
        "crash": (f"NEW core {d['core']} vec {d['vec']} PC {h(d['pc'])} RETS {h(d['rets'])} DBG {h(d['dbg'])} "
                  f"up {d['up_ms']} ms etm {h(d['etm3'])} {h(d['etm2'])} {h(d['etm1'])} {h(d['etm0'])}")
                 if crash_new and d.get("magic") == 0x43525348 else
                 (f"none new; last record: core {d['core']} vec {d['vec']} PC {h(d['pc'])} RETS {h(d['rets'])} "
                  f"SP {h(d['sp'])} up {d['up_ms']} ms (count {d['count']})" if d.get("magic") == 0x43525348 else "none"),
    }
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-n", type=int, default=1, help="rounds (default 1)")
    ap.add_argument("--hold", type=float, default=20.0, help="seconds READY must stay up (default 20)")
    ap.add_argument("--log", help="append each round as one JSON line")
    ap.add_argument("--diag-only", action="store_true", help="read and print DIAG only")
    ap.add_argument("--syms", help="image symbols JSON (--make-syms) for the RAM dump")
    ap.add_argument("--make-syms", nargs=2, metavar=("SYMBOLS_TXT", "OUT_JSON"))
    a = ap.parse_args()
    if a.make_syms:
        make_syms(*a.make_syms)
        return 0
    syms = json.load(open(a.syms)) if a.syms else None
    if not ports():
        sys.exit("ble_autotest: no X0X USB MIDI port")
    if a.diag_only:
        d = diag()
        if not d:
            sys.exit("ble_autotest: no DIAG reply")
        out = summary(d, d)
        out["t5w"] = t5w_text(t5w())
        x = c1x()
        out["core1"] = c1x_text(x)
        out["core1_raw"] = x
        if syms:
            out["ram"] = ram_dump(syms)
            out["adv"] = adv_dump(syms)
        print(json.dumps(out, indent=1))
        if a.log:
            with open(a.log, "a") as f:
                f.write(json.dumps({**out, "diag": d}) + "\n")
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
        x = c1x()
        s["core1"] = c1x_text(x)
        ram = ram_dump(syms) if syms else None
        if syms:
            s["adv"] = json.dumps(adv_dump(syms))
        s["round"], s["seconds"] = rnd, round(time.time() - t0, 1)
        print(f"--- round {rnd} ({s['seconds']} s)")
        for k in ("status", "hold", "guard", "start", "last_start", "hci", "alive", "rails", "reset", "crash", "core1", "t5w", "adv"):
            if k in s:
                print(f"  {k:7} {s[k]}")
        if a.log:
            with open(a.log, "a") as f:
                f.write(json.dumps({**s, "diag": d, "t5w_raw": t, "core1_raw": x, "ram": ram}) + "\n")
    print(f"=== {ok} of {a.n} rounds READY and stayed up")
    return 0 if ok == a.n else 1


if __name__ == "__main__":
    sys.exit(main())
