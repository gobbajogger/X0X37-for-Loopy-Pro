#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Time / phase alignment for AU vs firmware sound compares.

Sign-off uses **aligned** metrics only (F_agg >= 90% min, 99% target).
**Unaligned** sample-by-sample correlation at lag=0 is diagnostic — never the ship bar.

Alignment pipeline (Chicago + Miami):
  1. **Render start** — scenario manifest fixes MIDI/note-on time, pre-roll, and sample rate;
     candidate and AU ref renders share the same protocol (host x0x_host or AU export SOP).
  2. **Lag search** — before correlation or windowed spectrum/envelope on coupled segments,
     find integer-sample lag that maximizes normalized cross-correlation (same law as Miami
     ``compare.py`` lag correlation and ``bass303_test.c`` ``best_lag``).
  3. **Optional cyclic phase** — for LFO/wobble steady-state scenarios, search LFO phase offset
     over one period after lag lock (extends lag search; required when scenario tag says
     ``phase_locked_lfo``).

Metric classes:
  **Phase-invariant** (fair after lag search on a steady voiced window):
    - pitch (fundamental estimators on aligned windows)
    - strong relative spectrum (union mask on aligned steady segment)
    - steady envelope RMS (same windows, ref vs candidate at best lag)

  **Phase-locked** (lag search alone is insufficient — scenario must define sync):
    - note transients / attack onset (compare from aligned note-on, not arbitrary buffer start)
    - LFO/wobble shape when scenario requires AU/firmware LFO phase match (cyclic search or
      shared MIDI reset + render-start alignment)
    - sequencer-step-bound acid lines (gate timing vs transport)

**Not AU compare** (related): SMC rhythm FX gates must follow **sequencer transport phase**
([#372](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/372)); that is product law,
not the offline AU PCM harness — but wobble/mod scenarios still use render-start + cyclic rules above.

See Project store ``docs/chicago-miami-sound-execution.md`` (alignment section).
"""
from __future__ import annotations

import math
from typing import Sequence


def best_lag_samples(
    ref: Sequence[float],
    cand: Sequence[float],
    lag_min: int = -4096,
    lag_max: int = 4096,
) -> int:
    """Integer lag L maximizing sum ref[i] * cand[i+L] on overlapping samples.

    Positive L means candidate is delayed (shift cand left relative to ref).
    Mirrors host ``bass303_test.c`` / Miami ``compare.py`` lag-correlation approach.
    """
    n = min(len(ref), len(cand))
    if n < 8:
        return 0
    lag_min = max(lag_min, -(n - 8))
    lag_max = min(lag_max, n - 8)
    best_lag = 0
    best_score = -1.0
    for lag in range(lag_min, lag_max + 1):
        if lag >= 0:
            a = ref[: n - lag]
            b = cand[lag:n]
        else:
            a = ref[-lag:n]
            b = cand[: n + lag]
        if len(a) < 8:
            continue
        dot = sum(float(x) * float(y) for x, y in zip(a, b))
        na = math.sqrt(sum(float(x) * float(x) for x in a))
        nb = math.sqrt(sum(float(y) * float(y) for y in b))
        if na <= 0.0 or nb <= 0.0:
            continue
        score = dot / (na * nb)
        if score > best_score:
            best_score = score
            best_lag = lag
    return best_lag


def lag_aligned_correlation(
    ref: Sequence[float],
    cand: Sequence[float],
    lag_min: int = -4096,
    lag_max: int = 4096,
) -> tuple[int, float]:
    """Return (best_lag, Pearson r) on the overlapping region at best_lag.

    This is the correlation goal input for F_agg — not lag=0 raw r.
    """
    lag = best_lag_samples(ref, cand, lag_min, lag_max)
    n = min(len(ref), len(cand))
    if lag >= 0:
        a = ref[: n - lag]
        b = cand[lag:n]
    else:
        a = ref[-lag:n]
        b = cand[: n + lag]
    if len(a) < 8:
        return lag, 0.0
    dot = sum(float(x) * float(y) for x, y in zip(a, b))
    na = math.sqrt(sum(float(x) * float(x) for x in a))
    nb = math.sqrt(sum(float(y) * float(y) for y in b))
    if na <= 0.0 or nb <= 0.0:
        return lag, 0.0
    return lag, dot / (na * nb)
