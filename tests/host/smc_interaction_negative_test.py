#!/usr/bin/env python3
"""Require production integration assertions to reject broken routing/toggles."""
import os
from pathlib import Path
import subprocess
import tempfile
import sys
root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/app/smc_mixer.c').read_text()
mutants = {
    'pause-unbound': ('if (!mmc && note == 0x5Du) {', 'if (0) {'),
    'pause-no-release': ('if (note == 0x5Du) smc_lock_button(0);', 'if (0) smc_lock_button(0);'),
    'pause-no-drop-recovery': ('            else smc.lock_down = 0;', '            else /* negative: lost Pause release stays parked */;'),
    'pause-old-stop-led': ('return smc.locked || beat;', 'return !seq.playing;'),
    'play-no-pause': ('if ((seq.playing || seq.req_start || seq.req_cont) && !seq.req_stop)', 'if (0)'),
    'fx-button-navigates': ('if (smc_fx_try(note, mmc))\n        return;', 'if (0 && smc_fx_try(note, mmc))\n        return;'),
    'lock-redirect-fader': ('if (smc.locked) return smc.lock_fader[n];', 'if (smc.locked) return smc_home_target(n);'),
    'lock-redirect-knob': ('r = smc.lock_knob[n];', 'r = NONE;'),
    'select-led-fx-only': ('if (row == 3 && view == V_FX && ui.overlay == O_NONE && !smk_clip_mode_on())', 'if (row == 3)'),
}
selected = sys.argv[1:] or list(mutants)
if any(name not in mutants for name in selected):
    raise SystemExit('Unknown mutation selector: ' + ', '.join(selected))
with tempfile.TemporaryDirectory(prefix='smc-interaction-negative-') as tmp:
    for name in selected:
        before, after = mutants[name]
        assert source.count(before) == 1, name
        path = Path(tmp) / (name + '.c')
        path.write_text(source.replace(before, after, 1))
        led = name == 'select-led-fx-only'
        runner = 'smc_select_mode_test' if led else 'smc_lock_test'
        env = dict(os.environ, SMC_UI_SOURCE_FILE=str(path), SMC_LOCK_OUT=str(Path(tmp) / name),
                   SMC_SELECT_OUT=str(Path(tmp) / name))
        result = subprocess.run(['sh', 'tests/host/' + runner + '.sh'], cwd=root, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (Path(tmp) / (name + '.log')).write_text(result.stdout)
        if result.returncode == 0 or ('FAIL tests/host/' + runner + '.c:') not in result.stdout:
            raise SystemExit(f'{name}: expected runtime assertion, got {result.returncode}\n{result.stdout}')
        print('PASS rejected runtime mutation:', name)
