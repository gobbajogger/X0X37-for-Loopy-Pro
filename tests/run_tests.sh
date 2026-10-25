#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# X0X host tests: the maths, the sequencer + TB-3PO (against schwung-tb3po), each engine
# against its reference implementation, and the whole firmware app in the simulator
# (tests/scenarios/*.x0x: UI, patterns, persistence, MIDI clock), with screenshots and
# audio in build/scenarios/. Needs the reference sources (see tests/host/run_*.sh).
#   tests/run_tests.sh
#   tests/run_tests.sh <name>...           only the named tests (the names of the result lines). Also X0X_TESTS="<name> ...".
#   X0X_TEST_JOBS=1 tests/run_tests.sh    serial: one test at a time, in order.
#   The default is X0X_TEST_JOBS=8: host-only tests run 8 at a time. See test_class below.
#     Install, update, package, BLE and mocked-device tests always run serially, one at a time, after
#     the parallel tests end. Each parallel test gets its own HOME and TMPDIR (build/host/iso/<test>/).
#     The scenarios run serially after them. The result lines come in the serial order, after all tests end.
#   X0X_TEST_TIME_SCALE=1 tests/run_tests.sh    the real waits of the updater in install-slots-web.
#     The default is X0X_TEST_TIME_SCALE=20 (dev and branch runs): in web/test_install_slots.mjs, waits of
#     1000 ms or more take 1/20 of the time. The pre-install gate (tools/gate.sh) always uses 1.
#     build/host/.results/*.time gives the time of each test.
#   The tests in needs_gen (drum-pan, stereo, rec-live) include the headers in build/gen. host-build writes them.
#     In parallel mode, such a test starts only after host-build ends (build/host/iso/.gen-done), and it does
#     not hold a job slot while it waits. In serial mode, host-build runs before them.
#   The install-path tests (class S and W, but not rec-live: installers, mocked devices, packages, update loaders, BLE)
#     take a shared lock on the gate lock (flock -s on X0X_GATE_LOCK, default /workspace/x0x37-refs/gate.lock) while
#     they run. Thus they wait while a gate holds its exclusive lock, and a gate waits for them. Dev runs can share.
#     All other tests take no lock. The lock stays in the flock process (flock -o), so a test never holds it.
#     The gate sets X0X_GATE_HOLDS_LOCK=1 (it holds the exclusive lock), and then the tests take no lock.
# Reference sources (read-only): NINEW9_DIR, EIGHTW8_DIR, SCHWUNG303, TB3PO_REF (see tests/host/run_*.sh).
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # a hook sets these; a scratch repository must not use the repository of the hook
[ $# -gt 0 ] && X0X_TESTS="$*"
X0X_TESTS="${X0X_TESTS:-}"
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FAIL=0
OUT=build/host
mkdir -p "$OUT"
JOBS="${X0X_TEST_JOBS:-8}"
export X0X_TEST_TIME_SCALE="${X0X_TEST_TIME_SCALE:-20}"
export X0X_HOST_LOAD="${X0X_HOST_LOAD:-0}"   # x0x_host: the overload guard sees 0% until a "load N" line, not the time of a busy host
export X0X_TEST_RUN=1   # tools/status.sh (check 30): a test run never writes the live orchestrator folder
case "$JOBS" in ''|*[!0-9]*) JOBS=1 ;; esac
PHASE=0                 # parallel mode: W = class W, 1 = class P and G (parallel), 2 = class S (serial), 3 = class C (serial)
T_SEQ=0
RES="$OUT/.results"
# The class of a test. S: serial and alone (install, update, package, BLE, mocked device).
# W: install-slots-web (a long node test with a temp folder and a mocked keyboard, no port): it starts with
# phase 1 and runs beside the class P and G tests only. It ends before phase 2 starts.
# G: host-only, shares build/gen (one at a time). C: scenario (needs host-build; serial, because the
# simulator's CPU guard uses the host time, so a busy host changes the result). P: host-only, parallel.
test_class() {
    case "$1" in
    install-slots-web) TEST_CLASS=W ;;
    installer|installer-targets|installer-slots|installer-restore-status|slot-backup|slot-backup-durable|slot-archive-zone|slot-verify-order|slot-read-reply|slot-ack-integrity|slot-echo-7bit|ctl-publish-atomic|upload-tools|\
    breaks-page|projects-page|ble-acl-start-flags|ble-hci-completed|ble-acl-zero-credit|ble-link-epoch-reset|smc-reconnect-epoch|smc-ble-payload|ble-central-first-notification|ble-disconnect-status|ble-sysex-bounds|ble|ble-package|smk-package|smk-ota-entry*|smk-update-loader|smk-build-package*|\
    smk-clip-ble-clear|smk-clip-ble-notify|installer-ctl|ctl-read-framing|installer-backup-snapshot) TEST_CLASS=S ;;
    rec-live|rec-live-smk) TEST_CLASS=S ;;   # these read build/gen from host-build (phase 1)
    drum-pan|stereo|drum909-adpcm|drum909|copy-semantics|song-actions|ui-feedback|host-build|project-runtime|smc-reachability|smc-ui|smc-ui-negative|smc-drum-mutes|smc-drum-rs|smc-performance-abort|smc-record-feedback) TEST_CLASS=G ;;
    scenario-*|perf-scratch) TEST_CLASS=C ;;   # perf-scratch compares with scenario perfsave: it runs after the scenarios
    *) TEST_CLASS=P ;;
    esac
}
# Prepare fixtures only after selection and phase admission. Failure is a test failure.
prepare_test() {
    case "$1" in
    scenario-*) mkdir -p "build/scenarios/${1#scenario-}" ;;
    perf-scratch)
        printf 'static unsigned short host_scratch[2048] __attribute__((aligned(4), unused));\n#define LZ_SCRATCH host_scratch\n#define PERF_SCRATCH host_scratch\n' > $OUT/scratch_defs.h ;;
    esac
}
run_test() { prepare_test "$1" && lk "$@"; }
run_one() {             # key name command...: the result goes to $RES/key (and $RES/key.fail)
    k="$1"; n="$2"; shift 2
    t0=$(date +%s)
    if run_test "$n" "$@" > "$OUT/$n.log" 2>&1; then
        echo "  ok   $n" > "$RES/$k"
    else
        { echo "  FAIL $n (see $OUT/$n.log)"; tail -15 "$OUT/$n.log" | sed 's/^/       /'; } > "$RES/$k"
        : > "$RES/$k.fail"
    fi
    echo "$(( $(date +%s) - t0 )) s $n" > "$RES/$k.time"
}
GLOCK="${X0X_GATE_LOCK:-/workspace/x0x37-refs/gate.lock}"
install_path() { test_class "$1"; case "$TEST_CLASS" in W) return 0 ;; S) [ "$1" != rec-live ] ;; *) return 1 ;; esac; }
lk() {                  # name command...: start the command. An install-path test holds the shared gate lock while it runs
    n_="$1"; shift
    if install_path "$n_" && [ "${X0X_GATE_HOLDS_LOCK:-0}" != 1 ] && [ -w "$GLOCK" ] && command -v flock >/dev/null 2>&1; then
        flock -s -n "$GLOCK" true || echo "  $n_ waits for the gate lock (a gate holds it): $GLOCK" >&2
        flock -s -o "$GLOCK" "$@"
    else
        "$@"
    fi
}
needs_gen() {           # name: the test includes the headers in build/gen (host-build writes them)
    case "$1" in drum-pan|stereo|rec-live|copy-semantics|song-actions|ui-feedback|project-303-stream|project-protocol|project-runtime|project-stage) return 0 ;; *) return 1 ;; esac
}
gen_wait() {            # name: 0 if the test must wait for host-build (host-build runs in this run and did not end)
    needs_gen "$1" || return 1
    [ -e "$OUT/iso/.gen-done" ] && return 1
    [ -z "$X0X_TESTS" ] && return 0
    case " $X0X_TESTS " in *" host-build "*) return 0 ;; *) return 1 ;; esac
}
gen_lock() { until mkdir "$OUT/iso/.lock-gen" 2>/dev/null; do sleep 0.1; done; }
gen_unlock() { rmdir "$OUT/iso/.lock-gen"; }
run() {
    name="$1"; shift
    T_SEQ=$((T_SEQ + 1))
    if [ -n "$X0X_TESTS" ]; then case " $X0X_TESTS " in *" $name "*) ;; *) return ;; esac; fi
    if [ "$JOBS" -le 1 ]; then
        if run_test "$name" "$@" > "$OUT/$name.log" 2>&1; then
            echo "  ok   $name"
        else
            echo "  FAIL $name (see $OUT/$name.log)"
            tail -15 "$OUT/$name.log" | sed 's/^/       /'
            FAIL=1
        fi
        return
    fi
    test_class "$name"
    cls=$TEST_CLASS
    case "$PHASE$cls" in WW|1P|1G|2S|3C) ;; *) return ;; esac
    key=$(printf '%04d' "$T_SEQ")
    if [ "$cls" = S ] || [ "$cls" = C ]; then
        run_one "$key" "$name" "$@"
        return
    fi
    gw=0; gen_wait "$name" && gw=1
    [ "$cls" = W ] || [ $gw = 1 ] || read -r _ <&3       # wait for a free job slot (class W does not use one)
    (
        export HOME="$PWD/$OUT/iso/$name/home" TMPDIR="$PWD/$OUT/iso/$name/tmp"
        mkdir -p "$HOME" "$TMPDIR"
        if [ $gw = 1 ]; then      # wait for the end of host-build without a job slot, then take a slot
            until [ -e "$OUT/iso/.gen-done" ]; do sleep 0.1; done
            [ "$cls" = W ] || read -r _ <&3
        fi
        [ "$cls" = G ] && gen_lock
        run_one "$key" "$name" "$@"
        [ "$cls" = G ] && gen_unlock
        [ "$name" = host-build ] && : > "$OUT/iso/.gen-done"   # also after a FAIL: the waiting tests then start and report
        [ "$cls" = W ] || echo >&3
    ) &
}
note() {                # a skip line, in the serial order
    T_SEQ=$((T_SEQ + 1))
    if [ "$JOBS" -le 1 ]; then echo "$1"
    elif [ "$PHASE" = 2 ]; then echo "$1" > "$RES/$(printf '%04d' "$T_SEQ")"
    fi
}
tests_body() {
W="-O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror"
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"   # its reference is double on purpose
run fastmath-ubsan sh tests/host/run_fastmath_ubsan.sh  # negative exponent shifts must be defined
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
run seq-pingpong-resize python3 tests/host/seq_pingpong_resize_test.py   # 303 live-length shrink never emits disabled steps
run clk-lock sh -c "$CC $W -o $OUT/clk_lock_test tests/host/clk_lock_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c && $OUT/clk_lock_test"   # MID-15: one clock input
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run bass303_lite sh -c "$CC -std=c99 -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -o $OUT/bass303_lite_test tests/host/bass303_lite_test.c firmware/src/dsp/bass303.c -lm && $OUT/bass303_lite_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
run undo sh -c "$CC -O2 -Wall -Wextra -Wno-unused-function -o $OUT/undo_test tests/host/undo_test.c && $OUT/undo_test"
run undo-capacity sh tests/host/run_undo_capacity.sh
run lz sh -c "$CC $W -o $OUT/lz_test tests/host/lz_test.c && $OUT/lz_test"
run motion sh -c "$CC $W -o $OUT/motion_test tests/host/motion_test.c firmware/src/seq/motion.c && $OUT/motion_test"
run host-build sh host/build_host.sh   # it writes build/gen: the tests in needs_gen wait for it (see the top)
run project-303-stream sh tests/host/run_project_303_stream.sh
run project-protocol sh tests/host/run_project_protocol.sh
run editor-ota-dispatch python3 -B tests/host/editor_ota_dispatch_test.py
run storage-scratch sh tests/host/run_storage_scratch.sh
run project-stage sh tests/host/run_project_stage.sh
run project-runtime sh tests/host/project_runtime_test.sh
run font-pack python3 tests/font_pack_test.py
run smc-protocol sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -o $OUT/smc_protocol_test tests/host/smc_protocol_test.c && $OUT/smc_protocol_test"
run smc-fader python3 tests/host/smc_fader_test.py
run smc-buttons python3 tests/host/smc_buttons_test.py
run smc-queue python3 tests/host/smc_queue_test.py
run smc-solo-overflow python3 tests/host/smc_solo_overflow_test.py
run smc-host-data7 python3 tests/host/smc_host_data7_test.py
run smc-host-overflow-data7 python3 tests/host/smc_host_overflow_data7_test.py
run smc-sender-isolation python3 tests/host/smc_sender_isolation_test.py
run smc-ble-payload python3 tests/host/smc_ble_payload_test.py
run smc-link python3 tests/host/smc_link_test.py
run smc-reconnect-epoch python3 tests/host/smc_ble_reconnect_epoch_test.py   # prevent stale central MIDI after BLE reconnect
run ble-central-first-notification python3 tests/host/ble_central_first_notification_test.py
run ble-midi-receive sh tests/host/ble_midi_receive_test.sh
run ble-capture-replay python3 tests/host/ble_capture_replay_test.py
run ble-acl-start-flags sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble -o $OUT/ble_acl_start_flags_test tests/host/ble_acl_start_flags_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_acl_start_flags_test"
run ble-hci-completed sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble -o $OUT/ble_hci_completed_packets_test tests/host/ble_hci_completed_packets_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_hci_completed_packets_test"
run ble-acl-zero-credit sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble -o $OUT/ble_acl_zero_credit_test tests/host/ble_acl_zero_credit_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_acl_zero_credit_test"
run ble-link-epoch-reset sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble -o $OUT/ble_link_epoch_reset_test tests/host/ble_link_epoch_reset_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_link_epoch_reset_test"
run ble-disconnect-status sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src/ble -o $OUT/ble_disconnect_status_test tests/host/ble_disconnect_status_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_disconnect_status_test"
run smc-reachability sh tests/host/smc_reachability_test.sh
run smc-ui sh tests/host/smc_ui_test.sh
run smc-ui-negative python3 tests/host/smc_ui_negative_test.py
run smc-lock sh tests/host/smc_lock_test.sh
run smc-interaction-negative python3 tests/host/smc_interaction_negative_test.py
run smc-mapping python3 tests/host/smc_mapping_test.py
run smc-drum-mutes sh tests/host/smc_drum_mutes_test.sh
run smc-drum-rs sh tests/host/smc_drum_rs_test.sh
run smc-performance-abort sh tests/host/smc_performance_abort_test.sh
run smc-filter python3 tests/host/smc_filter_test.py
run smc-feedback python3 tests/host/smc_feedback_test.py
run smc-record-feedback sh tests/host/smc_record_feedback_test.sh
run smc-part-select sh tests/host/smc_part_select_test.sh
run smc-clip-visibility sh tests/host/smc_clip_visibility_test.sh
run drum-pan sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/drum_pan_test tests/host/drum_pan_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c -lm && $OUT/drum_pan_test"
run stereo sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/stereo_test tests/host/stereo_test.c firmware/src/dsp/master.c firmware/src/dsp/fxbus.c -lm && $OUT/stereo_test"
run master sh -c "$CC $W -Wno-double-promotion -o $OUT/master_test tests/host/master_test.c firmware/src/dsp/master.c -lm && $OUT/master_test"
run drum909-adpcm sh tests/host/run_drum909_adpcm.sh   # the 909 cymbals as IMA ADPCM (A/B study reference)
for t in drum909 drum808 bass303 breaks; do
    [ -f "tests/host/run_$t.sh" ] && run "$t" sh "tests/host/run_$t.sh"
done
run install-gate-guard sh tests/host/test_install_gate_guard.sh   # tools/require_gate_pass.sh: the install needs the gate PASS of the exact tree
run queue-guard sh tests/host/test_queue_guard.sh   # tools/require_gate_pass.sh check 6: each open QUEUE.md line is in the install commit
run install-guards sh tests/host/test_install_guards.sh   # tools/fm1_install.py (checks 3, 14): foreground or screen session, one hardware job at a time
run install-head sh tests/host/test_install_head.sh   # tools/fm1_install.py (check 4): the package bootloader bytes = the reference read-back
run release-tree sh tests/host/test_release_tree.sh   # tools/release_tree_check.sh (check 7): no refs/jieli or refs/oscuro path, no refs-oscuro commit in a release
run prepush-trim sh tests/host/test_prepush_trim.sh   # tools/prepush-check.sh: the box update hook runs the quick check, so the client does not
run prepush-allow sh tests/host/test_prepush_allow.sh   # tools/pre-push.hook part 0 (check 6): push only to the box path or x0x37.allowedRemote
run gate-configs sh tests/host/test_gate_configs.sh   # tools/gate.sh: the dev and rel configs, the caller build variables removed
run gate-trap bash tests/host/test_gate_trap.sh   # tools/gate.sh (checks 5, 13): the KILLED lines name the sender PID and cwd, SCRIPT CHANGED in gate.log
run quickcheck-plan python3 tests/host/test_quickcheck_plan.py
run ctl-compile python3 tests/host/test_ctl_compile.py
run host-setup python3 tests/host/test_host_setup.py
run runner-admission python3 tests/host/test_runner_admission.py
run update-names bash tests/host/test_update_names.sh   # tools/update.hook parts 1c, 1d (checks 9, 8): new branch names, integration needs -o by=merge-integrator
run gate-reason python3 tests/host/test_gate_reason.py
run validation-inputs python3 tests/host/test_validation_inputs.py
run dev-validate python3 tests/host/test_dev_validate.py DevelopmentValidation
run smk-update-contract python3 tests/host/test_smk_update_contract.py
run gen-order sh tests/host/test_gen_order.sh   # this file: drum-pan and stereo start after host-build (build/gen), with 8 jobs and 1 job
run quickcheck-devbug bash tests/host/test_quickcheck_devbug.sh   # tools/quickcheck.sh device bug step (checks 16, 17): an INLOG replay test that fails on the parent
run status-lanes bash tests/host/test_status_lanes.sh   # tools/status.sh (check 12): lane/<agent>-* commits count as agent output
run status-isolation bash tests/host/test_status_isolation.sh   # tools/status.sh (check 30): output folder, no write to the live orchestrator folder
run loop-bars sh tests/host/run_loop_bars.sh            # engine.c loop_bars: the bar guess uses the loop's rate
# Shared installer protocol and model guards use synthetic devices; package parity uses SMK builds.
run installer python3 tests/install_test.py
# SMK-37 update fixtures use the Pro app/loader. Elite and MKE identities exercise the same layout.
run locked-settings python3 tools/check_locked_settings.py   # hardware-relevant settings
run smk-flash sh -c "$CC -O2 -Wall -Wno-unused-function -DBOARD_SMK37=1 -Ifirmware/hal -o $OUT/smk_flash_test \
    tests/smk_flash_test.c && $OUT/smk_flash_test"
run smk-storage sh -c "$CC -O2 -DBOARD_SMK37=1 -o $OUT/storage_test_smk tests/storage_test.c && $OUT/storage_test_smk"
run smk-apc40-map sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_apc40_map_test tests/smk_apc40_map_test.c && $OUT/smk_apc40_map_test"
run smk-apc40 python3 tests/smk_apc40_test.py
run smk-clip-ble-clear sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/ble -o $OUT/smk_clip_ble_clear_test tests/smk_clip_ble_clear_test.c && $OUT/smk_clip_ble_clear_test"
run smk-clip-keys sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_clip_keys_test tests/smk_clip_keys_test.c && $OUT/smk_clip_keys_test"
run smk-chord-recognize sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_chord_recognize_test tests/smk_chord_recognize_test.c && $OUT/smk_chord_recognize_test"
run smk-clip-chord-display sh tests/host/smk_clip_chord_display_test.sh
run smk-ck-page-layout python3 tests/smk_ck_page_layout_test.py   # CLP-75: no top bar on the stage page, help line at the bottom
run smk-arrows sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_arrows_test tests/smk_arrows_test.c && $OUT/smk_arrows_test"   # < > route and hold repeat
run smk-arrows-owner python3 tests/smk_arrows_owner_test.py   # one owner for each < > press in clip mode (CLP-27)
run apc40-tables sh -c "python3 tools/apc_pad_colors.py --pad-h $OUT/apc40_pad_default_gen.h --lcd-h $OUT/apc40_lcd565_gen.h --gamma-h $OUT/apc40_pad_gamma_gen.h && \
    $CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -I$OUT -o $OUT/apc40_tables_test tests/apc40_tables_test.c && $OUT/apc40_tables_test"   # the LCD table and the pad calibration start point against their tool
run smk-clip-idle python3 tests/smk_clip_idle_test.py   # CLP-05: idle clip squares = half of the group full shade
run smk-clip-idle-pad python3 tests/smk_clip_idle_pad_test.py   # CLP-13: idle clip pads send the full owner level
run apc40-pad-owner python3 tests/apc40_pad_owner_test.py   # CLP-13: the owner-calibrated pad table, the export, the clip pad bytes
run smk-padcal sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_padcal_test tests/smk_padcal_test.c && $OUT/smk_padcal_test"   # CLP-79: pad calibration logic
run padcal-view python3 tests/padcal_view_test.py   # CLP-79: selected LCD square boost, entry names; pads unchanged
run smk-padcal-export sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_padcal_export_bin tests/smk_padcal_test.c && python3 tests/smk_padcal_export_test.py $OUT/smk_padcal_export_bin"   # CLP-79: export round trip
run smk-padcal-wiring python3 tests/smk_padcal_wiring_test.py   # CLP-79: menu row, keys, knobs, pads, LCD
run pad-placement-doc python3 tests/pad_placement_doc_test.py   # PAD_MAP.md physical pad placement = PADLED_SLOT, SMK_PADMUX
run padcal-flag python3 tests/padcal_flag_test.py   # X0X_DEV_PADCAL: flag 0 (release) has no pad calibration code, flag 1 keeps it
run apc40-lcd565-csv python3 tests/apc40_lcd565_csv_test.py   # CLP-05: the LCD table is plain RGB565 of the Akai CSV, all 128
run apc-palette python3 tests/apc_palette_test.py   # one source, hue order, distinct colours, INLOG replay
run installer-targets python3 tests/make_pages_test.py
run diag-size python3 tests/diag_size_test.py           # a76: ED_DIAG stays in its buffers
run smk-inlog sh -c "$CC -std=c99 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/smk_inlog_test tests/smk_inlog_test.c && $OUT/smk_inlog_test"   # a76: inbound recorder ring
run ble-central-diag python3 -B tests/ble_central_diag_test.py
run inlog-tool python3 tests/inlog_tool_test.py      # a76: ED_INLOG dump text
run smk-clip-ble-notify sh -c "$CC -std=c99 -O2 -Wall -Wextra -Wno-unused-function -Werror -Ifirmware/src -Ifirmware/hal -Ifirmware/src/ble -o $OUT/smk_clip_ble_notify_test tests/smk_clip_ble_notify_test.c && $OUT/smk_clip_ble_notify_test"   # a76: no pad colour while notify is off
run smk-clip-dark sh tests/smk_clip_dark_test.sh   # a76: real clip and pad LED code: dark 10 s after Loopy goes away
run smk-clip-usb-clear sh tests/host/smk_clip_usb_clear_test.sh   # CLP-18: a link end clears only the cells of that link (real usb.c)
run smk-clip-isolation python3 tests/host/smk_clip_isolation_test.py
run smk-clip-knob sh tests/host/smk_clip_knob_test.sh   # clip mode knobs 2-8 send CC 48-54 on USB and Bluetooth (real usb.c)
run smk-fader-pickup sh tests/host/smk_fader_pickup_test.sh   # fader pickup and input draining across project holds
run smk-clip-wait sh tests/host/smk_clip_wait_test.sh   # CLP-80: the clip screen waits for Loopy Pro (no host: no grid)
run smk-apc40-violet python3 tests/smk_apc40_violet_test.py   # a76: violet, purple and pink clips are not blue
if [ -f build/smk37/x0x.bin ] && [ -f build/smk37/loader/ota.bin ] && [ -n "${AC79_SDK:-}" ]; then
    run smk-package sh -c "python3 tools/fm1pkg_make.py --board smk37 build/smk37/x0x.bin build/smk37/loader/ota.bin $OUT/smk_new.fwsc >/dev/null && \
        python3 tests/smk_pkg_test.py $OUT/smk_new.fwsc"
    run smk-ota-entry sh -c "$CC -DBOARD_SMK37=1 -o $OUT/ota_test_smk tests/ota_test.c && $OUT/ota_test_smk $OUT/smk_new.fwsc"
    for m in 1 2; do                     # the MODEL=elite / mke builds of ota.c
        run smk-ota-entry-model$m sh -c "$CC -DBOARD_SMK37=1 -DT_MODEL=$m -o $OUT/ota_test_smk_m$m tests/ota_test.c && \
            $OUT/ota_test_smk_m$m $OUT/smk_new.fwsc"
    done
    run smk-update-loader sh -c "head -c 200000 build/smk37/x0x.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py --board smk37 $OUT/old_app.bin build/smk37/loader/ota.bin $OUT/smk_old.fwsc >/dev/null && \
        $CC -DBOARD_SMK37=1 -o $OUT/ldr_test_smk tests/ldr_test.c && $OUT/ldr_test_smk $OUT/smk_old.fwsc $OUT/smk_new.fwsc"
else
    note "  skip SMK-37 Pro package tests (needs build/smk37 app/loader and AC79_SDK; run ./build.sh --board smk37)"
fi
[ -f build/x0x-smk37.fwsc ] && run smk-build-package python3 tests/smk_pkg_test.py build/x0x-smk37.fwsc
[ -f build/x0x-smk37-elite.fwsc ] && run smk-build-package-elite python3 tests/smk_pkg_test.py build/x0x-smk37-elite.fwsc "SMK-37 Elite_900"
[ -f build/x0x-smk37-mke.fwsc ] && run smk-build-package-mke python3 tests/smk_pkg_test.py build/x0x-smk37-mke.fwsc "MKE-P37_900"
# SMK-37 Bluetooth MIDI: the HCI mailbox, the BLE-MIDI host, the image check, the park handshake (tests/ble_test.c);
# the controller image in a package, Python and JavaScript (tests/ble_pkg_test.py, needs the SMK build)
run ble-sysex-bounds sh -c "$CC -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o $OUT/ble_sysex_bounds_test tests/host/ble_sysex_bounds_test.c firmware/src/ble/ble_midi_host.c && $OUT/ble_sysex_bounds_test"
run ble sh -c "$CC $W -Ifirmware/hal -Ifirmware/src/ble -o $OUT/ble_test tests/ble_test.c -lpthread && $OUT/ble_test"
[ -f build/x0x-smk37.fwsc ] && run ble-package python3 tests/ble_pkg_test.py
run upload-images sh -c "python3 tests/host/make_test_loops.py build/test_loops && \
    python3 tools/upload_breaks.py --rate 22050 --nslots 3 --dry-run build/test_upload build/test_loops/loop*.wav && \
    python3 tools/upload_breaks.py --dry-run build/test_upload44 build/test_loops/loop*.wav && \
    python3 tools/upload_breaks.py --rate 22050 --slots 3 --dry-run build/test_upload44s3 build/test_loops/loop1.wav"   # 22.05 kHz (3 x 80), the 44.1 kHz default (80/120/44), slot 3
run upload-tools python3 tests/upload_tools_test.py   # upload_breaks.py on a mocked device; one-shot trim
command -v node >/dev/null 2>&1 && run breaks-web node web/test_breaks.mjs   # the browser uploader: upload_breaks.py's bytes
command -v node >/dev/null 2>&1 && run breaks-page node web/test_breaks_page.mjs   # the page against a mocked device
command -v node >/dev/null 2>&1 && run projects-web node web/test_projects.mjs   # exact project and sample bundles, transfer and recovery
command -v node >/dev/null 2>&1 && run projects-page node web/test_projects_page.mjs   # actual project page handlers against a mocked device
command -v node >/dev/null 2>&1 && run install-slots-web node web/test_install_slots.mjs   # the installer keeps the slots over a slot map change
run slot-backup python3 tests/x0x_slots_test.py            # tools/x0x_slots.py against a mocked device
run slot-backup-durable python3 tests/x0x_slots_durable_backup_test.py   # directory fsync before OTA
run slot-archive-zone python3 tests/x0x_slots_zone_guard_test.py   # CRC-valid malformed slot zones fail before flash writes
run slot-verify-order python3 tests/x0x_slots_verify_order_test.py   # fail closed before a later slot write
run slot-read-reply python3 tests/x0x_slots_read_reply_test.py   # malformed/noncanonical SMP_READ backup framing
run slot-ack-integrity python3 tests/x0x_slots_ack_integrity_test.py   # refuse stale or malformed SMP_WRITE/BEGIN/END/ERASE ACKs
run slot-echo-7bit python3 tests/x0x_slots_echo_7bit_test.py   # refuse aliased non-7-bit sample-slot read/write echoes
run slot-migration-matching python3 tests/x0x_slots_migration_matching_test.py   # safe maximum matching for pre-OTA slot backup restoration
run installer-slots python3 tests/install_slots_test.py   # fm1_install.py: backup, install, restore over a slot map change
run installer-restore-status python3 tests/fm1_install_restore_status_test.py   # installed-but-unrestored slots are never a successful install
run installer-ota-finish-ack python3 tests/fm1_install_finish_ack_test.py   # failed FINISH_CHECK/WRITE reply is not completion
run installer-backup-snapshot python3 tests/fm1_install_backup_snapshot_test.py   # refresh migration plan after verified SMP_INFO backup
run installer-ctl python3 tests/install_ctl_test.py       # fm1_install.py: the Bluetooth image read-back (CTL_READ) and its CRC32
run ctl-read-framing python3 tests/ctl_read_framing_test.py  # reject corrupt/overlong Bluetooth controller readback before accepting CRC
run ctl-publish-atomic python3 tests/add_ctl_atomic_test.py   # atomic controller-package publication and failure handling
# live recording on the whole app (host/x0x_host.c without its script main; needs build/gen from host-build)
run rec-live sh -c "$CC -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o $OUT/rec_live_test tests/host/rec_live_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c \
    firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/app/engine.c -lm && $OUT/rec_live_test"
run rec-live-smk sh -c "$CC -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o $OUT/rec_live_smk_test tests/host/rec_live_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c \
    firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/app/engine.c -lm && $OUT/rec_live_smk_test"
run copy-semantics sh tests/host/copy_semantics_test.sh
run song-actions sh tests/host/song_actions_test.sh
run corner-coverage python3 tests/host/corner_coverage_test.py
run ui-feedback sh tests/host/ui_feedback_test.sh
for s in tests/scenarios/*.x0x; do
    n=${s##*/}; n=${n%.x0x}
    run "scenario-$n" build/host/x0x_host "$s" "build/scenarios/$n"
done
# the device scratch (x0x.c): the PERF TEST patterns and the LZ hash table of a save in the same 4 KiB.
# The test must play the same sound as with its own buffers (scenario perfsave: a save just before the test).
run perf-scratch sh -c "X0X_HOST_DEFS='-include $OUT/scratch_defs.h' X0X_HOST_OUT=$OUT/x0x_host_scratch sh host/build_host.sh 2>/dev/null && \
    mkdir -p $OUT/perf-scratch && $OUT/x0x_host_scratch tests/scenarios/perf.x0x $OUT/perf-scratch && \
    $OUT/x0x_host_scratch tests/scenarios/perfsave.x0x $OUT/perf-scratch && \
    cmp $OUT/perf-scratch/perfsave.wav build/scenarios/perfsave/perfsave.wav"
}
if [ "$JOBS" -le 1 ]; then
    tests_body
else
    rm -rf "$RES" "$OUT/iso"; mkdir -p "$RES" "$OUT/iso"
    fifo="$OUT/iso/.jobs"; mkfifo "$fifo"; exec 3<>"$fifo"; rm -f "$fifo"
    i=0; while [ $i -lt "$JOBS" ]; do echo >&3; i=$((i + 1)); done
    for PHASE in W 1 2 3; do      # no wait after W: class W runs beside phase 1, and the wait of phase 1 ends it
        T_SEQ=0
        tests_body
        [ "$PHASE" = W ] || wait
    done
    exec 3>&-
    for f in $(ls "$RES" | grep -v -e '\.fail$' -e '\.time$' | sort); do cat "$RES/$f"; done
    ls "$RES" | grep -q '\.fail$' && FAIL=1
fi
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
