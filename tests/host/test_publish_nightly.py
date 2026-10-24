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

    bare = tmp / "bare.git"
    subprocess.check_call(["git", "init", "-q", "--bare", "-b", "nightly-pages", str(bare)])
    pushed = tmp / "pushed"
    publish_valid_nightly(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, pushed,
                          remote=str(bare), push=True)
    published = git(bare, "rev-parse", "nightly-pages")
    clone = tmp / "clone"
    subprocess.check_call(["git", "clone", "-q", "-b", "nightly-pages", str(bare), str(clone)])
    require((clone / "install" / "index.html").is_file(), "pushed installer")
    require("ctl.img" not in [path.name for path in (clone / "firmware").iterdir()], "controller image was hosted")
    blocked = tmp / "blocked"
    refused("gh-pages", lambda: publish_valid_nightly(
        repo, elite_record, pro_record, elite_fwsc, pro_fwsc, blocked,
        remote=str(bare), push=True, branch="gh-pages"))
    require(git(bare, "rev-parse", "nightly-pages") == published, "refused push moved the nightly branch")
    require(not blocked.exists(), "refused branch wrote a page")
    for branch in ("main", "master", "nightly"):
        refused(branch, lambda branch=branch: publish_valid_nightly(
            repo, elite_record, pro_record, elite_fwsc, pro_fwsc, tmp / ("out-" + branch),
            remote=str(bare), push=True, branch=branch))
    require(publish_nightly._destination("refs/heads/nightly-pages") == "nightly-pages",
            "qualified nightly-pages ref")
    calls = []
    real_run = publish_nightly.subprocess.run

    def spy(args, *a, **k):
        argv = list(args)
        calls.append(argv)
        if "push" in argv:
            raise AssertionError("git push reached: " + " ".join(map(str, argv)))
        return real_run(args, *a, **k)

    publish_nightly.subprocess.run = spy
    try:
        for branch in ("refs/heads/main", "refs/heads/gh-pages", "refs/heads/master", "refs/heads/nightly"):
            out = tmp / ("qualified-" + branch.replace("/", "-"))
            refused(branch.rsplit("/", 1)[-1], lambda branch=branch, out=out: publish_valid_nightly(
                repo, elite_record, pro_record, elite_fwsc, pro_fwsc, out,
                remote=str(bare), push=True, branch=branch))
            require(not out.exists(), "qualified ref wrote a page: " + branch)
    finally:
        publish_nightly.subprocess.run = real_run
    require(not any("push" in cmd for cmd in calls), "qualified ref reached git push: " + repr(calls))
    require(git(bare, "rev-parse", "nightly-pages") == published, "qualified ref moved the nightly branch")

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
