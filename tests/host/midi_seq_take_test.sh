#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host check: firmware usb.c midi_seq_take skips 0 across the 16-bit wrap.
# The function is compiled from usb.c. Device and native builds are not this script.
set -eu
cd "$(dirname "$0")/../.."
python3 tests/host/midi_seq_take_test.py
