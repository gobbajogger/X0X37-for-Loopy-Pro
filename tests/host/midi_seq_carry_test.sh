#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host check: usb.c compaction carries each accept sequence with its packet.
# Device and native builds are not this script.
set -eu
cd "$(dirname "$0")/../.."
python3 tests/host/midi_seq_carry_test.py
