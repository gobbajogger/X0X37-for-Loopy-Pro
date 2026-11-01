#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compatibility entry point for checked LOCAL nightly previews only.
# A completeness PASS is not publication authority. The old path performed a
# force-push and Pages build; it is retired before any gate, Git or Pages I/O.
set -e
cd "$(dirname "$0")/.."
if [ "${1:-}" = "--nightly" ]; then
  shift
  exec python3 tools/publish_nightly.py "$@"
fi
echo "Legacy Pages publication disabled; --nightly supports checked local previews only" >&2
exit 2
