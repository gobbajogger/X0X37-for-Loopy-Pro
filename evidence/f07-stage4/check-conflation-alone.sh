#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# FP-001 negative: installer_sha256 alone must fail closed (non-zero exit).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT
cat >"$TMP" <<'JSON'
{
  "schema": 1,
  "installer_sha256": "8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157",
  "install_url": "https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/"
}
JSON
set +e
python3 "$ROOT/tools/check_installer_digests.py" "$TMP"
rc=$?
set -e
if [[ "$rc" -eq 0 ]]; then
  echo "conflation_alone unexpectedly passed" >&2
  exit 1
fi
echo "conflation_alone_rejected exit=$rc"
exit 0
