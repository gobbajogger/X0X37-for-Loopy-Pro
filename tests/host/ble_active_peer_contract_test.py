#!/usr/bin/env python3
"""Check the default two-peer BLE source and locked controller inputs.

Static contract only. Radio behavior requires two physical SMC devices.
"""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "firmware/src/ble/ble_midi_central.h").read_text()
central = (ROOT / "firmware/src/ble/ble_midi_central.c").read_text()
app = (ROOT / "firmware/src/ble.c").read_text()
config = (ROOT / "ble/ctl/src/config.c").read_text()
locked = json.loads((ROOT / "tools/locked_settings.json").read_text())


def read_assignments(text):
    text = re.sub(r"/\\*.*?\\*/|//[^\\n]*", "", text, flags=re.S)
    return {
        key: re.sub(r"\\s+", " ", value.strip())
        for key, value in re.findall(
            r"^[A-Za-z_][\\w \\t\\*]*?\\b(\\w+)\\s*=\\s*([^;{]+);",
            text, re.M
        )
    }


actual = read_assignments(config)
expected = next(s["value"] for s in locked["settings"] if s["name"] == "ctl_config")
assert actual == expected, sorted(
    (k, actual.get(k), expected.get(k))
    for k in actual.keys() | expected.keys()
    if actual.get(k) != expected.get(k)
)
assert actual["config_btctler_le_hw_nums"] == "2"
assert actual["config_btctler_le_acl_total_nums"] == "3"
assert actual["config_btctler_le_master_multilink"] == "1"
assert "#define BLEC_TWO_PEER 1" in header
assert "#if !BLEC_TWO_PEER" in header, "keep explicit one-peer fallback for #700"
assert "void blec_pair_bind(blec_t *a, blec_t *b)" in central
assert "void blec_pair_rx(blec_t *a, blec_t *b" in central
assert "blec_pair_bind(&ble_r.host.c, &ble_r.host.c2);" in app
assert "blec_pair_poll(a, b, fm1_ms, want);" in app
assert re.search(r"if\\s*\\(slot\\s*>=\\s*2u\\)\\s*return\\s+0;", app)
assert "blec_midi_send(slot ? &ble_r.host.c2 : &ble_r.host.c, msg, len)" in app
assert "ble_central_link_notify_slot(1, want && blec_connected(b));" in app
print("two-peer firmware defaults / locked controller / second-slot feedback: PASS")
