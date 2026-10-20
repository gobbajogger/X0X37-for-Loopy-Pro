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
#   X0X_JOBS  independent compile jobs (1 = serial)           default 4, maximum 64
set -eu
TC=${TC:-/tc}; REPO=${REPO:-/w}; OUT=${OUT:-$REPO/ble/ctl/out}
T=$TC/pi32v2/bin
mkdir -p "$OUT"
# An unsuccessful invocation must not leave a previous image looking successful.
rm -f "$OUT/ctl.elf" "$OUT/ctl.img" "$OUT/ctl.map" "$OUT/cc_warn.txt" "$OUT/ld.txt" "$OUT/undef.txt" "$OUT/sections.txt" "$OUT/symbols.txt"
command -v setsid >/dev/null || { echo "Controller compilation needs setsid" >&2; exit 1; }
JOBS=${X0X_JOBS-4}
case "$JOBS" in ''|*[!0-9]*|0|0*) echo "Invalid X0X_JOBS: $JOBS (use 1..64)" >&2; exit 1 ;; esac
[ "${#JOBS}" -le 2 ] && [ "$JOBS" -le 64 ] || { echo "Invalid X0X_JOBS: $JOBS (use 1..64)" >&2; exit 1; }
CF="-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Oz -flto -fno-common -fallow-pointer-null \
    -fprefer-gnu-section -mllvm -pi32v2-large-program=true -ffunction-sections -fdata-sections -fms-extensions -ffreestanding \
    -Wall -Wextra -Wno-unused-parameter -Werror"
# Work in /tmp: a shared folder (Colima, Docker Desktop) can serve stale file sizes after an edit.
B=$(mktemp -d "${TMPDIR:-/tmp}/ctlbuild.XXXXXX")
PIDS=""
stop_jobs() {
  # Each compiler has its own process group, including any driver children.
  # Also signal the direct PID in case interruption precedes setsid execution.
  for pid in $PIDS; do kill -TERM "-$pid" 2>/dev/null || true; kill -TERM "$pid" 2>/dev/null || true; done
  for pid in $PIDS; do kill -KILL "-$pid" 2>/dev/null || true; kill -KILL "$pid" 2>/dev/null || true; done
  for pid in $PIDS; do wait "$pid" 2>/dev/null || true; done
  PIDS=""
}
cleanup() { stop_jobs; rm -rf "$B"; }
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
mkdir -p "$B/obj" "$OUT"
cp -r "$REPO/ble/ctl/src" "$REPO/ble/ctl/gen" "$REPO/ble/ctl/lib" "$REPO/ble/ctl/ctl.ld" "$B/"
cp "$REPO/firmware/hal/x0x_ble_abi.h" "$B/src/"
cd "$B"; OBJS=""; : > obj/cc_warn.txt
# Keep link order exactly as the serial source glob order.
# Batches bound the job count on POSIX sh, including older shells without wait -n.
: > obj/units
failed=0; count=0
join_jobs() {
  remaining=${PIDS# }
  for pid in $PIDS; do
    case "$remaining" in *" "*) remaining=${remaining#* } ;; *) remaining="" ;; esac
    if ! wait "$pid"; then
      failed=1
    fi
    # Even a successful driver can leave children. Stop its group before dropping
    # ownership, including at batch boundaries. Only unjoined drivers need waits.
    kill -KILL "-$pid" 2>/dev/null || true
    PIDS="$remaining"
    if [ "$failed" != 0 ]; then stop_jobs; break; fi
  done
  PIDS=""; count=0
}
for f in src/*.c src/*.S gen/*.c; do
  o=obj/$(basename "$f").o
  case "$f" in gen/*) W="-Wno-error -w";; *) W="";; esac
  # Redirect both streams per unit; output is merged in source order after joining.
  setsid "$T/clang" $CF $W -Isrc -c "$f" -o "$o" > "$o.log" 2>&1 &
  PIDS="$PIDS $!"; count=$((count + 1))
  printf '%s\n' "$o" >> obj/units
  OBJS="$OBJS $o"
  if [ "$count" -ge "$JOBS" ]; then join_jobs; [ "$failed" = 0 ] || break; fi
done
[ -z "$PIDS" ] || join_jobs
while IFS= read -r o; do cat "$o.log" >> obj/cc_warn.txt; done < obj/units
cp obj/cc_warn.txt "$OUT/"
if [ "$failed" != 0 ]; then echo "CC FAIL (see $OUT/cc_warn.txt)"; cat obj/cc_warn.txt; exit 1; fi

L=$B/lib
set +e
$T/ld --plugin $T/LLVMgold.so --plugin-opt=mcpu=r3 --plugin-opt=-mattr=+fprev1 \
  --plugin-opt=-inline-threshold=0 --plugin-opt=-global-merge-on-const --plugin-opt=-enable-ipra=true --plugin-opt=-pi32v2-large-program=true \
  \
  --gc-sections -T ctl.ld -Map obj/ctl.map -o obj/ctl.elf $OBJS \
  --start-group $L/btctrler.a $L/wl_rf_common.a $L/crypto_toolbox_Osize.a $L/lib_ccm_aes.a $L/libc.a $L/libcompiler_rt.a --end-group \
  > obj/ld.txt 2>&1
RC=$?
set -e
grep -o "undefined reference to \`[^']*'" obj/ld.txt | sed "s/.*\`//;s/'//" | sort -u > obj/undef.txt || true
echo "ld exit $RC, undefined symbols: $(wc -l < obj/undef.txt)"
grep -v -E 'undefined reference|In function' obj/ld.txt | head -20 || true
cp obj/cc_warn.txt obj/ld.txt obj/undef.txt obj/ctl.map "$OUT/"
[ "$RC" = 0 ] || exit 1
$T/objdump -h obj/ctl.elf > "$OUT/sections.txt"
$T/objdump -t obj/ctl.elf > "$OUT/symbols.txt"
cp obj/ctl.elf obj/ctl.map "$OUT/"
if [ -s obj/cc_warn.txt ]; then echo "compiler warnings:"; cat obj/cc_warn.txt; fi
