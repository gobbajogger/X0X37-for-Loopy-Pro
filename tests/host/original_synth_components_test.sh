#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Closed original components, local/private reference tests; no active firmware.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT" && pwd)
SRC='experiments/original_synth/master_unmuted.c experiments/original_synth/fixed_fx.c experiments/original_synth/miami_fx_chain.c experiments/original_synth/control_math.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_fx_conversion.c'
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_synth_components_test.c $SRC -lm -o "$OUT/$mode"
 "$OUT/$mode"
done
