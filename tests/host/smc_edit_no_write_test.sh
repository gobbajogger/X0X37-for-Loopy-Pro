#!/bin/sh
# PR629 SEQ/EDIT-split consumer gate: actual SEQ writes plus EDIT audition no hidden writes.
# Intentionally requires the split UI; pre-split source is a runtime negative control.
set -eu
cd "$(dirname "$0")/../.."
export SMC_VERIFY_EDIT_NO_WRITE=1
exec sh tests/host/smc_record_feedback_test.sh
