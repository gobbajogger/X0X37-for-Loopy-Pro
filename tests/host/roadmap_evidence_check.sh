#!/bin/sh
# ROADMAP evidence check: PRESENT paths + recorded test_refs; pin agreement.
# Usage: roadmap_evidence_check.sh [tree_root] [evidence_dir]
# Strict final gate: roadmap_evidence_check.sh --strict [--candidate-sha <40hex>] [tree_root] [evidence_dir]
# Strict flags may appear anywhere; --roadmap supplies the pinned file.
# Strict requires --candidate-sha or X0X_CANDIDATE_SHA.
# Default tree_root=.; default evidence_dir=<tree_root>/docs/roadmap-evidence
# Reporting (default): structural checks only; FAIL/NOT_RUN and UNPROVEN are allowed.
# Strict: pinned Master + legacy extras + manifest, PRESENT/PASS, exact SHA and paths.
# Exit 0=accepted, 1=evidence failure, 2=usage/setup error.
set -eu

STRICT=0
# Detect strict anywhere before parsing; legacy reporting argv stays untouched.
for arg do
  if [ "$arg" = --strict ]; then STRICT=1; fi
done
CANDIDATE_SHA=${X0X_CANDIDATE_SHA:-}
ROADMAP=
ROADMAP_CLI_PIN=
ROADMAP_CLI_PIN_SET=0
if [ "$STRICT" -eq 1 ]; then
  operands=0
  ROOT=.
  EVID=
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --strict) shift ;;
      --candidate-sha|--roadmap|--roadmap-pin)
        option=$1
        if [ "$#" -lt 2 ] || [ -z "$2" ]; then
          echo "ERROR: $option requires a value" >&2
          exit 2
        fi
        case "$2" in --*) echo "ERROR: $option requires a value" >&2; exit 2 ;; esac
        case "$option" in
          --candidate-sha) CANDIDATE_SHA=$2 ;;
          --roadmap) ROADMAP=$2 ;;
          --roadmap-pin)
            if [ "$ROADMAP_CLI_PIN_SET" -eq 1 ]; then
              echo "ERROR: duplicate --roadmap-pin" >&2
              exit 2
            fi
            ROADMAP_CLI_PIN=$2
            ROADMAP_CLI_PIN_SET=1 ;;
        esac
        shift 2 ;;
      -*) echo "ERROR: unexpected strict argument: $1" >&2; exit 2 ;;
      *)
        operands=$((operands + 1))
        case "$operands" in
          1) ROOT=$1 ;;
          2) EVID=$1 ;;
          *) echo "ERROR: unexpected strict argument: $1" >&2; exit 2 ;;
        esac
        shift ;;
    esac
  done
  EVID=${EVID:-"$ROOT/docs/roadmap-evidence"}
  ROADMAP=${ROADMAP:-"$ROOT/docs/ROADMAP.md"}
else
  ROOT=${1:-.}
  EVID=${2:-"$ROOT/docs/roadmap-evidence"}
fi

if [ ! -d "$EVID" ]; then
  echo "ERROR: evidence dir missing: $EVID" >&2
  exit 2
fi

export ROOT EVID STRICT CANDIDATE_SHA ROADMAP ROADMAP_CLI_PIN ROADMAP_CLI_PIN_SET
python3 - <<'PY'
import json, os, sys, glob

root = os.environ["ROOT"]
evid = os.environ["EVID"]

# These constants are reviewed with the pinned roadmap. Caller pins are only
# assertions; neither JSON, environment nor CLI may select a different blob.
ROADMAP_PIN = "a204a54c989d6f323d619e70f799427603756541"
REQUIRED_EXTRAS = frozenset("""
DONE-001
DONE-002
DONE-003
DONE-004
DONE-005
DONE-006
DONE-007
DONE-008
DONE-009
DONE-010
DONE-011
DONE-012
DONE-013
DONE-014
DONE-015
DONE-016
DONE-017
DONE-018
DONE-019
DONE-020
DONE-021
FIX-809-REVERT
FIX-INSTALLER-BACKUP
FIX-SMC-ALIGN
FIX-UI-TILES
PR-789
PR-790
PR-791
PR-792
PR-793
PR-794
PR-795
PR-796
PR-797
PR-798
PR-799
PR-800
PR-801
PR-802
PR-803
PR-804
PR-805
PR-806
PR-807
PR-808
PR-809
PR-810
PR-811
PR-820
PR-821
ROW-80982d592fa7
ROW-938cc69314d2
ROW-e7ebadf91a40
ROW-fcf14c2de61c
""".split())

def strict_main():
    import re
    import shlex
    import hashlib
    from collections import defaultdict

    candidate = os.environ["CANDIDATE_SHA"]
    if not re.fullmatch(r"[0-9a-fA-F]{40}", candidate):
        print("ERROR: strict mode requires --candidate-sha <40hex> or "
              "X0X_CANDIDATE_SHA=<40hex>", file=sys.stderr)
        return 2

    for expected in (os.environ.get("X0X_ROADMAP_PIN", ""),
                     os.environ["ROADMAP_CLI_PIN"]):
        if expected and (not re.fullmatch(r"[0-9a-fA-F]{40}", expected)
                         or expected.lower() != ROADMAP_PIN):
            print("ERROR: requested roadmap pin differs from checker pin", file=sys.stderr)
            return 2

    roadmap = os.environ["ROADMAP"]
    try:
        with open(roadmap, "rb") as f:
            roadmap_bytes = f.read()
        blob = hashlib.sha1(b"blob " + str(len(roadmap_bytes)).encode("ascii") +
                            b"\0" + roadmap_bytes).hexdigest()
        # STRICT_CHECK_ROADMAP_BLOB: hash actual bytes, never a JSON claim.
        if blob != ROADMAP_PIN:
            raise ValueError(f"blob={blob} expected={ROADMAP_PIN}")
        roadmap_text = roadmap_bytes.decode("utf-8")
    except (OSError, ValueError, UnicodeError) as e:
        print(f"ERROR: roadmap file missing or not the pinned blob: {roadmap}: {e}",
              file=sys.stderr)
        return 2

    # Only canonical numbered, legacy L and owner M tracker ids. Auxiliary
    # provenance/PR/QA prose rows are not canonical ids (see SCHEMA.md).
    master = set()
    in_master = False
    for line in roadmap_text.splitlines():
        if line.startswith("## "):
            in_master = bool(re.fullmatch(r"## Master tracker(?:\s.*)?", line))
        if in_master and line.startswith("|"):
            cell = line.split("|", 2)[1].strip()
            match = re.match(r"(#\d+|[LM]\d+)(?=\s|$)", cell)
            if match:
                master.add(match[1])
    if not master:
        print("ERROR: pinned roadmap has no Master tracker ids", file=sys.stderr)
        return 2
    print(f"STRICT_INFO master_rows={len(master)}")

    failures = defaultdict(list)

    def bad(row, reason):
        failures[row].append(reason)

    manifest_rows = set()
    manifest = os.path.join(evid, "REQUIRED_ROWS.txt")
    try:
        with open(manifest, "rb") as f:
            manifest_bytes = f.read()
        print("STRICT_INFO manifest_sha256=" + hashlib.sha256(manifest_bytes).hexdigest())
        for number, line in enumerate(manifest_bytes.decode("utf-8").splitlines(), 1):
            row = line.strip()
            if not row or "/" in row or "\\" in row or row in (".", ".."):
                bad("REQUIRED_ROWS", f"invalid_id:line_{number}")
            elif row in manifest_rows:
                bad(row, "duplicate_required_id")
            else:
                manifest_rows.add(row)
    except (OSError, UnicodeError) as e:
        bad("REQUIRED_ROWS", f"manifest_unreadable:{e}")
    if not manifest_rows:
        bad("REQUIRED_ROWS", "empty_manifest")
    # STRICT_CHECK_MANIFEST: the editable manifest must cover the trusted set.
    for row in sorted(master - manifest_rows):
        bad(row, "master_row_missing_from_manifest")
    for row in sorted(REQUIRED_EXTRAS - manifest_rows):
        bad(row, "extra_row_missing_from_manifest")
    for row in sorted(manifest_rows - master - REQUIRED_EXTRAS):
        bad(row, "manifest_id_not_in_master_or_extras")
    required = master | REQUIRED_EXTRAS | manifest_rows

    # Include hidden and underscore-prefixed JSON: reject every extra file.
    strict_files = sorted(
        entry.path for entry in os.scandir(evid) if entry.name.endswith(".json")
    )
    actual = {os.path.basename(p)[:-5] for p in strict_files if os.path.isfile(p)}
    # STRICT_CHECK_MASTER_JSON: independent of manifest membership.
    for row in sorted(master - actual):
        bad(row, "master row has no evidence json")
    # STRICT_CHECK_A: required inventory, including unexpected JSON files.
    for row in sorted(required - actual):
        bad(row, "missing_required_json")
    for row in sorted(actual - required):
        bad(row, "unexpected_json")

    tree_root = os.path.realpath(root)

    def tree_file(path):
        if not isinstance(path, str) or not path or os.path.isabs(path):
            return False
        try:
            full = os.path.realpath(os.path.join(tree_root, path))
            return os.path.commonpath((tree_root, full)) == tree_root and os.path.isfile(full)
        except (OSError, ValueError):
            return False

    # Read literal run entries only; never execute the test runner or its commands.
    run_names = set()
    runner_error = None
    try:
        if not tree_file("tests/run_tests.sh"):
            raise OSError("missing tests/run_tests.sh under tree root")
        with open(os.path.join(tree_root, "tests/run_tests.sh"), encoding="utf-8") as f:
            for line in f:
                # Tokenize without executing shell text. Recognize command positions
                # after separators and control words, including `else run name ...`.
                lexer = shlex.shlex(line, posix=True, punctuation_chars=";&|(){}")
                lexer.whitespace_split = True
                command_start = True
                try:
                    while True:
                        token = lexer.get_token()
                        if token is None:
                            break
                        if token in (";", "&&", "||", "|", "&", "(", ")", "{", "}"):
                            command_start = True
                        elif command_start and token in ("if", "then", "else", "elif", "do", "!"):
                            continue
                        else:
                            if command_start and token == "run":
                                name = lexer.get_token()
                                if name and re.fullmatch(r"[A-Za-z0-9_.-]+", name):
                                    run_names.add(name)
                            command_start = False
                except ValueError:
                    # A multiline quoted command may continue on the next line;
                    # any preceding literal run name has already been recorded.
                    continue
    except (OSError, UnicodeError) as e:
        runner_error = str(e)

    pin = None
    for row in sorted(actual):
        try:
            with open(os.path.join(evid, row + ".json"), encoding="utf-8") as f:
                data = json.load(f)
            if not isinstance(data, dict):
                raise ValueError("expected JSON object")
        except (OSError, ValueError, UnicodeError) as e:
            bad(row, f"malformed_json:{e}")
            continue

        for field in ("row_id", "roadmap_pin_commit", "roadmap_blob", "status", "proof_source"):
            if field not in data or data[field] in (None, ""):
                bad(row, f"missing_{field}")
        if data.get("row_id") != row:
            bad(row, "row_id_filename_mismatch")
        if isinstance(data.get("roadmap_blob"), str) and data["roadmap_blob"].lower() != ROADMAP_PIN:
            bad(row, "roadmap_blob_not_checker_pin")
        row_pin = (data.get("roadmap_pin_commit"), data.get("roadmap_blob"))
        if all(isinstance(v, str) and re.fullmatch(r"[0-9a-fA-F]{40}", v) for v in row_pin):
            if pin is None:
                pin = row_pin
            elif row_pin != pin:
                bad(row, "pin_disagree")
        else:
            bad(row, "invalid_pin")

        present = data.get("status") == "PRESENT"
        # STRICT_CHECK_B: every required row must be PRESENT.
        if row in required and not present:
            bad(row, f"status_not_PRESENT:{data.get('status')!r}")

        paths = data.get("code_paths", [])
        if present and (not isinstance(paths, list) or not paths):
            bad(row, "missing_code_paths")
        if not isinstance(paths, list):
            bad(row, "bad_code_paths")
        else:
            # STRICT_CHECK_E_PATHS: code paths must resolve to files inside the tree.
            for cp in paths:
                if not tree_file(cp):
                    bad(row, f"missing_or_outside_code_path:{cp!r}")

        refs = data.get("test_refs", [])
        # STRICT_CHECK_C: PRESENT needs refs, and every recorded result must PASS.
        if present and (not isinstance(refs, list) or not refs):
            bad(row, "missing_test_refs")
        if not isinstance(refs, list):
            bad(row, "bad_test_refs")
            continue
        for i, ref in enumerate(refs):
            if not isinstance(ref, dict):
                bad(row, f"bad_test_ref:{i}")
                continue
            if ref.get("result") != "PASS":
                bad(row, f"result_not_PASS:{i}:{ref.get('result')!r}")
            # STRICT_CHECK_D: results must belong to the exact candidate.
            sha = ref.get("source_sha")
            if not isinstance(sha, str) or sha.lower() != candidate.lower():
                bad(row, f"source_sha_mismatch:{i}")
            # STRICT_CHECK_E_REFS: proof file and literal runner entry must exist.
            if not tree_file(ref.get("file")):
                bad(row, f"missing_or_outside_test_file:{i}:{ref.get('file')!r}")
            test = ref.get("test")
            if not isinstance(test, str) or test not in run_names:
                bad(row, f"unknown_run_test:{i}:{test!r}")
            if runner_error:
                bad(row, f"runner_unreadable:{runner_error}")

    for row, reasons in sorted(failures.items()):
        print(f"STRICT_FAIL {row}: " + ",".join(reasons))
    print(f"STRICT_INFO failure_rows={len(failures)} "
          f"failure_reasons={sum(map(len, failures.values()))}")
    result = "FAIL" if failures else "PASS"
    print(f"STRICT_RESULT {result} rows={len(actual)} required={len(required)}")
    return 1 if failures else 0

if os.environ["STRICT"] == "1":
    sys.exit(strict_main())

files = sorted(
    p for p in glob.glob(os.path.join(evid, "*.json"))
    if os.path.isfile(p) and not os.path.basename(p).startswith("_")
)
if not files:
    print("ERROR: no evidence JSON files in " + evid, file=sys.stderr)
    sys.exit(2)

present_ok = present_bad = unproven = 0
pin_commit = pin_blob = None
pin_disagree = False
any_present_bad = False

def fail_why(reasons):
    return ",".join(reasons) if reasons else "unknown"

for path in files:
    name = os.path.basename(path)
    stem = name[:-5] if name.endswith(".json") else name
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        print(f"{stem}: PRESENT_BAD:malformed_json:{e}")
        present_bad += 1
        any_present_bad = True
        continue

    reasons = []
    for req in ("row_id", "roadmap_pin_commit", "roadmap_blob", "status", "proof_source"):
        if req not in data or data[req] in (None, ""):
            reasons.append(f"missing_{req}")

    status = data.get("status")
    row_id = data.get("row_id")
    if row_id is not None and row_id != stem:
        reasons.append("row_id_filename_mismatch")

    rc = data.get("roadmap_pin_commit")
    rb = data.get("roadmap_blob")
    if rc and rb:
        if pin_commit is None:
            pin_commit, pin_blob = rc, rb
        elif (rc, rb) != (pin_commit, pin_blob):
            pin_disagree = True
            reasons.append("pin_disagree")

    if status == "UNPROVEN":
        if reasons and any(r.startswith("missing_") or r == "row_id_filename_mismatch" for r in reasons):
            # still report as UNPROVEN but note schema issues in why for visibility
            print(f"{stem}: UNPROVEN")
        else:
            print(f"{stem}: UNPROVEN")
        unproven += 1
        continue

    if status != "PRESENT":
        reasons.append(f"bad_status:{status!r}")
        print(f"{stem}: PRESENT_BAD:{fail_why(reasons)}")
        present_bad += 1
        any_present_bad = True
        continue

    code_paths = data.get("code_paths")
    if not isinstance(code_paths, list) or not code_paths:
        reasons.append("missing_code_paths")
    else:
        for cp in code_paths:
            if not isinstance(cp, str) or not cp:
                reasons.append("empty_code_path")
                continue
            full = os.path.join(root, cp)
            if not os.path.isfile(full):
                reasons.append(f"missing_path:{cp}")

    test_refs = data.get("test_refs")
    if not isinstance(test_refs, list) or not test_refs:
        reasons.append("missing_test_refs")
    else:
        for i, tr in enumerate(test_refs):
            if not isinstance(tr, dict):
                reasons.append(f"bad_test_ref:{i}")
                continue
            res = tr.get("result")
            if res not in ("PASS", "FAIL", "NOT_RUN"):
                reasons.append(f"empty_or_bad_result:{i}")
            sha = tr.get("source_sha")
            if not isinstance(sha, str) or not sha.strip():
                reasons.append(f"missing_source_sha:{i}")
            if not tr.get("file") or not tr.get("test"):
                reasons.append(f"incomplete_test_ref:{i}")

    if reasons:
        print(f"{stem}: PRESENT_BAD:{fail_why(reasons)}")
        present_bad += 1
        any_present_bad = True
    else:
        print(f"{stem}: PRESENT_OK")
        present_ok += 1

missing = unproven  # UNPROVEN counts as MISSING
total = present_ok + present_bad + unproven
print(
    f"TOTALS present_ok={present_ok} present_bad={present_bad} "
    f"UNPROVEN={unproven} MISSING={missing} rows={total}"
)

if pin_commit is None or pin_blob is None:
    print("ROADMAP_PIN blob=UNKNOWN commit=UNKNOWN")
    print("ERROR: no pin found in evidence files", file=sys.stderr)
    sys.exit(1)

print(f"ROADMAP_PIN blob={pin_blob} commit={pin_commit}")

rc = 0
if pin_disagree:
    print("ERROR: evidence files disagree on ROADMAP pin", file=sys.stderr)
    rc = 1
if any_present_bad or present_bad:
    rc = 1
sys.exit(rc)
PY
