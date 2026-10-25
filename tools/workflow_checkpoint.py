#!/usr/bin/env python3
"""Local decision aid; never grants acceptance, merges, publication or artifact trust."""
import argparse
import hashlib
import json
import re
import time
from datetime import datetime, timezone
from pathlib import Path

SHA = re.compile(r"^[0-9a-f]{64}$")


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def hashes(value):
    return isinstance(value, dict) and bool(value) and all(
        isinstance(k, str) and bool(k) and isinstance(v, str) and SHA.fullmatch(v)
        for k, v in value.items())


def valid(value):
    if not isinstance(value, dict) or value.get("schema") != 1:
        return False
    for field in ("role", "workflow_revision", "last_accepted_stage"):
        if not isinstance(value.get(field), str) or not value[field]:
            return False
    if not hashes(value.get("input_identity")) or not hashes(value.get("evidence_identity")):
        return False
    heads = value.get("observed_remote_heads")
    if not isinstance(heads, dict) or not heads or not all(
            isinstance(k, str) and bool(k) and isinstance(v, str) and
            re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", v) for k, v in heads.items()):
        return False
    if value.get("read_status") != "success":
        return False
    for field in ("pending_work", "retry_conditions"):
        if not isinstance(value.get(field), list) or not all(
                isinstance(item, str) and item for item in value[field]):
            return False
    outputs = value.get("outputs")
    if not isinstance(outputs, dict) or not outputs:
        return False
    for name, output in outputs.items():
        if not isinstance(name, str) or not name or not isinstance(output, dict):
            return False
        if not hashes(output.get("closure")) or not hashes(output.get("policy")):
            return False
        proof = output.get("accepted_evidence")
        if not isinstance(proof, dict) or proof.get("status") != "ACCEPTED" or not hashes(proof.get("identity")):
            return False
        if not isinstance(output.get("sha256"), str) or not SHA.fullmatch(output["sha256"]):
            return False
    return True


def decide(previous, current):
    if not valid(current):
        return {"action": "WORK_REQUIRED", "reason": "invalid_or_failed_observation", "reuse": [], "affected": []}
    if not valid(previous):
        return {"action": "WORK_REQUIRED", "reason": "missing_or_invalid_checkpoint", "reuse": [], "affected": sorted(current["outputs"])}
    reuse = sorted(name for name, output in current["outputs"].items()
                   if output == previous["outputs"].get(name))
    affected = sorted(set(current["outputs"]) - set(reuse))
    same = previous == current
    no_change = same and not current["pending_work"] and not current["retry_conditions"]
    return {"action": "NO_CHANGE" if no_change else "WORK_REQUIRED",
            "reason": "accepted_inputs_unchanged" if no_change else "changed_identity_or_pending_work",
            "reuse": reuse, "affected": affected}


def load(path):
    try:
        with open(path) as stream:
            return json.load(stream)
    except (OSError, ValueError):
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("observation")
    parser.add_argument("--checkpoint", required=True)
    parser.add_argument("--receipt", help="optional local JSON receipt; never updates the accepted checkpoint")
    args = parser.parse_args()
    if args.receipt:
        destination = Path(args.receipt)
        for source in (Path(args.checkpoint), Path(args.observation)):
            try:
                aliases = destination.resolve() == source.resolve() or (
                    destination.exists() and source.exists() and destination.samefile(source))
            except OSError as error:
                parser.error("cannot establish receipt path safety: " + str(error))
            if aliases:
                parser.error("receipt must not alias checkpoint or observation")
    started = datetime.now(timezone.utc).isoformat()
    tick = time.monotonic()
    previous, current = load(args.checkpoint), load(args.observation)
    decision = decide(previous, current)
    receipt = {"schema": 1, "started_at": started,
               "ended_at": datetime.now(timezone.utc).isoformat(),
               "checkpoint_before": digest(previous) if previous is not None else None,
               "checkpoint_after": digest(previous) if previous is not None else None,
               "input_before": previous.get("input_identity") if isinstance(previous, dict) else None,
               "input_after": current.get("input_identity") if isinstance(current, dict) else None,
               "action": decision["action"], "outcome": decision,
               "retries": 0, "produced_verified_outputs": [],
               "stage_elapsed_seconds": time.monotonic() - tick,
               "time_to_owner_installable_seconds": None,
               "tokens": None, "telemetry_status": "unavailable"}
    if args.receipt:
        with open(args.receipt, "w") as stream:
            json.dump(receipt, stream, indent=2, sort_keys=True)
            stream.write("\n")
    print(json.dumps(receipt, sort_keys=True))
    return 0 if decision["action"] == "NO_CHANGE" else 10


if __name__ == "__main__":
    raise SystemExit(main())
