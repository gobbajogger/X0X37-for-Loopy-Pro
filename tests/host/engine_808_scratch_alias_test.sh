# SPDX-License-Identifier: GPL-3.0-only
# 808 scratch inside the out_l/out_r window must be bit-identical to the separate
# t0/tr buffers of BASE_REF: split windows, deferred, failed and partial CPU2, ASan/UBSan.
# Needs the private original-synth inputs and sh host/build_host.sh --generate-only.
set -e
cd "$(dirname "$0")/../.."; D=${OUT:-build/host}/engine-808-alias; rm -rf $D; mkdir -p $D; BASE=${BASE_REF:-origin/build/0.0.22}
git show $BASE:firmware/src/app/engine.c > $D/engine-base.c
U=$(python3 - <<'PY'
import importlib.util
spec=importlib.util.spec_from_file_location('b','tools/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
b.synth_build_config();b.require_original_synth_sources()
print(' '.join(['firmware/src/dsp/drum909.c','firmware/src/dsp/drum808.c','firmware/src/dsp/x0x_param.c','firmware/src/dsp/breaks.c','firmware/src/dsp/fxbus.c','firmware/src/dsp/master.c','firmware/src/seq/sequencer.c','firmware/src/seq/tb3po.c','firmware/src/seq/pattern.c','firmware/src/seq/motion.c','firmware/src/app/factory_banks.c',*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS]))
PY
)
for mode in normal san; do
 for dw in 1 0; do
 F="-O1 -g -ffp-contract=off -std=c99 -w -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=$dw -fno-fast-math -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -D_POSIX_C_SOURCE=200809L -Iexperiments/original_synth -Iexperiments/wobble -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen"
 [ $mode = san ] && F="$F -fsanitize=address,undefined -fno-sanitize-recover=all"
 for v in base new; do
  src=$D/engine-base.c; [ $v = new ] && src=firmware/src/app/engine.c
  cc $F -Dseq_until_event=output_window_until_event -Dplat_cpu2_run=output_window_cpu2_run -Dplat_cpu2_wait=output_window_cpu2_wait -c $src -o $D/eng-$v.o
  cc $F tests/host/engine_output_window_test.c $U $D/eng-$v.o -lm -o $D/h-$v
  ASAN_OPTIONS=detect_leaks=0 $D/h-$v $D/$mode-$dw-$v.pcm | tail -1
 done
 cmp $D/$mode-$dw-base.pcm $D/$mode-$dw-new.pcm && echo "BITEXACT $mode DEV_WOBBLE=$dw $(stat -c%s $D/$mode-$dw-new.pcm) B $(sha256sum < $D/$mode-$dw-new.pcm | cut -c1-16)"
 done
done
