#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production storage fault tests and byte comparison with the reviewed baseline.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/storage-scratch}
mkdir -p "$OUT"
ST_BASELINE=${ST_BASELINE:-e75fb2a584aa22978f352697a5e36fc9f886c3c5}
git show "$ST_BASELINE:firmware/src/storage.c" > "$OUT/baseline-storage.c"
BASELINE_SOURCE=$(cd "$OUT" && pwd)/baseline-storage.c
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Wno-unused-function ${SAN_FLAGS:-} \
    tests/storage_scratch_test.c -o "$OUT/faults"
"$OUT/faults"
${CC:-cc} -std=c99 -O2 -DBOARD_SMK37=1 -DSTORAGE_SHARED=1 \
    tests/storage_bytes_test.c -o "$OUT/current"
"$OUT/current" "$OUT/current-nor.bin"
${CC:-cc} -std=c99 -O2 -DBOARD_SMK37=1 "-DSTORAGE_IMPL=\"$BASELINE_SOURCE\"" \
    tests/storage_bytes_test.c -o "$OUT/baseline"
"$OUT/baseline" "$OUT/baseline-nor.bin"
cmp "$OUT/current-nor.bin" "$OUT/baseline-nor.bin"
python3 - "$OUT/current-nor.bin" <<'PY'
import hashlib, pathlib, sys
print('Baseline/current complete synthetic NOR byte comparison PASS; sha256=' +
      hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest())
PY
