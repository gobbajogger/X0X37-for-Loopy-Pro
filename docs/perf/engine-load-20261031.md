# Engine load, 2026-10-31

## Change

- Device builds (tools/build.py) add `-DX0X_FAST_FMA`. `original_rne_fmaf`
  is then a plain `a*b + c` (one rounding of the product, one of the sum).
  The AC79 has no hardware FMA, so the exact helper was a 64-bit integer
  routine in software.
- `fused_reference.c` keeps the exact body (for host tests and references).
- `chicago_voice.c` keeps the exact FMA (`X0X_EXACT_FMA_HERE`). Fast FMA
  there moved the output by -78 dBFS peak, over the -90 dBFS limit.
- Master limiter: the look-ahead index uses `& (MST_LA - 1)`, not `%`.
- Host tests stay exact (no `X0X_FAST_FMA` in the host build).

## Audio difference (host, exact build vs. fast build, 16-bit WAV)

All cases: max diff and RMS diff = none (bit-identical).
Cases: idle, Miami 1 note, Miami 8 notes, Chicago 1 note, Chicago 8 notes,
Miami random 40 notes, Chicago random 40 notes, drums pattern, break pattern.

## Host time (ns per frame, x86, budget 22676)

| Case | Stage | Before | After |
|---|---|---|---|
| Miami 1 note | 303A | 188.6 | 171.1 |
| Miami 8 notes | 303A | 201.1 | 187.6 |
| Miami random | 303A | 167.5 | 126.0 |
| Chicago 1 note | 303B | 98.8 | 94.4 |
| Chicago 8 notes | 303B | 104.5 | 100.1 |
| Chicago random | 303B | 73.8 | 70.9 |
| Idle | 303B | 3.3 | 2.2 |
| Drums | 909 / FX | 10.2 / 23.5 | 10.3 / 23.5 |
| Break | BRK / FX | 4.4 / 32.5 | 4.4 / 32.6 |
| Idle | MST | 11.6 | 11.3 |

## Soft FMA calls

| | Before | After |
|---|---|---|
| Static call sites in x0x.dis | 51 | 28 (chicago_voice only) |
| Dynamic per frame, Miami 1 note | 1.87 | 0 |
| Dynamic per frame, Miami 8 notes | 1.84 | 0 |
| Dynamic per frame, Chicago 1 / 8 notes | 0.44 | 0.31 |

The device has no `__adddf3`-style soft-float libcalls. The FPU does single
precision. Device time was not measured (no flash in this change).
