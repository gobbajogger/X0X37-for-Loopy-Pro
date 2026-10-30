#!/usr/bin/env python3
"""Compile real SMC note/UI functions with stub hardware and verify event ownership.

The production functions are extracted each time. This is a host-focused
regression, not a physical device/firmware/bit-exact audio validation.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "firmware/src/app/smc_mixer.c").read_text()
template = (ROOT / "tests/host/smc_production_action_smoke.c").read_text()


def extract(name: str) -> str:
    match = re.search(
        r"^(?:static )?(?:void|int|uint8_t|uint32_t) " + re.escape(name) +
        r"\([^;\n]*\)\n\{", source, flags=re.M)
    if not match:
        raise RuntimeError(f"production function missing: {name}")
    depth = 0
    start = source.index("{", match.start())
    for i in range(start, len(source)):
        depth += (source[i] == "{") - (source[i] == "}")
        if depth == 0:
            return source[match.start():i + 1]
    raise RuntimeError(f"production function not closed: {name}")


begin = "/* PRODUCTION_FUNCTIONS_BEGIN */"
end = "/* PRODUCTION_FUNCTIONS_END */"
if template.count(begin) != 1 or template.count(end) != 1:
    raise RuntimeError("action smoke template must mark one replaceable source slot")
body = "\n\n".join(extract(name) for name in (
    "smc_is_solo_release", "smc_held", "smc_ui_active", "smc_button", "smc_release"))
combined = template.replace(begin + "\n" + end, begin + "\n" + body + "\n" + end)

out = Path(os.environ.get("SMC_ACTION_OUT", ROOT / "build/host/smc-action-source"))
out.mkdir(parents=True, exist_ok=True)
(out / "test.c").write_text(combined)
cc = shlex.split(os.environ.get("CC", "cc"))
flags = shlex.split(os.environ.get("SAN_FLAGS", ""))
cmd = [*cc, "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
       "-pedantic", *flags, "-I" + str(ROOT / "firmware/src"),
       str(out / "test.c"), "-o", str(out / "test")]
subprocess.run(cmd, check=True, cwd=ROOT)
subprocess.run([str(out / "test")], check=True, cwd=ROOT)
