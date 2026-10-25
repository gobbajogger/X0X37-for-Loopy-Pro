#!/bin/sh
# Production UI/engine/SMC muted fader policy; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
OUT=${SMC_MUTED_OUT:-build/host/smc-muted-fader}
mkdir -p "$OUT"
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' "${SMK_FADER_SOURCE:-firmware/src/app/main_fm1.c}" > "$OUT/smk_muted_faders.inc"
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
printf '#include "%s/firmware/src/app/engine.c"\n' "$PWD" > "$OUT/engine.c"
cat >> "$OUT/engine.c" <<'WRAPPER'
#include <math.h>
#include <string.h>
float muted_test_fx_gain(int which) {
    float gain = which == 0 ? fx.rv_level : which == 1 ? fx.dl_level : fx.volume;
    return fx.muted & (1u << which) ? 0.0f : gain;
}
float muted_test_signal(int which,int stereo) {
    float dry[256], rev[256], dly[256], left[256], right[256], energy=0;
    for(int j=0;j<256;j++){float x=j&16?.1f:-.1f;dry[j]=which==2?x:0;rev[j]=which==0?x:0;dly[j]=which==1?x:0;}
    /* Let the actual reverb/delay ring fill. Repeated deterministic input
     * observes effective DSP output, including a preexisting effect tail. */
    for(int block=0;block<300;block++) {
        if(stereo)fxbus_process_st(&fx,dry,dry,rev,dly,left,right,256);
        else {fxbus_process(&fx,dry,rev,dly,left,256);memset(right,0,sizeof right);}
        if(block>250)for(int j=0;j<256;j++)energy+=fabsf(left[j])+fabsf(right[j]);
    }
    return energy;
}
WRAPPER
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c"
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
SOURCE=${SMC_UI_SOURCE_FILE:-$PWD/firmware/src/app/smc_mixer.c}
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
 -DX0X_HOST $DARWIN ${SAN_FLAGS:-} -DSMC_SOURCE_FILE=\"$SOURCE\" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
 tests/host/smc_muted_fader_test.c "$OUT/engine.c" $U -lm -o "$OUT/test"
"$OUT/test" late
"$OUT/test"
