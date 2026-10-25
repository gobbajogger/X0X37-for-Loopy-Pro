#!/usr/bin/env python3
"""Require production integration assertions to reject broken routing/toggles."""
import os
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/app/smc_mixer.c').read_text()
mutants = {
    'play-no-pause': ('if ((seq.playing || seq.req_start || seq.req_cont) && !seq.req_stop)', 'if (0)'),
    'select-no-repeat': ('int repeat_drum = strip < NKIT && ui.part == strip && ui.overlay == O_NONE;', 'int repeat_drum = 0;'),
    'lock-redirect-fader': ('if (smc.locked) return smc.lock_fader[n];', 'if (smc.locked) return smc_home_target(n);'),
    'lock-redirect-knob': ('r = smc.lock_knob[n];', 'r = NONE;'),
}
with tempfile.TemporaryDirectory(prefix='smc-interaction-negative-') as tmp:
    for name, (before, after) in mutants.items():
        assert source.count(before) == 1, name
        path = Path(tmp) / (name + '.c')
        path.write_text(source.replace(before, after, 1))
        env = dict(os.environ, SMC_UI_SOURCE_FILE=str(path), SMC_LOCK_OUT=str(Path(tmp) / name))
        result = subprocess.run(['sh', 'tests/host/smc_lock_test.sh'], cwd=root, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (Path(tmp) / (name + '.log')).write_text(result.stdout)
        if result.returncode == 0 or 'FAIL tests/host/smc_lock_test.c:' not in result.stdout:
            raise SystemExit(f'{name}: expected runtime assertion, got {result.returncode}\n{result.stdout}')
        print('PASS rejected runtime mutation:', name)
