#!/bin/sh
# Reproduce the §10 HOST MEASURED sizes. Not a firmware build.
set -e
TC="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
CLANG="$TC/pi32v2/bin/clang"
OBJDUMP="$TC/pi32v2/bin/objdump"
NM="$TC/pi32v2/bin/nm"
FLAGS="-target pi32v2 -Os -ffunction-sections -fdata-sections -fno-builtin -mcpu=r3 -Wall -Wno-unused-function -c"
DIR="$(cd "$(dirname "$0")" && pwd)"
for OPT in A B C D R; do
    "$CLANG" $FLAGS -DSEQ_CHORD_OPT=\'$OPT\' "$DIR/seq_chord_label.c" -o "$DIR/seq_chord_label_$OPT.o"
    echo "======= OPT $OPT ======="
    "$OBJDUMP" -h "$DIR/seq_chord_label_$OPT.o"
    echo "--- nm ---"
    "$NM" -S "$DIR/seq_chord_label_$OPT.o"
    echo
done
