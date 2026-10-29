#!/usr/bin/env python3
"""Source and locked-config contract for an intentionally single-peer SMC build.

This checks the exact input tree, not controller RAM use, device radio, or firmware.
"""
import json
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / 'firmware/src/ble/ble_midi_central.h').read_text()
source = (ROOT / 'firmware/src/ble/ble_midi_central.c').read_text()
config = (ROOT / 'ble/ctl/src/config.c').read_text()
locked = json.loads((ROOT / 'tools/locked_settings.json').read_text())
settings = next(s['value'] for s in locked['settings'] if s['name'] == 'ctl_config')

def parse_config(text):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    return {k: re.sub(r'\s+', ' ', v.strip()) for k, v in re.findall(
        r'^[A-Za-z_][\w \t\*]*?\b(\w+)\s*=\s*([^;{]+);', text, re.M)}

actual = parse_config(config)
assert actual == settings, sorted((k, actual.get(k), settings.get(k))
                                  for k in actual.keys() | settings.keys()
                                  if actual.get(k) != settings.get(k))
assert actual['config_btctler_le_hw_nums'] == '1'
assert actual['config_btctler_le_acl_total_nums'] == '5'
assert actual['config_btctler_le_master_multilink'] == '0'
assert '#define BLEC_ONE_PEER_RECOVERY 1' in header
assert 'void blec_pair_rx(blec_t *a, blec_t *b' in source
assert '(void)b; if (p) blec_rx(a, h4, p, n, now);' in source
assert 'if (!want) blec_stop(a); blec_poll(a, now);' in source
app = (ROOT / 'firmware/src/ble.c').read_text()
assert re.search(r'if\s*\(slot\s*>=\s*1u\)\s*return\s+0;', app)
assert 'blec_pair_poll(a, b, fm1_ms, want);' in app
boot = (ROOT / 'firmware/src/app/main_fm1.c').read_text()
assert 'ble_prepare(1);' in boot and 'ble_start();' in boot
assert 'plat_ble_set(proj.set.bt & 1u)' in boot
project = (ROOT / 'firmware/src/app/project.c').read_text()
assert 'device_bt = set.bt;' in project
assert 'rc == -1 && !(device_bt & PROJ_DEVICE_BT)' in project
assert 'PROJ_BT_SMC_CENTRAL' in project
print('single-peer controller: source/locked, slot 0, saved role, boot defaults PASS')
