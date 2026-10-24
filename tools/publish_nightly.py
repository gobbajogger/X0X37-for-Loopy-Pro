#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Publish a nightly installer only after Elite and Pro development validation.

publish_valid_nightly() is the function the Validate stage calls when a nightly
is valid. It checks both development records again, builds the installer page,
and pushes that page only if every check passed. A failed or invalid build is
not written and not pushed. Production gh-pages is not a destination.

Call site to wire after #134 merges
------------------------------------
Do not put this call inside tools/stage_handoff.py. That module only records
the stage thread. handoff(), record_thread(), and advance() also move Audit,
Implement, Test, and Publish, and advance() enters Validate at the start of
the stage.

On cursor/stage-handoff-fbdd at 11caf84, the Validate stage ends by calling
handoff() (tools/stage_handoff.py line 168) with the next stage:

    handoff(ledger, "Merge", thread_id, timestamp)

Wire the publish immediately before that call, in the Validate completion
path, and skip the handoff when publish raises:

    from publish_nightly import publish_valid_nightly
    publish_valid_nightly(
        repo, elite_record, pro_record, elite_fwsc, pro_fwsc, out,
        remote=os.environ["X0X_NIGHTLY_REMOTE"], push=True)
    handoff(ledger, "Merge", thread_id, timestamp)
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dev_validate  # noqa: E402
import make_pages  # noqa: E402
from make_site import product_of  # noqa: E402

LABEL = "DEV / HARDWARE TEST — DEVICE NOT TESTED"
BANNER = (
    "DEV / HARDWARE TEST — DEVICE NOT TESTED. "
    "Only an Elite and Pro development build that passed validation is listed. "
    "A failed or invalid build is not published."
)
NIGHTLY_PAGES = "https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/"
BRANCH = "nightly-pages"
# gh-pages is production. main/master and the firmware nightly branch are source.
FORBIDDEN = {"gh-pages", "main", "master", "nightly"}
PRODUCTS = {"pro": "SMK-37 Pro_900", "elite": "SMK-37 Elite_900"}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
FWSC_NAME = re.compile(r"^[A-Za-z0-9._-]+\.fwsc$")


class NightlyPublishError(ValueError):
    """The nightly is not valid, so the installer page was not published."""


def _commit_of(record):
    commit = record.get("commit")
    if not isinstance(commit, str) or not HEX40.fullmatch(commit):
        raise NightlyPublishError("validation record has no full commit")
    return commit


def _verify_records(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, elite_ctl, pro_ctl):
    verified = []
    for model, record_path, fwsc, ctl in (
        ("elite", elite_record, elite_fwsc, elite_ctl),
        ("pro", pro_record, pro_fwsc, pro_ctl),
    ):
        record_path, fwsc = Path(record_path), Path(fwsc)
        if not record_path.is_file() or not fwsc.is_file():
            raise NightlyPublishError(f"{model} validation record or package is missing")
        try:
            preview = json.loads(record_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            raise NightlyPublishError(f"{model} validation record is unreadable") from error
        if preview.get("model") != model or preview.get("result") != "DEV CHECKED" or preview.get("config") != "dev":
            raise NightlyPublishError(f"{model} record is not a passing development validation")
        commit = _commit_of(preview)
        try:
            record = dev_validate.verify(repo, commit, record_path, fwsc, ctl)
        except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
            raise NightlyPublishError(f"{model} did not pass validation: {error}") from error
        if record.get("model") != model or record.get("commit") != commit:
            raise NightlyPublishError(f"{model} record does not match the validated commit")
        raw = fwsc.read_bytes()
        if product_of(raw) != PRODUCTS[model] or b"FELUCCA-LOADER-1" not in raw:
            raise NightlyPublishError(f"{model} package is not the development build {PRODUCTS[model]}")
        verified.append((model, record, fwsc, Path(ctl) if ctl else None))
    elite, pro = verified
    if elite[1]["commit"] != pro[1]["commit"] or elite[1].get("tree") != pro[1].get("tree"):
        raise NightlyPublishError("Elite and Pro are not the same validated source")
    return elite[1]["commit"], verified


def _pkg_name(pkg):
    if not isinstance(pkg, str) or "\\" in pkg or pkg.startswith("http"):
        return None
    name = Path(pkg).name
    if not FWSC_NAME.fullmatch(name):
        return None
    return name


def _model_entry_ok(key, model):
    if key not in PRODUCTS or not isinstance(model, dict):
        return False
    if model.get("product") != PRODUCTS[key]:
        return False
    if model.get("fromRelease") or model.get("qualified") is False or model.get("installable") is False:
        return False
    if model.get("revoked") or model.get("blocked") or model.get("withdrawn"):
        return False
    if model.get("hardware_tested") not in (None, False) or model.get("status") not in (None, "dev-checked"):
        return False
    sha = model.get("sha256")
    size = model.get("size")
    if not isinstance(sha, str) or not HEX64.fullmatch(sha):
        return False
    if not isinstance(size, int) or isinstance(size, bool) or size < 960:
        return False
    return _pkg_name(model.get("pkg")) is not None


def _version_ok(entry):
    if not isinstance(entry, dict):
        return False
    if entry.get("status") != "dev-checked" or entry.get("config") != "dev":
        return False
    if entry.get("hardware_tested") is not False or entry.get("label") != LABEL:
        return False
    if entry.get("qualified") is False or entry.get("installable") is False:
        return False
    if entry.get("revoked") or entry.get("blocked") or entry.get("withdrawn"):
        return False
    if not isinstance(entry.get("version"), str) or not entry["version"]:
        return False
    if not HEX40.fullmatch(str(entry.get("source_commit", ""))):
        return False
    models = entry.get("models")
    if not isinstance(models, dict) or not models:
        return False
    if any(key not in PRODUCTS for key in models):
        return False
    return all(_model_entry_ok(key, model) for key, model in models.items())


def _kept_versions(catalog, merge_from, version):
    """Prior nightlies stay only when their bytes still match the recorded hash."""
    kept = []
    firmware = (Path(merge_from) / "firmware") if merge_from else None
    for entry in catalog.get("versions", []):
        if entry.get("version") == version or not _version_ok(entry):
            continue
        if firmware is None:
            continue
        fresh = dict(entry)
        fresh["models"] = {}
        good = True
        for key, model in entry["models"].items():
            name = _pkg_name(model["pkg"])
            path = firmware / name
            if not path.is_file() or make_pages.sha256_file(path) != model["sha256"]:
                good = False
                break
            copied = dict(model)
            copied["pkg"] = f"../firmware/{name}"
            fresh["models"][key] = copied
        if good:
            kept.append(fresh)
    return kept


def _new_version(commit, verified):
    version = "nightly-" + commit[:12]
    models = {}
    copies = {}
    for model, record, fwsc, ctl in verified:
        name = f"x0x37-{version}-elite.fwsc" if model == "elite" else f"x0x37-{version}.fwsc"
        raw_size = fwsc.stat().st_size
        entry = {
            "product": PRODUCTS[model],
            "pkg": f"../firmware/{name}",
            "sha256": make_pages.sha256_file(fwsc),
            "size": raw_size,
            "status": "dev-checked",
            "hardware_tested": False,
        }
        if ctl:
            entry["controller_sha256"] = make_pages.sha256_file(ctl)
        if record.get("artifacts", {}).get("package", {}).get("sha256") != entry["sha256"]:
            raise NightlyPublishError(f"{model} package hash is not the validated artifact")
        models[model] = entry
        copies[name] = fwsc
    return version, {
        "version": version,
        "label": LABEL,
        "status": "dev-checked",
        "config": "dev",
        "hardware_tested": False,
        "qualified": True,
        "installable": True,
        "source_commit": commit,
        "source_tree": verified[0][1].get("tree"),
        "models": models,
    }, copies


def _catalog(commit, verified, merge):
    version, entry, copies = _new_version(commit, verified)
    previous = make_pages.load_catalog((Path(merge) / "versions.json") if merge else None)
    versions = _kept_versions(previous, merge, version)
    versions.append(entry)
    versions.sort(key=lambda item: str(item["version"]), reverse=True)
    catalog = {
        "repo": make_pages.REPO,
        "pages": NIGHTLY_PAGES,
        "channel": "nightly",
        "importsReleases": False,
        "latest": version,
        "versions": versions,
    }
    return version, catalog, copies


def _branch(branch):
    if branch in FORBIDDEN or not re.fullmatch(r"[A-Za-z0-9._/-]{1,80}", branch):
        raise NightlyPublishError(f"refusing to publish a nightly onto {branch}")
    if branch.startswith("-") or branch.endswith("/") or ".." in branch.split("/"):
        raise NightlyPublishError(f"refusing to publish a nightly onto {branch}")
    return branch


def _push(site, remote, branch):
    branch = _branch(branch)
    if not remote:
        raise NightlyPublishError("nightly remote is not set; production pages are not a nightly destination")
    subprocess.run(["git", "init", "-q", "-b", branch], cwd=site, check=True)
    subprocess.run(["git", "add", "-A"], cwd=site, check=True)
    message = f"Nightly installer ({branch})"
    subprocess.run(
        ["git", "-c", "user.name=Nightly publish", "-c", "user.email=nightly@localhost",
         "commit", "-q", "-m", message],
        cwd=site, check=True)
    subprocess.run(["git", "push", "-q", "-f", remote, f"HEAD:{branch}"], cwd=site, check=True)
    shutil.rmtree(site / ".git")


def publish_valid_nightly(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, out,
                          elite_ctl=None, pro_ctl=None, remote=None, push=False,
                          branch=BRANCH, merge=None):
    """Publish the nightly installer when both development builds verify.

    Returns the page directory. Raises NightlyPublishError without writing `out`
    and without pushing when either build is missing, failed, or not the matching
    development package. `push` sends the page to `branch` on `remote`.
    """
    if push:
        remote = remote or os.environ.get("X0X_NIGHTLY_REMOTE")
        _branch(branch)
        if not remote:
            raise NightlyPublishError("nightly remote is not set; production pages are not a nightly destination")
    commit, verified = _verify_records(
        repo, elite_record, pro_record, elite_fwsc, pro_fwsc, elite_ctl, pro_ctl)
    version, catalog, copies = _catalog(commit, verified, merge)
    out = Path(out)
    if out.exists() and not out.is_dir():
        raise NightlyPublishError("nightly output exists and is not a directory")
    out.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=".nightly-", dir=str(out.parent)))
    backup = None
    published = False
    try:
        try:
            make_pages.write_site(staging, catalog, copies, Path(merge) if merge else None, banner=BANNER)
            if push:
                _push(staging, remote, branch)
            if out.exists():
                backup = out.with_name(out.name + ".replaced")
                if backup.exists():
                    shutil.rmtree(backup)
                out.rename(backup)
            staging.rename(out)
            published = True
        except subprocess.CalledProcessError as error:
            raise NightlyPublishError("nightly page was not pushed") from error
    finally:
        if staging.exists():
            shutil.rmtree(staging, ignore_errors=True)
        if not published and backup and backup.exists() and not out.exists():
            backup.rename(out)
        if published and backup and backup.exists():
            shutil.rmtree(backup)
    print(f"nightly page {version} ({commit[:12]}) -> {out}")
    if push:
        print(f"pushed nightly installer branch {branch}")
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", default=".")
    parser.add_argument("--elite-record", required=True, type=Path)
    parser.add_argument("--elite-fwsc", required=True, type=Path)
    parser.add_argument("--elite-ctl", type=Path)
    parser.add_argument("--pro-record", required=True, type=Path)
    parser.add_argument("--pro-fwsc", required=True, type=Path)
    parser.add_argument("--pro-ctl", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--merge", type=Path, help="previous nightly page whose valid packages to keep")
    parser.add_argument("--remote", help="git remote for the nightly installer page")
    parser.add_argument("--branch", default=BRANCH)
    parser.add_argument("--push", action="store_true")
    args = parser.parse_args(argv)
    try:
        publish_valid_nightly(
            args.repo, args.elite_record, args.pro_record, args.elite_fwsc, args.pro_fwsc, args.out,
            elite_ctl=args.elite_ctl, pro_ctl=args.pro_ctl, remote=args.remote, push=args.push,
            branch=args.branch, merge=args.merge)
    except NightlyPublishError as error:
        print("nightly not published: " + str(error), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
