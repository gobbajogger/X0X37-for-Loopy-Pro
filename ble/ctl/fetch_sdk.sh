#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
# Fetch JieLi's files for the Bluetooth controller image from JieLi's own repository
# (fw-AC79_AIoT_SDK on gitee) at a pinned commit, into ble/ctl/lib (not tracked, not distributed).
# Every file is checked with SHA-256. A local SDK checkout ($SDK_LOCAL, default
# ~/fw-AC79_AIoT_SDK) is used when its file has the same hash.
set -eu
COMMIT=d179b4484759423312073f5fbb232501aa491047
BASE="https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/raw/$COMMIT"
LOCAL="${SDK_LOCAL:-$HOME/fw-AC79_AIoT_SDK}"
DEST="$(dirname "$0")/lib"
mkdir -p "$DEST"
sha() { shasum -a 256 "$1" 2>/dev/null | cut -d' ' -f1 || sha256sum "$1" | cut -d' ' -f1; }
while read -r H P; do
  [ -z "$H" ] && continue
  F="$DEST/$(basename "$P")"
  if [ -f "$F" ] && [ "$(sha "$F")" = "$H" ]; then echo "ok    $P"; continue; fi
  if [ -f "$LOCAL/$P" ] && [ "$(sha "$LOCAL/$P")" = "$H" ]; then cp "$LOCAL/$P" "$F"; echo "local $P"; continue; fi
  curl -fsSL "$BASE/$P" -o "$F.tmp"
  [ "$(sha "$F.tmp")" = "$H" ] || { echo "SHA-256 MISMATCH: $P" >&2; rm -f "$F.tmp"; exit 1; }
  mv "$F.tmp" "$F"; echo "fetch $P"
done <<'LIST'
9ef4262444b49171c4a5eab906e3939b1d4943350ac4993c18d8474d01d773de cpu/wl82/liba/btctrler.a
ec50bb31af02ee3e2db9d87726c1b18b317f2acd72f233c9ce7b79a89ea324b2 cpu/wl82/liba/wl_rf_common.a
373251c39f8de0851204b623601b348254023e98b3d0b062ecab743b0c3aa35e cpu/wl82/liba/crypto_toolbox_Osize.a
bd3dd110197ae0084826538bfb5e432a6fadb3a001acadfdbd5f3a409fb5da2c cpu/wl82/liba/lib_ccm_aes.a
1170ac6819b2574bb78c89ab88c2f62074f0be227714f926e4d7c035ac1566a4 include_lib/newlib/pi32v2-lib/libc.a
141997ab6977c0c4abfb857bc32a897eb345f8d593e27e74c33bdebcf3fd5009 include_lib/newlib/pi32v2-lib/libcompiler_rt.a
697c798b6a9c992c75ee3959aa58d121d4eb3015474250ad8f9156a9a22fe6bb apps/common/net/wifi_conf.c
c71d239df91726fc519c6eb72d318ec65820627232b2f796219e87dcf35d0ab4 LICENSE
LIST
