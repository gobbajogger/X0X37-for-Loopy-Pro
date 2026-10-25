# SPDX-License-Identifier: GPL-3.0-only
"""Fit explicit host-only slide tau using apparent pitch trajectory.

Train note48->55 and evaluate note60->67 without retuning. No firmware changes.
Private reference files and the host renderer run only from main().
"""
import importlib.util
import json
import pathlib
import subprocess
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


def render(root, note, tau, out):
    args = [44100, note, 100, 64, 0, 0, 0, 1, 0, 64, 127, 0, 64, 2, 0, 0, 0]
    if tau is not None:
        args += ["--experimental-slide-tau-ms", tau]
    subprocess.run([str(root / "production303"), str(out), *map(str, args)], check=True)


def main(root=None):
    root = pathlib.Path(root) if root is not None else pathlib.Path(__file__).resolve().parents[3]
    ref = measure.contour_hz(read_f32(root / "audio" / "overlap_glide_mid-r0.au.f32", stereo=True), 48)
    candidates = []
    for tau in (80, 90, 100, 110, 120, 130, 140, 150, 160):
        out = root / "audio" / f"experimental-slide-tau{tau}-n48.f32"
        render(root, 48, tau, out)
        hz = measure.contour_hz(read_f32(out), 48)
        candidates.append(dict(tau_ms=tau, apparent_hz=hz, rmse_cents=measure.trajectory_rmse_cents(hz, ref)))
    best = min(candidates, key=lambda x: x["rmse_cents"])
    holdref = measure.contour_hz(read_f32(root / "audio" / "slide-holdout-n60-r0.au.f32", stereo=True), 60)
    heldout = root / "audio" / "experimental-slide-n60.f32"
    render(root, 60, best["tau_ms"], heldout)
    hold = measure.contour_hz(read_f32(heldout), 60)
    baseline = root / "audio" / "baseline-slide-n60.f32"
    render(root, 60, None, baseline)
    basehold = measure.contour_hz(read_f32(baseline), 60)
    base = measure.contour_hz(read_f32(root / "audio" / "overlap_glide_mid-r0.dsp.f32"), 48)
    result = dict(
        name="EXPERIMENTAL_HOST_ONLY_CHICAGO_SLIDE_TAU",
        chicago_glide=.5,
        source_slide_pot=64,
        rate=44100,
        centers_seconds=[.3, .4, .5, .6, .7],
        train_reference_hz=ref,
        candidates=candidates,
        selected=best,
        baseline_train_rmse_cents=measure.trajectory_rmse_cents(base, ref),
        holdout_reference_hz=holdref,
        holdout_experimental_hz=hold,
        holdout_baseline_hz=basehold,
        holdout_experimental_rmse_cents=measure.trajectory_rmse_cents(hold, holdref),
        holdout_baseline_rmse_cents=measure.trajectory_rmse_cents(basehold, holdref),
        limitations="Apparent frequency from 50ms local harmonic fits, not internal oscillator trajectory; one glide setting and upward interval, only two octaves; no amplitude/spectral/envelope match claimed",
    )
    (root / "evidence" / "experimental-slide-calibration.json").write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    main()
