#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import subprocess,hashlib
root=Path(__file__).resolve().parents[2];q=root/'build/wobble-independent-mix';q.mkdir(parents=True,exist_ok=True)
engine=(root/'firmware/src/app/engine.c').read_text()
wrapper=q/'exact_engine_wrapper.c';wrapper.write_text('#include "'+str(root/'firmware/src/app/engine.c')+'"\nvoid qa_stereo(void){wobble_set(&dev_wobble,W_STEREO,1.0f);}\n')
ref=engine.replace('static wobble_t dev_wobble;','static wobble_t dev_wobble;\nstatic float qa_right[2][256];')
ref=ref.replace('#if !X0X_DEV_WOBBLE\n    for (b = 0;', '    for (b = 0;')
ref=ref.replace('        synth_render(&bass_slot[b], bb[b], (int)n);','        if (!wobble_backend_stereo(&bass_slot[b], bb[b], qa_right[b], (int)n))\n            synth_render(&bass_slot[b], bb[b], (int)n);',1)
ref=ref.replace('    }\n#endif\n    pt = plat_cycles();','    }\n    pt = plat_cycles();')
start=ref.index('#if X0X_DEV_WOBBLE\n        /* Render/mix one part at a time')
end=ref.index('#endif',start)+len('#endif')
ref=ref[:start]+ref[end:]
ref=ref.replace('bb[1-b]','qa_right[b]')
ref=ref.replace('#include "../','#include "').replace('#include "engine.h"','#include "app/engine.h"').replace('#include "plat.h"','#include "app/plat.h"')
ref+='\nvoid qa_stereo(void){wobble_set(&dev_wobble,W_STEREO,1.0f);}\n'
(q/'reference_engine.c').write_text(ref)
host=(root/'host/x0x_host.c').read_text().replace('"../firmware/','"'+str(root/'firmware')+'/')
host=host.replace('        if (!strcmp(cmd, "wait"))','        if (!strcmp(cmd,"qa_stereo")) qa_stereo();\n        else if (!strcmp(cmd, "wait"))')
host='void qa_stereo(void);\n'+host
(q/'qa_host.c').write_text(host)
sc=(root/'tests/scenarios/synth_engine_regression.x0x').read_text().replace('load 0','load 0\nqa_stereo\nparam 5 2 1 100\nparam 5 2 2 95')
(q/'side-sends.x0x').write_text(sc)
units='firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c experiments/wobble/wobble.c'.split()
flags=['-O2','-ffp-contract=off','-std=c99','-Wall','-Wextra','-Wno-unused-function','-Wno-unused-parameter','-DX0X_HOST','-D_DARWIN_C_SOURCE','-DX0X_DEV_PADCAL=1','-DX0X_DEV_WOBBLE=1','-Ifirmware/src','-Ifirmware/src/app','-Ifirmware/src/dsp','-Ibuild/gen']
for name,src in [('side_exact',wrapper),('side_reference',q/'reference_engine.c')]:
 exe=q/name;out=q/(name+'-render');out.mkdir(exist_ok=True)
 subprocess.run(['clang']+flags+[str(q/'qa_host.c')]+units+[str(src),'-lm','-o',str(exe)],check=True)
 subprocess.run([str(exe),str(q/'side-sends.x0x'),str(out)],check=True)
a=(q/'side_exact-render/adapter-equivalence.wav').read_bytes();b=(q/'side_reference-render/adapter-equivalence.wav').read_bytes();assert a==b
print('PASS exact sequential scratch mixer equals independent dedicated planar arrays with nonzero side and sends:',len(a),hashlib.sha256(a).hexdigest())
