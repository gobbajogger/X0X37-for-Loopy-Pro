#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Build Felucca on macOS (see BUILDING.md).
#   ./build.sh [--release X.Y] [--board smk37]
#   BOARD            target board: smk37 (default; SMK-37 Elite/Pro). FM-1 is no longer built (owner 2026-10-30).
#   MODEL            smk37: pro (default), elite or mke: the model name the keyboard reports
#   JIELI_TOOLCHAIN  JieLi Linux toolchain (default: ~/.jieli/toolchain)
#   AC79_SDK         JieLi AC79 SDK checkout (default: ~/fw-AC79_AIoT_SDK)
set -e
cd "$(dirname "$0")"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
PY="${PYTHON:-python3}"

"$PY" -c 'import PIL' 2>/dev/null || { echo "build.sh: $PY has no Pillow (pip3 install Pillow)"; exit 1; }
[ -x "$JIELI_TOOLCHAIN/pi32v2/bin/clang" ] || { echo "build.sh: no toolchain in $JIELI_TOOLCHAIN (run tools/get_toolchain.sh)"; exit 1; }
[ -f "$AC79_SDK/cpu/wl82/tools/uboot.boot" ] || { echo "build.sh: no JieLi AC79 SDK in $AC79_SDK"; exit 1; }
if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    docker info >/dev/null 2>&1 || { echo "build.sh: Docker is not running"; exit 1; }
fi
export BOARD="${BOARD:-smk37}"   # read by tools/build.py (--board overrides it)
case "$BOARD $*" in *fm1*|*FM1*|*fm-1*) echo "build.sh: FM-1 is not a build target; use --board smk37 (MODEL=elite|pro)"; exit 2 ;; esac
exec "$PY" tools/build.py "$@"
