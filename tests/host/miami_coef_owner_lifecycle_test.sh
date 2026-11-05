#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Strict non-UI actual-owner lifecycle check. No aggregate runner is invoked.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/miami-coef-owner-lifecycle}
mkdir -p "$OUT"
CC=${CC:-cc}
case "$(uname -s)" in
 Darwin) gc_sections="-Wl,-dead_strip" ;;
 *) gc_sections="-Wl,--gc-sections" ;;
esac
# SAN_FLAGS is optional and is passed to both compilation and linking.
# Deliberate word splitting permits a caller's space-separated sanitizer flags.
"$CC" -std=c99 -O2 -Wall -Wextra -Werror \
 -Wmissing-braces -Wmissing-field-initializers \
 -fno-fast-math -ffp-contract=off -frounding-math \
 -ffunction-sections -fdata-sections $gc_sections ${SAN_FLAGS:-} \
 -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV \
 -Iexperiments/original_synth -Iexperiments/wobble -Ifirmware/src/dsp \
 tests/host/miami_coef_owner_lifecycle_test.c \
 firmware/src/dsp/original_miami_backend.c \
 experiments/original_synth/chicago_allocator.c \
 experiments/original_synth/chicago_arp_control.c \
 experiments/original_synth/chicago_global_mg.c \
 experiments/original_synth/chicago_key_admission.c \
 experiments/original_synth/chicago_scheduler.c \
 experiments/original_synth/chicago_selector.c \
 experiments/original_synth/control_math.c \
 experiments/original_synth/fixed_fx.c \
 experiments/original_synth/fused_reference.c \
 experiments/original_synth/master.c \
 experiments/original_synth/miami_control_owner.c \
 experiments/original_synth/miami_fx_chain.c \
 experiments/original_synth/miami_geometry.c \
 experiments/original_synth/miami_ms20.c \
 experiments/original_synth/miami_note_context.c \
 experiments/original_synth/miami_output.c \
 experiments/original_synth/miami_pitch.c \
 experiments/original_synth/miami_program.c \
 experiments/original_synth/miami_selected_voice.c \
 experiments/original_synth/miami_voice_owner.c \
 experiments/original_synth/miami_voice_snapshot.c \
 experiments/original_synth/oscillator_motion.c \
 experiments/original_synth/program_envelope.c \
 experiments/original_synth/state_codec.c \
 experiments/original_synth/voice_gain.c \
 experiments/wobble/miami_decimator_adapter.c \
 experiments/wobble/miami_decimator_component.c \
 experiments/wobble/miami_fx_conversion.c \
 -lm -o "$OUT/owner-lifecycle"
"$OUT/owner-lifecycle"
