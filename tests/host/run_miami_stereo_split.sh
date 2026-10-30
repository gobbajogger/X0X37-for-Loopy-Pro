#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/miami-stereo-split
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off \
  -DX0X_SYNTH_HOST_TEST -Ifirmware/src -Iexperiments/wobble \
  tests/host/miami_stereo_split_test.c \
  -lm -o build/miami-stereo-split/test
build/miami-stereo-split/test
