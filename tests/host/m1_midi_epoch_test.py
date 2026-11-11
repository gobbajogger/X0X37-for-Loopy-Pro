#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile exact production MIDI queue functions with an IRQ-source mock.

No queue algorithm is copied into this fixture. Extraction byte ranges and
instrumentation are recorded. This verifies a latest-cut queue contract, not
the backend PANIC operation, native SDK build, or hardware behavior.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--baseline-usb", type=Path, required=True)
    args = parser.parse_args()
    root, out = args.root.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    usb, plat, host = (root / p for p in ("firmware/src/usb.c", "firmware/src/app/plat_fm1.c", "host/x0x_host.c"))
    manifests = []

    def span(path, begin, end, signature):
        data = path.read_bytes()
        raw = data[begin:end]
        manifests.append({"source": str(path), "signature": signature,
                          "source_sha256": sha(data), "offset": begin,
                          "size_bytes": len(raw), "sha256": sha(raw),
                          "line": data[:begin].count(b"\n") + 1})
        return raw

    def function(path, signature):
        data = path.read_bytes()
        begin = data.index(signature.encode("ascii"))
        end = data.index(b"\n}\n", begin) + 3
        return span(path, begin, end, signature)

    def block(path, first, after):
        data = path.read_bytes()
        begin = data.index(first.encode("ascii"))
        end = data.index(after.encode("ascii"), begin)
        return span(path, begin, end, first + " ... " + after)

    def line(path, starts):
        data = path.read_bytes()
        begin = data.index(starts.encode("ascii"))
        return span(path, begin, data.index(b"\n", begin)+1, starts)

    ring_macro = line(root / "firmware/src/x0x.c", "#define RING_PUBLISH()")
    declarations = block(usb, "#define MQ ", "/* CIN 8,")
    predicates = b""
    for sig in ("static int midi_pkt_is_note_off(", "static int midi_pkt_is_rt_transport(",
                "static int midi_pkt_is_link_end(", "static int midi_pkt_is_keep(",
                "static int midi_pkt_is_pitch_bend("):
        predicates += function(usb, sig)
    ring = function(usb, "static int midi_ring_put_note_off(")
    native = declarations + ring_macro
    native += predicates + ring + function(usb, "static int midi_in_put(")
    native += b"\n#ifndef FELUCCA_LOADER\n"
    native += function(usb, "static __attribute__((noinline)) void midi_link_check(")
    signatures = ("uint32_t plat_midi_mark(void)", "int plat_midi_passed(uint32_t mark)",
                  "int plat_midi_pending(void)", "uint32_t plat_midi_cut(void)",
                  "int plat_midi_in_epoch(uint32_t *pkt, uint32_t *epoch)",
                  "int plat_midi_in(uint32_t *pkt)")
    for sig in signatures:
        native += function(plat, sig)
    native += b"\n#endif\n"
    transformations = []

    def observe(source, old, new):
        count = source.count(old)
        if not count:
            raise ValueError("Missing observation target: " + old.decode())
        transformations.append({"original": old.decode(), "instrumented": new.decode(), "count": count})
        return source.replace(old, new)

    scheduled = observe(native, ring_macro, ring_macro + b"\n#ifndef FELUCCA_LOADER\n#undef RING_PUBLISH\n#define RING_PUBLISH() test_publish()\n#endif\n")
    for statement in (b"*pkt = midi_in_q[mi_r % MQ];", b"*epoch = mi_fresh[mi_r % MQ] ? mi_epoch : 0u;",
                      b"mi_r++;", b"mi_epoch = epoch;"):
        scheduled = observe(scheduled, statement, statement + b" test_publish();")
    scheduled = observe(scheduled, b"mi_fresh[i] = 0u;", b"mi_fresh[i] = 0u, test_publish();")
    # A comma expression preserves the one-statement body of an unbraced if.
    assignments = re.findall(rb"(?:\bq|\bfresh|\bmidi_in_q|\bmi_fresh)\[[^;\n]+?\]\s*=\s*[^;\n]+;", scheduled)
    for statement in dict.fromkeys(assignments):
        if b"test_publish()" not in statement:
            scheduled = observe(scheduled, statement, statement[:-1] + b", test_publish();")
    host_header = block(host, "#define MQ ", "int engine_midi_held(")
    for sig in signatures:
        host_header += function(host, sig)
    baseline = b"#define midi_ring_put_note_off baseline_midi_ring_put_note_off\n"
    baseline += function(args.baseline_usb.resolve(), "static int midi_ring_put_note_off(")
    baseline += b"#undef midi_ring_put_note_off\n"
    test = Path(__file__).with_suffix(".c")
    compiler = shlex.split(os.environ.get("CC", "cc"))
    modes = (("strict", ["-O2"]), ("asan-ubsan", ["-O1", "-g", "-fsanitize=address,undefined",
              "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]))
    results = []
    for target, header, defines in (("native-raw", native, []),
                                     ("native-scheduled", scheduled, ["-DTEST_INSTRUMENTED=1"]),
                                     ("host", host_header, ["-DTEST_HOST=1"]),
                                     ("loader", native, ["-DFELUCCA_LOADER=1"])):
        target_out = out / target
        target_out.mkdir(exist_ok=True)
        (target_out / "m1_epoch_production.h").write_bytes(header)
        (target_out / "m1_epoch_baseline_ring.h").write_bytes(baseline)
        for mode, optimization in modes:
            exe = target_out / mode
            command = compiler + ["-std=c11", "-pedantic", "-Wall", "-Wextra", "-Werror", "-Wconversion",
                      "-Wshadow", "-Wstrict-prototypes"] + optimization + defines + ["-I", str(target_out),
                      str(test), "-o", str(exe)]
            (target_out / (mode + ".command.json")).write_text(json.dumps(command, indent=2) + "\n")
            with (target_out / (mode + ".compile.log")).open("wb") as log:
                comp = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=False)
            result = {"target": target, "mode": mode, "compile_exit": comp.returncode}
            if comp.returncode == 0:
                with (target_out / (mode + ".run.log")).open("wb") as log:
                    run = subprocess.run([str(exe)], stdout=log, stderr=subprocess.STDOUT, check=False)
                result["run_exit"] = run.returncode
            results.append(result)
    (out / "production-source-manifest.json").write_text(json.dumps({
        "source_root": str(root), "members": manifests, "native_header_sha256": sha(native),
        "scheduled_header_sha256": sha(scheduled), "host_header_sha256": sha(host_header),
        "baseline_header_sha256": sha(baseline), "transformations": transformations,
        "instrumentation": "native-raw/host/loader function bytes unchanged; native-scheduled adds exactly recorded observation hooks after packet/tag reads and cursor increments, inside cut tag clearing and after epoch publication, and after producer payload/tag writes. RING_PUBLISH retains compiler memory barrier. fm1 IRQ enable-bit MMIO is mocked. Baseline helper symbol renamed only by preprocessor.",
        "test_sha256": sha(test.read_bytes()), "runner_sha256": sha(Path(__file__).read_bytes())}, indent=2) + "\n")
    (out / "results.json").write_text(json.dumps({"producer": os.environ.get("X0X_TEST_PRODUCER", "UNSPECIFIED; see invocation receipt"), "results": results,
        "claim_scope": "Latest-cut MIDI queue contract only; no backend PANIC/native/device/formal-review claim"}, indent=2) + "\n")
    print(json.dumps(results, indent=2))
    return int(any(r.get("compile_exit") != 0 or r.get("run_exit") != 0 for r in results))


if __name__ == "__main__":
    raise SystemExit(main())
