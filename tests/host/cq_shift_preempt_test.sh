#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# A replaced command moves one slot per masked step. The consumer must not see a tear.
# Device and native builds are not this script.
set -eu
cd "$(dirname "$0")/../.."
python3 tests/host/cq_shift_preempt_test.py
