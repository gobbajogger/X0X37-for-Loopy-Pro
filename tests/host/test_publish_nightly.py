#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A nightly installer page is published only for two passing development builds."""
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
os.environ.pop("X0X_NIGHTLY_REMOTE", None)

import dev_validate  # noqa: E402
import publish_nightly  # noqa: E402
import validation_inputs  # noqa: E402
from publish_nightly import LABEL, NightlyPublishError, publish_valid_nightly  # noqa: E402


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def make_pkg(product, path):
    raw = bytearray(0x2000)
    for i, ch in enumerate(product[:20]):
        raw[i * 0x30 + 0x2F] = (ord(ch) + i + 1) & 0xFF
    for i in range(len(product), 20):
        raw[i * 0x30 + 0x2F] = 0x7D
    raw[0x400:0x415] = b"FELUCCA-LOADER-1"
    Path(path).write_bytes(raw)


def write_record(repo, model, fwsc, directory):
    """A record dev_validate.verify accepts. The publish gate re-checks it."""
    directory.mkdir()
    commit = git(repo, "rev-parse", "HEAD")
    tree = dev_validate.tree(repo, commit)
    plan = {"base": commit, "scope": "fixture behavior",
            "checks": [{"reason": "changed fixture", "argv": [sys.executable, "test.py"]}]}
    build_log = "app code area 100 B, 40 B free\nRAM .data+.bss 40 B of 100; pool 100 B of 10000\n"
    (directory / "build.log").write_text(build_log)
    (directory / "check-0.log").write_text("affected test passed\n")
    inputs = {
        "schema": validation_inputs.SCHEMA,
        "mode": "dev",
        "source": tree,
        "configuration": {
            "model": model, "config": "dev", "controller": False,
            "plan": plan, "dependencies": [],
        },
    }
    data = {
        "schema": 2,
        "validation_inputs": {
            "schema": validation_inputs.SCHEMA,
            "sha256": validation_inputs.digest(validation_inputs.encoded(inputs)),
            "inputs": inputs,
        },
        "result": "DEV CHECKED",
        "config": "dev",
        "model": model,
        "commit": commit,
        "tree": tree,
        "plan": plan,
        "controller": False,
        "budgets": dev_validate.budgets(build_log),
        "logs": {
            "build": {"returncode": 0, "sha256": digest(directory / "build.log")},
            "check-0": {"returncode": 0, "sha256": digest(directory / "check-0.log")},
        },
        "artifacts": {"package": {"file": Path(fwsc).name, "sha256": digest(fwsc)}},
    }
    record = directory / "record.json"
    record.write_text(json.dumps(data))
    checked = dev_validate.verify(repo, commit, record, fwsc)
    require(checked["model"] == model, "fixture record did not verify")
    return record


def refused(message, fn):
    try:
        fn()
    except NightlyPublishError as error:
        require(message in str(error), f"expected {message!r} in {error}")
        return
    raise SystemExit("invalid nightly was published")


def main():
    tmp = Path(tempfile.mkdtemp(prefix="nightly-publish-"))
    # Isolated test fixture: supply a default Git identity for the temporary
    # local repositories, not a custom author for a project commit.
    default_git = tmp / "fixture-gitconfig"
    default_git.write_text("[user]\n name = DnBOX Fixture Default\n email = fixture@example.invalid\n")
    os.environ["GIT_CONFIG_GLOBAL"] = str(default_git)
    os.environ["GIT_CONFIG_NOSYSTEM"] = "1"
    for key in ("GIT_AUTHOR_NAME", "GIT_AUTHOR_EMAIL", "GIT_COMMITTER_NAME", "GIT_COMMITTER_EMAIL"):
        os.environ.pop(key, None)
    repo = tmp / "repo"
    repo.mkdir()
    subprocess.check_call(["git", "init", "-q", "-b", "main"], cwd=repo)
    (repo / "notes.txt").write_text("fixture\n")
    subprocess.check_call(["git", "-C", str(repo), "add", "."])
    subprocess.check_call(["git", "-C", str(repo), "-c", "user.name=t", "-c", "user.email=t@t",
                           "commit", "-qm", "fixture"])
    elite_fwsc, pro_fwsc = tmp / "elite.fwsc", tmp / "pro.fwsc"
    make_pkg("SMK-37 Elite_900", elite_fwsc)
    make_pkg("SMK-37 Pro_900", pro_fwsc)
    elite_record = write_record(repo, "elite", elite_fwsc, tmp / "elite")
    pro_record = write_record(repo, "pro", pro_fwsc, tmp / "pro")
    commit = git(repo, "rev-parse", "HEAD")

    page = tmp / "page"
    publish_valid_nightly(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, page, push=False)
    catalog = json.loads((page / "versions.json").read_text())
    require(catalog["channel"] == "nightly" and catalog["importsReleases"] is False, "nightly catalog flags")
    require(catalog["latest"].startswith("nightly-"), catalog["latest"])
    version = catalog["versions"][0]
    require(version["label"] == LABEL and version["status"] == "dev-checked", "hardware-test label")
    require(version["hardware_tested"] is False and version["source_commit"] == commit, "source commit")
    require(set(version["models"]) == {"elite", "pro"}, "both models")
    for key, product in (("elite", "SMK-37 Elite_900"), ("pro", "SMK-37 Pro_900")):
        require(version["models"][key]["product"] == product, product)
        name = Path(version["models"][key]["pkg"]).name
        require((page / "firmware" / name).is_file(), name)
    install = (page / "install" / "index.html").read_text()
    landing = (page / "index.html").read_text()
    require("DEVICE NOT TESTED" in install and '"importsReleases":false' in install and '"channel":"nightly"' in install,
            "installer page")
    require(LABEL in landing and "failed or invalid build is not published" in landing, "landing banner")
    require("/*CATALOG*/" not in install, "catalog was not inlined")

    missing = tmp / "missing"
    refused("missing", lambda: publish_valid_nightly(
        repo, elite_record, tmp / "no-record.json", elite_fwsc, pro_fwsc, missing, push=False))
    require(not missing.exists(), "failed publish created a page")

    swapped = tmp / "swapped"
    refused("not a passing", lambda: publish_valid_nightly(
        repo, pro_record, elite_record, pro_fwsc, elite_fwsc, swapped, push=False))
    require(not swapped.exists(), "swapped models were published")

    tampered = tmp / "tampered.fwsc"
    raw = bytearray(elite_fwsc.read_bytes())
    raw[-1] ^= 0x01
    tampered.write_bytes(raw)
    bad_pkg = tmp / "bad-pkg"
    refused("did not pass", lambda: publish_valid_nightly(
        repo, elite_record, pro_record, tampered, pro_fwsc, bad_pkg, push=False))
    require(not bad_pkg.exists(), "tampered package was published")

    failed = json.loads(elite_record.read_text())
    failed["result"] = "FAILED"
    failed_path = tmp / "failed-record.json"
    failed_path.write_text(json.dumps(failed))
    failed_out = tmp / "failed-out"
    refused("not a passing", lambda: publish_valid_nightly(
        repo, failed_path, pro_record, elite_fwsc, pro_fwsc, failed_out, push=False))
    require(not failed_out.exists(), "failed record was published")

    # DnBOX rules retire the legacy Git publisher: generated sites contain .fwsc.
    # Both explicit and environment-selected push paths must fail before Git I/O.
    bare = tmp / "bare.git"
    subprocess.check_call(["git", "init", "-q", "--bare", str(bare)])
    def reject_push(label, **options):
        target = tmp / ("rejected-" + label)
        refused("DnBOX legacy Git push disabled", lambda: publish_valid_nightly(
            repo, elite_record, pro_record, elite_fwsc, pro_fwsc, target,
            push=True, **options))
        require(not target.exists(), "rejected push created output: " + label)

    reject_push("default-branch", remote=str(bare))
    reject_push("local-branch", remote=str(bare), branch="chat/scoped-test")
    reject_push("other-repo", remote="https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git",
                branch="chat/scoped-test")
    reject_push("canonical-remote", remote="https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git",
                branch="chat/scoped-test")
    reject_push("protected", remote=str(bare), branch="nightly")
    os.environ["X0X_NIGHTLY_REMOTE"] = str(bare)
    try:
        reject_push("environment", branch="chat/scoped-test")
    finally:
        os.environ.pop("X0X_NIGHTLY_REMOTE", None)
    require(subprocess.run(["git", "--git-dir", str(bare), "show-ref"],
                           capture_output=True).returncode == 1,
            "rejected nightly push wrote a Git ref")

    cli_push = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "publish_nightly.py"),
         "--repo", str(repo), "--elite-record", str(elite_record), "--elite-fwsc", str(elite_fwsc),
         "--pro-record", str(pro_record), "--pro-fwsc", str(pro_fwsc), "--out", str(tmp / "rejected-cli"),
         "--remote", str(bare), "--push"],
        capture_output=True, text=True)
    require(cli_push.returncode == 2 and "DnBOX legacy Git push disabled" in cli_push.stderr
            and not (tmp / "rejected-cli").exists(), "CLI allowed legacy Git push")

    previous = json.loads((page / "versions.json").read_text())
    older = json.loads(json.dumps(previous["versions"][0]))
    older["version"] = "nightly-older"
    broken = {
        "version": "broken", "label": LABEL, "status": "failed", "config": "dev",
        "hardware_tested": False, "source_commit": commit, "models": older["models"],
    }
    previous["versions"].extend([older, broken])
    (page / "versions.json").write_text(json.dumps(previous))
    (page / "firmware" / "nope.fwsc").write_bytes(b"not a package")
    merged = tmp / "merged"
    publish_valid_nightly(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, merged,
                          merge=page, push=False)
    merged_catalog = json.loads((merged / "versions.json").read_text())
    names = {item["version"] for item in merged_catalog["versions"]}
    require(names == {catalog["latest"], "nightly-older"}, names)
    require("broken" not in (merged / "install" / "index.html").read_text(), "failed build listed")
    require(not (merged / "firmware" / "nope.fwsc").exists(), "unrelated firmware copied")

    cli = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "publish_nightly.py"),
         "--repo", str(repo), "--elite-record", str(failed_path), "--elite-fwsc", str(elite_fwsc),
         "--pro-record", str(pro_record), "--pro-fwsc", str(pro_fwsc), "--out", str(tmp / "cli")],
        capture_output=True, text=True)
    require(cli.returncode == 2 and not (tmp / "cli").exists(), cli.stderr)
    shell = subprocess.run(
        ["sh", str(ROOT / "tools" / "publish_pages.sh"), "--nightly",
         "--repo", str(repo), "--elite-record", str(elite_record), "--elite-fwsc", str(elite_fwsc),
         "--pro-record", str(pro_record), "--pro-fwsc", str(pro_fwsc), "--out", str(tmp / "shell")],
        capture_output=True, text=True)
    require(shell.returncode == 0 and (tmp / "shell" / "install" / "index.html").is_file(),
            shell.stdout + shell.stderr)
    shutil.rmtree(tmp)
    print("PASS nightly publish")


if __name__ == "__main__":
    main()
