# emu360 heavy-load stand-in scene

Stand-in heavy scene written 2026-11-13 because the original scene used for
the published 25.1 gate was not found (not in this repo, its history, or
the DnBOX box archives). This folder is text-only: no firmware images,
audio, or run outputs.

## How to run

Needs a built `fm1-live` plus a firmware package (`.fwsc`) and its `x0x.elf`.
Point `EMU360_ROOT`, `BIN`, and `ELF` at those binaries if they are not in
the usual box layout (`EMU360_ROOT=/workspace/emu360-test`).

```bash
# Heavy-load gate (recommended)
EMU360_ROOT=/path/to/layout ./bin/heavy.sh elite /path/to/package.fwsc

# Equivalent direct invocation
FM1_STRICT=1 FM1_BENCH_KEYS=@/path/to/dnbox/emu360/scenes/heavy.keys \
  fm1-live --fwsc PACKAGE.fwsc --mhz 360 --shot 8000
```

`bin/heavy.sh` sets `FM1_STRICT=1`, `--mhz 360`, `--shot 8000`, and
`FM1_BENCH_KEYS` from `scenes/heavy.keys`. It subtracts a 2000 ms boot
shot so the verdict uses `late_during_test`, not the systematic startup
`late=1`. Regenerate the key script with `scenes/gen_heavy.py`.

## Gate

PASS when all of:

- `max_us` < 11,609
- `late_during_test` == 0
- complete half count is 648 (8 s shot)

## Calibration (Asia/Vladivostok, UTC+10)

| build | Elite `max_us` | Pro `max_us` |
|---|---:|---:|
| 25.1 (`78f81bad0`) on this scene | 11,566 | 11,571 |
| published official 25.1 gate | 11,548 | 11,564 |
| #872 `eb652e49` | 10,902 | 10,915 |

25.1 is 18 µs (Elite) and 7 µs (Pro) above the published point values, and
recorded 0 late during the test. #872 also passed this gate with 0 late
during the test.
