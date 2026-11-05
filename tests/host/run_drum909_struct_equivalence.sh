#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Byte comparisons (PCM on four buses + full drum909_t state) of this tree's 909
# against the pinned structural-lane base, with that base's own drum909.h and
# drum909_dsp.h (d9_tanh included), at -O0 and -O2, and the exhaustive tanh check.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/909-struct-proof
BASE=${BASE:-a06e20189141ff5fb8766e2c6152be460f4356c3}
rm -rf "$OUT" && mkdir -p "$OUT"
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
for f in drum909.c drum909.h drum909_dsp.h; do git show "$BASE:firmware/src/dsp/$f" > "$OUT/$f"; done
python3 - "$OUT" <<'PY'
import pathlib, re, sys
out = pathlib.Path(sys.argv[1])
src = (out / 'drum909.c').read_text()
src = re.sub(r'^#define X0X_(DRUM|IMA)_TABLES_DEFINE.*\n', '', src, flags=re.M)
src = src.replace('"../x0x_cold.h"', '"x0x_cold.h"')
header = (out / 'drum909.h').read_text()
names = sorted(set(re.findall(r'\b(drum909_\w+)\s*\(', header)))
(out / 'rename.h').write_text(''.join(f'#define {n} ref_{n}\n' for n in names) +
                                '#define x0x_dist_names ref_x0x_dist_names\n')
(out / 'reference.c').write_text(src)
PY
for OPT in 0 2; do
    FLAGS="-O$OPT -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math ${X0X_EXTRA:--fno-builtin} -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen"
    "${CC:-cc}" $FLAGS -include "$OUT/rename.h" -c "$OUT/reference.c" -o "$OUT/ref-O$OPT.o"
    "${CC:-cc}" $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/candidate-O$OPT.o"
    "${CC:-cc}" $FLAGS tests/host/drum909_emit_equivalence.c firmware/src/dsp/x0x_param.c \
        "$OUT/ref-O$OPT.o" "$OUT/candidate-O$OPT.o" -lm -o "$OUT/equivalence-O$OPT"
    "$OUT/equivalence-O$OPT"
    "${CC:-cc}" -O$OPT -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math -fno-builtin -Wall -Wextra -Werror \
        -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/drum909_tanh_pair_test.c -lm -o "$OUT/tanh-pair"
    "$OUT/tanh-pair"
done
