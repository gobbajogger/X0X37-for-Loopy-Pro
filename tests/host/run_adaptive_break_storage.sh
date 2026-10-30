#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Shipping Chicago/Miami profile and revision 13; no device.
set -eu
cd "$(dirname "$0")/../.."
exec python3 tests/host/run_break_features.py storage
