# SPDX-License-Identifier: GPL-3.0-only
"""Observable comparison and experimental host-only cutoff lookup; no DSP mutation.

Fit only relative harmonics on note48, test the selected pot on held-out note60.
A lookup entry is the best finite grid candidate, never a claim of equivalence.
Private captures load only from main(). Importing this module does not read them.
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


def load(path):
    x = np.fromfile(path, dtype="<f4").astype(float)
    if ".au." in path.name:
        x = x.reshape(-1, 2).mean(axis=1)
    assert np.isfinite(x).all()
    return x


def phase_pair(x, y):
    # Preserve phase separate from gain, using a single integer lag only.
    x = x[4410:17640]
    y = y[4410:17640]
    best = None
    for lag in range(-338, 339):
        a = x[max(0, lag):min(len(x), len(x) + lag)]
        b = y[max(0, -lag):min(len(y), len(y) - lag)]
        c = float(np.dot(a, b) / np.sqrt(np.dot(a, a) * np.dot(b, b)))
        if best is None or c > best["correlation"]:
            best = dict(lag=lag, correlation=c, gain=float(np.dot(a, b) / np.dot(a, a)))
    return best


def _workspace(root):
    return pathlib.Path(root) if root is not None else pathlib.Path(__file__).resolve().parents[3]


def main(root=None):
    root = _workspace(root)
    sr = measure.SR
    manifest = json.loads((root / "evidence" / "manifest.json").read_text())
    metrics = {}
    waves = {}
    for t in manifest:
        for engine in ("au", "dsp"):
            key = f"{t['id']}.{engine}"
            path = root / "audio" / f"{key}.f32"
            x = load(path)
            waves[key] = x
            if t["sequence"] < 2:
                metrics[key] = measure.features(x, t["note"])
            else:
                metrics[key] = measure.features(x, t["note"] + 7, .45, .7)
                metrics[key]["window"] = "posttransition .45-.7s, note55 target; stationarity unproven"
    for path in (root / "audio").glob("*grid*.f32"):
        note = int(path.name.split("-")[1][1:])
        metrics[path.name[:-4]] = measure.features(load(path), note)
    repeats = {}
    for t in manifest:
        if t["id"].endswith("-r0"):
            repeats[t["name"]] = {
                e: float(np.max(abs(waves[f"{t['name']}-r0.{e}"] - waves[f"{t['name']}-r1.{e}"])))
                for e in ("au", "dsp")
            }
    cal = [measure.grid_calibration_row(metrics, cut) for cut in (.2, .4, .6, .8, 1.)]
    expanded = []
    for cut in (.2, .4, .6, .8, 1.):
        for env in (0, 32, 64, 96, 127):
            for pot in list(range(0, 128, 16)) + [127]:
                k = f"expanded-n48-e{env}-c{pot}.dsp"
                if k not in metrics:
                    metrics[k] = measure.features(load(root / "audio" / f"{k}.f32"), 48)

        def resolve_holdout(hk):
            return measure.features(load(root / "audio" / f"{hk}.f32"), 60)

        expanded.append(measure.expanded_calibration_row(metrics, cut, resolve_holdout))
    # History: compare first vs second in each engine, retain level and spectra.
    history = {}
    for name in ("repeat", "accent_repeat"):
        history[name] = {}
        for e in ("au", "dsp"):
            x = waves[f"{name}-r0.{e}"]
            a = measure.features(x[:44100])
            b = measure.features(x[44100:])
            history[name][e] = dict(
                gain_db=float(measure.db(b["rms"] / a["rms"])),
                harmonic_distance_db=measure.distance(a, b),
                pitch_change_cents=float(b["cents"] - a["cents"]),
            )
    phase = {}
    for e in ("au", "dsp"):
        x = waves[f"repeat-r0.{e}"]
        phase[e] = phase_pair(x[:44100], x[44100:])
    measure.finalize_metric_rows(cal + expanded)
    result = dict(
        sample_rate=sr,
        metrics=metrics,
        fresh_repeat_max_error=repeats,
        calibration=cal,
        history=history,
        history_phase=phase,
        expanded_calibration=expanded,
    )
    (root / "evidence" / "measurements.json").write_text(json.dumps(result, indent=2))
    (root / "evidence" / "experimental-cutoff-lookup.json").write_text(json.dumps(dict(
        name="EXPERIMENTAL_HOST_ONLY_CHICAGO_SPECTRAL_LOOKUP",
        scope="note48 training, note60 held out; velocity100; Chicago waveform0 Gate; production constant filter-envelope diagnostic; reso0; 44.1kHz",
        acceptance="Exploratory post-analysis screen: >=3 reference partials above -40dB; <=3dB spectral RMSE on train and holdout; <=1dB holdout gain error. Not a fidelity target.",
        entries=expanded,
    ), indent=2))
    summary = dict(repeat_max=max(v for x in repeats.values() for v in x.values()), calibration=cal, expanded=expanded, history=history, phase=phase)
    print(json.dumps(summary, indent=2))
    return summary


if __name__ == "__main__":
    main()
