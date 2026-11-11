#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Allocate monotonic X0X37 open-beta release numbers in a Git ledger.

The number is not a commit count, a hash, or a clock. The ledger file is the
record. Exact source inputs keep the number they already have. A different
package cannot reuse an immutable receipt. Elite and Pro share one number and
keep separate model, package, and controller receipts.

source_tree is the 40-character Git id of commit^{tree}. Issuance checks that
the commit exists and that this id matches. The ledger commit is created in an
isolated checkout of the release-record branch. A remote name such as
origin is resolved with git remote get-url in the source repository; the
isolated checkout receives that URL. The source checkout is not reset and
its dirty state is left as it was.

This tool does not build firmware, publish a site, or talk to a device.
"""

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from firmware_build_labels import canonical_version, release_display, version_of_product

LEDGER = Path("releases/openbeta/ledger.json")
PRESERVED_UNVERSIONED = "892bf60d912eca93597dcf03be25a9756540c949"
COMMIT = re.compile(r"[0-9a-f]{40}\Z")
TREE = COMMIT
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
RECORD_BRANCH = re.compile(r"[A-Za-z0-9._/-]+\Z")
FORBIDDEN_BRANCHES = {"nightly", "main", "master", "gh-pages"}
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
        raise ReleaseError("source_tree must be the 40-character Git tree id")
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


def _git_tree(repo, source_commit):
    """Return the commit and its real Git tree id. This does not touch the checkout."""
    if not isinstance(source_commit, str) or not COMMIT.fullmatch(source_commit):
        raise ReleaseError("source_commit must be a full 40-character lowercase Git SHA")
    kind = _git(repo, "cat-file", "-t", source_commit, check=False)
    if kind.returncode or kind.stdout.strip() != "commit":
        raise ReleaseError("source_commit is not a commit in this repository")
    actual = _git(repo, "rev-parse", "--verify", source_commit + "^{tree}", check=False)
    if actual.returncode or not TREE.fullmatch(actual.stdout.strip()):
        raise ReleaseError("source commit has no Git tree")
    return source_commit, actual.stdout.strip()


def _ledger_of(repo, ledger=None):
    path = Path(ledger) if ledger is not None else Path(repo) / LEDGER
    return _load(path)


def _allocation(ledger, version):
    version = canonical_version(version)
    found = [item for item in ledger["allocations"] if item.get("version") == version]
    if len(found) != 1:
        raise ReleaseError("firmware version " + version + " is not allocated")
    return found[0]


def require_allocated(repo, version, source_commit, ledger=None):
    """The number was issued for this commit and Git tree. Model receipts may still be empty."""
    source_commit, source_tree = _git_tree(repo, source_commit)
    item = _allocation(_ledger_of(repo, ledger), version)
    if item.get("source_commit") != source_commit or item.get("source_tree") != source_tree:
        raise ReleaseError("firmware version " + item["version"] + " belongs to a different source")
    return item


def require_receipt(repo, version, source_commit, model, package_sha256, controller_sha256=None, ledger=None):
    """Catalog gate. The build may precede the receipt; publication may not."""
    item = require_allocated(repo, version, source_commit, ledger)
    receipt = item.get("models", {}).get(model)
    if not isinstance(receipt, dict):
        raise ReleaseError("allocated version has no " + str(model) + " receipt")
    if receipt.get("package_sha256") != package_sha256:
        raise ReleaseError("package hash does not match the allocated receipt")
    if controller_sha256 is not None and receipt.get("controller_sha256") != controller_sha256:
        raise ReleaseError("controller hash does not match the allocated receipt")
    return receipt


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
    names = _git(repo, "diff-tree", "--root", "--no-commit-id", "--name-only", "-r", "HEAD").stdout.split()
    if names != [str(LEDGER)]:
        raise ReleaseError("release commit is not ledger-only")
    return True


def _stamp(repo):
    """Read-only identity of the source checkout. Allocation must return this unchanged."""
    head = _git(repo, "rev-parse", "HEAD").stdout
    name = _git(repo, "rev-parse", "--abbrev-ref", "HEAD").stdout
    status = _git(repo, "status", "--porcelain").stdout
    return head, name, status


def _verify_source(repo, source_commit, source_tree):
    """The commit must exist, and source_tree must be that commit's Git tree."""
    source_commit, source_tree = _ids(source_commit, source_tree)
    kind = _git(repo, "cat-file", "-t", source_commit, check=False)
    if kind.returncode or kind.stdout.strip() != "commit":
        raise ReleaseError("source_commit is not a commit in this repository")
    actual = _git(repo, "rev-parse", "--verify", source_commit + "^{tree}", check=False)
    if actual.returncode or actual.stdout.strip() != source_tree:
        raise ReleaseError("source_tree does not match the Git tree of the source commit")
    return source_commit, source_tree


def _check_branch(branch):
    if (not isinstance(branch, str) or not RECORD_BRANCH.fullmatch(branch)
            or ".." in branch or branch in FORBIDDEN_BRANCHES):
        raise ReleaseError("release record branch is not allowed")
    return branch


def _is_transport(url):
    if "://" in url:
        return True
    head, sep, _tail = url.partition(":")
    return bool(sep) and "/" not in head


def _resolve_remote(repo, remote):
    """URL behind a source-repo remote name. A path is left as a location."""
    text = str(remote).strip()
    if not text:
        raise ReleaseError("serialized allocation needs a release-record remote")
    probed = _git(repo, "remote", "get-url", text, check=False)
    if probed.returncode == 0 and probed.stdout.strip():
        url = probed.stdout.strip()
    elif _is_transport(text) or text.startswith(("/", ".")) or "/" in text:
        url = text
    else:
        raise ReleaseError("remote " + text + " is not configured on the source repository")
    if _is_transport(url):
        return url
    path = Path(url)
    if not path.is_absolute():
        path = Path(repo) / path
    return str(path.resolve())


def _remote_has_branch(repo, remote, branch):
    """Read the record branch from the source repository, not the process cwd."""
    result = _git(repo, "ls-remote", "--heads", str(remote), branch, check=False)
    if result.returncode:
        raise ReleaseError(result.stderr.strip() or "cannot read the release record remote")
    return bool(result.stdout.strip())


def _prepare_record(repo, record, remote, branch):
    """Isolated checkout. It never checks out or resets the candidate source."""
    record.mkdir(parents=True)
    init = subprocess.run(["git", "init", "-q", "-b", branch, str(record)],
                          capture_output=True, text=True)
    if init.returncode:
        raise ReleaseError(init.stderr.strip() or "cannot create the release record checkout")
    _git(record, "remote", "add", "origin", str(remote))
    if not _remote_has_branch(repo, remote, branch):
        return
    fetched = _git(record, "fetch", "origin", branch, check=False)
    if fetched.returncode:
        raise ReleaseError(fetched.stderr.strip() or "release record fetch failed")
    _git(record, "checkout", "-q", "-B", branch, "FETCH_HEAD")


def _push_record(repo, remote, branch, apply, retries):
    """Compare-and-swap the ledger. A rejected push fetches the record again."""
    if not remote:
        raise ReleaseError("serialized allocation needs a release-record remote")
    url = _resolve_remote(repo, remote)
    branch = _check_branch(branch)
    last = ""
    for _attempt in range(retries):
        tmp = Path(tempfile.mkdtemp(prefix="x0x-release-record-"))
        try:
            record = tmp / "repo"
            _prepare_record(repo, record, url, branch)
            item, message = apply(record)
            if not _commit_ledger(record, message):
                return item
            pushed = _git(record, "push", "origin", "HEAD:refs/heads/" + branch, check=False)
            if pushed.returncode == 0:
                return item
            last = (pushed.stderr or pushed.stdout).strip()
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    raise ReleaseError("release record push collided too many times" + (": " + last if last else ""))


def allocate_committed(repo, source_commit, source_tree, remote=None, branch="release-records", retries=8):
    """Verify the source objects, then commit the ledger only in an isolated record checkout."""
    repo = Path(repo)
    if _git(repo, "rev-parse", "--is-inside-work-tree", check=False).stdout.strip() != "true":
        raise ReleaseError("release allocation needs the source work tree to verify the commit")
    before = _stamp(repo)
    source_commit, source_tree = _verify_source(repo, source_commit, source_tree)

    def apply(record):
        item = allocate_file(record / LEDGER, source_commit, source_tree)
        return item, "Allocate open-beta " + item["version"]

    item = _push_record(repo, remote, branch, apply, retries)
    if _stamp(repo) != before:
        raise ReleaseError("source checkout changed during allocation")
    return item


def bind_committed(repo, version, model, package_sha256, controller_sha256, controller_size,
                   remote, branch="release-records", retries=8):
    """Bind one model in the isolated record checkout. The source checkout stays put."""
    repo = Path(repo)
    before = _stamp(repo)

    def apply(record):
        item = bind_file(record / LEDGER, version, model, package_sha256, controller_sha256, controller_size)
        return item, "Bind open-beta " + version + " " + model

    item = _push_record(repo, remote, branch, apply, retries)
    if _stamp(repo) != before:
        raise ReleaseError("source checkout changed during allocation")
    return item


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
    bind.add_argument("--remote")
    bind.add_argument("--branch", default="release-records")
    sub.add_parser("show")
    args = parser.parse_args(argv)
    path = args.repo / LEDGER
    try:
        if args.command == "show":
            result = _load(path) if path.is_file() else empty_ledger()
        elif args.command == "allocate":
            _verify_source(args.repo, args.source_commit, args.source_tree)
            if args.remote:
                result = allocate_committed(args.repo, args.source_commit, args.source_tree,
                                            args.remote, args.branch)
            else:
                result = allocate_file(path, args.source_commit, args.source_tree)
        else:
            if args.remote:
                result = bind_committed(args.repo, args.version, args.model, args.package_sha256,
                                        args.controller_sha256, args.controller_size,
                                        args.remote, args.branch)
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
