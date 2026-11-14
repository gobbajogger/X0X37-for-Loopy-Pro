#!/bin/sh
set -eu
cd "$(dirname "$0")"
SRC=../audiokit-4.9.2/AudioKit/Core
c++ -std=c++11 -O2 -ffp-contract=off -fno-fast-math -I"$SRC/Soundpipe" -I"$SRC/SoundpipeExtension" -I"$SRC/Soundpipe/lib/dr_wav" portable_voice.cpp voice_probe.cpp host/*.o -Wl,--gc-sections -lm -o host/voice_probe
./host/voice_probe host/wavetables.f32 host/preset-params.f32 1024
