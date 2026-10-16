#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# X0X host tests: the maths, the sequencer + TB-3PO (against schwung-tb3po), each engine
# against its reference implementation, and the whole firmware app in the simulator
# (tests/scenarios/*.x0x: UI, patterns, persistence, MIDI clock), with screenshots and
# audio in build/scenarios/. Needs the reference sources (see tests/host/run_*.sh).
#   tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FAIL=0
OUT=build/host
mkdir -p "$OUT" build/scenarios
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -15 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
W="-O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror"
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"   # its reference is double on purpose
TB3PO_REF="${TB3PO_REF:-../schwung-tb3po/src/dsp/tb3po.c}"
seq_test() {
    if [ -f "$TB3PO_REF" ]; then
        ref="$(cd "$(dirname "$TB3PO_REF")" && pwd)/$(basename "$TB3PO_REF")"
        $CC -O2 -ffp-contract=off -Wall -Wextra "-DREF_TB3PO=\"$ref\"" -o $OUT/seq_test tests/host/seq_test.c \
            firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c || return 1
    else
        echo "(no schwung-tb3po checkout at $TB3PO_REF: the TB-3PO comparison is skipped)"
        $CC -O2 -ffp-contract=off -Wall -Wextra -o $OUT/seq_test tests/host/seq_test.c \
            firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c || return 1
    fi
    $OUT/seq_test
}
run sequencer seq_test
run clk-lock sh -c "$CC $W -o $OUT/clk_lock_test tests/host/clk_lock_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c && $OUT/clk_lock_test"   # MID-15: one clock input
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run bass303_lite sh -c "$CC -std=c99 -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -o $OUT/bass303_lite_test tests/host/bass303_lite_test.c firmware/src/dsp/bass303.c -lm && $OUT/bass303_lite_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
run undo sh -c "$CC -O2 -Wall -Wextra -Wno-unused-function -o $OUT/undo_test tests/host/undo_test.c && $OUT/undo_test"
run lz sh -c "$CC $W -o $OUT/lz_test tests/host/lz_test.c && $OUT/lz_test"
run motion sh -c "$CC $W -o $OUT/motion_test tests/host/motion_test.c firmware/src/seq/motion.c && $OUT/motion_test"
run drum-pan sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/drum_pan_test tests/host/drum_pan_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c -lm && $OUT/drum_pan_test"
run stereo sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/stereo_test tests/host/stereo_test.c firmware/src/dsp/master.c firmware/src/dsp/fxbus.c -lm && $OUT/stereo_test"
run master sh -c "$CC $W -Wno-double-promotion -o $OUT/master_test tests/host/master_test.c firmware/src/dsp/master.c -lm && $OUT/master_test"
run drum909-adpcm sh tests/host/run_drum909_adpcm.sh   # the 909 cymbals as IMA ADPCM (A/B study reference)
for t in drum909 drum808 bass303 breaks; do
    [ -f "tests/host/run_$t.sh" ] && run "$t" sh "tests/host/run_$t.sh"
done
run loop-bars sh tests/host/run_loop_bars.sh            # engine.c loop_bars: the bar guess uses the loop's rate
run storage sh -c "$CC -O2 -o $OUT/storage_test tests/storage_test.c && $OUT/storage_test"
# the update path, against the firmware package (Felucca's tests; needs ./build.sh)
if [ -f build/x0x.fwsc ]; then
    run ota-entry sh -c "$CC -o $OUT/ota_test tests/ota_test.c && $OUT/ota_test build/x0x.fwsc"
    if [ -z "${AC79_SDK:-}" ]; then
        echo "  skip update-loader (needs AC79_SDK, as the build)"
    else
    run update-loader sh -c "head -c 200000 build/x0x.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py $OUT/old_app.bin build/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -o $OUT/ldr_test tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc build/x0x.fwsc"
    fi
    run installer python3 tests/install_test.py
else
    echo "  skip update-path tests (no build/x0x.fwsc: run ./build.sh)"
fi
# SMK-37 Pro (BOARD_SMK37): the flash map and the store, the package layout, the update path on SMK-layout
# packages (made from the FM-1 app: only the layout matters), and the SMK build's own package if there is one
run locked-settings python3 tools/check_locked_settings.py   # hardware-relevant settings (tools/locked_settings.json)
run smk-flash sh -c "$CC -O2 -Wall -Wno-unused-function -DBOARD_SMK37=1 -Ifirmware/hal -o $OUT/smk_flash_test \
    tests/smk_flash_test.c && $OUT/smk_flash_test"
run smk-storage sh -c "$CC -O2 -DBOARD_SMK37=1 -o $OUT/storage_test_smk tests/storage_test.c && $OUT/storage_test_smk"
run smk-apc40-map sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_apc40_map_test tests/smk_apc40_map_test.c && $OUT/smk_apc40_map_test"
run smk-apc40 python3 tests/smk_apc40_test.py
run smk-clip-ble-clear sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/ble -o $OUT/smk_clip_ble_clear_test tests/smk_clip_ble_clear_test.c && $OUT/smk_clip_ble_clear_test"
run smk-clip-keys sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_clip_keys_test tests/smk_clip_keys_test.c && $OUT/smk_clip_keys_test"
run smk-arrows sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_arrows_test tests/smk_arrows_test.c && $OUT/smk_arrows_test"   # < > route and hold repeat
run smk-arrows-owner python3 tests/smk_arrows_owner_test.py   # one owner for each < > press in clip mode (CLP-27)
run installer-targets python3 tests/make_pages_test.py
run diag-size python3 tests/diag_size_test.py           # a76: ED_DIAG stays in its buffers
run smk-inlog sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_inlog_test tests/smk_inlog_test.c && $OUT/smk_inlog_test"   # a76: inbound recorder ring
run inlog-tool python3 tests/inlog_tool_test.py      # a76: ED_INLOG dump text
run smk-clip-ble-notify sh -c "$CC -std=c99 -O2 -Wall -Wextra -Wno-unused-function -Werror -Ifirmware/src -Ifirmware/hal -Ifirmware/src/ble -o $OUT/smk_clip_ble_notify_test tests/smk_clip_ble_notify_test.c && $OUT/smk_clip_ble_notify_test"   # a76: no pad colour while notify is off
run smk-clip-dark sh tests/smk_clip_dark_test.sh   # a76: real clip and pad LED code: dark 10 s after Loopy goes away
run smk-apc40-violet python3 tests/smk_apc40_violet_test.py   # a76: violet, purple and pink clips are not blue
if [ -f build/x0x.fwsc ] && [ -n "${AC79_SDK:-}" ]; then
    run smk-package sh -c "python3 tools/fm1pkg_make.py --board smk37 build/x0x.bin build/loader/ota.bin $OUT/smk_new.fwsc >/dev/null && \
        python3 tests/smk_pkg_test.py $OUT/smk_new.fwsc"
    run smk-ota-entry sh -c "$CC -DBOARD_SMK37=1 -o $OUT/ota_test_smk tests/ota_test.c && $OUT/ota_test_smk $OUT/smk_new.fwsc"
    for m in 1 2; do                     # the MODEL=elite / mke builds of ota.c
        run smk-ota-entry-model$m sh -c "$CC -DBOARD_SMK37=1 -DT_MODEL=$m -o $OUT/ota_test_smk_m$m tests/ota_test.c && \
            $OUT/ota_test_smk_m$m $OUT/smk_new.fwsc"
    done
    run smk-update-loader sh -c "head -c 200000 build/x0x.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py --board smk37 $OUT/old_app.bin build/loader/ota.bin $OUT/smk_old.fwsc >/dev/null && \
        $CC -DBOARD_SMK37=1 -o $OUT/ldr_test_smk tests/ldr_test.c && $OUT/ldr_test_smk $OUT/smk_old.fwsc $OUT/smk_new.fwsc"
else
    echo "  skip SMK-37 Pro package tests (needs build/x0x.fwsc and AC79_SDK)"
fi
[ -f build/x0x-smk37.fwsc ] && run smk-build-package python3 tests/smk_pkg_test.py build/x0x-smk37.fwsc
[ -f build/x0x-smk37-elite.fwsc ] && run smk-build-package-elite python3 tests/smk_pkg_test.py build/x0x-smk37-elite.fwsc "SMK-37 Elite_900"
[ -f build/x0x-smk37-mke.fwsc ] && run smk-build-package-mke python3 tests/smk_pkg_test.py build/x0x-smk37-mke.fwsc "MKE-P37_900"
# SMK-37 Bluetooth MIDI: the HCI mailbox, the BLE-MIDI host, the image check, the park handshake (tests/ble_test.c);
# the controller image in a package, Python and JavaScript (tests/ble_pkg_test.py, needs the SMK build)
run ble sh -c "$CC $W -Ifirmware/hal -Ifirmware/src/ble -o $OUT/ble_test tests/ble_test.c -lpthread && $OUT/ble_test"
[ -f build/x0x-smk37.fwsc ] && run ble-package python3 tests/ble_pkg_test.py
run upload-images sh -c "python3 tests/host/make_test_loops.py build/test_loops && \
    python3 tools/upload_breaks.py --rate 22050 --nslots 3 --dry-run build/test_upload build/test_loops/loop*.wav && \
    python3 tools/upload_breaks.py --dry-run build/test_upload44 build/test_loops/loop*.wav && \
    python3 tools/upload_breaks.py --rate 22050 --slots 3 --dry-run build/test_upload44s3 build/test_loops/loop1.wav"   # 22.05 kHz (3 x 80), the 44.1 kHz default (80/120/44), slot 3
run upload-tools python3 tests/upload_tools_test.py   # upload_breaks.py on a mocked device; one-shot trim
command -v node >/dev/null 2>&1 && run breaks-web node web/test_breaks.mjs   # the browser uploader: upload_breaks.py's bytes
command -v node >/dev/null 2>&1 && run breaks-page node web/test_breaks_page.mjs   # the page against a mocked device
command -v node >/dev/null 2>&1 && run install-slots-web node web/test_install_slots.mjs   # the installer keeps the slots over a slot map change
run slot-backup python3 tests/x0x_slots_test.py            # tools/x0x_slots.py against a mocked device
run installer-slots python3 tests/install_slots_test.py   # fm1_install.py: backup, install, restore over a slot map change
run installer-ctl python3 tests/install_ctl_test.py       # fm1_install.py: the Bluetooth image read-back (CTL_READ) and its CRC32
run host-build sh host/build_host.sh
# live recording on the whole app (host/x0x_host.c without its script main; needs build/gen from host-build)
run rec-live sh -c "$CC -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o $OUT/rec_live_test tests/host/rec_live_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c \
    firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/app/engine.c -lm && $OUT/rec_live_test"
if command -v emcc >/dev/null 2>&1 && command -v node >/dev/null 2>&1; then   # the browser build, when it can be built
    printf 'wav emu_ref.wav\nwait 500\ntapkey b0\ntapkey w0\ntapkey w8\ntap PLAY\nwait 2960\nwavstop\n' > build/host/emu_ref.x0x
    run emu sh -c "sh web/emu/build.sh >/dev/null 2>&1 && build/host/x0x_host build/host/emu_ref.x0x build/host >/dev/null && \
        node tests/host/emu_test.mjs build/emu/x0x.wasm build/host/emu_ref.wav"
    run emu-slots node tests/host/emu_slots_test.mjs build/emu/x0x.wasm build/test_loops/loop1.wav
fi
for s in tests/scenarios/*.x0x; do
    n=$(basename "$s" .x0x)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/x0x_host "$s" "build/scenarios/$n"
done
# the device scratch (x0x.c): the PERF TEST patterns and the LZ hash table of a save in the same 4 KiB.
# The test must play the same sound as with its own buffers (scenario perfsave: a save just before the test).
printf 'static unsigned short host_scratch[2048] __attribute__((aligned(4), unused));\n#define LZ_SCRATCH host_scratch\n#define PERF_SCRATCH host_scratch\n' > $OUT/scratch_defs.h
run perf-scratch sh -c "X0X_HOST_DEFS='-include $OUT/scratch_defs.h' X0X_HOST_OUT=$OUT/x0x_host_scratch sh host/build_host.sh 2>/dev/null && \
    mkdir -p $OUT/perf-scratch && $OUT/x0x_host_scratch tests/scenarios/perf.x0x $OUT/perf-scratch && \
    $OUT/x0x_host_scratch tests/scenarios/perfsave.x0x $OUT/perf-scratch && \
    cmp $OUT/perf-scratch/perfsave.wav build/scenarios/perfsave/perfsave.wav"
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
