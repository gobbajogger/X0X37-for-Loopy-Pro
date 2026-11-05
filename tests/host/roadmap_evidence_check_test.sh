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


# A small roadmap pins the synthetic fixture. Only a temporary checker COPY is
# repinned: --roadmap-pin and X0X_ROADMAP_PIN must never replace its constant.
REAL_CHECK=$CHECK
printf '## Master tracker\n| Item | Status |\n|---|---|\n| #1 fixture | pending |\n### More Master rows\n| Item | Status |\n|---|---|\n| M1 fixture | pending |\n| L1 fixture | pending |\n## Other section\n| #99 outside Master | ignored |\n' > "$TMP/ROADMAP.md"
python3 - "$CHECK" "$TMP/check.sh" "$TMP/ROADMAP.md" <<'PYFIX'
import hashlib, pathlib, sys
source, target, roadmap = map(pathlib.Path, sys.argv[1:])
b = roadmap.read_bytes()
pin = hashlib.sha1(b"blob " + str(len(b)).encode() + b"\0" + b).hexdigest()
s = source.read_text().replace(
    'ROADMAP_PIN = "a204a54c989d6f323d619e70f799427603756541"',
    'ROADMAP_PIN = "' + pin + '"')
target.write_text(s)
PYFIX
CHECK=$TMP/check.sh
unset X0X_ROADMAP_PIN

# --- Strict fixtures: synthetic files and run entries only; nothing is executed. ---
CANDIDATE=ce27812951555ff7ed9694bf99d20a65dd5f410b

write_strict_present() {
  write_good_present "$1" "$1/docs/roadmap-evidence"
  mkdir -p "$1/tests/host"
  printf 'fixture\n' > "$1/tests/host/example_test.c"
  printf '#!/bin/sh\nif [ 1 = 0 ]; then :; else run engines_selectable true; fi\n' > "$1/tests/run_tests.sh"
  cp "$TMP/ROADMAP.md" "$1/docs/ROADMAP.md"
  python3 - "$1" <<'PYFIX'
import hashlib, json, pathlib
import sys
root = pathlib.Path(sys.argv[1])
evid = root / "docs/roadmap-evidence"
# Independently enumerated legacy extras must remain required even if deleted
# from BOTH the manifest and JSON. They are not Master tracker rows.
extras = ['DONE-001', 'DONE-002', 'DONE-003', 'DONE-004', 'DONE-005', 'DONE-006', 'DONE-007', 'DONE-008', 'DONE-009', 'DONE-010', 'DONE-011', 'DONE-012', 'DONE-013', 'DONE-014', 'DONE-015', 'DONE-016', 'DONE-017', 'DONE-018', 'DONE-019', 'DONE-020', 'DONE-021', 'FIX-809-REVERT', 'FIX-INSTALLER-BACKUP', 'FIX-SMC-ALIGN', 'FIX-UI-TILES', 'PR-789', 'PR-790', 'PR-791', 'PR-792', 'PR-793', 'PR-794', 'PR-795', 'PR-796', 'PR-797', 'PR-798', 'PR-799', 'PR-800', 'PR-801', 'PR-802', 'PR-803', 'PR-804', 'PR-805', 'PR-806', 'PR-807', 'PR-808', 'PR-809', 'PR-810', 'PR-811', 'PR-820', 'PR-821', 'ROW-80982d592fa7', 'ROW-938cc69314d2', 'ROW-e7ebadf91a40', 'ROW-fcf14c2de61c']
d = json.loads((evid / "#1.json").read_text())
b = (root / "docs/ROADMAP.md").read_bytes()
d["roadmap_blob"] = hashlib.sha1(b"blob " + str(len(b)).encode() + b"\0" + b).hexdigest()
(evid / "#1.json").write_text(json.dumps(d))
for row in extras + ["M1", "L1"]:
    d["row_id"] = row
    (evid / (row + ".json")).write_text(json.dumps(d))
(evid / "REQUIRED_ROWS.txt").write_text("\n".join(["#1", "M1", "L1"] + extras) + "\n")
PYFIX
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
if ! printf '%s\n' "$LAST_OUT" | grep -q '^STRICT_RESULT PASS rows=57 required=57$' ||
   ! printf '%s\n' "$LAST_OUT" | grep -q '^STRICT_INFO master_rows=3$'; then
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
for kind in missing_row extra_row wrong_sha prefix_sha empty_sha SKIP missing_refs forged_test missing_code_path \
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
elif kind == "prefix_sha":
    d["test_refs"][0]["source_sha"] = d["test_refs"][0]["source_sha"][:12]
elif kind == "empty_sha":
    d["test_refs"][0]["source_sha"] = ""
elif kind == "SKIP":
    d["test_refs"][0]["result"] = "SKIP"
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
    wrong_sha|prefix_sha|empty_sha) reason=source_sha_mismatch ;;
    SKIP) reason=result_not_PASS ;;
    missing_refs) reason=missing_test_refs ;;
    forged_test) reason=unknown_run_test ;;
    missing_code_path) reason=missing_or_outside_code_path ;;
    bad_status) reason=status_not_PRESENT ;;
    bad_pin) reason=pin_disagree; row='#2' ;;
    malformed_json) reason=malformed_json ;;
  esac
  expect_strict_failure "STRICT_CONTRACT_$kind" "$fixture" "$reason" "$row"
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


# Exact exit checks also require a diagnostic, so a broken invocation cannot
# masquerade as a successful negative test.
expect_exit() {
  label=$1; want=$2; diagnostic=$3; shift 3
  set +e
  out=$("$@" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq "$want" ] && printf '%s\n' "$out" | grep -Fq "$diagnostic"; then
    note "PASS $label (exit=$rc; $diagnostic)"
    pass=$((pass + 1))
  else
    note "FAIL $label (expected exit=$want; $diagnostic; got $rc): $out"
    fail=$((fail + 1))
  fi
}

for kind in delete_both shrink_manifest delete_extra_both unknown_manifest; do
  fixture=$TMP/inventory_$kind
  write_strict_present "$fixture"
  python3 - "$fixture" "$kind" <<'PYFIX'
import pathlib, sys
root = pathlib.Path(sys.argv[1]); kind = sys.argv[2]
evid = root / "docs/roadmap-evidence"
manifest = evid / "REQUIRED_ROWS.txt"
row = "DONE-001" if kind == "delete_extra_both" else "#1"
if kind == "unknown_manifest":
    manifest.write_text(manifest.read_text() + "FORGED-1\n")
else:
    manifest.write_text("\n".join(x for x in manifest.read_text().splitlines() if x != row) + "\n")
    if kind != "shrink_manifest":
        (evid / (row + ".json")).unlink()
PYFIX
  case "$kind" in
    delete_both) reason='master row has no evidence json'; row='#1' ;;
    shrink_manifest) reason=master_row_missing_from_manifest; row='#1' ;;
    delete_extra_both) reason=extra_row_missing_from_manifest; row=DONE-001 ;;
    unknown_manifest) reason=manifest_id_not_in_master_or_extras; row=FORGED-1 ;;
  esac
  expect_strict_failure "STRICT_INVENTORY_$kind" "$fixture" "$reason" "$row"
done

# The differing bytes are never allowed to borrow the JSON's claimed pin.
sub=$TMP/substituted
write_strict_present "$sub"
python3 - "$sub/docs/ROADMAP.md" <<'PYFIX'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
p.write_text(p.read_text().replace("| #1 fixture | pending |\n", ""))
PYFIX
expect_exit STRICT_substituted_roadmap 2 'roadmap file missing or not the pinned blob' \
  sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$sub"
# Alter a byte without changing any tracker id: hashing must still reject it.
bytes_only=$TMP/changed_bytes
write_strict_present "$bytes_only"
printf '\nchanged bytes\n' >> "$bytes_only/docs/ROADMAP.md"
expect_exit STRICT_changed_bytes 2 'roadmap file missing or not the pinned blob' \
  sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$bytes_only"
expect_exit STRICT_explicit_substituted_roadmap 2 'roadmap file missing or not the pinned blob' \
  sh "$CHECK" "$SG" --strict --roadmap "$sub/docs/ROADMAP.md" --candidate-sha "$CANDIDATE"
expect_exit STRICT_nonfirst_flag 1 'STRICT_RESULT FAIL' \
  sh "$CHECK" "$TMP/inventory_delete_both" "$TMP/inventory_delete_both/docs/roadmap-evidence" \
  --strict --candidate-sha "$CANDIDATE"
expect_exit STRICT_flags_after_operands 0 'STRICT_RESULT PASS' sh "$CHECK" "$SG" "$SG/docs/roadmap-evidence" \
  --candidate-sha "$CANDIDATE" --roadmap "$SG/docs/ROADMAP.md" --strict
expect_exit STRICT_extra_operand 2 'ERROR: unexpected strict argument' \
  sh "$CHECK" "$SG" --strict --candidate-sha "$CANDIDATE" "$SG/docs/roadmap-evidence" extra
expect_exit STRICT_unknown_flag 2 'ERROR: unexpected strict argument' \
  sh "$CHECK" "$SG" --strict --candidate-sha "$CANDIDATE" --bogus
expect_exit STRICT_pin_cannot_replace_constant 2 'ERROR: requested roadmap pin differs from checker pin' \
  sh "$CHECK" --strict --candidate-sha "$CANDIDATE" --roadmap-pin "$PIN_B" "$SG"
expect_exit STRICT_env_pin_cannot_replace_constant 2 'ERROR: requested roadmap pin differs from checker pin' \
  env X0X_ROADMAP_PIN="$PIN_B" sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$SG"
# Both caller assertions are checked, even if a CLI pin is also provided.
SYNTH_PIN=$(python3 - "$TMP/ROADMAP.md" <<'PYFIX'
import hashlib, pathlib, sys
b = pathlib.Path(sys.argv[1]).read_bytes()
print(hashlib.sha1(b"blob " + str(len(b)).encode() + b"\0" + b).hexdigest())
PYFIX
)
expect_zero STRICT_matching_pin sh "$CHECK" --strict --candidate-sha "$CANDIDATE" \
  --roadmap-pin "$SYNTH_PIN" "$SG"
expect_exit STRICT_conflicting_env_pin 2 'ERROR: requested roadmap pin differs from checker pin' \
  env X0X_ROADMAP_PIN="$PIN_B" sh "$CHECK" --strict --candidate-sha "$CANDIDATE" \
  --roadmap-pin "$SYNTH_PIN" "$SG"
for flag in --candidate-sha --roadmap --roadmap-pin; do
  expect_exit "STRICT_missing_value_$flag" 2 'ERROR:' sh "$CHECK" "$SG" --strict "$flag"
done
missing=$TMP/missing_roadmap
write_strict_present "$missing"
rm "$missing/docs/ROADMAP.md"
expect_exit STRICT_missing_roadmap 2 'roadmap file missing or not the pinned blob' \
  sh "$CHECK" --strict --candidate-sha "$CANDIDATE" "$missing"

# This separate invocation uses the UNMODIFIED production pin. These generated
# PRESENT objects are synthetic checker fixtures, never firmware proof.
REAL_ROADMAP=$HERE/../../ROADMAP.pinned.md
if [ ! -f "$REAL_ROADMAP" ] && [ -f ./work/ROADMAP.pinned.md ]; then
  REAL_ROADMAP=./work/ROADMAP.pinned.md
fi
if [ -f "$REAL_ROADMAP" ]; then
  real=$TMP/real_pin
  write_strict_present "$real"
  python3 - "$real" "$REAL_ROADMAP" <<'PYFIX'
import json, pathlib, re, shutil, sys
root = pathlib.Path(sys.argv[1]); roadmap = pathlib.Path(sys.argv[2])
evid = root / "docs/roadmap-evidence"
d = json.loads((evid / "#1.json").read_text())
master = False; ids = []
for line in roadmap.read_text().splitlines():
    if line.startswith("## "):
        master = line.startswith("## Master tracker")
    if master and line.startswith("|"):
        m = re.match(r"(#\d+|[LM]\d+)(?=\s|$)", line.split("|")[1].strip())
        if m:
            ids.append(m[1])
assert len(ids) == len(set(ids)) == 149, ids
d["roadmap_blob"] = "a204a54c989d6f323d619e70f799427603756541"
for row in set(ids) | {p.stem for p in evid.glob("*.json")}:
    d["row_id"] = row
    (evid / (row + ".json")).write_text(json.dumps(d))
rows = set((evid / "REQUIRED_ROWS.txt").read_text().splitlines()) | set(ids)
(evid / "REQUIRED_ROWS.txt").write_text("\n".join(sorted(rows)) + "\n")
shutil.copyfile(roadmap, root / "docs/ROADMAP.md")
PYFIX
  expect_zero STRICT_REAL_PIN sh "$REAL_CHECK" "$real" --strict --candidate-sha "$CANDIDATE" \
    --roadmap "$REAL_ROADMAP" --roadmap-pin "$PIN_B"
  if ! printf '%s\n' "$LAST_OUT" | grep -q '^STRICT_INFO master_rows=149$' ||
     ! printf '%s\n' "$LAST_OUT" | grep -q '^STRICT_RESULT PASS rows=203 required=203$'; then
    note "FAIL STRICT_REAL_PIN inventory: $LAST_OUT"
    fail=$((fail + 1))
  fi
else
  note "SKIP STRICT_REAL_PIN: $REAL_ROADMAP not present (synthetic checks still run)"
fi

note "----"
note "SELFTEST pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
