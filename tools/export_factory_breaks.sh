#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Export the designated stock ADPCM without regenerating or replacing its source bytes.
# Default firmware keeps an empty bank; installation uses the post-build code tail.
set -e
cd "$(dirname "$0")/.."
python3 tools/break_pack.py --from-export assets/factory-breaks \
    --bin assets/factory-breaks/factory-break-pack.bin \
    --js web/factory_break_pack.js
