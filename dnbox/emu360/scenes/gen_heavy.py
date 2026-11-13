#!/usr/bin/env python3
"""Generate a deterministic heavy-load FM1_BENCH_KEYS script for fm1-live.

Official dnbox360/gate.sh and its scene were not found. This scene programs
the SMK-37 step grid (white keys = steps, black keys = drum voices) to the
same worst-case shape as tests/host/audio_stress_test.c, then plays it at
high tempo / 1/32.

Boot is already V_SEQ / PART_909. Events start at t >= 2200 ms so a 2000 ms
boot shot measures startup late without this window.

Usage: gen_heavy.py [out.keys]
"""
from __future__ import annotations
import os, sys

END = int(os.environ.get("END_MS", "7900"))
SKIP_WET_FX = os.environ.get("SKIP_WET_FX", "1") == "1"  # default off: max wet FX lates on 25.1
CH_303A, CH_303B = 1, 2
WHITE_ALL = [5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24, 26, 28, 29, 31]
WHITE = WHITE_ALL  # all 16 steps, 8 voices, no extra wet FX (cal11 8/8 no-FX was 10837 / 0 late)
BLACK = [6, 8, 10, 13, 15, 18, 20, 22, 25, 27, 30]
E_SELECT, E_ALGO, E_K1, E_K2, E_K3, E_K4 = 1, 2, 4, 5, 6, 7

ev: list[str] = []
midi_busy = 0

def add(ms, token):
    ev.append(f"{int(ms)}:{token}")

def midi(ms, *bs):
    global midi_busy
    t = max(int(ms), midi_busy)
    add(t, "m" + "".join(f"{b:02x}" for b in bs))
    midi_busy = t + 2
    return t

def key(ms, name, down=True, vel=None):
    if vel is not None and down:
        add(ms, f"{name}@{vel}")
    else:
        add(ms, f"{name}{'' if down else '^'}")

def pulse(ms, name, hold=50):
    key(ms, name, True)
    key(ms + hold, name, False)
    return ms + hold + 30

def enc(ms, n, clicks):
    add(ms, f"e{n}:{'+' if clicks >= 0 else ''}{clicks}")

def keys_down_up(t, keys, hold=40, gap=20):
    for k in keys:
        key(t, f"k{k}", True, vel=127)
    for k in keys:
        key(t + hold, f"k{k}", False)
    return t + hold + gap

def fill_drum_kit(t):
    for bk in BLACK[:10]:  # 10 of 11 voices (8=11001 PASS, 11=11846 / 2 late)
        t = keys_down_up(t, [bk], hold=35, gap=15)
        t = keys_down_up(t, WHITE, hold=40, gap=15)
    return t

# ---------------------------------------------------------------------------
# 2200: start PLAY, raise tempo (HOME+SELECT keeps V_SEQ), then fill the grid
# ---------------------------------------------------------------------------
t = 2200
# TEMPO: hold PATCH (HOME) and turn SELECT. Stays on the step grid.
# SEQ PLAY starts only after the grid/mix are programmed, so fill
# gestures do not stack on a playing 1/32 pattern (that spiked >15 ms).
# Tempo stays at the default 125 BPM for this calibration step.
# A single large SELECT burst is accelerated and clamps to 275, which
# overloads 16th-note all-voice playback (max_us ~14.2 ms, many lates).
t += 20

# 909 rate 1/32, fill all 11 voices × 16 steps
t = fill_drum_kit(t)
enc(t, E_ALGO, +1)  # 808
t += 50
t = fill_drum_kit(t)
enc(t, E_ALGO, +1)  # BRK
t += 50
t = keys_down_up(t, WHITE, hold=40, gap=20)
t = pulse(t, "SEQ")  # GROOVE
enc(t, E_K1, +80)
enc(t + 5, E_K3, +80)
t += 50
enc(t, E_ALGO, +1)  # 303B
t += 50
t = keys_down_up(t, WHITE, hold=40, gap=20)
enc(t, E_ALGO, +1)  # 303A
t += 50
t = keys_down_up(t, WHITE, hold=40, gap=20)
print(f"pattern programmed by {t} ms", file=sys.stderr)

# Leave V_SEQ so later notes cannot toggle steps off (PARA = B_EDIT → V_PART)
t = pulse(t, "PARA")

if not SKIP_WET_FX:
    # Wet mix / FX / master while the sequencer is already running
    t = pulse(t, "NOTE REPEAT")
    for _ in range(5):
        enc(t, E_K1, +40)
        enc(t + 5, E_K3, +100)
        enc(t + 10, E_K4, +100)
        t += 55
        enc(t, E_ALGO, +1)
        t += 35
    for _ in range(3):
        t = pulse(t, "NOTE REPEAT")
    enc(t, E_K1, +80)
    enc(t + 5, E_K2, +80)
    enc(t + 10, E_K3, +60)
    enc(t + 15, E_K4, +40)
    t += 50
    t = pulse(t, "FX")
    enc(t, E_K1, +80)
    t += 50
    t = pulse(t, "FX")
    enc(t, E_K2, +80)
    t += 50

# Hold both Miami voices + Chicago
t = midi(t, 0x90 | CH_303A, 48, 100)
t = midi(t + 2, 0x90 | CH_303A, 63, 100)
t = midi(t + 2, 0x90 | CH_303B, 53, 100)
t = pulse(t + 20, "SEQ PLAY")
print(f"PLAY at {t} ms; shot ends {END}", file=sys.stderr)

# Late fader sweep (ignored until ~200 UI frames ≈ 6 s)
ft = max(t + 20, 6800)
for raw in range(0, 1024, 80):
    for f in range(16, 20):
        add(ft, f"a{f}:{raw}")
    ft += 20
for f in range(16, 20):
    add(ft, f"a{f}:1023")
ft = pulse(ft + 20, "FADER BANK")
for raw in range(0, 1024, 80):
    for f in range(16, 20):
        add(ft, f"a{f}:{raw}")
    ft += 20
for f in range(16, 20):
    add(ft, f"a{f}:1023")

rt = t + 200
while rt <= END - 200:
    midi(rt, 0x80 | CH_303B, 53, 0)
    midi(rt + 20, 0x90 | CH_303B, 53, 100)
    rt += 400

midi(END + 40, 0x80 | CH_303A, 48, 0)
midi(END + 42, 0x80 | CH_303A, 63, 0)
midi(END + 44, 0x80 | CH_303B, 53, 0)

ev.sort(key=lambda s: int(s.split(":", 1)[0]))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__) or ".", "heavy.keys")
text = ",".join(ev)
with open(out, "w") as f:
    f.write(text + "\n")
print(
    f"wrote {out}: {len(ev)} events, {len(text)} bytes, "
    f"first={ev[0].split(':')[0]} last={ev[-1].split(':')[0]}",
    file=sys.stderr,
)
