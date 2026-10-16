#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
#
# Compile and link the Bluetooth controller image. Runs on Linux x86-64 with the JieLi pi32v2
# toolchain (the only host it exists for), normally inside a throwaway container that
# ble/ctl/build_ctl.sh starts:
#   TC   the toolchain root (has pi32v2/bin/clang)         default /tc
#   REPO the X0X checkout                                    default /w
#   OUT  where ctl.elf, ctl.map and the reports go           default $REPO/ble/ctl/out
set -eu
TC=${TC:-/tc}; REPO=${REPO:-/w}; OUT=${OUT:-$REPO/ble/ctl/out}
T=$TC/pi32v2/bin
CF="-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Os -flto -fno-common -fallow-pointer-null \
    -fprefer-gnu-section -ffunction-sections -fdata-sections -fms-extensions -ffreestanding \
    -mllvm -pi32v2-large-program=true -Wall -Wextra -Wno-unused-parameter -Werror -DCTL_HOST"
# Work in /tmp: a shared folder (Colima, Docker Desktop) can serve stale file sizes after an edit.
B=/tmp/ctlbuild; rm -rf "$B"; mkdir -p "$B/obj" "$OUT"
cp -r "$REPO/ble/ctl/src" "$REPO/ble/ctl/src_host" "$REPO/ble/ctl/gen" "$REPO/ble/ctl/lib" "$REPO/ble/ctl/ctl.ld" "$B/"
cp "$REPO/firmware/hal/x0x_ble_abi.h" "$B/src/"
cd "$B"; OBJS=""; : > obj/cc_warn.txt
for f in src/*.c src/*.S src_host/*.c gen/*.c; do
  o=obj/$(basename "$f").o
  case "$f" in gen/*) W="-Wno-error -w";; *) W="";; esac
  $T/clang $CF $W -Isrc -c "$f" -o "$o" 2>>obj/cc_warn.txt || { echo "CC FAIL $f"; cat obj/cc_warn.txt; exit 1; }
  OBJS="$OBJS $o"
done
L=$B/lib
set +e
$T/ld --plugin $T/LLVMgold.so --plugin-opt=mcpu=r3 --plugin-opt=-mattr=+fprev1 \
  --plugin-opt=-inline-threshold=5 --plugin-opt=-global-merge-on-const --plugin-opt=-enable-ipra=true \
  --plugin-opt=-pi32v2-large-program=true --plugin-opt=-function-sections --plugin-opt=-data-sections \
  --gc-sections -T ctl.ld -Map obj/ctl.map -o obj/ctl.elf $OBJS \
  --start-group $L/btstack.a $L/btctrler.a $L/wl_rf_common.a $L/crypto_toolbox_Osize.a $L/lib_ccm_aes.a $L/libc.a $L/libcompiler_rt.a --end-group \
  > obj/ld.txt 2>&1
RC=$?
set -e
grep -o "undefined reference to \`[^']*'" obj/ld.txt | sed "s/.*\`//;s/'//" | sort -u > obj/undef.txt || true
echo "ld exit $RC, undefined symbols: $(wc -l < obj/undef.txt)"
grep -v -E 'undefined reference|In function' obj/ld.txt | head -20 || true
cp obj/cc_warn.txt obj/ld.txt obj/undef.txt "$OUT/"
[ "$RC" = 0 ] || exit 1
$T/objdump -h obj/ctl.elf > "$OUT/sections.txt"
$T/objdump -t obj/ctl.elf > "$OUT/symbols.txt"
cp obj/ctl.elf obj/ctl.map "$OUT/"
if [ -s obj/cc_warn.txt ]; then echo "compiler warnings:"; cat obj/cc_warn.txt; fi
