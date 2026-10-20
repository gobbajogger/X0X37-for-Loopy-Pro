#!/usr/bin/env python3
"""Run affected host checks serially and retain each output and exit status."""
import json, os, subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[2]
out = Path(__file__).resolve().parent
commands = [
 ['python3','tests/host/smc_mapping_test.py'],
 ['python3','tests/host/smc_fader_test.py'],
 ['python3','tests/host/smc_buttons_test.py'],
 ['python3','tests/host/smc_filter_test.py'],
 ['sh','tests/host/smc_part_select_test.sh'],
 ['sh','tests/host/smc_reachability_test.sh'],
 ['sh','tests/host/smc_clip_visibility_test.sh'],
 ['sh','tests/host/smc_drum_mutes_test.sh'],
 ['sh','tests/host/smc_performance_abort_test.sh'],
]
env = dict(os.environ, X0X_HOST_DEFS='-D_DARWIN_C_SOURCE', PYTHONDONTWRITEBYTECODE='1')
results = []
for argv in commands:
 name = Path(argv[1]).stem
 log = out / (name + '.log')
 with log.open('w') as stream:
  p = subprocess.run(argv, cwd=root, env=env, stdout=stream, stderr=subprocess.STDOUT)
 results.append(dict(argv=argv, returncode=p.returncode, log=str(log)))
 print(name, p.returncode, flush=True)
(out/'host-results.json').write_text(json.dumps(results, indent=2)+'\n')
raise SystemExit(any(r['returncode'] for r in results))
