#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host keyboard and SMC panic routes. Device and native runs are not this script.
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-route.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
if [ ! -f build/gen/felucca_font.h ] || [ ! -f build/gen/x0x_drum_samples.h ] || \
   [ ! -f build/gen/x0x_drum_tables.h ] || [ ! -f build/gen/x0x_break_bank.h ]; then
    sh host/build_host.sh --generate-only
fi
units="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c experiments/original_synth/chicago_allocator.c experiments/original_synth/chicago_arp_control.c experiments/original_synth/chicago_control_owner.c experiments/original_synth/chicago_drive.c experiments/original_synth/chicago_dynamics.c experiments/original_synth/chicago_engine_output.c experiments/original_synth/chicago_full_mg.c experiments/original_synth/chicago_fx2_compact.c experiments/original_synth/chicago_fx2_wrapper.c experiments/original_synth/chicago_global_mg.c experiments/original_synth/chicago_global_stage.c experiments/original_synth/chicago_key_admission.c experiments/original_synth/chicago_modulation.c experiments/original_synth/chicago_mp4.c experiments/original_synth/chicago_note_context.c experiments/original_synth/chicago_output.c experiments/original_synth/chicago_packed.c experiments/original_synth/chicago_program.c experiments/original_synth/chicago_program_output.c experiments/original_synth/chicago_scheduler.c experiments/original_synth/chicago_selector.c experiments/original_synth/chicago_smallfx.c experiments/original_synth/chicago_tuning.c experiments/original_synth/chicago_voice.c experiments/original_synth/chicago_voice_owner.c experiments/original_synth/chicago_voice_snapshot.c experiments/original_synth/control_math.c experiments/original_synth/engine_events.c experiments/original_synth/fixed_fx.c experiments/original_synth/fused_reference.c experiments/original_synth/lossless_table.c experiments/original_synth/master.c experiments/original_synth/miami_control_owner.c experiments/original_synth/miami_fx_chain.c experiments/original_synth/miami_geometry.c experiments/original_synth/miami_ms20.c experiments/original_synth/miami_note_context.c experiments/original_synth/miami_output.c experiments/original_synth/miami_pitch.c experiments/original_synth/miami_program.c experiments/original_synth/miami_selected_voice.c experiments/original_synth/miami_startup.c experiments/original_synth/miami_voice_owner.c experiments/original_synth/miami_voice_snapshot.c experiments/original_synth/oscillator_motion.c experiments/original_synth/program_envelope.c experiments/original_synth/state_codec.c experiments/original_synth/voice_gain.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_fx_conversion.c firmware/src/dsp/original_chicago_backend.c firmware/src/dsp/original_miami_backend.c firmware/src/app/original_runtime.c firmware/src/app/original_sound.c"
host_defs=
if [ "$(uname -s)" = Darwin ]; then host_defs=-D_DARWIN_C_SOURCE; fi
san=
if "${CC:-cc}" -fsanitize=address,undefined -x c -o "$out/san-probe" - <<'EOF' >/dev/null 2>&1
int main(void) { return 0; }
EOF
then
    san="-fsanitize=address,undefined -fno-sanitize-recover=all"
else
    echo "panic route sanitizer: NOT_RUN"
fi
# shellcheck disable=SC2086
"${CC:-cc}" -O1 -g $san -ffp-contract=off -std=c99 \
    -Wno-unused-function -Wno-unused-parameter -DX0X_HOST -DBOARD_SMK37=1 \
    -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=1 -DX0X_ORIGINAL_SYNTH=1 -fno-fast-math -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -Iexperiments/original_synth -Iexperiments/wobble \
    $host_defs -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/panic_route_test.c $units -lm -o "$out/route"
"$out/route"
