# SPDX-License-Identifier: GPL-3.0-only
"""Bounded apparent pitch trajectories, using local harmonic fits (not internal pitch).

Private reference files load only when this file is run or main() is called.
"""
import importlib.util
import json
import pathlib
import sys

import numpy as np


def _measure():
    path = pathlib.Path(__file__).resolve().with_name("measure.py")
    name = "audio303_measure"
    if name in sys.modules:
        return sys.modules[name]
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


measure = _measure()


def read_f32(path, stereo=False):
    x = np.fromfile(path, dtype="<f4").astype(float)
    return x.reshape(-1, 2).mean(axis=1) if stereo else x


def main(root=None):
    root = pathlib.Path(root) if root is not None else pathlib.Path(__file__).resolve().parents[3]
    out = {}
    for name in ("overlap_glide0", "overlap_glide_mid", "overlap_retrigger"):
        out[name] = {}
        for eng in ("au", "dsp"):
            x = read_f32(root / "audio" / f"{name}-r0.{eng}.f32", stereo=(eng == "au"))
            out[name][eng] = [measure.apparent_hz_at(x, center) for center in measure.CENTERS]
    (root / "evidence" / "apparent-pitch-contours.json").write_text(json.dumps(out, indent=2))
    print(json.dumps(out, indent=2))
    return out


if __name__ == "__main__":
    main()
