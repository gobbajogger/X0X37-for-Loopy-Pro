#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
REF=${1:?reference directory};OUT=${2:?output directory}
mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
python3 tools/gen_miami_program_private.py "$REF" "$OUT/miami_program_private.h" "$OUT/saved109.bin" > "$OUT/generation.json"
cmp experiments/original_synth/miami_program_private.h "$OUT/miami_program_private.h"
COMMON='experiments/original_synth/miami_program.c experiments/original_synth/fused_reference.c'
WAVE='experiments/original_synth/oscillator_motion.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 extra='';[ "$arithmetic" != soft ]||extra='-DORIGINAL_USE_SOFT_FMA'
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  for suite in program gain_domain voice_gain master miami_output;do
   case "$suite" in
    program) src='tests/host/original_miami_program_test.c';more='';;
    gain_domain) src='tests/host/original_gain_domain_test.c';more="$WAVE";;
    voice_gain) src='tests/host/original_voice_gain_test.c';more="$WAVE experiments/original_synth/voice_gain.c";;
    master) src='tests/host/original_master_lifecycle_test.c';more='experiments/original_synth/master.c experiments/wobble/miami_fx_conversion.c';;
    miami_output) src='tests/host/original_miami_output_test.c';more='experiments/original_synth/miami_output.c experiments/original_synth/master.c experiments/original_synth/miami_fx_chain.c experiments/original_synth/fixed_fx.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_fx_conversion.c';;
   esac
   exe="$OUT/$suite-$arithmetic-$mode"
   ${CC:-cc} -std=c99 $flags $extra -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble $src $COMMON $more -lm -o "$exe"
   if [ "$suite" = program ];then "$exe" "$OUT/saved109.bin" > "$exe.log";else "$exe" > "$exe.log";fi
  done
 done
done
printf 'PASS original program + real gain domain + current-gain rendering, native/software FMA and normal/ASan/UBSan\n'
