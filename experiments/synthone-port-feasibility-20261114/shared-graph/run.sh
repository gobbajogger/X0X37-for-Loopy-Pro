#!/bin/sh
set -eu
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo 'usage: sh run.sh SYNTHONE_SOURCE_ROOT [o2|os|sanitize]' >&2
  exit 2
fi
# Resolve caller-relative input before changing to the experiment directory.
case "$1" in
  /*) source_input=$1 ;;
  *) source_input=$(pwd)/$1 ;;
esac
variant=${2:-o2}
case "$variant" in
  o2) opt='-O2'; sanitize='' ;;
  os) opt='-Os'; sanitize='' ;;
  sanitize) opt='-O1 -g -fno-omit-frame-pointer'; sanitize='-fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all' ;;
  *) echo 'expected o2, os or sanitize' >&2; exit 2 ;;
esac
cd "$(dirname "$0")"
mkdir -p build
build=$(mktemp -d "build/${variant}.XXXXXX")
python3 prepare_tables.py "$source_input" "$build/public-morphs.f32" "$build/table-report.json" > "$build/table-summary.json"
# All dependencies are verified before invoking a compiler. Fresh directories and
# an explicit object list exclude stale or unrelated build objects.
core=../audiokit-4.9.2/AudioKit/Core
includes="-I. -I$core/Soundpipe -I$core/SoundpipeExtension -I$core/Soundpipe/lib/dr_wav"
common="-fno-builtin -ffunction-sections -fdata-sections -ffp-contract=off -fno-fast-math $opt $sanitize"
"${CC:-cc}" --version | head -1 > "$build/compiler.txt"
"${CXX:-c++}" --version | head -1 >> "$build/compiler.txt"
printf '%s\n' "$common" > "$build/options.txt"
objects=''
for name in base ftbl adsr crossfade osc fosc noise moogladder butbp buthp port phasor; do
  "${CC:-cc}" -std=gnu99 -DNO_LIBSNDFILE $common $includes -c "$core/Soundpipe/modules/$name.c" -o "$build/$name.o"
  objects="$objects $build/$name.o"
done
"${CC:-cc}" -std=gnu99 -DNO_LIBSNDFILE $common $includes -c "$core/SoundpipeExtension/modules/oscmorph2d.c" -o "$build/oscmorph2d.o"
"${CXX:-c++}" -std=c++11 $common $includes -c ../probe/portable_voice.cpp -o "$build/oracle_voice.o"
"${CXX:-c++}" -std=c++11 $common $includes -c shared_graph.cpp -o "$build/shared_graph.o"
"${CXX:-c++}" -std=c++11 $common $includes -c tests/shared_graph_test.cpp -o "$build/shared_graph_test.o"
"${CXX:-c++}" $common $objects "$build/oscmorph2d.o" "$build/oracle_voice.o" "$build/shared_graph.o" "$build/shared_graph_test.o" -Wl,--gc-sections -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free -lm -o "$build/shared_graph_test"
ASAN_OPTIONS=detect_leaks=1 "$build/shared_graph_test" "$build/public-morphs.f32" > "$build/result.json"
cat "$build/result.json"
