# Independent durable regression review

Source and startup fix: PASS. See STARTUP-REVIEW.md for independent reproduction, safety reasoning and separate48-case ASan/UBSan startup-edge fixture. Runtime file remains SHA256 `7c3748bc563ecf7b21eac213c2c07721bf2128f55cf10a073ab9cfb64329767b`, Git blob `aeb7861c24f6383784b2fc6350a06e768a8a33fb`.

Durable tests reviewed and independently run from task94/repo:

- `SMK_FADER_STATE_OUT=build/host/independent-startup-review sh tests/host/smk_fader_state_test.sh`: PASS240 process-fresh cold combinations and96 production UI/engine sequences/14908 checks. Log durable-state.log.
- `SMK_FADER_STATE_OUT=build/host/independent-startup-sanitized SAN_FLAGS='-fsanitize=address,undefined' sh tests/host/smk_fader_state_test.sh`: same PASS with address/undefined sanitizers. Log durable-state-sanitized.log.
- `SMK_FADER_STATE_OUT=build/host/independent-startup-negative SMK_FADER_COLD_ONLY=1 SMK_FADER_SOURCE=/Users/mac/Documents/Codex/2026-10-25/task-94/installed-main_fm1.c sh tests/host/smk_fader_state_test.sh`: FAIL as expected, first call frame206/parked100 publishes one write and saves100 instead of64. Log durable-negative.log. This is a meaningful installed-source negative, not a compile-error control.
- `python3 tests/host/smk_fader_scanner_test.py`: PASS959525 scanner assertions with ASan/UBSan. Log durable-scanner.log. Limits documented in REVIEW.md remain applicable.
- tests/run_tests.sh registers both smk-fader-state and smk-fader-scanner selectors; git diff --check PASS.

The startup shell launches separate processes for each static-state case, extracts actual smk_faders and links production UI, store, engine queue, motion and DSP. Its engine wrapper adds only an accessor for actual mix_level. Engine writes forward to the real setter. Banks/strips, below/above parked positions, early/late startup and initial held bank buttons are exercised; next unchanged sample stays parked, release is consumed and a fresh crossing works. The additional independent48-case fixture covers equal-target/busy and a later new bank-button edge.

The scanner script remains the reviewed extraction-based host model, not a hardware/emulated-peripheral qualification. Two registered selectors do not claim every user symptom is fixed.

Current final test identity (SHA256):

- main_fm1.c: `7c3748bc563ecf7b21eac213c2c07721bf2128f55cf10a073ab9cfb64329767b`
- smk_fader_state_test.c: `bd0e3ccc02131cddaa82f27639824438e5aee3ce5d9976bdea2936c3229a94e5`
- smk_fader_state_test.sh: `4c495cab1783b6fb9cf09528d18a91757082ddaac2c380070193c486a9480592`
- smk_fader_scanner_test.py: `e8fc1e703f39f3613a97f73290f3ddb18d30ac236ca3f3c909ac56f98ae58bc4`
- tests/run_tests.sh: `7fe250e576218b46088b43b974f538155ac797a2de2287699b7195cbfa9fb91a`

After the optional mute-policy audit reached its final stable source, the reviewer reran all240 cold processes and96 normal sequences with ASan/UBSan on the exact final C blob above: PASS (durable-final-sanitized.log). The exact final blob also failed the installed-source negative as expected (durable-final-negative.log, line98 no-write assertion). Review did not run or accept the separate opt-in mute-policy audit behavior requirements; that branch does not execute in normal/cold modes. The reported75 desired-behavior mismatches belong to the separate open investigation and are not hidden by these passing startup tests.

No runtime review blocker found. Native image budget, model packages and device verification remain parent-owned separate gates.

Final source-only registration follow-up: reviewed smk-fader-state in TEST_CLASS=G alongside other tests that generate shared build/gen assets. The runner executes generator-class tests serially, preventing concurrent generated-header writes. smk-fader-scanner remains an ordinary isolated-output test. Runtime and both fixture identities above are unchanged; no behavioral rerun was required for classification-only change. Parent reports registered serial three-selector PASS; this reviewer verifies the registration source and final runner hash, not a new execution of that parent run.

Final optional-tooling portability follow-up: reviewed scanner script SHA256 e8fc1e703f39f3613a97f73290f3ddb18d30ac236ca3f3c909ac56f98ae58bc4. The compiler flags now expand shlex.split(os.environ.get("SAN_FLAGS", "")) instead of hardcoding address/undefined sanitizers; subprocess still receives an argument array, with no shell execution. Default uses the ordinary compiler and explicitly supplied SAN_FLAGS enables available sanitizer tooling. Extracted production functions, generated C assertions and test input sequences are unchanged. Inspected parent scanner-final-default.log and scanner-final-sanitized.log: each records PASS959525 assertions. Prior independent sanitizer run covers the same generated C. No other behavioral rerun warranted for this compiler-option-only change; git diff --check PASS.
