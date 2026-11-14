#!/usr/bin/env python3
"""Verify exact inputs, then compile only the new immutable-alias boundary test."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import pack_tables
import verify_closure

HERE = Path(__file__).resolve().parent
AMBIENT_INCLUDE_VARIABLES = ("CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH")


def compiler_environment():
    return {key: value for key, value in os.environ.items() if key not in AMBIENT_INCLUDE_VARIABLES}


def run(command, **kwargs):
    return subprocess.run([str(x) for x in command], check=True, env=compiler_environment(), **kwargs)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--graph-root", required=True, type=Path)
    p.add_argument("--source-root", required=True, type=Path)
    p.add_argument("--source-manifest", required=True, type=Path)
    p.add_argument("--input", required=True, type=Path, help="existing raw 52x4096 little-endian binary32 artifact")
    p.add_argument("--variant", choices=("o2", "os", "sanitize"), default="o2")
    args = p.parse_args()
    try:
        original_graph = verify_closure.verify(args.graph_root)
        payload, mapping = pack_tables.plan(args.source_manifest, args.source_root, args.input)
        build_root = HERE / "build"
        build_root.mkdir(exist_ok=True)
        build = Path(tempfile.mkdtemp(prefix=args.variant + ".", dir=build_root))
        graph = verify_closure.stage(original_graph, build / "closure")
        test_source = build / "alias_graph_test.cpp"
        test_source.write_bytes((HERE / "tests/alias_graph_test.cpp").read_bytes())
        packed, mapfile = build / "packed-morphs.f32", build / "map.json"
        pack_tables.publish_local([(packed, payload), (mapfile, (json.dumps(mapping, indent=2) + "\n").encode())],
                                  [args.input, args.source_manifest], args.source_root)
        mapping = pack_tables.verify_packed(args.source_manifest, args.source_root, args.input, packed, mapfile)
        raw_digest = hashlib.sha256(args.input.read_bytes()).hexdigest()
        packed_digest = hashlib.sha256(packed.read_bytes()).hexdigest()
        common = ["-fno-builtin", "-ffunction-sections", "-fdata-sections", "-ffp-contract=off", "-fno-fast-math"]
        common += ["-O2"] if args.variant == "o2" else ["-Os"] if args.variant == "os" else [
            "-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined,float-cast-overflow",
            "-fno-sanitize-recover=all"]
        core = graph.parent / "audiokit-4.9.2/AudioKit/Core"
        includes = ["-I" + str(path) for path in (graph, graph.parent / "probe", core / "Soundpipe",
                                                  core / "SoundpipeExtension", core / "Soundpipe/lib/dr_wav")]
        cc, cxx = os.environ.get("CC", "cc"), os.environ.get("CXX", "c++")
        compilers = [subprocess.check_output([compiler, "--version"], text=True, env=compiler_environment()).splitlines()[0] for compiler in (cc, cxx)]
        (build / "options.json").write_text(json.dumps({"compilers": compilers, "flags": common}, indent=2) + "\n")
        objects = []
        for name in ("base", "ftbl", "adsr", "crossfade", "osc", "fosc", "noise", "moogladder", "butbp", "buthp", "port", "phasor"):
            obj = build / (name + ".o")
            run([cc, "-std=gnu99", "-DNO_LIBSNDFILE"] + common + includes + ["-c", core / "Soundpipe/modules" / (name + ".c"), "-o", obj])
            objects.append(obj)
        obj = build / "oscmorph2d.o"
        run([cc, "-std=gnu99", "-DNO_LIBSNDFILE"] + common + includes + ["-c", core / "SoundpipeExtension/modules/oscmorph2d.c", "-o", obj])
        objects.append(obj)
        for source, name in ((graph.parent / "probe/portable_voice.cpp", "voice"),
                             (graph / "shared_graph.cpp", "graph"),
                             (test_source, "alias_test")):
            obj = build / (name + ".o")
            run([cxx, "-std=c++11"] + common + includes + ["-c", source, "-o", obj])
            objects.append(obj)
        executable = build / "alias_graph_test"
        run([cxx] + common + objects + ["-Wl,--gc-sections", "-lm", "-o", executable])
        env = dict(compiler_environment(), ASAN_OPTIONS="detect_leaks=1")
        result = subprocess.check_output([str(executable), str(args.input.resolve()), str(packed)] +
                                         [str(row["offset_bytes"]) for row in mapping["logical_map"]], env=env, text=True)
        report = json.loads(result)
        # Files and input closure are checked after compute, not merely before link.
        verify_closure.verify(graph)
        verify_closure.verify(original_graph)
        if test_source.read_bytes() != (HERE / "tests/alias_graph_test.cpp").read_bytes():
            raise ValueError("new harness changed during compile/test")
        pack_tables.verify_packed(args.source_manifest, args.source_root, args.input, packed, mapfile)
        if hashlib.sha256(args.input.read_bytes()).hexdigest() != raw_digest or hashlib.sha256(packed.read_bytes()).hexdigest() != packed_digest:
            raise ValueError("table payload changed during graph test")
        report.update({"variant": args.variant, "raw_input_sha256": raw_digest, "packed_sha256": packed_digest,
                       "payload_hashes_unchanged": True, "source_dependencies_unchanged": True,
                       "compile_from_verified_allowlist": True, "ambient_include_paths_removed": True,
                       "compilers": compilers, "graph_commit": verify_closure.GRAPH_COMMIT})
        (build / "result.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print("alias test rejected or failed: " + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
