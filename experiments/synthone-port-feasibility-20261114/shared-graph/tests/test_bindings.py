#!/usr/bin/env python3
"""Focused fail-closed input/dependency tests; no C++ suite or downloads."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent.parent
EXPECTED_PAYLOAD_SHA256 = "0d35b5422cb5d3e0e349a66c9e0a50f2980b6287e68f8d94f55640d3f8168bd5"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    require(len(sys.argv) == 2, "usage: python3 tests/test_bindings.py SYNTHONE_SOURCE_ROOT")
    spec = importlib.util.spec_from_file_location("checked_table_preparer", HERE / "prepare_tables.py")
    preparer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(preparer)
    # Check the fixed manifest before using any of its paths as copy destinations.
    preparer.verify_dependencies()
    source = Path(sys.argv[1]).resolve()
    dependency_data = json.loads((HERE / "DEPENDENCIES.json").read_text())
    manifest_data = json.loads((HERE / "../public-source-manifest.json").read_text())
    input_rows = [row for row in manifest_data if row["repository"] == "AudioKit/AudioKitSynthOne"
                  and row["commit"] == "6466a3715c96b7ecf1dd255a218cbd571408a314"
                  and "BandlimitedWavetables/" in row["path"]]
    with tempfile.TemporaryDirectory(prefix="synthone-bindings-") as temporary:
        root = Path(temporary)
        graph = root / "experiment/shared-graph"
        graph.mkdir(parents=True)
        for name in ["prepare_tables.py", "DEPENDENCIES.json"]:
            shutil.copyfile(HERE / name, graph / name)
        for row in dependency_data["dependencies"]:
            destination = graph / row["path"]
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(HERE / row["path"], destination)
        inputs = root / "public inputs with spaces"
        for row in input_rows:
            destination = inputs / row["path"]
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / row["path"], destination)
        output, report = root / "payload.f32", root / "report.json"
        modes = [((), False), (("-O",), False), ((), True)]

        def invoke(mode, expected, input_root=inputs):
            flags, optimized_environment = mode
            environment = os.environ.copy()
            environment.pop("PYTHONOPTIMIZE", None)
            if optimized_environment:
                environment["PYTHONOPTIMIZE"] = "1"
            result = subprocess.run([sys.executable, *flags, str(graph / "prepare_tables.py"),
                                     str(input_root), str(output), str(report)],
                                    cwd=root, env=environment, text=True, capture_output=True)
            require(result.returncode == expected,
                    "unexpected helper exit: " + result.stderr)
            if expected:
                require("input verification failed:" in result.stderr, "missing verification error")

        for mode in modes:
            invoke(mode, 0)
            require(hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED_PAYLOAD_SHA256,
                    "projected payload differs from pinned source float32 representation")
        original_payload, original_report = output.read_bytes(), report.read_bytes()

        def rejected():
            for mode in modes:
                invoke(mode, 2)
                require(output.read_bytes() == original_payload, "failed preparation replaced payload")
                require(report.read_bytes() == original_report, "failed preparation replaced report")

        # Normal and optimized Python must reject altered dependency bytes.
        dependency = graph / "../audiokit-4.9.2/AudioKit/Core/Soundpipe/modules/adsr.c"
        original = dependency.read_bytes()
        dependency.write_bytes(b"!" + original[1:])
        rejected()
        # Updating the dependency manifest to match cannot substitute the fixed closure.
        dep_manifest = graph / "DEPENDENCIES.json"
        dep_original = dep_manifest.read_bytes()
        substituted = json.loads(dep_original)
        entry = next(row for row in substituted["dependencies"] if row["path"].endswith("/adsr.c"))
        changed = dependency.read_bytes()
        entry["sha256"] = hashlib.sha256(changed).hexdigest()
        entry["git_blob_sha"] = hashlib.sha1(b"blob " + str(len(changed)).encode() + b"\0" + changed).hexdigest()
        dep_manifest.write_text(json.dumps(substituted))
        rejected()
        dep_manifest.write_bytes(dep_original)
        dependency.write_bytes(original)
        dependency.unlink()
        rejected()
        dependency.write_bytes(original)

        table = inputs / "AudioKitSynthOne/DSP/BandlimitedWavetables/Triangle/triangle_0000.json"
        table_original = table.read_bytes()
        table.write_bytes(b"!" + table_original[1:])
        rejected()
        # A self-consistent caller input manifest is still not the pinned manifest.
        source_manifest = graph / "../public-source-manifest.json"
        source_original = source_manifest.read_bytes()
        substituted = json.loads(source_original)
        entry = next(row for row in substituted if row["path"].endswith("/triangle_0000.json"))
        entry["sha256"] = hashlib.sha256(table.read_bytes()).hexdigest()
        source_manifest.write_text(json.dumps(substituted))
        rejected()
        source_manifest.write_bytes(source_original)
        table.write_bytes(table_original)
        table.unlink()
        rejected()
        table.write_bytes(table_original)

        index = inputs / "AudioKitSynthOne/DSP/BandlimitedWavetables/bandlimitedWaveforms.json"
        original = index.read_bytes()
        swapped = json.loads(original)
        swapped[0], swapped[1] = swapped[1], swapped[0]
        index.write_text(json.dumps(swapped))
        rejected()
        index.write_bytes(original)
        for mode in modes:
            invoke(mode, 2, root / "missing input root")
        # Restore and confirm no failed check poisoned a later valid preparation.
        invoke(modes[0], 0)
        require(hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED_PAYLOAD_SHA256,
                "restored source representation changed")
    print(json.dumps({"python_modes": 3, "positive_preparations": 4,
                      "negative_preparations": 24, "stale_output_preserved_on_rejection": True,
                      "self_consistent_manifest_substitutions_rejected": True}))


if __name__ == "__main__":
    main()
