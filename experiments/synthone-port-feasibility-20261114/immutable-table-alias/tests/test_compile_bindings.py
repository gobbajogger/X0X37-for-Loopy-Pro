#!/usr/bin/env python3
"""RED controls and fixed-closure probes for unlisted compiler headers."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import run_tests
import verify_closure


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_compile_bindings.py GRAPH_ROOT")
    root = Path(__file__).resolve().parents[1]
    (root / "build").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="binding-regressions.", dir=root / "build") as directory:
        work = Path(directory)
        untrusted = verify_closure.stage(Path(sys.argv[1]), work / "untrusted tree with spaces")
        shadow = untrusted / "soundpipe.h"
        shadow.write_text('#error UNPINNED_GRAPH_HEADER_SELECTED\n')
        # The intended file verifier accepts extras. The new stage must omit them.
        verify_closure.verify(untrusted)
        source = work / "probe.cpp"
        source.write_text('#include "shared_graph.hpp"\n#include <cstdio>\n')
        cxx = os.environ.get("CXX", "c++")

        def command(graph):
            core = graph.parent / "audiokit-4.9.2/AudioKit/Core"
            return [cxx, "-std=c++11", "-E", "-I" + str(graph), "-I" + str(graph.parent / "probe"),
                    "-I" + str(core / "Soundpipe"), "-I" + str(core / "SoundpipeExtension"),
                    "-I" + str(core / "Soundpipe/lib/dr_wav"), str(source)]

        red = subprocess.run(command(untrusted), env=run_tests.compiler_environment(),
                             stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        if red.returncode == 0 or "UNPINNED_GRAPH_HEADER_SELECTED" not in red.stderr:
            raise AssertionError("header-shadow RED control did not trigger")
        staged = verify_closure.stage(untrusted, work / "trusted closure")
        if (staged / "soundpipe.h").exists():
            raise AssertionError("unlisted header copied into compiler closure")
        run_tests.run(command(staged), stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        poison = work / "ambient"
        poison.mkdir()
        (poison / "cstdio").write_text('#error UNPINNED_AMBIENT_HEADER_SELECTED\n')
        previous = {key: os.environ.get(key) for key in run_tests.AMBIENT_INCLUDE_VARIABLES}
        try:
            for key in run_tests.AMBIENT_INCLUDE_VARIABLES:
                os.environ[key] = str(poison)
            red = subprocess.run(command(staged), stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
            if red.returncode == 0 or "UNPINNED_AMBIENT_HEADER_SELECTED" not in red.stderr:
                raise AssertionError("ambient-include RED control did not trigger")
            if any(key in run_tests.compiler_environment() for key in run_tests.AMBIENT_INCLUDE_VARIABLES):
                raise AssertionError("ambient include variable retained")
            run_tests.run(command(staged), stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        finally:
            for key, value in previous.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value
        try:
            verify_closure.stage(untrusted, work / "trusted closure")
        except FileExistsError:
            pass
        else:
            raise AssertionError("existing compiler tree accepted")
        print(json.dumps({"compile_binding_checks": 7, "header_shadow_red_detected": True,
                          "ambient_header_red_detected": True, "staged_closure_excludes_extras": True,
                          "status": "PASS"}))


if __name__ == "__main__":
    main()
