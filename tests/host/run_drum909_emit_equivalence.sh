#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host-only byte comparisons for emit/rim/clap, against the pinned lane base.
# Does not require the upstream 9W9 checkout. Generated headers: -Ibuild/gen.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/909-emit-proof
BASE=a18561f17977be8592cef5a3d3cbebf70e4dbf1b
mkdir -p "$OUT"
git show "$BASE:firmware/src/dsp/drum909.c" > "$OUT/base.c"
python3 - "$OUT" <<'PY'
import pathlib, re, sys
out = pathlib.Path(sys.argv[1])
src = (out / 'base.c').read_text()
src = re.sub(r'^#define X0X_(DRUM|IMA)_TABLES_DEFINE.*\n', '', src, flags=re.M)
src = src.replace('"../x0x_cold.h"', '"x0x_cold.h"')
# Rename public functions and the label array; immutable lookup tables are shared.
header = pathlib.Path('firmware/src/dsp/drum909.h').read_text()
names = sorted(set(re.findall(r'\b(drum909_\w+)\s*\(', header)))
(out / 'rename.h').write_text(''.join(f'#define {n} ref_{n}\n' for n in names) +
                                '#define x0x_dist_names ref_x0x_dist_names\n')
(out / 'reference.c').write_text(src)
PY
for OPT in 0 2; do
    FLAGS="-O$OPT -std=c99 -DX0X_HOST -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen"
    "${CC:-cc}" $FLAGS -include "$OUT/rename.h" -c "$OUT/reference.c" -o "$OUT/ref-O$OPT.o"
    "${CC:-cc}" $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/candidate-O$OPT.o"
    "${CC:-cc}" $FLAGS tests/host/drum909_emit_equivalence.c firmware/src/dsp/x0x_param.c \
        "$OUT/ref-O$OPT.o" "$OUT/candidate-O$OPT.o" -lm -o "$OUT/equivalence-O$OPT"
    "$OUT/equivalence-O$OPT"
done
