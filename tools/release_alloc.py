#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Allocate monotonic X0X37 open-beta release numbers in a Git ledger.

The number is not a commit count, a hash, or a clock. The ledger file is the
record. Exact source inputs keep the number they already have. A different
package cannot reuse an immutable receipt. Elite and Pro share one number and
keep separate model, package, and controller receipts.

This tool does not build firmware, publish a site, or talk to a device.
"""

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from firmware_build_labels import canonical_version, release_display, version_of_product

LEDGER = Path("releases/openbeta/ledger.json")
PRESERVED_UNVERSIONED = "892bf60d912eca93597dcf03be25a9756540c949"
COMMIT = re.compile(r"[0-9a-f]{40}\Z")
TREE = re.compile(r"[0-9a-f]{64}\Z")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
NAMES = {"elite": "SMK-37 Elite", "pro": "SMK-37 Pro", "mke": "MKE-P37", "fm1": "FM-1"}


class ReleaseError(ValueError):
    """The ledger refused this allocation or receipt."""


def empty_ledger():
    return {
        "schema": 1,
        "channel": "openbeta",
        "display_suffix": "_openbeta",
        "preserved_unversioned": [PRESERVED_UNVERSIONED],
        "allocations": [],
    }


def _load(path):
    if not path.is_file():
        return empty_ledger()
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise ReleaseError("release ledger is unreadable") from exc
    if (data.get("schema") != 1 or data.get("channel") != "openbeta"
            or data.get("display_suffix") != "_openbeta"
            or not isinstance(data.get("allocations"), list)
            or not isinstance(data.get("preserved_unversioned"), list)):
        raise ReleaseError("release ledger schema is not the open-beta record")
    return data


def _save(path, ledger):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def _lock_path(path):
    key = hashlib.sha256(str(path.resolve()).encode()).hexdigest()[:16]
    return Path(tempfile.gettempdir()) / ("x0x-release-" + key + ".lock")


def _update(path, mutator):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    lock_path = _lock_path(path)
    with lock_path.open("a+") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        try:
            ledger = _load(path)
            changed, result = mutator(ledger)
            if changed:
                _save(path, ledger)
            return result
        finally:
            fcntl.flock(lock.fileno(), fcntl.LOCK_UN)


def _ids(source_commit, source_tree):
    if not isinstance(source_commit, str) or not COMMIT.fullmatch(source_commit):
        raise ReleaseError("source_commit must be a full 40-character lowercase Git SHA")
    if not isinstance(source_tree, str) or not TREE.fullmatch(source_tree):
        raise ReleaseError("source_tree must be a 64-character lowercase tree hash")
    return source_commit, source_tree


def _components(item):
    return item["major"], item["minor"], item["patch"]


def _next_components(allocations):
    """Next X.Y.Z. Patch 99 rolls to the next minor; minor 99 rolls to the next major."""
    if not allocations:
        return 0, 1, 0
    major, minor, patch = _components(max(allocations, key=_components))
    patch += 1
    if patch > 99:
        minor += 1
        patch = 0
    if minor > 99:
        major += 1
        minor = 0
        patch = 0
    if major > 99:
        raise ReleaseError("firmware version codec is exhausted at 99.99.99")
    return major, minor, patch


def _version_text(major, minor, patch):
    return "%d.%d.%d" % (major, minor, patch)


def product_for(model, version):
    """Package identity for this model and machine version. Display suffix is not included."""
    if model not in NAMES:
        raise ReleaseError("unsupported model: " + str(model))
    version = canonical_version(version)
    major, minor, patch = (int(part) for part in version.split("."))
    product = "%s_9%02d%02d%02d" % (NAMES[model], major, minor, patch)
    if len(product) > 20:
        raise ReleaseError("product identity exceeds 20 characters")
    if version_of_product(product, model) != version:
        raise ReleaseError("version codec did not round-trip")
    return product


def _plan(ledger, source_commit, source_tree):
    source_commit, source_tree = _ids(source_commit, source_tree)
    if source_commit in ledger["preserved_unversioned"]:
        raise ReleaseError("preserved unversioned source cannot be given a numeric release")
    for item in ledger["allocations"]:
        if item["source_tree"] == source_tree:
            if item["source_commit"] != source_commit:
                raise ReleaseError("exact inputs already belong to a different source commit")
            return item, False
    major, minor, patch = _next_components(ledger["allocations"])
    version = _version_text(major, minor, patch)
    item = {
        "version": version,
        "major": major,
        "minor": minor,
        "patch": patch,
        "display": release_display(version),
        "source_commit": source_commit,
        "source_tree": source_tree,
        "immutable": False,
        "models": {},
    }
    return item, True


def allocate_file(path, source_commit, source_tree):
    """Return the release for these exact inputs, creating the next number if needed."""
    def mutate(ledger):
        item, created = _plan(ledger, source_commit, source_tree)
        if created:
            ledger["allocations"].append(item)
        return created, item
    return _update(path, mutate)


def _receipt(model, version, package_sha256, controller_sha256, controller_size):
    if not isinstance(package_sha256, str) or not SHA256.fullmatch(package_sha256):
        raise ReleaseError("package_sha256 must be 64 lowercase hexadecimal characters")
    if not isinstance(controller_sha256, str) or not SHA256.fullmatch(controller_sha256):
        raise ReleaseError("controller_sha256 must be 64 lowercase hexadecimal characters")
    if type(controller_size) is not int or isinstance(controller_size, bool) or controller_size < 1:
        raise ReleaseError("controller_size must be a positive integer")
    return {
        "product": product_for(model, version),
        "package_sha256": package_sha256,
        "controller_sha256": controller_sha256,
        "controller_size": controller_size,
    }


def bind_file(path, version, model, package_sha256, controller_sha256, controller_size):
    """Record one model's bytes. The same bytes may be recorded again. Different bytes may not."""
    version = canonical_version(version)
    receipt = _receipt(model, version, package_sha256, controller_sha256, controller_size)

    def mutate(ledger):
        found = [item for item in ledger["allocations"] if item["version"] == version]
        if len(found) != 1:
            raise ReleaseError("release " + version + " is not in the ledger")
        item = found[0]
        current = item["models"].get(model)
        if current is not None:
            if current != receipt:
                raise ReleaseError("changed release bytes cannot reuse " + version)
            return False, item
        if item["immutable"]:
            raise ReleaseError("immutable release " + version + " cannot gain a model")
        item["models"][model] = receipt
        if {"elite", "pro"} <= set(item["models"]):
            item["immutable"] = True
        return True, item
    return _update(path, mutate)


def _git(repo, *args, check=True):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True)
    if check and result.returncode:
        raise ReleaseError(result.stderr.strip() or "git " + " ".join(args) + " failed")
    return result


def _commit_ledger(repo, message):
    _git(repo, "add", "--", str(LEDGER))
    staged = _git(repo, "diff", "--cached", "--name-only").stdout.split()
    if staged == []:
        return False
    if staged != [str(LEDGER)]:
        raise ReleaseError("release allocation would commit more than the ledger")
    _git(repo, "-c", "user.name=X0X release record", "-c", "user.email=release-record@localhost",
         "commit", "-q", "-m", message, "--", str(LEDGER))
    names = _git(repo, "diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD").stdout.split()
    if names != [str(LEDGER)]:
        raise ReleaseError("release commit is not ledger-only")
    return True


def _fetch(repo, remote, branch):
    result = _git(repo, "fetch", remote, branch, check=False)
    return result.returncode == 0


def allocate_committed(repo, source_commit, source_tree, remote=None, branch="release-records", retries=8):
    """Allocate and commit the ledger. A rejected push reloads the remote record and retries."""
    repo = Path(repo)
    path = repo / LEDGER
    if not (repo / ".git").exists():
        raise ReleaseError("release allocation needs a Git repository")
    for _attempt in range(retries):
        if _git(repo, "status", "--porcelain").stdout.strip():
            raise ReleaseError("release repository is dirty; allocation was not committed")
        if remote and _fetch(repo, remote, branch):
            changed = _git(repo, "diff", "--name-only", "HEAD", "FETCH_HEAD").stdout.split()
            if any(name != str(LEDGER) for name in changed):
                raise ReleaseError("remote release branch changes more than the ledger")
            _git(repo, "reset", "--hard", "FETCH_HEAD")
        item = allocate_file(path, source_commit, source_tree)
        if not _commit_ledger(repo, "Allocate open-beta " + item["version"]):
            return item
        if not remote:
            return item
        pushed = _git(repo, "push", remote, "HEAD:refs/heads/" + branch, check=False)
        if pushed.returncode == 0:
            return item
        _git(repo, "reset", "--hard", "HEAD~1")
    raise ReleaseError("release allocation collided too many times")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path("."))
    sub = parser.add_subparsers(dest="command", required=True)
    alloc = sub.add_parser("allocate")
    alloc.add_argument("--source-commit", required=True)
    alloc.add_argument("--source-tree", required=True)
    alloc.add_argument("--remote")
    alloc.add_argument("--branch", default="release-records")
    bind = sub.add_parser("bind")
    bind.add_argument("--version", required=True)
    bind.add_argument("--model", required=True, choices=sorted(NAMES))
    bind.add_argument("--package-sha256", required=True)
    bind.add_argument("--controller-sha256", required=True)
    bind.add_argument("--controller-size", required=True, type=int)
    sub.add_parser("show")
    args = parser.parse_args(argv)
    path = args.repo / LEDGER
    try:
        if args.command == "show":
            result = _load(path) if path.is_file() else empty_ledger()
        elif args.command == "allocate":
            if args.remote:
                result = allocate_committed(args.repo, args.source_commit, args.source_tree,
                                            args.remote, args.branch)
            else:
                result = allocate_file(path, args.source_commit, args.source_tree)
        else:
            result = bind_file(path, args.version, args.model, args.package_sha256,
                               args.controller_sha256, args.controller_size)
    except ReleaseError as exc:
        print("REFUSED: " + str(exc), file=sys.stderr)
        return 3
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
