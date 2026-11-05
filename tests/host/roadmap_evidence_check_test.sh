#!/bin/sh
# RED/GREEN/MUTANT self-test for roadmap_evidence_check.sh
# Uses temp fixtures only; no network. POSIX sh + the checker under test.
set -eu

HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
CHECK=${CHECK:-"$HERE/roadmap_evidence_check.sh"}
TMP=${TMPDIR:-/tmp}/roadmap_evidence_check_test_$$
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

note "----"
note "SELFTEST pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
