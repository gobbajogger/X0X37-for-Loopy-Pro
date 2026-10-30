#!/usr/bin/env python3
"""Extract current production page-preview helpers for a self-contained host test.

The test compiles real implementation bodies from smc_mixer.c, with only
main-loop UI service, timing, and physical hardware mocked by the harness.
It fails closed if the signature range changes or the set of functions drifts.
"""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("--source", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
s = a.source.read_text()
start_tag = "static smc_page_peek_screen_t smc_peek_screen(void)\n"
end_tag = "static void smc_notice(int n)\n"
if s.count(start_tag) != 1 or s.count(end_tag) != 1:
    p.error("expected one current SMC page-preview source range")
start = s.index(start_tag)
end = s.index(end_tag, start)
body = s[start:end]
expected = (
    "smc_peek_screen", "smc_peek_apply", "smc_peek_eligible",
    "smc_peek_begin", "smc_peek_finish", "smc_peek_tick", "smc_show",
)
for name in expected:
    if body.count("static " + ("smc_page_peek_screen_t " if name == "smc_peek_screen" else "void " if name != "smc_peek_eligible" else "int ") + name + "(") != 1:
        p.error(f"missing or repeated production {name}")
if "smc_ui_begin(n, knob)" not in body or "smc_page_peek_step(" not in body:
    p.error("production popup entry/expiry contract missing")
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(body)
print(f"extracted {len(expected)} SMC production helpers ({len(body)} UTF-8 chars)")
