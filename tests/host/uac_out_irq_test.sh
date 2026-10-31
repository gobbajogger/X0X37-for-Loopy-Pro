#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production USB audio receive/mix under a deterministic nested TIMER5 schedule.
# SRC can point at the pinned unpatched source for the Orchestrator's RED run.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
OUT=$(mktemp -d "$PWD/build/host/uac-out-irq.XXXXXX")
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
mkdir -p "$OUT/src" "$OUT/hal"
cp tests/host/uac_out_irq_fake_usb.h "$OUT/hal/fm1_usb.h"
python3 - "${SRC:-firmware/src}/usb.c" "$OUT/src/usb.c" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
marker = "        uo.pos += uo.step;\n"
if source.count(marker) != 1:
    raise SystemExit("uac-out-irq: expected exactly one mixer frame boundary")
# Instrument only the temporary copy. Keep production RX, ring and DSP logic.
Path(sys.argv[2]).write_text(source.replace(marker, marker + "        uac_out_test_tick();\n"))
PY
"${CC:-cc}" -std=gnu99 -O2 -w ${CFLAGS:-} -I"$OUT" -o "$OUT/uac_out_irq_test" \
    tests/host/uac_out_irq_test.c -lm
"$OUT/uac_out_irq_test"
