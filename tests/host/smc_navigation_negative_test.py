#!/usr/bin/env python3
"""Runtime mutants of the production dispatch must fail the navigation fixture."""
import os, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
source=(root/'firmware/src/app/smc_mixer.c').read_text()
mutants={
 'keyboard-redirects-locked-fader':('if (smc.locked) return smc.lock_fader[n];','if (smc.locked) return smc_home_target(n);'),
 'SMC-navigation-does-not-resume':('smc_resuming = 1; smc_resume(); smc.locked = 0;','smc_resuming = 1; /* missing resume */ smc.locked = 0;'),
 'SMC-navigation-does-not-recapture':('if (ui.overlay != O_ASK) smc_capture();','if (0) smc_capture();'),
 'wrong-send-bank':('smc.bank == 1 ? MX_REV : MX_DLY','smc.bank == 1 ? MX_DLY : MX_REV'),
 'held-mode-key-cycles':('if (smc.nav_down & bit) return;','if (0) return;'),
 'page-does-not-wrap':('(cur_page() + d + pg.n) % pg.n','cur_page()'),
}
with tempfile.TemporaryDirectory(prefix='smc-nav-mutants-') as tmp:
 for name,(before,after) in mutants.items():
  if source.count(before)!=1: raise SystemExit(f'{name}: mutation anchor count {source.count(before)}')
  path=Path(tmp)/(name+'.c');path.write_text(source.replace(before,after))
  env=dict(os.environ,SMC_UI_SOURCE_FILE=str(path),SMC_UI_TEST_FILE='tests/host/smc_navigation_test.c',SMC_UI_OUT=str(Path(tmp)/name))
  result=subprocess.run(['sh','tests/host/smc_ui_test.sh'],cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  if result.returncode==0 or 'FAIL tests/host/' not in result.stdout: raise SystemExit(name+': failed to detect as runtime assertion\n'+result.stdout)
  print('PASS runtime mutant detected:',name)
