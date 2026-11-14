"""Prove that focused tests catch removed safety gates in disposable copies."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent


def main():
    source = (ROOT / "model.py").read_text()
    # Each mutation weakens exactly one relevant gate. Never edit the real model.
    cases = [
        ("publication_cas_removed", "test_local_edit_after_validation_stales_commit",
         "        if revision != tx.base_revision or revision != self.revision:\n"
         "            return dict(code=\"CONFLICT\", revision=self.revision)\n"
         "        if tx.manifest is None or expected_digest != tx.manifest[1]:",
         "        if tx.manifest is None or expected_digest != tx.manifest[1]:"),
        ("owner_gate_removed", "test_busy_begin_grants_no_abort_authority",
         "        if owner != tx.owner:\n            return None, dict(code=\"FOREIGN\")\n",
         ""),
        ("cancelled_request_resurrected", "test_cancellation_tombstone_precedes_delayed_begin",
         "        known = self.requests.get(request)\n        if known is not None:\n",
         "        known = None  # NEGATIVE CONTROL: forget cancelled request\n        if known is not None:\n"),
    ]
    results = []
    for name, test, old, new in cases:
        count = source.count(old)
        # The request lookup exists in BEGIN and CANCEL; mutate BEGIN only.
        assert count == (2 if name == "cancelled_request_resurrected" else 1), (name, count)
        with tempfile.TemporaryDirectory(prefix="sync-contract-negative-") as scratch:
            p = Path(scratch)
            (p / "model.py").write_text(source.replace(old, new, 1))
            (p / "test_model.py").write_text((ROOT / "test_model.py").read_text())
            completed = subprocess.run(
                [sys.executable, "-m", "unittest", "test_model.ContractTests." + test],
                cwd=p, capture_output=True, text=True, check=False)
            detected = completed.returncode != 0 and "FAIL:" in completed.stderr
            results.append(dict(mutation=name, returncode=completed.returncode,
                                detected=detected, test=test))
            if not detected:
                raise AssertionError((name, completed.stdout, completed.stderr))
    print(json.dumps(dict(negative_controls=results, all_detected=True), indent=2))


if __name__ == "__main__":
    main()
