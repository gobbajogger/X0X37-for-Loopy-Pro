#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# SMK-37 pads under a settings list or HELP: every pad action still fires, and the
# pad LED picture is the same as with the settings closed (whole app, host build).
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${X0X_PADS_TEST_OUT:-build/host/pads-in-settings}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
python3 - "$OUT" <<'PY'
import sys
from pathlib import Path
root=Path('.').resolve();out=Path(sys.argv[1])
s=(root/'tests/host/rec_live_test.c').read_text().split('int main(void)')[0]
s=s.replace('#include "../../host/x0x_host.c"',f'#include "{root}/host/x0x_host.c"')
s+='\n'+(root/'tests/host/pads_in_settings_cases.inc').read_text()
(out/'probe.c').write_text(s)
PY
${CC:-cc} ${SAN_FLAGS:-} -O1 -g -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1 \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    "$OUT/probe.c" $ORIGINAL_SOURCES -lm -o "$OUT/test"
"$OUT/test"
