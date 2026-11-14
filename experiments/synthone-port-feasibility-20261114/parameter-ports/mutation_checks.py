#!/usr/bin/env python3
"""Fresh authenticated compiler closure for builds and mutation qualification."""
import sys
if not sys.flags.isolated:
    raise SystemExit("invoke with python3 -I mutation_checks.py SYNTHONE_SOURCE_ROOT [or --preflight SOURCE_ROOT]")
sys.dont_write_bytecode = True
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
from types import ModuleType

ROOT = Path(__file__).resolve().parent
EXPECTED_VERIFIER_SHA256 = "6870f7644af8e8d974143a60b56b1e722ff3cbcd1205911cf591692ef7ff6ce3"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checker_at(project):
    path = project / "verify_inputs.py"
    source = path.read_bytes()
    require(hashlib.sha256(source).hexdigest() == EXPECTED_VERIFIER_SHA256, "source verifier changed")
    checker = ModuleType("authenticated_inputs")
    checker.__file__ = str(path)
    exec(compile(source, str(path), "exec", dont_inherit=True), checker.__dict__)
    return checker


def checked_copy(source, destination, row):
    blob = source.read_bytes()
    require(source.is_file() and not source.is_symlink(), "nonregular snapshot input")
    require(len(blob) == row["bytes"] and hashlib.sha256(blob).hexdigest() == row["sha256"],
            "snapshot input changed: " + source.name)
    identity = hashlib.sha1(b"blob " + str(len(blob)).encode() + b"\0" + blob).hexdigest()
    require(identity == row["git_blob_sha"], "snapshot Git blob changed: " + source.name)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(blob)
    require(sha(destination) == row["sha256"], "snapshot write mismatch")


def prepare(source_root):
    checker = checker_at(ROOT)
    checker.verify(source_root)
    data = checker.dependency_data()
    (ROOT / "build").mkdir(exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix="stage-", dir=ROOT / "build")).resolve()
    project = stage / "experiment/parameter-ports"
    project.mkdir(parents=True)
    for row in data["dependencies"] + data["project_inputs"]:
        destination = (project / row["path"]).resolve()
        require(stage in destination.parents, "snapshot path escapes stage")
        checked_copy(ROOT / row["path"], destination, row)
    for name in ("DEPENDENCIES.json", "verify_inputs.py"):
        (project / name).write_bytes((ROOT / name).read_bytes())
    staged_checker = checker_at(project)
    staged_checker.dependency_data()
    master = stage / "master"
    for row in staged_checker.master_inputs():
        expected = {"bytes": row["size"], "sha256": row["sha256"], "git_blob_sha": row["blob_sha"]}
        checked_copy(source_root / row["path"], master / row["path"], expected)
    generated = project / "include"
    generation = subprocess.run([sys.executable, "-I", "-B", str(project / "generate_metadata.py"),
                                 str(master), str(generated)], cwd=stage, text=True, capture_output=True)
    require(generation.returncode == 0, "staged metadata generation failed: " + generation.stderr)
    require({p.name for p in generated.iterdir()} == {"reference_metadata.hpp", "metadata.json"},
            "unexpected generated input")
    allowed = {(project / row["path"]).resolve(): row["sha256"]
               for row in data["dependencies"] + data["project_inputs"]}
    allowed[(generated / "reference_metadata.hpp").resolve()] = sha(generated / "reference_metadata.hpp")
    for path in stage.rglob("*"):
        if path.is_file():
            path.chmod(0o444)
    out = stage / "out"
    out.mkdir()
    return {"stage": stage, "project": project, "generated": generated, "out": out,
            "allowed": allowed, "units": {}, "links": {}, "input_check": checker.verify(source_root)}


def compile_one(build, name, source, expected, compiler, standard, flags):
    out, project = build["out"], build["project"]
    obj, dep = out / (name + ".o"), out / (name + ".d")
    core = project.parent / "audiokit-4.9.2/AudioKit/Core/Soundpipe"
    includes = ["-I" + str(project), "-I" + str(project / "generated"),
                "-I" + str(build["generated"]), "-I" + str(core), "-I" + str(core / "lib/dr_wav")]
    command = [compiler, "-std=" + standard, *flags, *includes, "-MMD", "-MF", str(dep),
               "-MT", "unit", "-c", str(source), "-o", str(obj)]
    result = subprocess.run(command, cwd=project, text=True, capture_output=True)
    (out / (name + ".compile.log")).write_text(result.stdout + result.stderr)
    require(result.returncode == 0, "compilation failed: " + name + result.stderr)
    dependency_text = dep.read_text().replace("\\\n", " ")
    require(dependency_text.startswith("unit:"), "unexpected dependency-file target")
    names = shlex.split(dependency_text[len("unit:"):], posix=True)
    paths = {Path(token.replace("$$", "$")).resolve() for token in names}
    require(paths == {path.resolve() for path in expected}, "compiler input closure mismatch: " + name)
    for path in paths:
        require(path in build["allowed"] and sha(path) == build["allowed"][path],
                "compiler read an unverified or changed input: " + path.name)
    build["units"][name] = {"source": str(source.relative_to(build["stage"])),
                            "dependencies": {str(path.relative_to(build["stage"])): sha(path) for path in sorted(paths)},
                            "object_sha256": sha(obj)}
    return obj


def link(build, name, objects, flags, wrap):
    for key, obj in objects:
        require(sha(obj) == build["units"][key]["object_sha256"], "object changed before link")
    binary = build["out"] / name
    wrappers = ["-Wl,--wrap=" + key for key in ("malloc", "calloc", "realloc", "free", "pow")] if wrap else []
    result = subprocess.run([build["cxx"], *flags, *(str(obj) for _, obj in objects),
                             "-Wl,--gc-sections", *wrappers, "-lm", "-o", str(binary)],
                            cwd=build["project"], text=True, capture_output=True)
    require(result.returncode == 0, "link failed: " + name + result.stderr)
    build["links"][name] = {"objects": {key: sha(obj) for key, obj in objects}, "binary_sha256": sha(binary)}
    return binary


def execute(build, name, binary, expect=0):
    require(sha(binary) == build["links"][name]["binary_sha256"], "binary changed before execution")
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1"
    result = subprocess.run([str(binary)], cwd=build["project"], env=env, text=True, capture_output=True)
    (build["out"] / (name + ".stdout")).write_text(result.stdout)
    (build["out"] / (name + ".stderr")).write_text(result.stderr)
    require(result.returncode == expect, "unexpected runtime result: " + name + result.stderr)
    return result


def closure(build, status):
    for path, expected in build["allowed"].items():
        require(sha(path) == expected, "staged input changed during execution")
    result = {"status": status, "verified_inputs": build["input_check"],
              "compilers": build["compiler_versions"], "flags": build["flags"],
              "units": build["units"], "links": build["links"]}
    (build["out"] / "COMPILE-CLOSURE.json").write_text(json.dumps(result, indent=2) + "\n")


def build_objects(source_root, variant):
    build = prepare(source_root)
    build["cc"], build["cxx"] = shutil.which("cc"), shutil.which("c++")
    require(build["cc"] and build["cxx"], "host C/C++ compiler unavailable")
    build["compiler_versions"] = {key: subprocess.run([build[key], "--version"], text=True,
                                      capture_output=True, check=True).stdout.splitlines()[0] for key in ("cc", "cxx")}
    flags = ["-fno-builtin", "-ffunction-sections", "-fdata-sections", "-ffp-contract=off", "-fno-fast-math",
             "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"]
    flags += ["-Os"] if variant == "os" else ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined,float-cast-overflow", "-fno-sanitize-recover=all"] if variant == "sanitize" else ["-O2"]
    build["flags"] = flags
    project = build["project"]
    core = project.parent / "audiokit-4.9.2/AudioKit/Core/Soundpipe"
    headers = {core / "soundpipe.h", core / "lib/dr_wav/dr_wav.h"}
    port = core / "modules/port.c"
    mapping, map_header = project / "generated/parameter_map.cpp", project / "generated/parameter_map.hpp"
    candidate, candidate_header = project / "parameter_ports.cpp", project / "parameter_ports.hpp"
    full, smoke = project / "tests/ports_test.cpp", project / "tests/projection_smoke.cpp"
    generated = build["generated"] / "reference_metadata.hpp"
    objects = {}
    objects["port"] = compile_one(build, "port", port, {port, *headers}, build["cc"], "gnu99", [*flags, "-DNO_LIBSNDFILE"])
    objects["parameter_map"] = compile_one(build, "parameter_map", mapping, {mapping, map_header}, build["cxx"], "c++11", flags)
    objects["parameter_ports"] = compile_one(build, "parameter_ports", candidate, {candidate, candidate_header, map_header, *headers}, build["cxx"], "c++11", flags)
    for name, source in (("ports_test", full), ("projection_smoke", smoke)):
        objects[name] = compile_one(build, name, source, {source, candidate_header, map_header, generated, *headers}, build["cxx"], "c++11", flags)
    full_binary = link(build, "ports_test", [(key, objects[key]) for key in ("port", "parameter_map", "parameter_ports", "ports_test")], flags, True)
    smoke_binary = link(build, "projection_smoke", [(key, objects[key]) for key in ("port", "parameter_map", "parameter_ports", "projection_smoke")], flags, False)
    build.update(objects=objects, full_binary=full_binary, smoke_binary=smoke_binary)
    return build


def changed(text, old, new):
    require(old in text, "mutation target absent")
    return text.replace(old, new, 1)


def mutation_cases(project):
    source = (project / "parameter_ports.cpp").read_text()
    mapping = (project / "generated/parameter_map.cpp").read_text()
    return [
    ("duplicate_traversal", "map", changed(mapping, "= {0,1,2,5", "= {1,1,2,5")),
    ("wrong_unsmoothed_slot", "map", changed(mapping, "= {0,1,2,255,255", "= {0,1,2,0,255")),
    ("shared_filter_state", "core", changed(source, "common_.c2 * state.yt1", "common_.c2 * selected_[0].yt1")),
    ("stale_half_time_coefficients", "core", changed(source, "if (common_.prvhtim != common_.htime)", "if (common_.prvhtim == -100.0f)")),
    ("half_time_uses_new_target", "core", changed(source, "common_.htime = clamp_half_time(outputs_[parameter]);", "common_.htime = clamp_half_time(value);")),
    ("eager_initial_tick", "core", changed(source, "apply_restore(values, sample_rate);\n    return true;", "apply_restore(values, sample_rate);\n    tick();\n    return true;")),
    ("restore_clears_coefficients", "core", changed(source, "common_.prvhtim = -100.0;", "common_.prvhtim = -100.0;\n    common_.c1 = common_.c2 = 0;")),
    ("missing_unsmoothed_write", "core", changed(source, "if (slot == unsmoothed_slot) outputs_[parameter] = value;", "if (slot == unsmoothed_slot) (void)value;")),
    ("stale_sample_rate", "core", changed(source, "common_.onedsr = 1.0 / common_.sr;", "if (common_.onedsr == 0) common_.onedsr = 1.0 / common_.sr;")),
    ("omitted_partial_alias_guard", "core", changed(source, "(values != outputs && overlaps(ib, ie, ob, oe))", "false")),
    ("missing_snapshot_rate", "core", changed(source, "view.state.sr = common_.sr;", "view.state.sr = 0;")),
]


def mutations(source_root):
    build = build_objects(source_root, "o2")
    baseline = execute(build, "ports_test", build["full_binary"])
    baseline_result = json.loads(baseline.stdout)
    require(baseline_result.get("all150_output_comparisons", 0) > 0 and
            baseline_result.get("seven_field_selected_state_comparisons", 0) > 0 and
            baseline_result.get("static_action_heap_calls") == 0, "baseline is not the known representation test")
    project = build["project"]
    core = project.parent / "audiokit-4.9.2/AudioKit/Core/Soundpipe"
    headers = {core / "soundpipe.h", core / "lib/dr_wav/dr_wav.h"}
    results = []
    for name, kind, text in mutation_cases(project):
        candidate = build["out"] / (name + ".cpp")
        candidate.write_text(text)
        build["allowed"][candidate.resolve()] = sha(candidate)
        expected = {candidate, project / "generated/parameter_map.hpp"}
        if kind == "core":
            expected |= {project / "parameter_ports.hpp", *headers}
        obj = compile_one(build, name, candidate, expected, build["cxx"], "c++11", build["flags"])
        keys = [("port", build["objects"]["port"]),
                (name, obj) if kind == "map" else ("parameter_map", build["objects"]["parameter_map"]),
                (name, obj) if kind == "core" else ("parameter_ports", build["objects"]["parameter_ports"]),
                ("ports_test", build["objects"]["ports_test"])]
        binary = link(build, name, keys, build["flags"], True)
        result = execute(build, name, binary, 1)
        require(result.stderr.startswith("FAIL line "), "failure is not an attributable runtime assertion")
        results.append({"mutation": name, "compile_exit": 0, "link_exit": 0, "run_exit": 1,
                        "runtime_assertion": result.stderr.strip()})
    (build["out"] / "mutation-results.json").write_text(json.dumps(results, indent=2) + "\n")
    closure(build, "passing_fresh_baseline_and_runtime_mutations")
    return {"runtime_mutations_detected": len(results), "compile_failures_counted": 0,
            "baseline": "fresh_verified_full_test_passed"}


def main():
    args = sys.argv[1:]
    if len(args) == 3 and args[0] == "--build":
        source, variant = Path(args[1]).resolve(), args[2]
        require(variant in ("smoke", "o2", "os", "sanitize"), "invalid build variant")
        build = build_objects(source, variant)
        name = "projection_smoke" if variant == "smoke" else "ports_test"
        binary = build["smoke_binary"] if variant == "smoke" else build["full_binary"]
        result = execute(build, name, binary)
        closure(build, "smoke_only_full_test_not_executed" if variant == "smoke" else "fresh_full_test_passed")
        print(result.stdout.strip())
    elif len(args) == 2 and args[0] == "--preflight":
        build = build_objects(Path(args[1]).resolve(), "o2")
        execute(build, "projection_smoke", build["smoke_binary"])
        closure(build, "mutation_preflight_only_full_baseline_not_executed")
        print(json.dumps({"fresh_mutation_object_closure": True, "smoke_passed": True,
                          "full_baseline_executed": False, "mutation_acceptance": False}))
    elif len(args) == 1:
        print(json.dumps(mutations(Path(args[0]).resolve())))
    else:
        raise ValueError("usage: python3 -I mutation_checks.py SYNTHONE_SOURCE_ROOT [or --preflight SOURCE_ROOT]")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        print("build verification failed: " + str(error), file=sys.stderr)
        raise SystemExit(2)
