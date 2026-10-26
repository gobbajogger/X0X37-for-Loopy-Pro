#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
#
# Build the Bluetooth controller image (ctl.img) on YOUR computer. Nothing of it is distributed
# with X0X: this script fetches JieLi's files from JieLi's own repository at a pinned commit,
# checks each SHA-256, builds the image and checks it. See ble/README.md and LICENSING.md.
#
#   ble/ctl/build_ctl.sh            fetch, build, check: ble/ctl/out/ctl.img
#
# Needs the JieLi pi32v2 toolchain (tools/get_toolchain.sh; Linux x86-64 only) and either Linux
# x86-64 or Docker (any OS: an x86-64 container; on an ARM Mac it runs emulated).
#   JIELI_TOOLCHAIN  the toolchain root (default ~/.jieli/toolchain)
#   CTL_NATIVE=1     build without Docker (Linux x86-64 only)
#   X0X_JOBS=1      compile serially (default 4; range 1..64)
#   CTL_NO_CACHE=1   keep output local; do not replace the shared installer cache
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
TC=${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}
[ -x "$TC/pi32v2/bin/clang" ] || { echo "No JieLi toolchain at $TC (tools/get_toolchain.sh)." >&2; exit 1; }
python3 "$REPO/tools/check_locked_settings.py"   # regression guard (tools/locked_settings.json)
sh "$HERE/fetch_sdk.sh"
python3 "$HERE/gen_rf_tables.py"
rm -f "$HERE/out/ctl.elf" "$HERE/out/ctl.img"
if [ "${CTL_NATIVE:-0}" = 1 ]; then
  TC="$TC" REPO="$REPO" sh "$HERE/build_in_container.sh"
else
  command -v docker >/dev/null || { echo "Docker is needed (or CTL_NATIVE=1 on Linux x86-64)." >&2; exit 1; }
  docker run --rm --platform linux/amd64 -e X0X_JOBS="${X0X_JOBS-4}" -v "$TC:/tc:ro" -v "$REPO:/w" debian:bookworm-slim \
    sh /w/ble/ctl/build_in_container.sh
fi
python3 "$HERE/mkimage.py" "$HERE/out/ctl.elf" "$HERE/out/ctl.img"
# a copy for tools/fm1_install.py, which adds it to each SMK-37 install by itself (--no-ctl skips it)
if [ "${CTL_NO_CACHE:-0}" != 1 ]; then
  mkdir -p "$HOME/.config/x0x" && cp "$HERE/out/ctl.img" "$HOME/.config/x0x/ctl.img"
  echo "Controller cache updated: $HOME/.config/x0x/ctl.img"
fi
echo "Done: $HERE/out/ctl.img"
