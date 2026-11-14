#!/usr/bin/env python3
"""New narrow source-binding tests; no DSP oracle or compiler invocation."""
import hashlib
import json
import os
from pathlib import Path
import py_compile
import shutil
import subprocess
import sys
import tempfile
from types import ModuleType

HERE = Path(__file__).resolve().parent.parent


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    require(len(sys.argv) == 2, "usage: python3 tests/test_inputs.py SYNTHONE_SOURCE_ROOT")
    source = Path(sys.argv[1]).resolve()
    checker_path = HERE / "verify_inputs.py"
    checker = ModuleType("pinned_inputs")
    checker.__file__ = str(checker_path)
    exec(compile(checker_path.read_bytes(), str(checker_path), "exec", dont_inherit=True), checker.__dict__)
    checker.verify(source)
    deps = json.loads((HERE / "DEPENDENCIES.json").read_text())
    manifest = json.loads((HERE / "../public-source-manifest.json").read_text())
    master = [row for row in manifest if row["repository"] == "AudioKit/AudioKitSynthOne"
              and row["commit"] == checker.COMMIT and ("/DSP/Kernel/S1DSPKernel" in row["path"]
              or row["path"].endswith("/DSP/S1Parameter.h") or row["path"].endswith("/DSP/Audio Unit/S1AudioUnit.mm"))]
    with tempfile.TemporaryDirectory(prefix="parameter-inputs-") as temporary:
        root = Path(temporary);project = root / "experiment/parameter-ports"
        project.mkdir(parents=True)
        for name in ["verify_inputs.py", "DEPENDENCIES.json"]:
            shutil.copyfile(HERE / name, project / name)
        for row in deps["dependencies"] + deps["project_inputs"]:
            dest = project / row["path"];dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(HERE / row["path"], dest)
        shutil.copyfile(HERE / "mutation_checks.py", project / "mutation_checks.py")
        inputs = root / "public source with spaces"
        for row in master:
            dest = inputs / row["path"];dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / row["path"], dest)
        modes = [((), False), (("-O",), False), ((), True)]

        def invoke(mode, passed, extra_environment=None):
            flags, optimized = mode;env = os.environ.copy();env.pop("PYTHONOPTIMIZE", None)
            if optimized:env["PYTHONOPTIMIZE"] = "1"
            env.update(extra_environment or {})
            result = subprocess.run([sys.executable, *flags, str(project / "verify_inputs.py"), str(inputs)],
                                    cwd=root, env=env, text=True, capture_output=True)
            require(result.returncode == (0 if passed else 2), "unexpected source verification result")

        for mode in modes:invoke(mode, True)
        dependency = project / "../audiokit-4.9.2/AudioKit/Core/Soundpipe/modules/port.c"
        old = dependency.read_bytes();dependency.write_bytes(b"!" + old[1:])
        for mode in modes:invoke(mode, False)
        dep_manifest = project / "DEPENDENCIES.json";dep_original = dep_manifest.read_bytes()
        altered = json.loads(dep_original);row = next(row for row in altered["dependencies"] if row["path"].endswith("/port.c"))
        row["sha256"] = hashlib.sha256(dependency.read_bytes()).hexdigest()
        dep_manifest.write_text(json.dumps(altered))
        for mode in modes:invoke(mode, False)
        dep_manifest.write_bytes(dep_original);dependency.write_bytes(old)
        header = inputs / "AudioKitSynthOne/DSP/Kernel/S1DSPKernel.hpp"
        old = header.read_bytes();header.write_bytes(b"!" + old[1:])
        for mode in modes:invoke(mode, False)
        source_manifest = project / "../public-source-manifest.json";source_original = source_manifest.read_bytes()
        altered = json.loads(source_original);row = next(row for row in altered if row["path"].endswith("/S1DSPKernel.hpp"))
        row["sha256"] = hashlib.sha256(header.read_bytes()).hexdigest();source_manifest.write_text(json.dumps(altered))
        for mode in modes:invoke(mode, False)
        source_manifest.write_bytes(source_original);header.write_bytes(old)
        enum = inputs / "AudioKitSynthOne/DSP/S1Parameter.h";old = enum.read_bytes();enum.unlink()
        for mode in modes:invoke(mode, False)
        enum.write_bytes(old)
        # Quoted lookup must not reach a source-local or dependency-local shadow.
        shadows = ["tests/reference_metadata.hpp", "tests/parameter_ports.hpp", "soundpipe.h",
                   "../audiokit-4.9.2/AudioKit/Core/Soundpipe/modules/soundpipe.h",
                   "../audiokit-4.9.2/AudioKit/Core/Soundpipe/dr_wav.h"]
        for name in shadows:
            shadow = project / name;shadow.parent.mkdir(parents=True, exist_ok=True)
            shadow.write_text("// unverified include shadow\n")
            for mode in modes:invoke(mode, False)
            shadow.unlink()
        for key in checker.AMBIENT_COMPILER_PATHS:
            for mode in modes:invoke(mode, False, {key: str(root / "ambient include path")})
        for mode in modes:invoke(mode, True)
        # An authenticated .py must never execute unrelated cached bytecode.
        cache = project / "__pycache__"
        cache.mkdir()
        attack = root / "unverified_cache_payload.py"
        marker = "UNVERIFIED_VERIFIER_CACHE_EXECUTED"
        attack.write_text("print(" + repr(marker) + ")\nraise SystemExit(97)\n")
        cached = cache / ("verify_inputs." + sys.implementation.cache_tag + ".pyc")
        py_compile.compile(str(attack), cfile=str(cached), doraise=True,
                           invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH)
        loader_probe = '''from pathlib import Path
from types import ModuleType
import json, sys
path = Path(sys.argv[1])
driver = ModuleType("source_driver")
driver.__file__ = str(path)
exec(compile(path.read_bytes(), str(path), "exec", dont_inherit=True), driver.__dict__)
print(json.dumps(driver.checker_at(path.parent).verify(Path(sys.argv[2]))))
'''
        probe = subprocess.run([sys.executable, "-I", "-c", loader_probe,
                                str(project / "mutation_checks.py"), str(inputs)],
                               cwd=root, text=True, capture_output=True)
        require(probe.returncode == 0 and marker not in probe.stdout + probe.stderr,
                "build verifier executed unverified bytecode")
        require(json.loads(probe.stdout)["verified_master_files"] == 16,
                "build verifier source did not execute")
        generated = root / "generated reference"
        probe = subprocess.run([sys.executable, "-I", str(project / "generate_metadata.py"),
                                str(inputs), str(generated)], cwd=root, text=True, capture_output=True)
        require(probe.returncode == 0 and marker not in probe.stdout + probe.stderr,
                "metadata verifier executed unverified bytecode")
        require((generated / "reference_metadata.hpp").is_file(), "metadata generator source did not execute")
    print(json.dumps({"python_modes": 3, "positive_source_checks": 6, "negative_source_checks": 51,
                      "quoted_include_shadows_rejected": 5, "ambient_compiler_paths_rejected": 7,
                      "self_consistent_manifest_substitutions_rejected": True,
                      "unverified_bytecode_loaders_bypassed": 2}))


if __name__ == "__main__":
    main()
