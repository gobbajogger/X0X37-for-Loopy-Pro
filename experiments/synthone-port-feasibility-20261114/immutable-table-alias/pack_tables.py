#!/usr/bin/env python3
"""Exact immutable-row packing of pinned public Synth One binary32 morphs.

Generated payloads are local test inputs, not a public source deliverable.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
import tempfile

SOURCE_COMMIT = "6466a3715c96b7ecf1dd255a218cbd571408a314"
SOURCE_MANIFEST_SHA256 = "c08dbf689ce4295acd565f52a16b5af32835edab4695133c012da9a7dd4f8020"
RAW_SHA256 = "874b3167827dbd7e07125afc0cf9cf4b4a642790b572878fdb9bd14544184979"
GUARDED_SHA256 = "0d35b5422cb5d3e0e349a66c9e0a50f2980b6287e68f8d94f55640d3f8168bd5"
ROWS, SAMPLES, SAMPLE_BYTES = 52, 4096, 4
RAW_ROW_BYTES = SAMPLES * SAMPLE_BYTES
GUARDED_ROW_BYTES = RAW_ROW_BYTES + SAMPLE_BYTES
GUARD = b"\0\0\0\0"
SUFFIXES = ("0000", "2696", "1348", "0674", "0337", "0168", "0084",
            "0042", "0021", "0010", "0005", "0002", "0001")
SHAPES = ("triangle", "square", "pwm", "sawtooth")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False)


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate JSON key: " + key)
        result[key] = value
    return result


def load_json(data):
    return json.loads(data, object_pairs_hook=unique_keys,
                      parse_constant=lambda token: (_ for _ in ()).throw(
                          ValueError("nonfinite JSON token: " + token)))


def verify_sources(manifest_path, source_root):
    manifest_bytes = manifest_path.read_bytes()
    require(sha256(manifest_bytes) == SOURCE_MANIFEST_SHA256,
            "source manifest does not match the reviewed public closure")
    manifest = load_json(manifest_bytes)
    rows = [row for row in manifest
            if row["repository"] == "AudioKit/AudioKitSynthOne"
            and row["commit"] == SOURCE_COMMIT
            and "/BandlimitedWavetables/" in row["path"]]
    require(len(rows) == 54, "expected 54 pinned public table inputs")
    contents = {}
    source_root = source_root.resolve(strict=True)
    for row in rows:
        path = (source_root / row["path"]).resolve(strict=True)
        require(source_root in path.parents, "table input escapes source root")
        data = path.read_bytes()
        require(len(data) == row["size"], "source size mismatch: " + row["path"])
        require(sha256(data) == row["sha256"], "source hash mismatch: " + row["path"])
        blob = hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()
        require(blob == row["blob_sha"], "source blob mismatch: " + row["path"])
        contents[Path(row["path"]).name] = data
    index = load_json(contents["bandlimitedWaveforms.json"])
    names = [shape + "_" + suffix for suffix in SUFFIXES for shape in SHAPES]
    require(index == names, "source table order mismatch")
    # Check converted bytes against the existing raw artifact, not a second table
    # interpretation. Explicit <f is independent of Python's native byte order.
    converted = bytearray()
    for i, name in enumerate(names):
        record = load_json(contents[name + ".json"])
        require(record["phase"] == 0, "unexpected phase")
        require(record["type"] == ((1, 2, 0, 3)[i] if i < 4 else 10), "unexpected type")
        samples = record["content"]
        require(len(samples) == SAMPLES and all(type(v) in (int, float) and math.isfinite(v)
                                               for v in samples), "invalid source samples")
        row = struct.pack("<4096f", *samples)
        require(all(-2 <= v <= 2 for v in struct.unpack("<4096f", row)),
                "source morph domain exceeded")
        converted.extend(row)
    require(sha256(converted) == RAW_SHA256, "source conversion differs from pinned binary32")
    return names, bytes(converted)


def plan(manifest_path, source_root, raw_path):
    require(struct.calcsize("f") == 4 and struct.pack("<f", 1.0) == b"\0\0\x80?",
            "IEEE-754 binary32 conversion required")
    names, converted = verify_sources(manifest_path, source_root)
    raw = raw_path.read_bytes()
    require(len(raw) == ROWS * RAW_ROW_BYTES, "raw input must contain exactly 52x4096 binary32 samples")
    require(sha256(raw) == RAW_SHA256 and raw == converted, "raw input differs from pinned public tables")
    unique, first, logical, payload = {}, [], [], bytearray()
    for i, name in enumerate(names):
        row = raw[i * RAW_ROW_BYTES:(i + 1) * RAW_ROW_BYTES] + GUARD
        if row not in unique:
            unique[row] = len(unique)
            first.append(i)
            payload.extend(row)
        u = unique[row]
        logical.append({"logical_index": i, "name": name, "unique_index": u,
                        "offset_bytes": u * GUARDED_ROW_BYTES, "guarded_sha256": sha256(row)})
    require(len(unique) == 50 and first == list(range(48)) + [50, 51],
            "unexpected exact duplicate topology")
    mapping = {"schema": 1, "encoding": "IEEE754-binary32-little-endian",
               "source_commit": SOURCE_COMMIT, "source_manifest_sha256": SOURCE_MANIFEST_SHA256,
               "raw_input_sha256": RAW_SHA256, "logical_rows": ROWS, "unique_rows": len(unique),
               "samples_per_row": SAMPLES, "positive_zero_guards_per_row": 1,
               "row_bytes": GUARDED_ROW_BYTES, "payload_bytes": len(payload),
               "payload_sha256": sha256(payload), "unique_first_logical_indices": first,
               "logical_map": logical}
    return bytes(payload), mapping


def verify_packed(manifest_path, source_root, raw_path, payload_path, map_path):
    expected_payload, expected_map = plan(manifest_path, source_root, raw_path)
    payload = payload_path.read_bytes()
    require(payload == expected_payload, "packed payload differs (including length, row bits or guards)")
    mapping = load_json(map_path.read_bytes())
    # Canonical token equality also rejects booleans/floats in integer fields.
    require(canonical(mapping) == canonical(expected_map), "logical map or source binding differs")
    return mapping


def publish_local(outputs, protected_paths=(), source_root=None):
    destinations = [path.resolve() for path, _ in outputs]
    require(len(set(destinations)) == len(destinations), "output paths must be distinct")
    protected = {path.resolve() for path in protected_paths}
    for path in destinations:
        require(path not in protected, "output would overwrite an input")
        if source_root is not None:
            require(path != source_root.resolve() and source_root.resolve() not in path.parents,
                    "output must be outside public source root")
        require(not path.exists() or path.is_file(), "output is not a regular file")
    pending = []
    try:
        for path, data in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
                pending.append((Path(stream.name), path))
                stream.write(data)
        for temp, path in pending:
            os.replace(temp, path)
    finally:
        for temp, _ in pending:
            temp.unlink(missing_ok=True)


def extract_guarded(input_path, output_path):
    data = input_path.read_bytes()
    require(len(data) == ROWS * GUARDED_ROW_BYTES and sha256(data) == GUARDED_SHA256,
            "guarded input does not match existing pinned graph artifact")
    require(all(data[i * GUARDED_ROW_BYTES + RAW_ROW_BYTES:(i + 1) * GUARDED_ROW_BYTES] == GUARD
                for i in range(ROWS)), "guard is not original positive zero")
    raw = b"".join(data[i * GUARDED_ROW_BYTES:i * GUARDED_ROW_BYTES + RAW_ROW_BYTES]
                   for i in range(ROWS))
    require(sha256(raw) == RAW_SHA256, "extracted raw hash differs")
    publish_local([(output_path, raw)], [input_path])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    extract = sub.add_parser("extract", help="strip checked +0 guards from the existing graph artifact")
    extract.add_argument("input", type=Path)
    extract.add_argument("output", type=Path)
    for command in ("pack", "verify"):
        p = sub.add_parser(command)
        p.add_argument("--source-manifest", required=True, type=Path)
        p.add_argument("--source-root", required=True, type=Path)
        p.add_argument("--input", required=True, type=Path)
        p.add_argument("--payload", required=True, type=Path)
        p.add_argument("--map", required=True, type=Path)
    args = parser.parse_args()
    try:
        if args.command == "extract":
            extract_guarded(args.input, args.output)
            return 0
        if args.command == "pack":
            payload, mapping = plan(args.source_manifest, args.source_root, args.input)
            publish_local([(args.payload, payload),
                           (args.map, (json.dumps(mapping, indent=2) + "\n").encode())],
                          [args.source_manifest, args.input], args.source_root)
        else:
            mapping = verify_packed(args.source_manifest, args.source_root, args.input,
                                    args.payload, args.map)
        print(json.dumps({key: mapping[key] for key in ("logical_rows", "unique_rows", "payload_bytes",
                                                       "payload_sha256")}))
    except (OSError, ValueError, KeyError, TypeError, OverflowError, struct.error) as error:
        print("alias input verification failed: " + str(error), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
