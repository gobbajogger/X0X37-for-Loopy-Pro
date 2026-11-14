#!/bin/sh
set -eu
cd "$(dirname "$0")"
SRC=../audiokit-4.9.2/AudioKit/Core
TC=${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the verified compiler bundle root}
mkdir -p pi32v2
"$TC/common/bin/clang" -x c++ -std=c++11 -target pi32v2 -mcpu=r3 -mfprev1 -Os -ffunction-sections -fdata-sections -ffp-contract=off -fno-fast-math -fno-exceptions -fno-rtti -I"$SRC/Soundpipe" -I"$SRC/SoundpipeExtension" -I"$SRC/Soundpipe/lib/dr_wav" -I"$SRC/Soundpipe/lib/faust" -idirafter "$TC/pi32v2/include" -c portable_voice.cpp -o pi32v2/portable_voice.o
size pi32v2/portable_voice.o
