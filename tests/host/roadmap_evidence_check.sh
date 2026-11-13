#!/bin/sh
# ROADMAP evidence check: PRESENT paths + recorded test_refs; pin agreement.
# Usage: roadmap_evidence_check.sh [tree_root] [evidence_dir]
# Strict final gate: roadmap_evidence_check.sh --strict [--candidate-sha <40hex>] [tree_root] [evidence_dir]
# Strict requires --candidate-sha (before paths) or X0X_CANDIDATE_SHA.
# Default tree_root=.; default evidence_dir=<tree_root>/docs/roadmap-evidence
# Reporting (default): structural checks only; FAIL/NOT_RUN and UNPROVEN are allowed.
# Strict: REQUIRED_ROWS.txt coverage, all PRESENT/PASS, candidate SHA and tree checks.
# Exit 0=accepted, 1=evidence failure, 2=usage/setup error.
set -eu

STRICT=0
CANDIDATE_SHA=${X0X_CANDIDATE_SHA:-}
if [ "${1:-}" = --strict ]; then
  STRICT=1
  shift
  if [ "${1:-}" = --candidate-sha ]; then
    if [ "$#" -lt 2 ]; then
      echo "ERROR: --candidate-sha requires a 40hex SHA" >&2
      exit 2
    fi
    CANDIDATE_SHA=$2
    shift 2
  fi
fi

ROOT=${1:-.}
EVID=${2:-"$ROOT/docs/roadmap-evidence"}

if [ ! -d "$EVID" ]; then
  echo "ERROR: evidence dir missing: $EVID" >&2
  exit 2
fi

export ROOT EVID STRICT CANDIDATE_SHA
python3 - <<'PY'
import json, os, sys, glob

root = os.environ["ROOT"]
evid = os.environ["EVID"]
tree_root = os.path.realpath(root)

def tree_file(path):
    if not isinstance(path, str) or not path or os.path.isabs(path):
        return False
    try:
        full = os.path.realpath(os.path.join(tree_root, path))
        return os.path.commonpath((tree_root, full)) == tree_root and os.path.isfile(full)
    except (OSError, ValueError):
        return False

def strict_main():
    import re
    from collections import defaultdict

    candidate = os.environ["CANDIDATE_SHA"]
    if not re.fullmatch(r"[0-9a-fA-F]{40}", candidate):
        print("ERROR: strict mode requires --candidate-sha <40hex> or "
              "X0X_CANDIDATE_SHA=<40hex>", file=sys.stderr)
        return 2

    failures = defaultdict(list)

    def bad(row, reason):
        failures[row].append(reason)

    required = set()
    manifest = os.path.join(evid, "REQUIRED_ROWS.txt")
    try:
        with open(manifest, encoding="utf-8") as f:
            for number, line in enumerate(f, 1):
                row = line.strip()
                if not row or "/" in row or "\\" in row or row in (".", ".."):
                    bad("REQUIRED_ROWS", f"invalid_id:line_{number}")
                elif row in required:
                    bad(row, "duplicate_required_id")
                else:
                    required.add(row)
    except (OSError, UnicodeError) as e:
        bad("REQUIRED_ROWS", f"manifest_unreadable:{e}")
    if not required:
        bad("REQUIRED_ROWS", "empty_manifest")

    # Include hidden and underscore-prefixed JSON: reject every extra file.
    strict_files = sorted(
        entry.path for entry in os.scandir(evid) if entry.name.endswith(".json")
    )
    actual = {os.path.basename(p)[:-5] for p in strict_files if os.path.isfile(p)}
    # STRICT_CHECK_A: required inventory, including unexpected JSON files.
    for row in sorted(required - actual):
        bad(row, "missing_required_json")
    for row in sorted(actual - required):
        bad(row, "unexpected_json")

    # Read literal run entries only; never execute the test runner or its commands.
    run_names = set()
    runner_error = None
    try:
        if not tree_file("tests/run_tests.sh"):
            raise OSError("missing tests/run_tests.sh under tree root")
        # Keep word/quote context across physical lines. This deliberately accepts
        # only literal command entries, not arbitrary shell evaluation. Unsupported
        # here-documents and backticks fail closed rather than expose their text.
        def runner_tokens(source):
            word = []
            started = False
            quoted = False
            i = 0
            while i < len(source):
                char = source[i]
                if char == "\\" and i + 1 < len(source) and source[i + 1] == "\n":
                    i += 2
                    continue
                if char == "#" and not started:
                    end = source.find("\n", i)
                    i = len(source) if end < 0 else end
                    continue
                if char in " \t\r\n;&|()<>":
                    if started:
                        yield "quoted" if quoted else "word", "".join(word)
                        word, started, quoted = [], False, False
                    if char not in " \t\r":
                        if source[i:i + 2] == "<<":
                            raise ValueError("here-documents are unsupported in the literal test registry")
                        yield "operator", char
                    i += 1
                    continue
                started = True
                if char in "\"'":
                    quoted = True
                    quote = char
                    i += 1
                    while i < len(source) and source[i] != quote:
                        if quote == '"' and source[i] == "\\" and i + 1 < len(source):
                            following = source[i + 1]
                            if following in '$`"\\\n':
                                if following != "\n":
                                    word.append(following)
                                i += 2
                                continue
                        word.append(source[i])
                        i += 1
                    if i == len(source):
                        raise ValueError("unterminated shell quote in test registry")
                elif char == "\\":
                    quoted = True
                    i += 1
                    if i == len(source):
                        raise ValueError("unterminated shell escape in test registry")
                    word.append(source[i])
                elif char == "`":
                    raise ValueError("backticks are unsupported in the literal test registry")
                else:
                    word.append(char)
                i += 1
            if started:
                yield "quoted" if quoted else "word", "".join(word)

        with open(os.path.join(tree_root, "tests/run_tests.sh"), encoding="utf-8") as f:
            tokens = iter(runner_tokens(f.read()))
            command_start = True
            try:
                for kind, token in tokens:
                    if kind == "operator":
                        command_start = token in "\n;&|()"
                    elif kind == "word" and command_start and token in ("if", "then", "else", "elif", "do", "!", "{", "}"):
                        continue
                    else:
                        if command_start and token == "run":
                            name_kind, name = next(tokens, (None, None))
                            if name_kind in ("word", "quoted") and re.fullmatch(r"[A-Za-z0-9_.-]+", name):
                                run_names.add(name)
                        command_start = False
            except ValueError as e:
                run_names.clear()
                runner_error = str(e)
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
schema_bad = False

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
        if reasons:
            print(f"{stem}: UNPROVEN_BAD:{fail_why(reasons)}")
            schema_bad = True
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
            if not tree_file(cp):
                reasons.append(f"missing_or_outside_code_path:{cp}")

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
if schema_bad or any_present_bad or present_bad:
    rc = 1
sys.exit(rc)
PY
