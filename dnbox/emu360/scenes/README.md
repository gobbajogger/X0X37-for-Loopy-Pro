# emu360 heavy-load gate

Official `/workspace/zdprobe/dnbox360/gate.sh` and its scene were not found
on this box (or in gobbajogger/X0X37-for-Loopy-Pro across the searched
branches and PR bodies). This is a new deterministic driver.

## Command

    bin/heavy.sh elite|pro [fwsc]

Uses already-built 25.1 packages and prepared flash under
`$EMU360_ROOT` or `$ROOT` (on the DnBOX box, `/workspace/emu360-test`) when `fwsc` is omitted. Prints `max_us`, raw
`late`, `late_during_test`, and PASS/FAIL.

PASS = `max_us < 11609` and `late_during_test == 0`.

`late_during_test = late_heavy - late_boot`. A 2000 ms boot-only shot of
the same script (events start at 2220 ms) measures the systematic
startup `late=1` seen on the default (light) scene. Official warmup
handling was not found; PR #863 says "0 late during the test". Do not
set `SKIP_BOOT=1` for a verdict.

Scene: `scenes/heavy.keys` (regenerate with `scenes/gen_heavy.py`).
About 5 s boot + 22–25 s for the 8 s shot on this box.

## Scene (25.1 calibration)

Programs the SMK-37 step grid (white keys = steps, black keys = drum
voices) after boot, then SEQ PLAY:

- 909 and 808: 10 of 11 GM voices, all 16 steps, default 1/16 at 125 BPM
- Both 303s (default Miami on A, Chicago on B): all 16 steps
- BREAK: all 16 steps, COMPLEX/ROLL raised
- Live MIDI holds: Miami 2 voices + Chicago 1 (jack is 31250 baud; a
  dense live flood overflows `MIDI_QUEUE_MAX` and is dropped)
- Default mix/FX (max wet sends / tape / drive, as in
  `tests/host/audio_stress_test.c`, late on 25.1 in this emu)

## 25.1 results (Asia/Vladivostok, UTC+10)

| model | max_us | late | late_during_test | last_us | result | wall heavy | when |
|-------|--------|------|------------------|---------|--------|------------|------|
| Elite | 11566  | 1    | 0                | 7614    | PASS   | 22.424 s   | 2026-11-13 01:13 +10 |
| Pro   | 11571  | 1    | 0                | 7616    | PASS   | 24.736 s   | 2026-11-13 01:13 +10 |

Published official gate: Elite 11,548 µs / Pro 11,564 µs / 0 late
(run-to-run about 11,484–11,578). This scene is 18 µs / 7 µs above
those point values, inside that band. A second Elite shot repeated
11566 µs exactly.

Logs: `out/heavy/elite-cal14`, `out/heavy/elite-25.1` (copy),
`out/heavy/pro-25.1`.
