#!/usr/bin/env python3
"""Reject compiling dual-peer ownership regressions with production runtime tests."""
import os
from pathlib import Path
import subprocess
import tempfile
import sys
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'firmware/src/app/smc_mixer.c').read_text()
mutants = {
    'shared-ingress': ('smc_input_t *in = &smc_inputs[slot];', 'smc_input_t *in = &smc_inputs[0];'),
    'shared-state': ('#define smc (smc_states[smc_slot])', '#define smc (smc_states[0])'),
    'lost-solo-union': ('engine_smc_solo_targets(parts, drums[0], drums[1]);',
                        'engine_smc_solo_targets(smc.solo, smc.drum_solo[0], smc.drum_solo[1]);'),
     'reset-rearms-other-fx': (
        'memset(smc_states, 0, sizeof smc_states);\n    for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) {',
        'for (smc_slot = 0; smc_slot < SMC_PEERS; smc_slot++) {\n        memset(&smc, 0, sizeof smc);'),
    'wrong-feedback-peer': ('plat_ble_smc_send_slot(smc_slot, msg, 3)', 'plat_ble_smc_send_slot(0, msg, 3)'),
}
selected = sys.argv[1:] or list(mutants)
if any(name not in mutants for name in selected):
    raise SystemExit('Unknown mutation selector')
with tempfile.TemporaryDirectory(prefix='smc-dual-negative-') as directory:
    root = Path(directory)
    for name in selected:
        before, after = mutants[name]
        if before not in source:
            raise RuntimeError('Missing mutation site: ' + name)
        candidate = root / (name + '.c')
        candidate.write_text(source.replace(before, after))
        output = root / name
        env = dict(os.environ, SMC_UI_SOURCE_FILE=str(candidate), SMC_MULTI_OUT=str(output))
        result = subprocess.run(['sh', 'tests/host/smc_multi_peer_test.sh'], cwd=ROOT,
                                env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if not (output / 'test').is_file() or 'error:' in result.stdout:
            raise RuntimeError('Compiler rejection is not a negative PASS: ' + name + '\n' + result.stdout[-5000:])
        if result.returncode == 0 or 'FAIL ' not in result.stdout:
            raise RuntimeError('Mutation survived or did not fail an assertion: ' + name + '\n' + result.stdout[-5000:])
        print('PASS rejected runtime mutant', name, next(line for line in result.stdout.splitlines() if line.startswith('FAIL ')), flush=True)
print(f'PASS: {len(selected)} compiling dual-peer ownership mutants rejected')
