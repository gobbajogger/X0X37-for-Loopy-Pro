#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Independent original303 adapter and whole-app regression; never device I/O.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
    -Ifirmware/src tests/host/synth_engine_review_test.c firmware/src/dsp/bass303.c \
    -lm -o build/host/synth_engine_review_test
build/host/synth_engine_review_test
sh host/build_host.sh --generate-only
python3 - <<'PY'
import hashlib, os, pathlib, subprocess
base_ref=os.environ.get('SYNTH_REVIEW_BASE','08d3164a265123cb4c84e5104642fd93257d63d7')
baseline=pathlib.Path('build/host/baseline_engine.c')
baseline.write_bytes(subprocess.check_output(['git','show',base_ref+':firmware/src/app/engine.c']))
units='firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c'.split()
flags=['-O2','-ffp-contract=off','-std=c99','-Wall','-Wextra','-Wno-unused-function','-Wno-unused-parameter','-DX0X_HOST','-D_DARWIN_C_SOURCE','-Ifirmware/src','-Ifirmware/src/app','-Ifirmware/src/dsp','-Ibuild/gen']
for name,engine in [('base',str(baseline)),('adapter','firmware/src/app/engine.c')]:
    exe=f'build/host/review_{name}'
    subprocess.run([os.environ.get('CC','cc')]+flags+['host/x0x_host.c']+units+[engine,'-lm','-o',exe],check=True)
    out=pathlib.Path(f'build/host/review-{name}');out.mkdir(exist_ok=True)
    subprocess.run([exe,'tests/scenarios/synth_engine_regression.x0x',str(out)],check=True)
a=pathlib.Path('build/host/review-base/adapter-equivalence.wav').read_bytes()
b=pathlib.Path('build/host/review-adapter/adapter-equivalence.wav').read_bytes()
assert a==b, 'whole-app303 WAV regression'
print(f'PASS integrated whole-app original/adapter WAV identical: {len(a)} bytes sha256={hashlib.sha256(a).hexdigest()} base={base_ref}')
PY
