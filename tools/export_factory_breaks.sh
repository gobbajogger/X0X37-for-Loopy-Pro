#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Write the two factory loops where a person can keep them, and the pack the installer offers.
# The default firmware build does not compile these bytes into the image.
set -e
cd "$(dirname "$0")/.."
mkdir -p assets/factory-breaks build/gen
sh tools/gen_builtin_break.sh build/gen assets/factory-breaks
python3 tools/break_pack.py --from-export assets/factory-breaks \
    --bin assets/factory-breaks/factory-break-pack.bin \
    --js web/factory_break_pack.js
