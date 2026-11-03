#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pinned kernel fixture, negative mutant and operation counts; no timing."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PIN = "8be16486e33f65b31c76710e1d825b6aa701c2c9"
TREE = "f952ab4455723e4a1a68fdb3a8eea22d2da59710"
BLOB = "f74384b3f5e8850339fe4d42309b36fc10dd6b3b"
KERNEL = "experiments/original_synth/miami_ms20.c"


def run(command, **kwargs):
    proc = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, **kwargs)
    if proc.returncode:
        raise SystemExit(f"Command failed ({proc.returncode}): {shlex.join(map(str, command))}\n"
                         + proc.stdout + proc.stderr)
    return proc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "build/miami-zero-xmod-assessment")
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / KERNEL).read_text()
    assert run(["git", "rev-parse", PIN + "^{tree}"]).stdout.strip() == TREE
    assert run(["git", "hash-object", KERNEL]).stdout.strip() == BLOB
    # Require the entire existing tracked source closure, not just kernel identity.
    changed = run(["git", "diff", PIN, "--", "experiments", "firmware", "tools/build.py"]).stdout
    assert not changed, "consumed production source differs from pin"
    spec = importlib.util.spec_from_file_location("build_recipe", ROOT / "tools/build.py")
    build = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(build)
    units = [x for x in build.ORIGINAL_SYNTH_UNITS if x != KERNEL]
    units.append("firmware/src/dsp/original_miami_backend.c")
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-fast-math",
             "-ffp-contract=off", "-DORIGINAL_USE_SOFT_FMA", "-DMIAMI_FX_NO_HOST_FENV",
             "-Iexperiments/original_synth", "-Iexperiments/wobble", "-Ifirmware/src",
             "-Ifirmware/src/dsp"]
    commands = []

    def execute(command):
        commands.append(shlex.join(map(str, command)))
        return run(list(map(str, command)))

    objects = []
    for i, unit in enumerate(units + [KERNEL, "tests/host/miami_zero_xmod_assessment.c"]):
        obj = out / f"unit-{i}.o"
        extra = ["-Doriginal_miami_voice_render_add=fixture_dispatch"] if unit.endswith("original_miami_backend.c") else []
        execute(cc + flags + extra + ["-MMD", "-MF", obj.with_suffix(".d"), "-c", unit, "-o", obj])
        objects.append(obj)
    externals = re.findall(r"^int (original_\w+)\(", source, flags=re.M)
    assert "original_miami_voice_render_add" in externals
    renames = [f"-D{name}=fixture_{name.removeprefix('original_')}" for name in externals]
    mutant_old = "phase[i+1]-=(uint32_t)step_i;"
    count_old = "for(unsigned i=0;i<4;i+=2){"
    assert source.count(mutant_old) == source.count(count_old) == 1
    variants = {
        "baseline": source,
        "mutant": source.replace(mutant_old, "phase[i+1]+=(uint32_t)step_i;"),
        "counted": "extern void assessment_count(float,float,float);\n" + source.replace(
            count_old, count_old + "\nassessment_count(gain[i],gain[i+1],xmod);"),
    }
    results = {"source_commit": PIN, "source_tree": TREE, "kernel_blob": BLOB,
               "compiler": execute(cc + ["--version"]).stdout.splitlines()[0],
               "profiles": {}, "phase_arithmetic": {
                   "status": "REUSED_AS_SUPPLIED_NOT_RERUN", "reported_cases": 996430,
                   "original_receipt_authenticated_here": False}}
    for mode, text in variants.items():
        generated = out / f"{mode}.c"
        generated.write_text(text)
        variant_obj = out / f"{mode}.o"
        execute(cc + flags + renames + ["-MMD", "-MF", variant_obj.with_suffix(".d"),
                                      "-c", generated, "-o", variant_obj])
        binary = out / mode
        execute(cc + flags + [*objects, variant_obj, "-lm", "-o", binary])
        results["profiles"][mode] = {}
        for factory in (0, 15):
            pcm = out / f"{mode}-p{factory}.pcm"
            command = list(map(str, [binary, factory, pcm]))
            commands.append(shlex.join(command))
            proc = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            record = {"exit": proc.returncode}
            if mode == "mutant":
                assert proc.returncode == 23 and "PHASE_MISMATCH" in proc.stderr, proc
                record["expected_failure"] = proc.stderr.strip()
            else:
                assert proc.returncode == 0, proc.stderr
                record.update(json.loads(proc.stdout))
                data = pcm.read_bytes()
                assert len(data) == 32768 * 8
                record["pcm_bytes"] = len(data)
                record["pcm_sha256"] = hashlib.sha256(data).hexdigest()
                if mode == "counted":
                    assert record["pair_samples"] > 0
                    assert data == (out / f"baseline-p{factory}.pcm").read_bytes(), "PCM changed"
                    record["pcm_matches_baseline"] = True
            results["profiles"][mode][str(factory)] = record
    dependencies = set()
    for depfile in out.glob("*.d"):
        dependencies.update(shlex.split(depfile.read_text().replace("\\\n", " ").split(":", 1)[1]))
    results["input_sha256"] = {}
    for name in sorted(dependencies):
        path = Path(name)
        if not path.is_absolute():
            path = ROOT / path
        label = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        results["input_sha256"][label] = hashlib.sha256(path.read_bytes()).hexdigest()
    results["input_sha256"]["tools/build.py"] = hashlib.sha256((ROOT / "tools/build.py").read_bytes()).hexdigest()
    results["input_sha256"][str(Path(__file__).relative_to(ROOT))] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    (out / "commands.txt").write_text("\n".join(commands) + "\n")
    (out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
