#!/bin/sh
# Reproduce the HOST MEASURED sizes in measurements.txt. Not a firmware build.
set -e
TC="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
CLANG="$TC/pi32v2/bin/clang"
OBJDUMP="$TC/pi32v2/bin/objdump"
NM="$TC/pi32v2/bin/nm"
FLAGS="-target pi32v2 -Os -ffunction-sections -fdata-sections -fno-builtin -mcpu=r3 -Wall -Wno-unused-function -c"
DIR="$(cd "$(dirname "$0")" && pwd)"
"$CLANG" $FLAGS -DSEQ_CHORD_TIER2=0 "$DIR/seq_chord_name.c" -o "$DIR/seq_chord_name_t1.o"
"$CLANG" $FLAGS -DSEQ_CHORD_TIER2=1 "$DIR/seq_chord_name.c" -o "$DIR/seq_chord_name_t2.o"
"$OBJDUMP" -h "$DIR/seq_chord_name_t1.o"
"$NM" -S "$DIR/seq_chord_name_t1.o"
"$OBJDUMP" -h "$DIR/seq_chord_name_t2.o"
"$NM" -S "$DIR/seq_chord_name_t2.o"
