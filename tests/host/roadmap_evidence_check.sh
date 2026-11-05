#!/bin/sh
# ROADMAP evidence check: PRESENT paths + recorded test_refs; pin agreement.
# Usage: roadmap_evidence_check.sh [tree_root] [evidence_dir]
# Default tree_root=.; default evidence_dir=<tree_root>/docs/roadmap-evidence
# Exit 0 only when every PRESENT row is OK and all pins agree.
set -eu

ROOT=${1:-.}
EVID=${2:-"$ROOT/docs/roadmap-evidence"}

if [ ! -d "$EVID" ]; then
  echo "ERROR: evidence dir missing: $EVID" >&2
  exit 2
fi

export ROOT EVID
python3 - <<'PY'
import json, os, sys, glob

root = os.environ["ROOT"]
evid = os.environ["EVID"]

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
