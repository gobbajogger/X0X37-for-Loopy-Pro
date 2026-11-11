#!/bin/sh
# RED/GREEN/MUTANT self-test for roadmap_evidence_check.sh
# Uses temp fixtures only; no network. POSIX sh + the checker under test.
set -eu

HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
CHECK=${CHECK:-"$HERE/roadmap_evidence_check.sh"}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/roadmap_evidence_check_test.XXXXXX")
trap 'rm -rf "$TMP"' EXIT INT HUP TERM
mkdir -p "$TMP"

pass=0
fail=0

note() { printf '%s\n' "$*"; }

expect_nonzero() {
  name=$1
  shift
  set +e
  out=$("$@" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    note "PASS $name (exit=$rc)"
    pass=$((pass + 1))
  else
    note "FAIL $name (expected non-zero, got 0)"
    note "  output: $out"
    fail=$((fail + 1))
  fi
}

expect_zero() {
  name=$1
  shift
  set +e
  out=$("$@" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    note "PASS $name (exit=0)"
    pass=$((pass + 1))
  else
    note "FAIL $name (expected 0, got $rc)"
    note "  output: $out"
    fail=$((fail + 1))
  fi
  # stash last out for GREEN inspection
  LAST_OUT=$out
}

PIN_C=5f578fc0642a1200a5163802bcd69277c511bfc7
PIN_B=a204a54c989d6f323d619e70f799427603756541

write_good_present() {
  tree=$1
  evid=$2
  mkdir -p "$tree/firmware/src/app" "$evid"
  printf 'x\n' > "$tree/firmware/src/app/engine.c"
  cat > "$evid/#1.json" << JSON
{
  "row_id": "#1",
  "roadmap_pin_commit": "$PIN_C",
  "roadmap_blob": "$PIN_B",
  "status": "PRESENT",
  "code_paths": ["firmware/src/app/engine.c"],
  "test_refs": [
    {
      "file": "tests/host/example_test.c",
      "test": "engines_selectable",
      "result": "PASS",
      "source_sha": "ce27812951555ff7ed9694bf99d20a65dd5f410b"
    }
  ],
  "proof_source": "fixture",
  "notes": "GREEN fixture"
}
JSON
}

# --- GREEN ---
G=$TMP/green
write_good_present "$G" "$G/docs/roadmap-evidence"
# also one UNPROVEN
cat > "$G/docs/roadmap-evidence/#2.json" << JSON
{
  "row_id": "#2",
  "roadmap_pin_commit": "$PIN_C",
  "roadmap_blob": "$PIN_B",
  "status": "UNPROVEN",
  "proof_source": "fixture",
  "notes": "no proof yet"
}
JSON
expect_zero GREEN sh "$CHECK" "$G" "$G/docs/roadmap-evidence"
printf '%s\n' "$LAST_OUT" | head -20
printf '%s\n' "$LAST_OUT" | grep -q 'PRESENT_OK' || { note "FAIL GREEN missing PRESENT_OK"; fail=$((fail+1)); pass=$((pass-1)); }
printf '%s\n' "$LAST_OUT" | grep -q 'UNPROVEN' || { note "FAIL GREEN missing UNPROVEN"; fail=$((fail+1)); }
printf '%s\n' "$LAST_OUT" | grep -q "ROADMAP_PIN blob=$PIN_B commit=$PIN_C" || {
  note "FAIL GREEN pin line"; fail=$((fail+1));
}

# --- RED: missing path ---
R1=$TMP/red_missing_path
write_good_present "$R1" "$R1/docs/roadmap-evidence"
rm -f "$R1/firmware/src/app/engine.c"
expect_nonzero RED_missing_path sh "$CHECK" "$R1" "$R1/docs/roadmap-evidence"
# MUTANT: if checker skipped path existence, this RED would pass wrongly.
note "MUTANT RED_missing_path: drop the os.path.isfile(code_path) check -> RED would exit 0; this case catches it."

# --- RED: empty test result ---
R2=$TMP/red_empty_result
write_good_present "$R2" "$R2/docs/roadmap-evidence"
python3 - <<PY
import json
p="$R2/docs/roadmap-evidence/#1.json"
d=json.load(open(p))
d["test_refs"][0]["result"]=""
json.dump(d, open(p,"w"), indent=2)
PY
expect_nonzero RED_empty_result sh "$CHECK" "$R2" "$R2/docs/roadmap-evidence"
note "MUTANT RED_empty_result: treat empty result as PASS (or skip result validation) -> RED would exit 0; this case catches it."

# --- RED: disagreeing pin ---
R3=$TMP/red_pin
write_good_present "$R3" "$R3/docs/roadmap-evidence"
cat > "$R3/docs/roadmap-evidence/#2.json" << JSON
{
  "row_id": "#2",
  "roadmap_pin_commit": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "roadmap_blob": "$PIN_B",
  "status": "UNPROVEN",
  "proof_source": "fixture"
}
JSON
expect_nonzero RED_disagreeing_pin sh "$CHECK" "$R3" "$R3/docs/roadmap-evidence"
note "MUTANT RED_disagreeing_pin: keep only the first file's pin and never compare later files -> RED would exit 0; this case catches it."

# --- RED: malformed JSON ---
R4=$TMP/red_malformed
write_good_present "$R4" "$R4/docs/roadmap-evidence"
printf '{not-json\n' > "$R4/docs/roadmap-evidence/#1.json"
expect_nonzero RED_malformed_json sh "$CHECK" "$R4" "$R4/docs/roadmap-evidence"
note "MUTANT RED_malformed_json: wrap json.load in a bare except that continues as UNPROVEN/OK -> RED would exit 0; this case catches it."

# --- Strict fixtures: synthetic files and run entries only; nothing is executed. ---
CANDIDATE=ce27812951555ff7ed9694bf99d20a65dd5f410b

write_strict_present() {
  write_good_present "$1" "$1/docs/roadmap-evidence"
  mkdir -p "$1/tests/host"
  printf 'fixture\n' > "$1/tests/host/example_test.c"
  printf '#!/bin/sh\nif [ 1 = 0 ]; then :; else run engines_selectable true; fi\n' > "$1/tests/run_tests.sh"
  printf '#1\n' > "$1/docs/roadmap-evidence/REQUIRED_ROWS.txt"
}

expect_strict_failure() {
  label=$1
  fixture=$2
  reason=$3
  row=${4:-'#1'}
  set +e
  out=$(sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$fixture" "$fixture/docs/roadmap-evidence" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 1 ] &&
     printf '%s\n' "$out" | grep -F "STRICT_FAIL $row: " | grep -Fq "$reason" &&
     printf '%s\n' "$out" | grep -q '^STRICT_RESULT FAIL rows=[0-9][0-9]* required=[0-9][0-9]*$'; then
    note "PASS $label (strict exit=1; $reason)"
    pass=$((pass + 1))
  else
    note "FAIL $label (expected strict exit=1 with $row: $reason; got $rc)"
    note "  output: $out"
    fail=$((fail + 1))
  fi
}

SG=$TMP/strict_green
write_strict_present "$SG"
expect_zero STRICT_GREEN sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$SG" "$SG/docs/roadmap-evidence"
if [ "$LAST_OUT" != 'STRICT_RESULT PASS rows=1 required=1' ]; then
  note "FAIL STRICT_GREEN summary: $LAST_OUT"
  fail=$((fail + 1))
fi

# The four Watchdog negatives must remain accepted by legacy reporting.
for kind in FAIL NOT_RUN UNPROVEN missing_test_file; do
  fixture=$TMP/strict_$kind
  write_strict_present "$fixture"
  python3 - "$fixture" "$kind" <<'PY'
import json, pathlib, sys
root = pathlib.Path(sys.argv[1])
kind = sys.argv[2]
p = root / "docs/roadmap-evidence/#1.json"
d = json.loads(p.read_text())
if kind in ("FAIL", "NOT_RUN"):
    d["test_refs"][0]["result"] = kind
elif kind == "UNPROVEN":
    d["status"] = "UNPROVEN"
else:
    d["test_refs"][0]["file"] = "tests/host/does_not_exist.c"
p.write_text(json.dumps(d))
PY
  expect_zero "LEGACY_$kind" sh "$CHECK" "$fixture" "$fixture/docs/roadmap-evidence"
  case "$kind" in
    FAIL|NOT_RUN) reason=result_not_PASS ;;
    UNPROVEN) reason=status_not_PRESENT ;;
    missing_test_file) reason=missing_or_outside_test_file ;;
  esac
  expect_strict_failure "STRICT_RED_$kind" "$fixture" "$reason"
done

# Additional contract probes isolate checks (a)-(e) for real mutant detection.
for kind in missing_row extra_row wrong_sha missing_refs forged_test missing_code_path \
            bad_status bad_pin malformed_json; do
  fixture=$TMP/contract_$kind
  write_strict_present "$fixture"
  python3 - "$fixture" "$kind" <<'PY'
import json, pathlib, sys
root = pathlib.Path(sys.argv[1])
kind = sys.argv[2]
evid = root / "docs/roadmap-evidence"
p = evid / "#1.json"
d = json.loads(p.read_text())
if kind == "missing_row":
    (evid / "REQUIRED_ROWS.txt").write_text("#1\n#2\n")
elif kind in ("extra_row", "bad_pin"):
    d["row_id"] = "#2"
    if kind == "bad_pin":
        d["roadmap_pin_commit"] = "a" * 40
        (evid / "REQUIRED_ROWS.txt").write_text("#1\n#2\n")
    (evid / "#2.json").write_text(json.dumps(d))
elif kind == "wrong_sha":
    d["test_refs"][0]["source_sha"] = "a" * 40
elif kind == "missing_refs":
    del d["test_refs"]
elif kind == "forged_test":
    d["test_refs"][0]["test"] = "forged_test"
    # A comment is not a run entry.
    with (root / "tests/run_tests.sh").open("a") as f:
        f.write("# run forged_test true\n")
        f.write("printf '%s\\n' 'run forged_test true'\n")
elif kind == "missing_code_path":
    (root / "firmware/src/app/engine.c").unlink()
elif kind == "bad_status":
    d["status"] = "UNKNOWN"
elif kind == "malformed_json":
    p.write_text("{not-json\n")
if kind not in ("missing_row", "extra_row", "bad_pin", "malformed_json"):
    p.write_text(json.dumps(d))
PY
  row='#1'
  case "$kind" in
    missing_row) reason=missing_required_json; row='#2' ;;
    extra_row) reason=unexpected_json; row='#2' ;;
    wrong_sha) reason=source_sha_mismatch ;;
    missing_refs) reason=missing_test_refs ;;
    forged_test) reason=unknown_run_test ;;
    missing_code_path) reason=missing_or_outside_code_path ;;
    bad_status) reason=status_not_PRESENT ;;
    bad_pin) reason=pin_disagree; row='#2' ;;
    malformed_json) reason=malformed_json ;;
  esac
  expect_strict_failure "STRICT_CONTRACT_$kind" "$fixture" "$reason" "$row"
done

# Runner text cannot manufacture a registered test. Restoring the line-based
# parser must make these negatives fail, while genuine inline entries stay green.
for kind in quoted_separator heredoc multiline_quote continued_argument escaped_quote embedded_brace quoted_keyword escaped_keyword; do
  fixture=$TMP/runner_$kind
  write_strict_present "$fixture"
  python3 - "$fixture" "$kind" <<'PYRUNNER'
import json, pathlib, sys
root = pathlib.Path(sys.argv[1])
p = root / "docs/roadmap-evidence/#1.json"
d = json.loads(p.read_text())
d["test_refs"][0]["test"] = "forged_test"
p.write_text(json.dumps(d))
runners = {
    "quoted_separator": "printf '%s\\n' ';' run forged_test true\n",
    "embedded_brace": "printf '%s\\n' a{run forged_test true}\n",
    "quoted_keyword": "'else' run forged_test true\n",
    "escaped_keyword": "e\\lse run forged_test true\n",
    "heredoc": "cat <<'EOF'\nrun forged_test true\nEOF\n",
    "multiline_quote": "printf '%s\\n' '\nrun forged_test true\n'\n",
    "continued_argument": "printf '%s\\n' \\\nrun forged_test true\n",
    "escaped_quote": 'printf "%s\\n" "escaped \\"; run forged_test true"\n',
}
(root / "tests/run_tests.sh").write_text(runners[sys.argv[2]])
PYRUNNER
  expect_strict_failure "STRICT_RUNNER_$kind" "$fixture" unknown_run_test
done

# Candidate setup errors must be exit 2; the env form must also support GREEN.
printf '#!/bin/sh\nrun engines_selectable true\n' > "$SG/tests/run_tests.sh"
expect_zero STRICT_GREEN_env env X0X_CANDIDATE_SHA="$CANDIDATE" sh "$CHECK" --strict "$SG" "$SG/docs/roadmap-evidence"
for value in '' not-a-sha; do
  set +e
  out=$(X0X_CANDIDATE_SHA="$value" sh "$CHECK" --strict "$SG" "$SG/docs/roadmap-evidence" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 2 ] && printf '%s\n' "$out" | grep -q 'ERROR: strict mode requires'; then
    note "PASS STRICT_candidate_setup (value='$value' exit=2)"
    pass=$((pass + 1))
  else
    note "FAIL STRICT_candidate_setup (value='$value' exit=$rc): $out"
    fail=$((fail + 1))
  fi
done

note "----"
note "SELFTEST pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
