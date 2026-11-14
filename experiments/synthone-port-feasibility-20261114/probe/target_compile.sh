#!/bin/sh
set -eu
cd "$(dirname "$0")"
SRC=../audiokit-4.9.2/AudioKit/Core
TC=${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the verified compiler bundle root}
mkdir -p pi32v2
set --
for file in "$SRC"/Soundpipe/modules/*.c "$SRC"/SoundpipeExtension/modules/oscmorph2d.c; do
 object="pi32v2/$(basename "$file" .c).o"
 "$TC/common/bin/clang" -target pi32v2 -mcpu=r3 -mfprev1 -Os -ffunction-sections -fdata-sections -fno-builtin -ffp-contract=off -fno-fast-math -DNO_LIBSNDFILE -I"$SRC/Soundpipe" -I"$SRC/SoundpipeExtension" -I"$SRC/Soundpipe/lib/faust" -I"$SRC/Soundpipe/lib/dr_wav" -idirafter "$TC/pi32v2/include" -c "$file" -o "$object"
 set -- "$@" "$object"
done
printf 'PI32V2_C_MODULE_COMPILE_PASS objects=%s\n' "$#"
size "$@"
