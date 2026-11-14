#!/bin/sh
set -eu
cd "$(dirname "$0")"
SRC=../audiokit-4.9.2/AudioKit/Core
mkdir -p host
for file in "$SRC"/Soundpipe/modules/*.c "$SRC"/SoundpipeExtension/modules/oscmorph2d.c; do
 cc -std=gnu99 -O2 -fno-builtin -ffunction-sections -fdata-sections -DNO_LIBSNDFILE -I"$SRC/Soundpipe" -I"$SRC/SoundpipeExtension" -I"$SRC/Soundpipe/lib/faust" -I"$SRC/Soundpipe/lib/dr_wav" -c "$file" -o "host/$(basename "$file" .c).o"
done
cc -std=gnu99 -O2 -fno-builtin -I"$SRC/Soundpipe" -I"$SRC/SoundpipeExtension" -I"$SRC/Soundpipe/lib/dr_wav" memory_probe.c host/*.o -Wl,--gc-sections -Wl,--wrap=malloc -lm -o host/memory_probe
./host/memory_probe
size host/memory_probe
