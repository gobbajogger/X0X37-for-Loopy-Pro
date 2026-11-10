#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host check: live-key panic order stays signed across the free-running wrap.
# Device and native builds are not this script.
set -eu
cd "$(dirname "$0")/../.."
python3 tests/host/live_key_wrap_test.py
