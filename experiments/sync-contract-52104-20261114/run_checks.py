"""Run every isolated prototype check and record exact executable-source hashes."""
from datetime import datetime, timezone
from hashlib import sha256
import json
from pathlib import Path
import platform
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def main():
    evidence = ROOT / "evidence"
    evidence.mkdir(exist_ok=True)
    inputs = ["model.py", "test_model.py", "explore.py", "negative_controls.py",
              "run_checks.py", "PROTOCOL.md", "review/adversarial_test.py",
              "review/adversarial_interleavings.py"]
    hashes = {f: sha256((ROOT / f).read_bytes()).hexdigest() for f in inputs}
    checks = [
        ("focused", ["-m", "unittest", "-v", "test_model.py"]),
        ("adversarial", ["review/adversarial_test.py"]),
        ("interleavings", ["explore.py"]),
        ("adversarial-interleavings", ["review/adversarial_interleavings.py"]),
        ("negative-controls", ["negative_controls.py"]),
    ]
    receipt = dict(started_at=datetime.now(timezone.utc).isoformat(),
                   evidence_class="synthetic semantic prototype", python=platform.python_version(),
                   platform=platform.platform(), source_sha256=hashes, checks=[])
    for name, args in checks:
        result = subprocess.run([sys.executable, *args], cwd=ROOT,
                                capture_output=True, text=True, check=False)
        log = result.stdout + result.stderr
        (evidence / (name + ".log")).write_text(log)
        check = dict(name=name, command="python3 " + " ".join(args),
                     returncode=result.returncode, status="PASS" if result.returncode == 0 else "FAIL",
                     log="evidence/" + name + ".log")
        counts = re.search(r"Ran (\d+) tests?", log)
        if counts:
            check["test_methods"] = int(counts.group(1))
        if name in ("interleavings", "negative-controls") and result.returncode == 0:
            parsed = json.loads(result.stdout)
            (evidence / (name + ".json")).write_text(json.dumps(parsed, indent=2) + "\n")
            check["result"] = parsed
        if name == "adversarial-interleavings" and result.returncode == 0:
            counts = re.search(r"(\d+) complete interleavings; (\d+) ledger", log)
            check.update(schedules=int(counts.group(1)), assertions=int(counts.group(2)))
        receipt["checks"].append(check)
        print(f'{name}: {check["status"]}, exit {result.returncode}', flush=True)
    after = {f: sha256((ROOT / f).read_bytes()).hexdigest() for f in inputs}
    if after != hashes:
        raise RuntimeError("inputs changed during verification")
    receipt["finished_at"] = datetime.now(timezone.utc).isoformat()
    receipt["all_passed"] = all(c["returncode"] == 0 for c in receipt["checks"])
    (evidence / "results.json").write_text(json.dumps(receipt, indent=2) + "\n")
    return 0 if receipt["all_passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
