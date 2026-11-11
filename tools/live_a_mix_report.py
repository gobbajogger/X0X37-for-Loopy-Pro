#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""LIVE-A L12 measurement only. Run on the authorized build box, never on a device.

Prepare the compiler adapter, then use the repository's unchanged host/build_host.sh:
  python3 tools/live_a_mix_report.py --prepare-host --source-root . --outdir build/live-a
  CC="$PWD/build/live-a/cc-live-a" X0X_HOST_OUT=build/live-a/x0x_host sh host/build_host.sh
  python3 tools/live_a_mix_report.py --source-root . --host build/live-a/x0x_host \
      --outdir build/live-a/before --label before
  python3 tools/live_a_mix_report.py --before before/report.json \
      --after after/report.json --outdir comparison

The adapter includes the original host TU unchanged. Only its fgets call is wrapped:
after normal boot it submits a bank request through the production API, optionally
copies factory A's musical line to B, and records actual admission at WAV start/end.
It adds no firmware command, level, trim, sound parameter or DSP implementation.
"""
import argparse
import csv
import hashlib
import importlib.util
import json
import os
import shlex
import subprocess
import wave
from pathlib import Path

LIMITATIONS = (
    "Unchanged tools/mix_report.py analysis: BS.1770 K-weighted, left channel only, "
    "ungated whole-loop LU; not gated integrated stereo LUFS. Peak is left-channel "
    "sample peak, not true peak. Raw solos disable the master compressor and PUMP; "
    "FX, part levels, inserts and output limiting remain. MIX is mix_report's "
    "programmed factory-sound groove, not a claim about every factory pattern. "
    "Bank rows use preset 0 and the same factory A line in either slot. No level "
    "changes, listening approval, native builds, RAM or device validation are implied."
)
ENGINE_IDS = {0: 0x4F434831, 1: 0x4F4D4931}
SOURCE_INPUT_DIRS = ("firmware", "experiments", "host", "tools", "build/gen")


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def object_digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def jobs():
    parts = ["909", "808", "303A", "303B", "BREAK"]
    result = [{"name": "MIX", "kwargs": {}},
              {"name": "MIX + 808, 303B", "kwargs": {"play": parts}},
              {"name": "MIX, raw", "kwargs": {"raw": True}}]
    result += [{"name": part, "kwargs": {"play": [part], "raw": True}} for part in parts]
    result += [{"name": "909 " + voice, "kwargs": {"play": ["909"], "voices": [voice], "raw": True}}
               for voice in ("BD", "CP", "OH")]
    for slot, part in enumerate(("303A", "303B")):
        for bank, name in enumerate(("Chicago", "Miami")):
            for mode in ("solo", "mix"):
                result.append({"name": f"{part} {name} {mode}", "slot": slot, "bank": bank,
                               "preset": 0, "mode": mode,
                               "kwargs": {"play": [part] if mode == "solo" else ["909", part, "BREAK"],
                                          "raw": mode == "solo"}})
    return result


def make_script(mix, job, bars):
    script = mix.script("fixture.wav", bars=bars, **job["kwargs"])
    script = script.replace("wav fixture.wav\n", "expect playing 1\nwav fixture.wav\n")
    note = "# matched factory A line in both slots\n" if "slot" in job else ""
    assertions = "load 0\nexpect tempo 125\n" + "".join(f"expect ppat{part} 1\n" for part in range(5))
    return assertions + note + script


def analyse_wav(mix, path):
    with wave.open(str(path), "rb") as wav:
        if (wav.getframerate(), wav.getsampwidth(), wav.getnchannels(), wav.getcomptype()) != (44100, 2, 2, "NONE"):
            raise ValueError("analysis requires 44.1 kHz, 16-bit, stereo PCM WAV")
        if not wav.getnframes():
            raise ValueError("empty WAV")
    metrics = mix.analyse(path)
    if metrics is None:
        raise ValueError("silent render: no numeric loudness evidence")
    return metrics


def verify_runtime(log, job):
    states = [json.loads(line[len("LIVE_A_STATE "):]) for line in log.splitlines()
              if line.startswith("LIVE_A_STATE ")]
    if len(states) != 4 or {(s["phase"], s["slot"]) for s in states} != {
            (phase, slot) for phase in ("start", "end") for slot in (0, 1)}:
        raise ValueError("missing or duplicate runtime bank/preset evidence")
    for state in states:
        bank = state["bank"]
        if (bank not in ENGINE_IDS or state["engine_id"] != ENGINE_IDS[bank]
                or state["sound_bank"] != bank or state["requested_bank"] != bank):
            raise ValueError("runtime bank identity mismatch")
        if (state["status"] != 1 or state["pending"] != 0 or state["edited"] != 0
                or state["preset"] != 0 or state["requested_preset"] != 0
                or state["source_preset"] < 0):
            raise ValueError("runtime preset/status mismatch")
        if "slot" in job and state["slot"] == job["slot"] and bank != job["bank"]:
            raise ValueError("requested bank was not admitted")
        if "slot" in job and state["matched"] != 1:
            raise ValueError("runtime musical fixture mismatch")
    for slot in (0, 1):
        pair = [s for s in states if s["slot"] == slot]
        if {k: v for k, v in pair[0].items() if k != "phase"} != {
                k: v for k, v in pair[1].items() if k != "phase"}:
            raise ValueError("runtime state changed during capture")
    if "slot" in job and len({s["line_hash"] for s in states}) != 1:
        raise ValueError("runtime musical fixture differs between slots")
    return states


def source_snapshot(root):
    files = {}
    for directory in SOURCE_INPUT_DIRS:
        for path in sorted((root / directory).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                files[path.relative_to(root).as_posix()] = digest(path)
    head = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(root), "status", "--porcelain"], text=True)
    return {"head": head, "dirty": dirty, "inputs": files, "inputs_sha256": object_digest(files)}


# No original host command selects AND observes admitted banks. This adapter
# reuses the complete existing interpreter; it does not add guessed commands.
ADAPTER = r'''
/* SPDX-License-Identifier: GPL-3.0-only */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
static char *live_a_fgets(char *, int, FILE *);
#define fgets live_a_fgets
#define main live_a_original_main
#include @HOST@
#undef main
#undef fgets
#include @RUNTIME@
static int live_a_started, live_a_slot = -1, live_a_bank = -1;
static unsigned live_a_line_hash(int slot)
{
    const unsigned char *p = (const unsigned char *)&proj.pat[0].bass[slot];
    unsigned hash = 2166136261u;
    for (size_t i = 0; i < sizeof proj.pat[0].bass[slot]; ++i) hash = (hash ^ p[i]) * 16777619u;
    return hash;
}
static void live_a_state(const char *phase)
{
    int matched = !memcmp(&proj.pat[0].bass[0], &proj.pat[0].bass[1], sizeof proj.pat[0].bass[0]);
    printf("LIVE_A_INPUTS @INPUTS@\n");
    for (int v = 0; v < NBASS; ++v) {
        int bank = engine_bass_bank(v), preset = engine_bass_preset_index(v);
        if (bank < 0 || bank > 1 || preset != 0 || engine_bass_status(v) != ORIGINAL_RUNTIME_OK ||
            engine_bass_preset_pending(v) || engine_bass_preset_edited(v) ||
            engine_bass_sound_bank(&proj.sound, v) != bank ||
            engine_bass_requested_bank(v) != bank || engine_bass_requested_preset(v) != preset ||
            (v == live_a_slot && (bank != live_a_bank || !matched))) {
            fprintf(stderr, "LIVE-A: bank/preset not admitted in slot %d\n", v); exit(2);
        }
        printf("LIVE_A_STATE {\"phase\":\"%s\",\"slot\":%d,\"bank\":%d,\"preset\":%d,"
               "\"requested_bank\":%d,\"requested_preset\":%d,\"status\":%d,\"pending\":%d,"
               "\"edited\":%d,\"sound_bank\":%d,\"engine_id\":%u,\"source_preset\":%d,"
               "\"matched\":%d,\"line_hash\":\"%08x\"}\n", phase, v, bank, preset,
               engine_bass_requested_bank(v), engine_bass_requested_preset(v), engine_bass_status(v),
               engine_bass_preset_pending(v), engine_bass_preset_edited(v),
               engine_bass_sound_bank(&proj.sound, v), original_sound_engine_id((unsigned)bank),
               original_sound_factory_source((unsigned)bank, (unsigned)preset), matched, live_a_line_hash(v));
    }
}
static char *live_a_fgets(char *buffer, int size, FILE *stream)
{
    if (!live_a_started) {
        live_a_started = 1;
        if (live_a_slot >= 0) {
            proj.pat[0].bass[1] = proj.pat[0].bass[0];
            if (engine_bass_select_preset(live_a_slot, live_a_bank, 0, &proj.sound)) {
                fprintf(stderr, "LIVE-A: requested bank unavailable\n"); exit(2);
            }
            run_ms(300); /* stopped: allow production quiet/admission boundary */
        }
    }
    char *result = fgets(buffer, size, stream);
    if (result && !strncmp(buffer, "wav ", 4)) live_a_state("start");
    if (result && !strncmp(buffer, "wavstop", 7)) live_a_state("end");
    return result;
}
int main(int argc, char **argv)
{
    if (argc != 3 && argc != 5) return 2;
    if (argc == 5) {
        if ((strcmp(argv[3], "0") && strcmp(argv[3], "1")) ||
            (strcmp(argv[4], "0") && strcmp(argv[4], "1"))) return 2;
        live_a_slot = atoi(argv[3]); live_a_bank = atoi(argv[4]);
    }
    return live_a_original_main(3, argv);
}
'''


def prepare_host(root, out, compiler):
    out.mkdir(parents=True, exist_ok=True)
    shim = out / "cc-live-a"
    module = Path(__file__).resolve()
    shim.write_text("#!/usr/bin/env python3\nimport importlib.util, sys\n"
                    f"spec = importlib.util.spec_from_file_location('live_a_mix_report', {str(module)!r})\n"
                    "module = importlib.util.module_from_spec(spec)\nspec.loader.exec_module(module)\n"
                    f"raise SystemExit(module.compile_host({str(root)!r}, {str(out)!r}, {compiler!r}, sys.argv[1:]))\n")
    shim.chmod(0o755)
    print(f"CC={shlex.quote(str(shim))} X0X_HOST_OUT={shlex.quote(str(out / 'x0x_host'))} sh host/build_host.sh")


def compile_host(root, out, compiler, arguments):
    """Forward asset generators; adapt only the existing host compiler invocation."""
    root, out = Path(root), Path(out)
    host_count = arguments.count("host/x0x_host.c")
    if not host_count:
        # gen_builtin_break.sh inherits CC while preparing host assets. Keep its
        # arguments and result unchanged; no host snapshot or adapter exists yet.
        return subprocess.run([*shlex.split(compiler), *arguments], cwd=root, check=False).returncode
    if host_count != 1 or arguments.count("-o") != 1:
        raise ValueError("unexpected host/build_host.sh compiler invocation")
    output_index = arguments.index("-o") + 1
    if (output_index >= len(arguments) or not arguments[output_index]
            or arguments[output_index].startswith("-") or arguments[output_index] == "host/x0x_host.c"):
        raise ValueError("unexpected host/build_host.sh compiler invocation")
    snapshot = source_snapshot(root)
    adapter = out / "live_a_mix_host.c"
    adapter.write_text(ADAPTER.replace("@HOST@", json.dumps(str(root / "host/x0x_host.c")))
                       .replace("@RUNTIME@", json.dumps(str(root / "firmware/src/app/original_runtime.h")))
                       .replace("@INPUTS@", snapshot["inputs_sha256"]))
    args = [str(adapter) if arg == "host/x0x_host.c" else arg for arg in arguments]
    result = subprocess.run([*shlex.split(compiler), *args], cwd=root, check=False)
    if result.returncode:
        return result.returncode
    if source_snapshot(root) != snapshot:
        raise ValueError("source inputs changed during host compilation")
    host = Path(arguments[output_index])
    if not host.is_absolute():
        host = root / host
    provenance = {"source": snapshot, "host_sha256": digest(host), "compiler": compiler,
                  "arguments": args, "adapter_sha256": digest(adapter),
                  "driver_sha256": digest(Path(__file__))}
    host.with_suffix(".provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    return 0


def compare(before, after):
    if before["schema"] != 1 or after["schema"] != 1 or before["fixture"] != after["fixture"]:
        raise ValueError("incompatible measurement fixture")
    left = {row["name"]: row for row in before["rows"]}
    right = {row["name"]: row for row in after["rows"]}
    if (not left or left.keys() != right.keys() or len(left) != len(before["rows"])
            or len(right) != len(after["rows"])):
        raise ValueError("different or duplicate measurement rows")
    result = []
    for name, base in left.items():
        candidate = right[name]
        if base["fixture_hash"] != candidate["fixture_hash"]:
            raise ValueError(f"changed musical fixture: {name}")
        if base.get("runtime") != candidate.get("runtime"):
            raise ValueError(f"changed runtime bank/preset/musical fixture: {name}")
        a, b = base["metrics"], candidate["metrics"]
        result.append({"name": name, "before_lu": a["lu"], "after_lu": b["lu"],
                       "delta_lu": b["lu"] - a["lu"], "before_peak": a["peak"],
                       "after_peak": b["peak"], "delta_peak": b["peak"] - a["peak"]})
    return result


def write_tables(out, rows, labels):
    with (out / "table.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    keys = list(rows[0])
    lines = [labels, "", LIMITATIONS, "", "| " + " | ".join(keys) + " |",
             "| " + " | ".join("---" for _ in keys) + " |"]
    lines += ["| " + " | ".join(f"{row[key]:.3f}" if isinstance(row[key], (float, int))
                                else str(row[key]) for key in keys) + " |" for row in rows]
    (out / "table.md").write_text("\n".join(lines) + "\n")


def render(root, host, out, label, bars):
    if any(out.iterdir()):
        raise ValueError("measurement outdir must be empty; preserve earlier evidence separately")
    provenance = json.loads(host.with_suffix(".provenance.json").read_text())
    snapshot = source_snapshot(root)
    if (digest(host) != provenance["host_sha256"]
            or snapshot != provenance["source"] or digest(Path(__file__)) != provenance["driver_sha256"]):
        raise ValueError("host/source provenance mismatch; rebuild measurement host")
    mix_path = root / "tools/mix_report.py"
    spec = importlib.util.spec_from_file_location("mix_report", mix_path)
    mix = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mix)
    # Do not rewrite or override mix_report globals: explicit source root determines samples.
    samples = {str(path.relative_to(root)): digest(path) for path in mix.BREAKS if path.exists()}
    if len(samples) == 1:
        raise ValueError("only one external BREAK sample present; use both or neither")
    fixture = {"bars": bars, "samples": samples, "mix_report_sha256": digest(mix_path),
               "driver_sha256": digest(Path(__file__)),
               "generated_samples": {name: digest(root / "build/gen" / name) for name in
                                     ("x0x_drum_samples.h", "x0x_break_bank.h")},
               "break_source": "external THINK/FUNKY" if samples else "built-in fallback",
               "analysis": "BS.1770 left-channel ungated; sample peak; 44100 Hz"}
    result = {"schema": 1, "label": label, "fixture": fixture, "provenance": provenance,
              "limitations": LIMITATIONS, "rows": []}
    for index, job in enumerate(jobs()):
        directory = out / f"{index:02d}"
        directory.mkdir(parents=True, exist_ok=False)
        script = make_script(mix, job, bars)
        path = directory / "fixture.x0x"
        path.write_text(script)
        command = [str(host), str(path), str(directory)]
        if "slot" in job:
            command += [str(job["slot"]), str(job["bank"])]
        proc = subprocess.run(command, cwd=root, capture_output=True, text=True,
                              env={**os.environ, "X0X_HOST_LOAD": "0"})
        (directory / "host.log").write_text(proc.stdout + proc.stderr)
        if proc.returncode:
            raise ValueError(f"host failed for {job['name']}; see {directory / 'host.log'}")
        fingerprints = [line.split()[1] for line in proc.stdout.splitlines() if line.startswith("LIVE_A_INPUTS ")]
        if fingerprints != [snapshot["inputs_sha256"]] * 2:
            raise ValueError("host was not built with the expected measurement adapter/source")
        states = verify_runtime(proc.stdout, job)
        wav = directory / "fixture.wav"
        metrics = analyse_wav(mix, wav)
        normalized_script = script.replace(str(root), "$SOURCE_ROOT")
        result["rows"].append({"name": job["name"], "fixture_hash": object_digest({"script": normalized_script, "job": job}),
                               "script_sha256": digest(path), "wav_sha256": digest(wav),
                               "runtime": states, "metrics": metrics})
    # Reject inputs changed while the run was in progress; partial files remain inspectable.
    if source_snapshot(root) != snapshot or any(digest(root / path) != sha for path, sha in samples.items()):
        raise ValueError("measurement inputs changed during capture")
    (out / "report.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    write_tables(out, [{"name": row["name"], "lu": row["metrics"]["lu"],
                        "peak": row["metrics"]["peak"]} for row in result["rows"]], label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--host", type=Path)
    parser.add_argument("--outdir", type=Path, required=True)
    parser.add_argument("--label", default="measurement")
    parser.add_argument("--bars", type=int, default=8)
    parser.add_argument("--prepare-host", action="store_true")
    parser.add_argument("--compiler", default="cc", help="real C compiler for the box adapter")
    parser.add_argument("--before", type=Path)
    parser.add_argument("--after", type=Path)
    args = parser.parse_args()
    if not 1 <= args.bars <= 128:
        parser.error("--bars must be 1..128")
    root, out = args.source_root.resolve(), args.outdir.resolve()
    for directory in SOURCE_INPUT_DIRS:
        if out == root / directory or root / directory in out.parents:
            parser.error("--outdir must be outside source input directories (use build/live-a)")
    out.mkdir(parents=True, exist_ok=True)
    if args.before or args.after:
        if not (args.before and args.after):
            parser.error("--before and --after are required together")
        before, after = (json.loads(path.read_text()) for path in (args.before, args.after))
        rows = compare(before, after)
        (out / "comparison.json").write_text(json.dumps({"before": before, "after": after, "rows": rows},
                                                       indent=2, allow_nan=False) + "\n")
        write_tables(out, rows, f"{before['label']} -> {after['label']}; deltas = after - before")
    elif args.prepare_host:
        prepare_host(root, out, args.compiler)
    elif args.host:
        render(root, args.host.resolve(), out, args.label, args.bars)
    else:
        parser.error("choose --prepare-host, --host, or --before with --after")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(f"LIVE-A measurement failed: {error}")
