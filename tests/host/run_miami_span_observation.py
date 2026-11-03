#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare unchanged PR #830 and timer-free temporary backend observations."""
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
PR830 = "e1c6b02bf6660142af3ae575a3b159cd051a011a"
BACKEND = "firmware/src/dsp/original_miami_backend.c"
KERNEL = "experiments/original_synth/miami_ms20.c"
PINS = {BACKEND: "2efde1794c8454b7af0f9c527f49abd03a28a783",
        KERNEL: "f74384b3f5e8850339fe4d42309b36fc10dd6b3b"}
REUSED = ["tests/host/miami_zero_xmod_assessment.c",
          "tests/host/run_miami_zero_xmod_assessment.py",
          "docs/perf/miami-zero-xmod-20261103/results.json"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "build/miami-span-observation")
    args = parser.parse_args()
    out = args.out.resolve()
    # Refuse source/output collisions and stale receipts from another execution.
    if out == ROOT or ROOT in out.parents and not out.is_relative_to(ROOT / "build"):
        parser.error("output must be outside the checkout or under build/")
    out.mkdir(parents=True, exist_ok=False)
    commands = []

    def execute(command, expected=0):
        command = list(map(str, command))
        commands.append(shlex.join(command))
        proc = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        (out / "commands.txt").write_text("\n".join(commands) + "\n")
        if proc.returncode != expected:
            raise RuntimeError(f"exit {proc.returncode}: {shlex.join(command)}\n{proc.stdout}{proc.stderr}")
        if expected:
            return proc
        return proc.stdout

    assert execute(["git", "rev-parse", PIN + "^{tree}"]).strip() == TREE
    for path, blob in PINS.items():
        assert execute(["git", "rev-parse", f"{PIN}:{path}"]).strip() == blob
        assert execute(["git", "hash-object", path]).strip() == blob
    assert not execute(["git", "diff", PIN, "--", "experiments", "firmware", "tools/build.py"])
    for path in REUSED:
        assert execute(["git", "hash-object", path]).strip() == execute(
            ["git", "rev-parse", f"{PR830}:{path}"]).strip(), path
    prior = json.loads((ROOT / REUSED[2]).read_text())
    reused_hashes = 0
    for path, digest in prior["input_sha256"].items():
        if Path(path).is_absolute():
            continue  # The three prior generated /tmp inputs are not present.
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest, path
        reused_hashes += 1

    spec = importlib.util.spec_from_file_location("recipe", ROOT / "tools/build.py")
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    cc = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-fast-math",
             "-ffp-contract=off", "-DORIGINAL_USE_SOFT_FMA", "-DMIAMI_FX_NO_HOST_FENV",
             "-Iexperiments/original_synth", "-Iexperiments/wobble", "-Ifirmware/src",
             "-Ifirmware/src/dsp", "-Itests/host"]
    objects = []

    def compile_unit(path, name, extra=()):
        obj = out / (name + ".o")
        execute(cc + flags + list(extra) + ["-MMD", "-MF", obj.with_suffix(".d"),
                                          "-c", path, "-o", obj])
        objects.append(obj)

    units = list(recipe.ORIGINAL_SYNTH_UNITS) + [BACKEND, "tests/host/miami_span_observation.c"]
    for i, unit in enumerate(units):
        extra = ["-Doriginal_miami_voice_render_add=fixture_dispatch"] if unit == BACKEND else []
        compile_unit(unit, f"unit-{i}", extra)
    # The unchanged comparison fixture still calls two separately compiled kernels.
    kernel = (ROOT / KERNEL).read_text()
    exports = re.findall(r"^int (original_\w+)\(", kernel, re.M)
    compile_unit(KERNEL, "fixture-kernel", [f"-D{name}=fixture_{name.removeprefix('original_')}"
                                          for name in exports])
    backend = (ROOT / BACKEND).read_text()
    exports = re.findall(r"^int (original_miami_backend_\w+)\(", backend, re.M)
    # Match the program pointer accessor too, whose '*' touches its name.
    exports += re.findall(r"^const original_miami_program \*(original_\w+)\(", backend, re.M)
    renames = [f"-D{name}=observed_{name.removeprefix('original_')}" for name in exports]
    span_marker = "    static float accL[64], accR[64];"
    voice_marker = "        if (!v->active) continue;\n        if (!completed(s, original_miami_voice_render_add("
    assert backend.count(span_marker) == backend.count(voice_marker) == 1
    observed_backend = ("#include <stdint.h>\n"
        "extern void span_observe_begin(uint32_t);\n"
        "extern void span_observe_voice(unsigned,uint32_t);\n" + backend.replace(
            span_marker, span_marker + "\n    span_observe_begin(n);").replace(
            voice_marker, "        if (!v->active) continue;\n        span_observe_voice(id, n);\n"
                          "        if (!completed(s, original_miami_voice_render_add("))
    generated = out / "observed-backend.c"
    generated.write_text(observed_backend)
    compile_unit(generated, "observed-backend",
                 renames + ["-Doriginal_miami_voice_render_add=fixture_dispatch"])
    binary = out / "span-observation"
    execute(cc + flags + objects + ["-lm", "-o", binary])
    results = {"source_commit": PIN, "source_tree": TREE, "source_blobs": PINS,
               "reused_fixture_commit": PR830, "pr830_input_hashes_verified": reused_hashes,
               "prior_generated_inputs_not_recovered": 3,
               "compiler": execute(cc + ["--version"]).splitlines()[0],
               "timing": "NOT_RUN_NO_CLOCKS_NO_SLOT_RESERVED", "programs": {}}
    for factory in (0, 15):
        baseline, observed_pcm = (out / f"{mode}-p{factory}.pcm" for mode in ("baseline", "observed"))
        stdout = execute([binary, factory, baseline, observed_pcm])
        (out / f"p{factory}.stdout.jsonl").write_text(stdout)
        records = [json.loads(line) for line in stdout.splitlines()]
        assert len(records) == 3 and records[0] == records[1]
        data = baseline.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        assert data == observed_pcm.read_bytes() and len(data) == 262144
        assert digest == prior["profiles"]["baseline"][str(factory)]["pcm_sha256"]
        results["programs"][str(factory)] = {"exit": 0, "fixture": records[0],
            "observations": records[2], "pcm_bytes": len(data), "pcm_sha256": digest,
            "pcm_matches_unchanged_fixture_and_pr830": True}
    # A false span-length observation must be rejected even with unchanged DSP.
    good_objects = objects[:-1]
    bad_source = out / "bad-observation-backend.c"
    marker = "span_observe_voice(id, n);"
    assert observed_backend.count(marker) == 1
    bad_source.write_text(observed_backend.replace(marker, "span_observe_voice(id, n + 1);"))
    compile_unit(bad_source, "bad-observation-backend",
                 renames + ["-Doriginal_miami_voice_render_add=fixture_dispatch"])
    bad_binary = out / "bad-observation"
    execute(cc + flags + good_objects + [objects[-1], "-lm", "-o", bad_binary])
    negative = execute([bad_binary, 0, out / "negative-baseline.pcm",
                        out / "negative-observed.pcm"], expected=31)
    assert negative.stderr.strip() == (
        "OBSERVATION_MISMATCH observation agrees with actual kernel call")
    results["negative_observation_test"] = {"exit": negative.returncode,
        "expected_failure": negative.stderr.strip(), "change": "reported n becomes n+1",
        "production_kernel_changed": False}
    (out / "negative.stdout").write_text(negative.stdout)
    (out / "negative.stderr").write_text(negative.stderr)
    dependencies = set()
    for depfile in out.glob("*.d"):
        dependencies.update(shlex.split(depfile.read_text().replace("\\\n", " ").split(":", 1)[1]))
    dependencies.update([str(Path(__file__)), "tools/build.py", *REUSED])
    results["input_sha256"] = {}
    for name in sorted(dependencies):
        path = Path(name)
        path = path if path.is_absolute() else ROOT / path
        label = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        results["input_sha256"][label] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "commands.txt").write_text("\n".join(commands) + "\n")
    (out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
