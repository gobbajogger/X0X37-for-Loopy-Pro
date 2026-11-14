#!/usr/bin/env python3
"""Verify the checked-in source closure and caller-provided public waveforms.

Generated float32 data is local test input, never a publication payload.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

HERE = Path(__file__).resolve().parent
EXPECTED_DEPENDENCIES_SHA256 = "00aa7392ffc13b59deef1a05315cc86cedd751522c54817744319676effc4500"
SYNTHONE_COMMIT = "6466a3715c96b7ecf1dd255a218cbd571408a314"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify_dependencies():
    manifest = HERE / "DEPENDENCIES.json"
    require(hashlib.sha256(manifest.read_bytes()).hexdigest() == EXPECTED_DEPENDENCIES_SHA256,
            "dependency manifest does not match the reviewed source closure")
    dependencies = json.loads(manifest.read_text())
    for row in dependencies["dependencies"]:
        blob = (HERE / row["path"]).read_bytes()
        require(len(blob) == row["bytes"], "dependency size mismatch: " + row["path"])
        require(hashlib.sha256(blob).hexdigest() == row["sha256"],
                "dependency SHA-256 mismatch: " + row["path"])
        identity = hashlib.sha1(b"blob " + str(len(blob)).encode() + b"\0" + blob).hexdigest()
        require(identity == row["git_blob_sha"], "dependency Git blob mismatch: " + row["path"])


def prepare(source_root, output, report_path):
    require(sys.byteorder == "little" and struct.calcsize("f") == 4,
            "the host fixture requires little-endian IEEE-754 float32")
    verify_dependencies()
    public_manifest = json.loads((HERE / "../public-source-manifest.json").read_text())
    rows = [row for row in public_manifest
            if row["repository"] == "AudioKit/AudioKitSynthOne"
            and row["commit"] == SYNTHONE_COMMIT
            and "BandlimitedWavetables/" in row["path"]]
    require(len(rows) == 54, "expected 52 waveform files, index and frequency file")
    for row in rows:
        blob = (source_root / row["path"]).read_bytes()
        require(len(blob) == row["size"], "public input size mismatch: " + row["path"])
        require(hashlib.sha256(blob).hexdigest() == row["sha256"],
                "public input SHA-256 mismatch: " + row["path"])
        identity = hashlib.sha1(b"blob " + str(len(blob)).encode() + b"\0" + blob).hexdigest()
        require(identity == row["blob_sha"], "public input Git blob mismatch: " + row["path"])
    base = source_root / "AudioKitSynthOne/DSP/BandlimitedWavetables"
    index = json.loads((base / "bandlimitedWaveforms.json").read_text())
    frequencies = json.loads((base / "bandlimitedWaveformFrequencies.json").read_text())["content"]
    require(len(index) == 52 and len(frequencies) == 13, "wrong row or frequency count")
    require(all(math.isfinite(f) for f in frequencies), "nonfinite frequency")
    require(all(a < b for a, b in zip(frequencies, frequencies[1:])), "unordered frequencies")
    suffixes = ["0000", "2696", "1348", "0674", "0337", "0168", "0084", "0042", "0021", "0010", "0005", "0002", "0001"]
    shapes = ["triangle", "square", "pwm", "sawtooth"]
    table_reports, payload = [], bytearray()
    for i, name in enumerate(index):
        require(name == shapes[i % 4] + "_" + suffixes[i // 4], "incorrect source table order")
        row = next(row for row in rows if row["path"].endswith("/" + name + ".json"))
        record = json.loads((source_root / row["path"]).read_text())
        require(record["phase"] == 0, "unexpected source phase")
        require(record["type"] == ([1, 2, 0, 3][i] if i < 4 else 10), "unexpected source type")
        values = record["content"]
        require(len(values) == 4096 and all(math.isfinite(v) for v in values),
                "incorrect sample count or nonfinite source sample")
        binary = struct.pack("<4096f", *values)
        f32 = struct.unpack("<4096f", binary)
        require(all(-2 <= v <= 2 for v in f32), "source morph table exceeds admitted domain")
        payload += binary + struct.pack("<f", 0)
        table_reports.append({"index": i, "name": name, "count": len(values),
                              "min_f32": min(f32), "max_f32": max(f32),
                              "outside_unit_range": sum(not -1 <= v <= 1 for v in f32),
                              "source_blob_sha": row["blob_sha"], "source_sha256": row["sha256"]})
    report = {"source_commit": SYNTHONE_COMMIT, "verified_public_table_files": len(rows),
              "morph_rows": 52, "samples_per_row": 4096, "guard_samples_per_row": 1,
              "frequencies": frequencies, "rows": table_reports,
              "morph_min_f32": min(row["min_f32"] for row in table_reports),
              "morph_max_f32": max(row["max_f32"] for row in table_reports),
              "outside_unit_range": sum(row["outside_unit_range"] for row in table_reports),
              "local_only_payload_bytes": len(payload),
              "local_only_payload_sha256": hashlib.sha256(payload).hexdigest(),
              "conversion": "source-order JSON values to float32 with positive-zero guards; no clipping or scaling"}
    # No output file is touched until every dependency and source input validates.
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report[key] for key in ["verified_public_table_files", "morph_rows",
          "samples_per_row", "morph_min_f32", "morph_max_f32", "outside_unit_range"]}))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path,
                        help="pinned Synth One repository root containing AudioKitSynthOne/DSP")
    parser.add_argument("output", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    try:
        prepare(args.source_root, args.output, args.report)
    except (OSError, ValueError, KeyError, TypeError, StopIteration, OverflowError, struct.error) as error:
        print("input verification failed: " + str(error), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
