#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Standalone inactive arithmetic component. No original binary/table/device input.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?usage: test.sh OUTPUT_DIR};mkdir -p "$OUT";OUT=$(cd "$OUT" && pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/wobble tests/host/miami_decimator_component_test.c experiments/wobble/miami_decimator_component.c -o "$OUT/$mode"
 "$OUT/$mode"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/wobble tests/host/miami_decimator_adapter_test.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_decimator_adapter.c -o "$OUT/adapter-$mode"
 "$OUT/adapter-$mode"
done
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -pedantic -c experiments/wobble/miami_decimator_component.c -o "$OUT/component.o"
size "$OUT/component.o"
"$OUT/normal" --bench

${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -pedantic -c experiments/wobble/miami_decimator_adapter.c -o "$OUT/adapter.o"
size "$OUT/adapter.o"

"$OUT/adapter-normal" --bench
