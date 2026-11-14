#!/bin/sh
set -eu
cd "$(dirname "$0")"
SRC=${SOUNDPIPE_ROOT:?Set SOUNDPIPE_ROOT to the pinned AudioKit Soundpipe directory}
CC=${CC:-cc}
mkdir -p build
python3 verify_inputs.py > source-contract.log
cat source-contract.log
python3 test_input_binding.py > input-binding.log
cat input-binding.log
for opt in Os O2; do
 "$CC" -std=c99 -"$opt" -Wall -Wextra -Werror -Wno-unused-parameter -ffp-contract=off -fno-fast-math -I"$SRC" -I"$SRC/lib/dr_wav" test_cache_list.c "$SRC/modules/port.c" -lm -o "build/test-$opt"
 "build/test-$opt" > "green-$opt.log"
 cat "green-$opt.log"
 "$CC" -std=c99 -"$opt" -ffp-contract=off -fno-fast-math -fsanitize=address,undefined -fno-omit-frame-pointer -I"$SRC" -I"$SRC/lib/dr_wav" test_cache_list.c "$SRC/modules/port.c" -lm -o "build/test-$opt-san"
 ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "build/test-$opt-san" > "green-$opt-san.log"
 cat "green-$opt-san.log"
done
python3 mutation_checks.py > mutation.log
cat mutation.log
