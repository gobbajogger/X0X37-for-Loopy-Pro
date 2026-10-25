#!/usr/bin/env python3
"""Differential for the main-loop SMC input pointer.

The handoff input and the working tree run the same production peer script.
Host -Oz text is supporting evidence only; it is not a native fit claim.
"""
import importlib.util
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INPUT = "61a38c274dbdf7aa98c9714e26263030952fea8d"
spec = importlib.util.spec_from_file_location(
    "smc_multi_peer_test", ROOT / "tests/host/smc_multi_peer_test.py")
peer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(peer)


def text_size(source):
    code = peer.make_code(source)
    with tempfile.TemporaryDirectory(prefix="smc-input-size-") as tmp:
        tmp = Path(tmp)
        c = tmp / "test.c"
        obj = tmp / "test.o"
        c.write_text(code)
        subprocess.run(
            ["clang", "-Oz", "-std=c99", "-ffunction-sections", "-fdata-sections",
             "-c", "-I" + str(ROOT / "firmware/src"), str(c), "-o", str(obj)],
            check=True)
        out = subprocess.check_output(["size", "-t", str(obj)], text=True)
    # text data bss dec hex filename
    line = [ln for ln in out.splitlines() if ln and not ln.startswith("text")][-1]
    return int(line.split()[0]), out


def main():
    old = subprocess.check_output(
        ["git", "show", f"{INPUT}:firmware/src/app/smc_mixer.c"], cwd=ROOT, text=True)
    new = (ROOT / "firmware/src/app/smc_mixer.c").read_text()
    if "smc_input_current" not in new:
        raise SystemExit("working tree is missing the input pointer")
    old_out = peer.compile_and_run(old)
    new_out = peer.compile_and_run(new)
    if old_out != new_out:
        print("OLD", old_out)
        print("NEW", new_out)
        raise SystemExit("peer script diverged from the handoff input")
    old_text, _ = text_size(old)
    new_text, _ = text_size(new)
    print(old_out, end="")
    print(f"PASS: input-pointer differential matched the {INPUT} peer script")
    print(f"HOST_OZ supporting only: harness text {old_text} -> {new_text}, delta {new_text - old_text}")


if __name__ == "__main__":
    main()
