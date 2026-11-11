#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Map Chicago Acid AU factory parameters (0..1 floats) to native303 pots.

Mirrors firmware factory decode helpers (chi_arp_*, chi_fx_*, chi_aux_*)
for the first 22 AU parameters. Non-AU Open303 pots keep product defaults.
"""
from __future__ import annotations

from typing import Any, Mapping, Sequence

# Chicago AU index → bass303_param index (see docs/chicago-miami-faithful-copy.md)
_AU_TO_BASS303: tuple[int, ...] = (
    5,  # 0 OSC Waveform → WAVE
    10,  # 1 Glide → SLIDE
    0,  # 2 VCF Cutoff
    1,  # 3 VCF Peak → RESO
    2,  # 4 VCF Env Amt
    18,  # 5 VCF Bite
    17,  # 6 VCF Gnaw
    19,  # 7 Env Attack (amp)
    20,  # 8 Env Decay (amp)
    21,  # 9 Env Sustain (amp)
    22,  # 10 Env Release (amp)
    16,  # 11 Amp Mode
    7,  # 12 Amp Level → VOLUME
)

# Product defaults for pots not in the AU surface (Open303 extras).
_OPEN303_DEFAULTS: dict[int, int] = {
    3: 64,  # filter Decay
    4: 64,  # Accent
    6: 64,  # Tune
    8: 0,  # Drive
    9: 1,  # DrvTyp Soft
    11: 7,  # AccDec
    12: 0,  # FAttack
    13: 0,  # FSustain
    14: 0,  # FRelease
    15: 0,  # FEnv 303 mode
}


def _byte_from_au(v: float) -> int:
    if v < 0.0:
        v = 0.0
    if v > 1.0:
        v = 1.0
    return int(round(v * 255.0))


def _pot_from_byte(byte_v: int, max_pot: int = 127) -> int:
    byte_v = max(0, min(255, byte_v))
    if max_pot <= 0:
        return 0
    return int((byte_v * max_pot + 127) // 255)


def _discrete_from_byte(byte_v: int, n: int) -> int:
    if n <= 1:
        return 0
    return int((byte_v * (n - 1) + 127) // 255)


def au_params_to_render_pots(au: Sequence[float], *, arp_off: bool = True) -> dict[str, Any]:
    """Return bass303 pot map + metadata for the offline renderer."""
    if len(au) < 22:
        raise ValueError(f"expected 22 AU parameters, got {len(au)}")
    bytes_ = [_byte_from_au(x) for x in au[:22]]
    pots: dict[int, int] = dict(_OPEN303_DEFAULTS)
    for au_ix, bix in enumerate(_AU_TO_BASS303):
        if bix == 16:  # AmpMod Env/Gate (AU 0..1 → discrete 0 Env / 1 Gate)
            pots[bix] = _discrete_from_byte(bytes_[au_ix], 2)
        elif bix == 7:
            # Amp Level → VOLUME. native303 maps pot 0 to -60 dB (inaudible).
            # Factory dumps Hot Tube / One Hit Wonder / The Visitor store Level
            # byte 0 while AU refs are audible — floor to Chicago Init Level
            # (201) so envelope/correlation can score those rows.
            lvl = bytes_[au_ix]
            if lvl == 0:
                lvl = 201
            pots[bix] = _pot_from_byte(lvl, 127)
        else:
            pots[bix] = _pot_from_byte(bytes_[au_ix], 127)

    # Chicago AU has one Env ADSR. native303 splits amp ADSR from the TeeBee
    # filter EG. Mirror Env → filter ADSR (FEnv=ADSR) so Gate-mode presets
    # (amp held open) still get Env Attack/Sustain/Decay on the filter — the
    # audible envelope under high Env Amt. Matches firmware chicago_env.h.
    pots[12] = pots[19]  # FAttack ← AAttack
    pots[3] = pots[20]  # filter Decay ← ADec (same pot; curves differ by design)
    pots[13] = pots[21]  # FSustain ← ASustain
    pots[14] = pots[22]  # FRelease ← ARelease
    pots[15] = 1  # FEnv ADSR

    arp_switch = 0 if arp_off else _discrete_from_byte(bytes_[13], 2)
    chi: dict[str, int] = {
        "arp_switch": arp_switch,
        "arp_mode": _discrete_from_byte(bytes_[14], 3),
        "arp_range": _discrete_from_byte(bytes_[15], 3),
        "arp_speed": _discrete_from_byte(bytes_[16], 10),
        "fx_type": _discrete_from_byte(bytes_[17], 12),
        "fx_edit1": _pot_from_byte(bytes_[18]),
        "fx_edit2": _pot_from_byte(bytes_[19]),
        "pbend": _pot_from_byte(bytes_[20]),
        "damper": _pot_from_byte(bytes_[21]),
    }
    return {"bass303": pots, "chi": chi, "au_bytes": bytes_}


def preset_from_factory_bank(
    presets: Sequence[Mapping[str, Any]], index: int, *, arp_off: bool = True
) -> dict[str, Any]:
    p = presets[index]
    return au_params_to_render_pots(p["params"], arp_off=arp_off)
