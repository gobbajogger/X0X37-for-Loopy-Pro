from pathlib import Path
import subprocess,tempfile,json
root=Path.cwd()
source=(root/'tests/host/smc_reachability_test.c').read_text()
source=source.replace('#include "../../host/x0x_host.c"','#include "'+str(root/'host/x0x_host.c')+'"')
marker='    fader(0,100); assert(smc.picked&1);'
assert source.count(marker)==1
source=source.replace(marker,marker+'\n    /* Stage-3 evidence: matching knob releases pickup and its crossing history. */\n    turn(0,-1); assert(!(smc.picked&1) && !(smc.seen&1));\n    fader(0,smc_scaled(r)); assert(smc.picked&1);')
runner=(root/'tests/host/smc_reachability_test.sh').read_text()
with tempfile.TemporaryDirectory() as td:
    c=Path(td)/'paired-knob.c';c.write_text(source)
    runner=runner.replace('cd "$(dirname "$0")/../.."','cd "'+str(root)+'"')
    runner=runner.replace('X0X_HOST_OUT="$OUT/host" sh host/build_host.sh > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }','test -f build/gen/felucca_font.h && test -f build/gen/x0x_drum_samples.h && test -f build/gen/x0x_break_bank.h')
    runner=runner.replace('tests/host/smc_reachability_test.c',str(c))
    sh=Path(td)/'run.sh';sh.write_text(runner)
    with (root/'evidence/smc-stage3/knob-ui/paired-knob-pickup.log').open('w') as log:
        p=subprocess.run(['sh',str(sh)],stdout=log,stderr=subprocess.STDOUT)
    if p.returncode: raise SystemExit(p.returncode)
print('PASS: new matching knobs reset paired fader pickup/crossing history; reaching new target reacquires pickup on all five injected/synthetic routes')
