#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/synth_engine
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src tests/host/synth_engine_test.c firmware/src/dsp/bass303.c -o build/synth_engine/test
build/synth_engine/test
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src tests/host/synth_registry_test.c firmware/src/dsp/bass303.c -o build/synth_engine/registry
build/synth_engine/registry
