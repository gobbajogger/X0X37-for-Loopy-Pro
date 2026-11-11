#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Map Miami Wobble AU factory parameters (0..1 floats) to wobble_set indices."""
from __future__ import annotations

from typing import Any, Sequence

# experiments/wobble/wobble.h enum order (first 18 AU parameters).
_WOBBLE_AU_COUNT = 18


def preset_from_factory_bank(
    presets: Sequence[dict[str, Any]], index: int
) -> dict[str, Any]:
    if index < 0 or index >= len(presets):
        raise IndexError(f"preset index {index} out of range")
    row = presets[index]
    au = row["params"]
    if len(au) < _WOBBLE_AU_COUNT:
        raise ValueError(f"preset {row.get('name')}: expected {_WOBBLE_AU_COUNT} params")
    wobble = {i: float(au[i]) for i in range(_WOBBLE_AU_COUNT)}
    return {
        "name": row["name"],
        "factoryPresetIndex": index,
        "wobble": wobble,
    }
