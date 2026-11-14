#!/usr/bin/env python3
"""Focused malformed-input/binding regressions; no graph or preset suite."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pack_tables as p
import verify_closure


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: test_packer.py SOURCE_MANIFEST SOURCE_ROOT RAW_INPUT GRAPH_ROOT")
    manifest, source, raw, graph = [Path(arg).resolve() for arg in sys.argv[1:]]
    cases = 0
    with tempfile.TemporaryDirectory(prefix="alias-tests-") as directory:
        temp = Path(directory)
        packed, mapping = p.plan(manifest, source, raw)
        assert len(packed) == 819400 and mapping["unique_rows"] == 50
        assert mapping["logical_map"][48]["offset_bytes"] == mapping["logical_map"][44]["offset_bytes"]
        assert mapping["logical_map"][49]["offset_bytes"] == mapping["logical_map"][45]["offset_bytes"]
        payload, mapfile = temp / "payload", temp / "map.json"
        p.publish_local([(payload, packed), (mapfile, json.dumps(mapping).encode())])
        p.verify_packed(manifest, source, raw, payload, mapfile)
        cases += 1

        def reject(call):
            nonlocal cases
            try:
                call()
            except (ValueError, OSError, KeyError, TypeError):
                cases += 1
            else:
                raise AssertionError("invalid input accepted")

        for changed in (raw.read_bytes()[:-4], raw.read_bytes() + b"\0\0\0\0",
                        bytes([raw.read_bytes()[0] ^ 1]) + raw.read_bytes()[1:],
                        b"".join(raw.read_bytes()[i:i + 4][::-1] for i in range(0, raw.stat().st_size, 4))):
            badraw = temp / "badraw"
            badraw.write_bytes(changed)
            reject(lambda: p.plan(manifest, source, badraw))
        for changed in (packed[:-4], packed + b"\0", bytes([packed[0] ^ 1]) + packed[1:],
                        packed[:16384] + struct.pack("<f", -0.) + packed[16388:]):
            payload.write_bytes(changed)
            reject(lambda: p.verify_packed(manifest, source, raw, payload, mapfile))
        payload.write_bytes(packed)
        for mutate in (lambda x: x["logical_map"][48].update(offset_bytes=0),
                       lambda x: x["logical_map"][48].update(offset_bytes=x["logical_map"][45]["offset_bytes"]),
                       lambda x: x["logical_map"][0].update(offset_bytes=819400),
                       lambda x: x.update(row_bytes=16384),
                       lambda x: x.update(logical_rows=52.0),
                       lambda x: x.update(schema=True),
                       lambda x: x.update(raw_input_sha256="0" * 64)):
            changed = json.loads(json.dumps(mapping))
            mutate(changed)
            mapfile.write_text(json.dumps(changed))
            reject(lambda: p.verify_packed(manifest, source, raw, payload, mapfile))
        mapfile.write_text(json.dumps(mapping)[:-1] + ',"schema":1}')
        reject(lambda: p.verify_packed(manifest, source, raw, payload, mapfile))
        mapfile.write_text(json.dumps(mapping))
        badmanifest = temp / "manifest"
        badmanifest.write_bytes(manifest.read_bytes() + b"\n")
        reject(lambda: p.plan(badmanifest, source, raw))
        source_copy = temp / "source with spaces"
        shutil.copytree(source / "AudioKitSynthOne/DSP/BandlimitedWavetables",
                        source_copy / "AudioKitSynthOne/DSP/BandlimitedWavetables")
        p.plan(manifest, source_copy, raw)
        cases += 1
        source_file = next((source_copy / "AudioKitSynthOne/DSP/BandlimitedWavetables").rglob("triangle_0001.json"))
        source_file.write_bytes(source_file.read_bytes() + b" ")
        reject(lambda: p.plan(manifest, source_copy, raw))
        reject(lambda: p.plan(manifest, temp / "absent", raw))
        reject(lambda: p.publish_local([(raw, packed)], [raw]))
        reject(lambda: p.publish_local([(payload, packed), (payload, b"x")]))
        reject(lambda: p.publish_local([(source / "new-output", packed)], source_root=source))
        reject(lambda: p.publish_local([(temp, packed)]))
        # A rejected CLI pack cannot alter previous payload/map files. Exercise
        # optimized Python too; production checks must not depend on assert.
        prior = (payload.read_bytes(), mapfile.read_bytes())
        for flags in ([], ["-O"]):
            command = [sys.executable] + flags + [str(Path(p.__file__)), "pack", "--source-manifest", str(badmanifest),
                       "--source-root", str(source), "--input", str(raw), "--payload", str(payload), "--map", str(mapfile)]
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            assert result.returncode == 2 and (payload.read_bytes(), mapfile.read_bytes()) == prior
            cases += 1
        # Bind compiler inputs independently of caller-provided root names.
        verify_closure.verify(graph)
        cases += 1
        shadow = temp / "graph"
        shadow.mkdir()
        for name in verify_closure.PINS:
            shutil.copy2(graph / name, shadow / name)
        (shadow / "shared_graph.cpp").write_bytes((shadow / "shared_graph.cpp").read_bytes() + b"\n")
        reject(lambda: verify_closure.verify(shadow))
        print(json.dumps({"focused_packer_cases": cases, "status": "PASS"}))


if __name__ == "__main__":
    main()
