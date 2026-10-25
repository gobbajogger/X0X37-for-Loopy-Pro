#!/usr/bin/env python3
"""Verify lock integration tests detect four production-path failures at runtime.

All mutations live in temporary source files. Compilation failures are errors,
never successful negative controls. No MIDI clients or hardware are used.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "firmware/src/app/smc_mixer.c").read_text()
out = Path(os.environ.get("SMC_LOCK_NEGATIVE_OUT", ROOT / "build/host/smc-lock-negative"))
out.mkdir(parents=True, exist_ok=True)
mutants = {
    "fader-captures-home": (
        "smc.lock_fader[i] = smc_bind(smc_target(i));",
        "smc.lock_fader[i] = smc_home_target(i);",
    ),
    "pattern-binding-removed": (
        "r.b = (uint8_t)(seq.ppat[part] + 1u);",
        "r.b = (uint8_t)(seq.ppat[part] * 0u);",
    ),
    "pickup-epoch-retained": (
        "smc.picked = smc.seen = 0;\n    smc.knob_i = 0; smc.pending = 0xFFu;",
        "/* negative: old pickup retained */\n    smc.knob_i = 0; smc.pending = 0xFFu;",
    ),
    "song-structure-ignored": (
        "smc.lock_song_revision != song_structure_revision ||",
        "0 ||",
    ),
}
for name, (old, new) in mutants.items():
    if source.count(old) != 1:
        raise RuntimeError(f"{name}: expected one production anchor, found {source.count(old)}")
    with tempfile.TemporaryDirectory(prefix="smc-lock-negative-") as tmp:
        candidate = Path(tmp) / "smc_mixer.c"
        candidate.write_text(source.replace(old, new, 1))
        env = dict(os.environ, SMC_UI_SOURCE_FILE=str(candidate), SMC_LOCK_OUT=str(out / name))
        result = subprocess.run(["sh", "tests/host/smc_lock_test.sh"], cwd=ROOT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (out / f"{name}.log").write_text(result.stdout)
        failures = [line for line in result.stdout.splitlines()
                    if line.startswith("FAIL tests/host/smc_lock_test.c:")]
        if not result.returncode or not failures:
            raise RuntimeError(f"{name}: expected runtime assertion failure; exit {result.returncode}\n{result.stdout}")
        print(f"PASS negative {name}: {failures[0]}")
print("smc_lock_negative: PASS (4 runtime failures detected; compile failures excluded)")
