#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A controller failure in a poll must give core 1 back to the app (BLE-13, 2026-10-31).
Source check: the poll entry runs through ctl_asm_guard_call, ctl_fail unwinds a poll, and
ble_start shows OFF when the controller does not start this boot."""
import re, sys
from pathlib import Path
R = Path(__file__).resolve().parents[2]
m = (R / "ble/ctl/src/ctl_main.c").read_text()
a = (R / "ble/ctl/src/ctl_port_asm.S").read_text()
b = (R / "firmware/src/ble.c").read_text()
bad = []
poll = re.search(r"void ctl_entry_poll\(void\)\s*\{(.*?)\n\}", m, re.S)
if not poll or "ctl_asm_guard_call" not in poll.group(1) or "if (failed)" not in poll.group(1):
    bad.append("ctl_entry_poll: not guarded, or polls after a failure")
fail = re.search(r"void ctl_fail\(u32 err\)\s*\{(.*?)\n\}", m, re.S)
if not fail or "ctl_asm_unwind" not in fail.group(1):
    bad.append("ctl_fail: no unwind to the app in a poll")
elif fail.group(1).find("ctl_asm_unwind") > fail.group(1).find("for (;;)"):
    bad.append("ctl_fail: the endless loop comes before the unwind")
for s in ("ctl_asm_guard_call:", "ctl_asm_unwind:"):
    if s not in a:
        bad.append("ctl_port_asm.S: no " + s)
st = re.search(r"if \(!ble_boot_on\)\s*\{(.*?)return;", b, re.S)
if not st or "XB_ST_OFF" not in st.group(1):
    bad.append("ble_start: no OFF state when the controller does not start")
for x in bad:
    print("FAIL", x)
print("ble-ctl-fail-return:", "FAIL" if bad else "PASS")
sys.exit(1 if bad else 0)
