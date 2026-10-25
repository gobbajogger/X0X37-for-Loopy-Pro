# SPDX-License-Identifier: GPL-3.0-only
"""Host velocity→cutoff / timbre measurement for the Chicago packet.

Reason: owner observation that 303 velocity changes level but does not map to
cutoff the way Chicago does. The prior joint host gain model at a33c2de is
REJECTED and must not be imported.
Purpose: measure brightness (harmonic tilt) vs RMS level on the same preset and
note, using approved derived Chicago AU metrics plus fresh host bass303 renders.
Effect: firmware defaults and the default renderer stay unchanged. This reports
whether the Gate reference supports a filter-velocity fit; it does not retune
production DSP. Native/AU success is not claimed here.
"""
from __future__ import annotations

import hashlib
import json
import math
import pathlib
import subprocess
import tempfile

import numpy as np

DIR = pathlib.Path(__file__).resolve().parent
REPO = DIR.parents[1]
REFERENCE = DIR / "data" / "approved-amplitude-reference.json"
BASS303_SHA256 = "321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f"
# Train: note 48 at Chicago cutoff .6, velocities 40 and 100.
# Held-out: note 48 velocities 80/127; note 60 velocities 40/80/100/127.
# Frozen before scoring. No held-out selection.
TRAIN_IDS = ("cut0.6-n48-r0", "vel40-r0")
HELDOUT_N48 = ("vel80-r0", "vel127-r0")
HELDOUT_N60_ANCHOR = "cut0.6-n60-r0"
HELDOUT_N60_VEL = (
    "velocity40-n60-r0",
    "velocity80-n60-r0",
    "velocity127-n60-r0",
)
# Reject filter-velocity equivalence if Chicago AU brightness transfer stays
# inside this band across the train velocity step (Gate packet is amplitude-led).
AU_FLAT_BRIGHTNESS_DB = 0.05
# Source must move brightness with velocity at least this much on the train step
# to count as an active velocity→cutoff path (separate from RMS).
SOURCE_MIN_BRIGHTNESS_STEP_DB = 0.5


def load_measure():
    import importlib.util

    path = DIR / "measure.py"
    name = "audio303_measure_vcut"
    if name in sys_modules():
        return sys_modules()[name]
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    sys_modules()[name] = mod
    spec.loader.exec_module(mod)
    return mod


def sys_modules():
    import sys

    return sys.modules


def brightness_db(relative_harmonics_db):
    """Mean of H2..H6 relative dB. Less negative is brighter."""
    rel = np.asarray(relative_harmonics_db, dtype=float)
    if rel.size < 6 or not np.isfinite(rel[1:6]).all():
        return None
    return float(np.mean(rel[1:6]))


def rms_db(rms):
    return 20.0 * math.log10(max(float(rms), 1e-12))


def velocity_cutoff_law(velocity):
    """Mirror firmware: 2**((velocity-100)*(1.5/100))."""
    v = max(1, min(127, int(velocity)))
    return float(2 ** ((v - 100) * (1.5 / 100.0)))


def velocity_gain_law(velocity):
    v = max(1, min(127, int(velocity)))
    return float(v) / 100.0


def index_rows(doc):
    by_id = {row["id"]: row for row in doc["rows"]}
    for row in doc.get("fresh_heldout", {}).get("rows", []):
        by_id[row["id"]] = row
    return by_id


def compile_renderer(dest, repo=REPO):
    dest = pathlib.Path(dest)
    cmd = [
        "cc",
        "-std=c99",
        "-O2",
        "-ffp-contract=off",
        f"-I{repo / 'firmware' / 'src'}",
        f"-I{repo / 'firmware' / 'src' / 'dsp'}",
        str(DIR / "render.c"),
        str(repo / "firmware" / "src" / "dsp" / "bass303.c"),
        "-lm",
        "-o",
        str(dest),
    ]
    done = subprocess.run(cmd, capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(done.stderr or done.stdout or f"compile failed {done.returncode}")
    return dest


def render_row(renderer, dest, row):
    prod = row["production"]
    args = [
        str(renderer),
        str(dest),
        str(row["frames"]),
        str(row["note"]),
        str(row["velocity"]),
        *[str(int(x)) for x in prod],
        str(row["sequence"]),
        str(row["accent"]),
        "0",
        "0",
    ]
    done = subprocess.run(args, capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(f"render {row['id']} exit {done.returncode}: {done.stderr}")
    return np.fromfile(dest, dtype="<f4")


def bass303_sha256(repo=REPO):
    path = repo / "firmware" / "src" / "dsp" / "bass303.c"
    return hashlib.sha256(path.read_bytes()).hexdigest()


def row_side_metrics(row, side):
    m = row["measurements"][side]
    return {
        "rms_db": rms_db(m["rms"]),
        "brightness_db": brightness_db(m["relative_harmonics_db"]),
        "relative_harmonics_db": list(m["relative_harmonics_db"]),
    }


def transfer(metrics_by_id, ids, anchor_id, key):
    anchor = metrics_by_id[anchor_id][key]
    out = {}
    for item in ids:
        value = metrics_by_id[item][key]
        out[item] = None if anchor is None or value is None else float(value - anchor)
    return out


def build_report(renderer=None, reference_path=REFERENCE, work_dir=None, repo=REPO):
    measure = load_measure()
    doc = json.loads(pathlib.Path(reference_path).read_text())
    rows = index_rows(doc)
    needed = list(TRAIN_IDS) + list(HELDOUT_N48) + [HELDOUT_N60_ANCHOR] + list(HELDOUT_N60_VEL)
    missing = [i for i in needed if i not in rows]
    if missing:
        raise KeyError(f"missing reference rows: {missing}")

    tmp_owned = work_dir is None
    work = pathlib.Path(work_dir) if work_dir else pathlib.Path(tempfile.mkdtemp(prefix="vcut-"))
    work.mkdir(parents=True, exist_ok=True)
    try:
        if renderer is None:
            renderer = compile_renderer(work / "production303", repo=repo)
        host = {}
        for rid in needed:
            audio = render_row(renderer, work / f"{rid}.f32", rows[rid])
            feat = measure.features(audio, note=rows[rid]["note"])
            host[rid] = {
                "rms_db": rms_db(feat["rms"]),
                "brightness_db": brightness_db(feat["relative_harmonics_db"]),
                "relative_harmonics_db": [float(x) for x in feat["relative_harmonics_db"]],
                "pitch_fit_at_search_boundary": bool(feat["pitch_fit_at_search_boundary"]),
            }

        au = {rid: row_side_metrics(rows[rid], "au") for rid in needed}
        stored_dsp = {rid: row_side_metrics(rows[rid], "dsp") for rid in needed}

        # Host must match the stored DSP features from the same packet (byte-render path).
        host_vs_stored = {}
        for rid in needed:
            h = np.asarray(host[rid]["relative_harmonics_db"], dtype=float)
            s = np.asarray(stored_dsp[rid]["relative_harmonics_db"], dtype=float)
            host_vs_stored[rid] = float(np.max(np.abs(h - s)))

        train_au_bright = transfer(au, ("vel40-r0",), "cut0.6-n48-r0", "brightness_db")
        train_host_bright = transfer(host, ("vel40-r0",), "cut0.6-n48-r0", "brightness_db")
        train_au_rms = transfer(au, ("vel40-r0",), "cut0.6-n48-r0", "rms_db")
        train_host_rms = transfer(host, ("vel40-r0",), "cut0.6-n48-r0", "rms_db")

        held_au_bright = transfer(
            au,
            list(HELDOUT_N48) + list(HELDOUT_N60_VEL),
            "cut0.6-n48-r0",
            "brightness_db",
        )
        # Note-60 brightness transfers use the note-60 velocity-100 anchor.
        held_au_bright_n60 = transfer(au, list(HELDOUT_N60_VEL), HELDOUT_N60_ANCHOR, "brightness_db")
        held_host_bright_n60 = transfer(host, list(HELDOUT_N60_VEL), HELDOUT_N60_ANCHOR, "brightness_db")
        held_host_bright_n48 = transfer(host, list(HELDOUT_N48), "cut0.6-n48-r0", "brightness_db")

        au_train_step = abs(train_au_bright["vel40-r0"] or 0.0)
        host_train_step = abs(train_host_bright["vel40-r0"] or 0.0)
        au_flat = au_train_step <= AU_FLAT_BRIGHTNESS_DB
        source_active = host_train_step >= SOURCE_MIN_BRIGHTNESS_STEP_DB
        host_matches_stored = max(host_vs_stored.values()) < 1e-9

        # Law readback (state), separate from audio.
        law = {
            str(v): {
                "velocity_gain": velocity_gain_law(v),
                "velocity_cutoff": velocity_cutoff_law(v),
            }
            for v in (40, 80, 100, 127)
        }

        if not host_matches_stored:
            status = "HOST_RENDER_MISMATCH"
            disposition = (
                "Fresh host renders disagree with stored DSP harmonics in the approved "
                "reference. Stop before any filter-velocity correction."
            )
        elif au_flat and source_active:
            status = "MEASUREMENT_COMPLETE_NO_GATE_FILTER_FIT"
            disposition = (
                "Chicago Gate AU brightness is flat across velocity while host bass303 "
                "already darkens/brightens. This Gate packet cannot calibrate a "
                "velocity→filter correction toward Chicago. Do not import the rejected "
                "joint gain model. Request non-Gate Chicago filter-velocity derived "
                "metrics before changing the firmware law."
            )
        elif not source_active:
            status = "SOURCE_FILTER_PATH_INACTIVE"
            disposition = (
                "Host brightness did not move enough with velocity on the train step. "
                "Investigate the velocity_cutoff path before fitting."
            )
        else:
            status = "MEASUREMENT_COMPLETE_AU_ALSO_MOVES"
            disposition = (
                "Both Chicago AU and host brightness move with velocity. A bounded "
                "depth fit may be considered after freezing a filter-only metric and "
                "margins; not applied in this stage."
            )

        report = {
            "schema_version": 1,
            "status": status,
            "disposition": disposition,
            "purpose": (
                "Separate velocity→timbre from velocity→level on the Gate diagnostic "
                "patch; compare approved Chicago AU derived harmonics to fresh host renders."
            ),
            "bass303_sha256_expected": BASS303_SHA256,
            "bass303_sha256_actual": bass303_sha256(repo),
            "rejected_joint_model": {
                "tip": "a33c2de208c227beab7203fbb2de9e244052eff6",
                "imported": False,
                "note": "Joint host gate/velocity gain model stays rejected and unused.",
            },
            "split": {
                "train": list(TRAIN_IDS),
                "heldout_n48": list(HELDOUT_N48),
                "heldout_n60_anchor": HELDOUT_N60_ANCHOR,
                "heldout_n60_velocities": list(HELDOUT_N60_VEL),
                "rule": "Frozen before scoring. No held-out selection. Gate AU metrics only.",
            },
            "thresholds": {
                "au_flat_brightness_db": AU_FLAT_BRIGHTNESS_DB,
                "source_min_brightness_step_db": SOURCE_MIN_BRIGHTNESS_STEP_DB,
            },
            "velocity_law_state": law,
            "train": {
                "au_brightness_transfer_db": train_au_bright,
                "host_brightness_transfer_db": train_host_bright,
                "au_rms_transfer_db": train_au_rms,
                "host_rms_transfer_db": train_host_rms,
                "au_brightness_step_abs_db": au_train_step,
                "host_brightness_step_abs_db": host_train_step,
                "au_gate_brightness_flat": au_flat,
                "host_velocity_cutoff_active": source_active,
            },
            "heldout": {
                "au_brightness_transfer_vs_n48_vel100_db": {
                    k: held_au_bright[k] for k in HELDOUT_N48
                },
                "au_brightness_transfer_vs_n60_vel100_db": held_au_bright_n60,
                "host_brightness_transfer_vs_n48_vel100_db": held_host_bright_n48,
                "host_brightness_transfer_vs_n60_vel100_db": held_host_bright_n60,
            },
            "host_vs_stored_dsp_max_abs_harmonic_db": host_vs_stored,
            "host_matches_stored_dsp": host_matches_stored,
            "rows": {
                rid: {
                    "note": rows[rid]["note"],
                    "velocity": rows[rid]["velocity"],
                    "production": list(rows[rid]["production"]),
                    "au": au[rid],
                    "host": host[rid],
                    "stored_dsp": stored_dsp[rid],
                }
                for rid in needed
            },
            "invariants": {
                "persistent_cutoff_pot": (
                    "Renderer sets BASS303_CUTOFF from production[0] only; velocity "
                    "multiplies fc via velocity_cutoff and must not write the pot."
                ),
                "volume_separate": "velocity_gain scales amp; velocity_cutoff scales fc.",
                "accent_separate": "Accent remains the accent flag / ENV path, not MIDI velocity alone.",
                "slide_continuity": (
                    "render.c sequence 2 keeps the gate and updates velocity_* on the "
                    "overlapping note; envelopes are not retriggered."
                ),
            },
            "limits": [
                "Gate AU trials only; not non-Gate Chicago filter-velocity proof.",
                "No licensed AU render in this cloud seat.",
                "No firmware coefficient change in this measurement stage.",
                "No flash, NIGHTLY merge, catalog publish, or Actions-as-proof.",
            ],
        }
        return report
    finally:
        if tmp_owned:
            # Leave temp dir for OS cleanup; avoid deleting mid-debug.
            pass


def write_results(report, md_path=None, json_path=None):
    md_path = pathlib.Path(md_path or DIR / "VELOCITY_CUTOFF_RESULTS.md")
    json_path = pathlib.Path(json_path or DIR / "data" / "velocity-cutoff-results.json")
    json_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")

    t = report["train"]
    lines = [
        "# Velocity→cutoff host measurement",
        "",
        f"Status: **{report['status']}**.",
        "",
        report["disposition"],
        "",
        "## Why this exists",
        "",
        "Owner: velocity in 303 works for level but should map lower velocity to lower",
        "cutoff and higher velocity to higher cutoff, separate from volume/accent and",
        "the persistent cutoff base. The rejected joint gain tip",
        "`a33c2de208c227beab7203fbb2de9e244052eff6` is not imported.",
        "",
        "## Commands",
        "",
        "```sh",
        "cc -std=c99 -O2 -ffp-contract=off -Ifirmware/src -Ifirmware/src/dsp \\",
        "  tools/audio303/render.c firmware/src/dsp/bass303.c -lm -o /tmp/production303",
        "python3 tools/audio303/velocity_cutoff.py",
        "python3 tools/audio303/test_velocity_cutoff.py",
        "```",
        "",
        f"bass303.c SHA-256 actual `{report['bass303_sha256_actual']}`",
        f"(expected `{report['bass303_sha256_expected']}`).",
        "",
        "## Train (note 48, cutoff pot 64 / Chicago .6 Gate patch)",
        "",
        "| Measure | Value |",
        "| --- | ---: |",
        f"| AU brightness transfer vel40−vel100 (dB) | {t['au_brightness_transfer_db']['vel40-r0']:.4f} |",
        f"| Host brightness transfer vel40−vel100 (dB) | {t['host_brightness_transfer_db']['vel40-r0']:.4f} |",
        f"| AU RMS transfer vel40−vel100 (dB) | {t['au_rms_transfer_db']['vel40-r0']:.4f} |",
        f"| Host RMS transfer vel40−vel100 (dB) | {t['host_rms_transfer_db']['vel40-r0']:.4f} |",
        f"| AU Gate brightness flat (≤{report['thresholds']['au_flat_brightness_db']} dB) | {t['au_gate_brightness_flat']} |",
        f"| Host velocity→cutoff active (≥{report['thresholds']['source_min_brightness_step_db']} dB) | {t['host_velocity_cutoff_active']} |",
        f"| Host matches stored DSP harmonics | {report['host_matches_stored_dsp']} |",
        "",
        "## Held-out brightness transfers (host)",
        "",
        "| Id | dB vs same-note vel100 |",
        "| --- | ---: |",
    ]
    for rid, val in report["heldout"]["host_brightness_transfer_vs_n48_vel100_db"].items():
        lines.append(f"| {rid} | {val:.4f} |")
    for rid, val in report["heldout"]["host_brightness_transfer_vs_n60_vel100_db"].items():
        lines.append(f"| {rid} | {val:.4f} |")
    lines += [
        "",
        "## Firmware law (state only)",
        "",
        " Neutral velocity 100: gain 1, cutoff multiplier 1.",
        " `velocity_cutoff = 2**((v-100)*1.5/100)`; `velocity_gain = v/100`.",
        " Pot cutoff is not written by note-on.",
        "",
        "| Velocity | gain | cutoff mult |",
        "| ---: | ---: | ---: |",
    ]
    for v, row in report["velocity_law_state"].items():
        lines.append(f"| {v} | {row['velocity_gain']:.6f} | {row['velocity_cutoff']:.6f} |")
    lines += [
        "",
        "## What this does not say",
        "",
        "- Not Chicago acoustic equivalence.",
        "- Not a native/AU Mac result.",
        "- Not authorization to change the 1.5 cutoff exponent without non-Gate metrics.",
        "- Not integration of the rejected joint amplitude model.",
        "",
        f"JSON: `{json_path.relative_to(REPO) if json_path.is_relative_to(REPO) else json_path}`.",
        "",
    ]
    md_path.write_text("\n".join(lines))
    return md_path, json_path


def main():
    report = build_report()
    md_path, json_path = write_results(report)
    print(json.dumps({"status": report["status"], "md": str(md_path), "json": str(json_path)}, indent=2))
    return 0 if report["status"].startswith("MEASUREMENT_COMPLETE") else 1


if __name__ == "__main__":
    raise SystemExit(main())
