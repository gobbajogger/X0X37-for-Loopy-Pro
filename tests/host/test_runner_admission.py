#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the real runner scheduler in scratch trees with small fake test bodies.

python3 tests/host/test_runner_admission.py [--baseline REV] [--output DIR]
The optional baseline uses the same fixtures and records work counts and timings.
No target build, device, or live status files are used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = (b"static unsigned short host_scratch[2048] __attribute__((aligned(4), unused));\n"
           b"#define LZ_SCRATCH host_scratch\n#define PERF_SCRATCH host_scratch\n")
NAMES = ["install-slots-web", "host-build", "stereo", "smc-reachability", "fastmath", "installer",
         "rec-live", "rec-live-smk", "scenario-alpha", "scenario-dash-name", "scenario-multi.x0x", "scenario-perfsave", "perf-scratch"]
WORKER = '''import json, os, pathlib, sys, time
n = sys.argv[1]
if n.startswith("tests/scenarios/"):
    n = "scenario-" + pathlib.Path(n).stem
    assert pathlib.Path(sys.argv[2]).is_dir()
if n == "perf-scratch":
    assert pathlib.Path("build/host/scratch_defs.h").read_bytes() == %r
    assert pathlib.Path("events").read_text().find('"scenario-perfsave", "end"') >= 0
if n == "stereo":
    assert pathlib.Path("build/host/iso/.gen-done").exists() or os.environ["X0X_TEST_JOBS"] == "1"
    assert pathlib.Path("generated").exists()
def event(kind):
    fd = os.open("events", os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o600)
    os.write(fd, (json.dumps([n, kind, time.time_ns()]) + "\\n").encode()); os.close(fd)
if n == "fastmath" and "install-slots-web" in os.environ["X0X_TESTS"].split() and os.environ["X0X_TEST_JOBS"] != "1":
    deadline = time.time() + 5
    while not pathlib.Path("w-start").exists():
        assert time.time() < deadline
        time.sleep(.01)
event("start")
if n == "install-slots-web": pathlib.Path("w-start").touch()
time.sleep({"install-slots-web": .7, "host-build": .15, "stereo": .07, "fastmath": .15}.get(n, .01))
if n == "host-build": pathlib.Path("generated").touch()
event("end")
sys.exit(7 if os.environ.get("FAIL_TEST") == n else 0)
''' % SCRATCH
MKDIR = '''#!/usr/bin/env python3
import json, os, sys
with open("mkdir-calls", "a") as f: f.write(json.dumps(sys.argv[1:]) + "\\n")
if os.environ.get("FAIL_SETUP") and "build/scenarios/alpha" in sys.argv: sys.exit(9)
os.execv("/bin/mkdir", ["mkdir"] + sys.argv[1:])
'''
FLOCK = '''#!/usr/bin/env python3
import json, os, sys
with open("flock-calls", "a") as f: f.write(json.dumps(sys.argv[1:]) + "\\n")
os.execvp(sys.argv[4], sys.argv[4:])
'''


def fixture(source):
    """Keep runner functions, loop and scenario setup; replace expensive probe bodies."""
    start = source.index("tests_body() {")
    end = source.rindex('if [ "$JOBS" -le 1 ]; then\n    tests_body')
    scenario = source[source.index("for s in tests/scenarios/"):source.index("run perf-scratch ")]
    body = "tests_body() {\n"
    body += "".join(f"run {n} python3 worker.py {n}\n" for n in NAMES[:NAMES.index("scenario-alpha")])
    body += 'note "  skip fixture package"\n' + scenario
    body += 'run perf-scratch python3 worker.py perf-scratch\n}\n'
    return source[:start] + body + source[end:]


def execute(source, jobs, selected, *, fail="", setup="", holds=False):
    with tempfile.TemporaryDirectory(prefix="runner-admission-") as tmp:
        root = Path(tmp)
        (root / "tests/scenarios").mkdir(parents=True)
        for n in ("alpha", "dash-name", "multi.x0x", "perfsave"):
            (root / f"tests/scenarios/{n}.x0x").touch()
        (root / "tests/run_tests.sh").write_text(fixture(source))
        (root / "worker.py").write_text(WORKER)
        (root / "build/host").mkdir(parents=True)
        host = root / "build/host/x0x_host"
        host.write_text('#!/bin/sh\nexec python3 worker.py "$@"\n'); host.chmod(0o755)
        (root / "bin").mkdir()
        for name, script in (("mkdir", MKDIR), ("flock", FLOCK)):
            p = root / "bin" / name; p.write_text(script); p.chmod(0o755)
        (root / "gate.lock").touch()
        if setup == "perf": (root / "build/host/scratch_defs.h").mkdir()
        # This scheduler fixture replaces test bodies and omits full-suite dependencies.
        # Do not inherit the outer suite's production dependency gate into this tree.
        env = dict(os.environ, PATH=str(root / "bin") + os.pathsep + os.environ["PATH"],
                   X0X_STRICT_PREFLIGHT="0",
                   X0X_TEST_JOBS=str(jobs), X0X_TESTS=selected, X0X_GATE_LOCK=str(root / "gate.lock"),
                   X0X_GATE_HOLDS_LOCK="1" if holds else "0", FAIL_TEST=fail,
                   FAIL_SETUP="1" if setup == "scenario" else "")
        before = time.monotonic()
        p = subprocess.run(["sh", "-x", "tests/run_tests.sh"], cwd=root, env=env,
                           text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        elapsed = time.monotonic() - before
        def lines(path):
            return [json.loads(x) for x in path.read_text().splitlines()] if path.exists() else []
        events = lines(root / "events")
        trace = p.stdout + "".join(f.read_text() for f in (root / "build/host").glob("*.log"))
        scratch = root / "build/host/scratch_defs.h"
        return dict(jobs=jobs, selected=selected, fail=fail, setup=setup, holds=holds,
                    rc=p.returncode, seconds=elapsed, events=events,
                    results=re.findall(r"^  (?:ok|FAIL) .*", p.stdout, re.M),
                    mkdir=lines(root / "mkdir-calls"), flock=lines(root / "flock-calls"),
                    scratch_sha256=hashlib.sha256(scratch.read_bytes()).hexdigest() if scratch.is_file() else None,
                    scratch_writes=scratch_write_count(trace),
                    scenario_basename_substitutions=len(re.findall(r"^\+{2,} basename tests/scenarios/", trace, re.M)),
                    test_class_substitutions=len(re.findall(r"^\+{2,} test_class ", trace, re.M)),
                    key_formats=len(re.findall(r"^\++ printf ['\"]?%04d", trace, re.M)),
                    trace=trace)


def scratch_write_count(trace):
    """How many times this run tried to write the perf-scratch header.

    bash xtraces the printf even when its redirection fails. dash does not: the
    attempt shows up as the open error for scratch_defs.h. A skipped prepare
    produces neither, so the count stays zero.
    """
    traced = re.findall(r"^\++ printf ['\"]?static unsigned short host_scratch", trace, re.M)
    if traced:
        return len(traced)
    return len(re.findall(r"^[^ \t+].*cannot create \S*scratch_defs\.h: Is a directory", trace, re.M))


def check(r, selected, *, fail="", setup=""):
    chosen = [n for n in NAMES if n in selected.split()]
    bad = fail or ({"scenario": "scenario-alpha", "perf": "perf-scratch"}.get(setup, ""))
    assert r["rc"] == bool(bad), (r["rc"], r["trace"])
    assert len(r["results"]) == len(chosen), r["results"]
    for line, n in zip(r["results"], chosen):
        assert line.startswith("  FAIL " + n if n == bad else "  ok   " + n), line
    starts = [e[0] for e in r["events"] if e[1] == "start"]
    assert sorted(starts) == sorted(n for n in chosen if not setup or n != bad), starts
    dirs = [a for call in r["mkdir"] for a in call if a.startswith("build/scenarios/")]
    assert sorted(dirs) == sorted("build/scenarios/" + n[9:] for n in chosen if n.startswith("scenario-")), dirs
    assert r["scratch_writes"] == int("perf-scratch" in chosen), r["scratch_writes"]
    if "perf-scratch" in chosen and setup != "perf":
        assert r["scratch_sha256"] == hashlib.sha256(SCRATCH).hexdigest()
    assert r["scenario_basename_substitutions"] == 0
    assert r["test_class_substitutions"] == 0
    assert r["key_formats"] == (len(chosen) + 1 if r["jobs"] > 1 else 0), r["key_formats"]
    locks = [x for x in r["flock"] if x[:2] == ["-s", "-o"]]
    wanted_locks = [n for n in chosen if n in ("install-slots-web", "installer")]  # install_path: not rec-live*
    assert len(locks) == (0 if r["holds"] else len(wanted_locks)), locks
    assert sorted(x[-1] for x in locks) == ([] if r["holds"] else sorted(wanted_locks)), locks
    if r["jobs"] > 1 and all(n in chosen for n in NAMES):
        times = {(n, kind): t for n, kind, t in r["events"]}
        assert times["host-build", "end"] <= times["stereo", "start"]
        for a, b in (("host-build", "smc-reachability"), ("stereo", "smc-reachability")):
            assert times[a, "end"] <= times[b, "start"] or times[b, "end"] <= times[a, "start"]
        timeline = sorted((t, 1 if kind == "start" else -1) for n, kind, t in r["events"]
                          if n in ("host-build", "stereo", "smc-reachability", "fastmath"))
        active = 0
        for _, delta in timeline:
            active += delta
            assert 0 <= active <= r["jobs"]
        assert times["install-slots-web", "start"] < times["fastmath", "end"]
        assert times["fastmath", "start"] < times["install-slots-web", "end"]
        assert max(times[n, "end"] for n in ("install-slots-web", "host-build", "stereo", "smc-reachability", "fastmath")) <= times["installer", "start"]
        assert times["rec-live-smk", "end"] <= times["scenario-alpha", "start"]
        assert times["scenario-alpha", "end"] <= times["scenario-perfsave", "start"]
        assert times["scenario-perfsave", "end"] <= times["perf-scratch", "start"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    source = (ROOT / "tests/run_tests.sh").read_text()
    records = []
    all_tests = " ".join(NAMES)
    for jobs in (1, 4):
        for selected, fail, setup, holds in [
            ("fastmath", "", "", False), (all_tests, "", "", False),
            (all_tests, "stereo", "", False), ("scenario-alpha", "", "scenario", False),
            ("perf-scratch", "", "perf", False),
            ("install-slots-web installer rec-live rec-live-smk", "", "", True),
        ]:
            r = execute(source, jobs, selected, fail=fail, setup=setup, holds=holds)
            check(r, selected, fail=fail, setup=setup)
            r["version"] = "candidate"; records.append(r)
    r = execute(source, 2, all_tests)
    check(r, all_tests)
    r["version"] = "candidate"; records.append(r)
    if args.baseline:
        base = subprocess.check_output(["git", "show", args.baseline + ":tests/run_tests.sh"], cwd=ROOT, text=True)
        for jobs in (1, 4):
            for selected in ("fastmath", all_tests):
                r = execute(base, jobs, selected); r["version"] = "baseline"; records.append(r)
                assert r["rc"] == 0, r["trace"]
    if args.output:
        args.output.mkdir(parents=True, exist_ok=True)
        for i, r in enumerate(records):
            (args.output / f"runner-{i:02d}.log").write_text(r.pop("trace"))
        (args.output / "runner.json").write_text(json.dumps(records, indent=2) + "\n")
    print(f"runner admission: {len(records)} cases passed")
    for r in records:
        if not r["fail"] and not r["setup"] and not r["holds"]:
            dirs = sum(a.startswith("build/scenarios/") for c in r["mkdir"] for a in c)
            print(f'{r["version"]} jobs={r["jobs"]} selected={r["selected"]!r}: '
                  f'{r["seconds"]:.3f}s, scenario mkdir={dirs}, scratch writes={r["scratch_writes"]}, key formats={r["key_formats"]}')


if __name__ == "__main__":
    main()
