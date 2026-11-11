#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate checked local nightly installer previews without publishing firmware.

The DnBOX owner policy permits exact-model local preview and records, but
forbids committing or pushing .fwsc/.img binaries and requires connected
GitHub API with its default author for text-only source delivery in the
canonical repository. The legacy `--push` option is kept for compatibility
with callers but always fails closed; it does not deploy any website or
write to `gobbajogger/x0x37-for-Loopy-Pro_nightly` (read-only provenance).
Historical stage-handoff examples are not executable publication authority.
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
import release_alloc  # noqa: E402
from firmware_build_labels import canonical_version, release_display, version_of_product  # noqa: E402
from make_site import product_of  # noqa: E402

LABEL = "DEV / HARDWARE TEST — DEVICE NOT TESTED"
BANNER = (
    "DEV / HARDWARE TEST — DEVICE NOT TESTED. "
    "Only an Elite and Pro development build that passed validation is listed. "
    "A failed or invalid build is not published."
)
NIGHTLY_PAGES = "https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/"
LIVE_REPOSITORY = "gobbajogger/x0x37-for-Loopy-Pro_nightly"
LIVE_REF = "gh-pages"
BRANCH = "nightly-pages"
# gh-pages is production. main/master and the firmware nightly branch are source.
FORBIDDEN = {"gh-pages", "main", "master", "nightly"}
PRODUCTS = {"pro": "SMK-37 Pro_900", "elite": "SMK-37 Elite_900"}


def _embedded_version(product, model):
    """None for a legacy model_900 package. Refuse any other non-codec identity."""
    if model not in PRODUCTS:
        raise NightlyPublishError("nightly model must be elite or pro")
    if product == PRODUCTS[model]:
        return None
    try:
        found = version_of_product(product, model)
    except (ValueError, TypeError, AttributeError) as exc:
        raise NightlyPublishError(f"{model} package identity {product!r} is not a validated version") from exc
    if found is None:
        raise NightlyPublishError(f"{model} package identity {product!r} is not a validated version")
    return found
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


def _verify_records(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, elite_ctl, pro_ctl, ledger=None):
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
        if b"FELUCCA-LOADER-1" not in raw:
            raise NightlyPublishError(f"{model} package has no update loader")
        try:
            product = product_of(raw)
        except (ValueError, OSError) as error:
            raise NightlyPublishError(f"{model} package identity is unreadable") from error
        embedded = _embedded_version(product, model)
        recorded = record.get("firmware_version")
        if embedded is None:
            if recorded is not None:
                raise NightlyPublishError(f"{model} legacy package has no numeric version")
        else:
            try:
                recorded = canonical_version(recorded)
            except (ValueError, TypeError) as error:
                raise NightlyPublishError(f"{model} validation record has no canonical firmware version") from error
            if recorded != embedded:
                raise NightlyPublishError(f"{model} package version does not match its validation record")
            firmware = record.get("firmware")
            digest = make_pages.sha256_file(fwsc)
            if not isinstance(firmware, dict) or firmware.get("product") != product or firmware.get("package_sha256") != digest:
                raise NightlyPublishError(f"{model} record does not match the package bytes")
            if firmware.get("firmware_version") != embedded:
                raise NightlyPublishError(f"{model} record identity does not match the package")
            controller_sha = make_pages.sha256_file(Path(ctl)) if ctl else None
            try:
                receipt = release_alloc.require_receipt(
                    repo, embedded, commit, model, digest, controller_sha, ledger)
            except release_alloc.ReleaseError as exc:
                raise NightlyPublishError(str(exc)) from exc
            record["_bound_controller_sha256"] = receipt["controller_sha256"]
        record = dict(record)
        record["_embedded_version"] = embedded
        record["_product"] = product
        verified.append((model, record, fwsc, Path(ctl) if ctl else None))
    elite, pro = verified
    if elite[1]["commit"] != pro[1]["commit"] or elite[1].get("tree") != pro[1].get("tree"):
        raise NightlyPublishError("Elite and Pro are not the same validated source")
    if elite[1].get("_embedded_version") != pro[1].get("_embedded_version"):
        raise NightlyPublishError("Elite and Pro firmware versions differ")
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
    product = model.get("product")
    if not isinstance(product, str):
        return False
    if product == PRODUCTS[key]:
        pass
    else:
        try:
            if version_of_product(product, key) is None:
                return False
        except (ValueError, TypeError, AttributeError):
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
    if not all(_model_entry_ok(key, model) for key, model in models.items()):
        return False
    decoded = []
    for key, model in models.items():
        product = model.get("product")
        if product == PRODUCTS[key]:
            decoded.append(None)
            continue
        if not isinstance(product, str):
            return False
        try:
            decoded.append(version_of_product(product, key))
        except (ValueError, TypeError, AttributeError):
            return False
    if any(item is None for item in decoded) and any(item is not None for item in decoded):
        return False
    if decoded[0] is None:
        return entry.get("firmware_version") in (None,) and entry.get("release_display") in (None,)
    if len(set(decoded)) != 1 or entry.get("firmware_version") != decoded[0]:
        return False
    if entry.get("version") != decoded[0] or entry.get("release_display") != release_display(decoded[0]):
        return False
    return True


def _same_numeric_release(old, new):
    """Exact republish of one allocated release. Any other same-version record collides."""
    if not new.get("firmware_version"):
        return False
    if (old.get("firmware_version") != new.get("firmware_version")
            or old.get("source_commit") != new.get("source_commit")
            or old.get("source_tree") != new.get("source_tree")
            or old.get("release_display") != new.get("release_display")):
        return False
    old_models = old.get("models") if isinstance(old.get("models"), dict) else {}
    new_models = new.get("models") if isinstance(new.get("models"), dict) else {}
    if set(old_models) != set(new_models):
        return False
    for key, model in new_models.items():
        prev = old_models.get(key)
        if not isinstance(prev, dict):
            return False
        for field in ("product", "sha256", "controller_sha256"):
            if prev.get(field) != model.get(field):
                return False
    return True


def _kept_versions(catalog, merge_from, version, incoming):
    """Prior nightlies stay only when their bytes still match the recorded hash."""
    kept = []
    firmware = (Path(merge_from) / "firmware") if merge_from else None
    for entry in catalog.get("versions", []):
        if entry.get("version") == version:
            if incoming.get("firmware_version") and not _same_numeric_release(entry, incoming):
                raise NightlyPublishError(
                    "catalog version " + str(version) + " already records a different source or package")
            continue
        if not _version_ok(entry):
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
    embedded = verified[0][1].get("_embedded_version")
    # The catalog version is the machine identity. A filename is not an identity.
    # Legacy packages keep the historical nightly label and stay unversioned.
    version = embedded if embedded else "nightly-" + commit[:12]
    display = release_display(embedded) if embedded else None
    models = {}
    copies = {}
    for model, record, fwsc, ctl in verified:
        name = f"x0x37-{version}-elite.fwsc" if model == "elite" else f"x0x37-{version}.fwsc"
        raw_size = fwsc.stat().st_size
        entry = {
            "product": record.get("_product") or PRODUCTS[model],
            "pkg": f"../firmware/{name}",
            "sha256": make_pages.sha256_file(fwsc),
            "size": raw_size,
            "status": "dev-checked",
            "hardware_tested": False,
        }
        if record.get("_bound_controller_sha256"):
            entry["controller_sha256"] = record["_bound_controller_sha256"]
        elif ctl:
            entry["controller_sha256"] = make_pages.sha256_file(ctl)
        if record.get("artifacts", {}).get("package", {}).get("sha256") != entry["sha256"]:
            raise NightlyPublishError(f"{model} package hash is not the validated artifact")
        models[model] = entry
        copies[name] = fwsc
    entry = {
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
    }
    if embedded:
        entry["firmware_version"] = embedded
        entry["release_display"] = display
    return version, entry, copies


def _catalog(commit, verified, merges):
    version, entry, copies = _new_version(commit, verified)
    versions = []
    seen = set()
    for merge in merges:
        if not merge:
            continue
        previous = make_pages.load_catalog(Path(merge) / "versions.json")
        for item in _kept_versions(previous, merge, version, entry):
            if item["version"] in seen:
                continue
            seen.add(item["version"])
            versions.append(item)
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


def _destination(branch):
    """Fold a fully qualified ref to its short name, then apply the denylist.

    `refs/heads/main` is the same destination as `main`. The check uses the short
    name, and the push uses that name too, so a qualified form cannot slip past.
    """
    if not isinstance(branch, str):
        raise NightlyPublishError("refusing to publish a nightly onto " + repr(branch))
    name = branch.strip()
    if name.startswith("+"):
        name = name[1:]
    while name.startswith("refs/heads/"):
        name = name[len("refs/heads/"):]
    parts = name.split("/")
    if (not name or name.lower() in FORBIDDEN or name.startswith("-")
            or any(part in {"", ".", ".."} for part in parts)
            or not re.fullmatch(r"[A-Za-z0-9._/-]{1,80}", name)):
        raise NightlyPublishError(f"refusing to publish a nightly onto {branch}")
    if not name.startswith(("wip/", "chat/")):
        raise NightlyPublishError("DnBOX source handoff branch must start with wip/ or chat/")
    return name


def _local_only_git_remote(remote):
    """Never perform Git network operations; use connected GitHub API for remote delivery.

    The local bare-repository path is retained solely for offline fixture tests.
    """
    if not isinstance(remote, (str, os.PathLike)):
        raise NightlyPublishError("Git network transport disabled; use connected GitHub API")
    path = Path(remote)
    if not path.is_absolute() or not path.is_dir() or not (path / "HEAD").is_file():
        raise NightlyPublishError("Git network transport disabled; use connected GitHub API")
    return str(path)


def _remote_tip(remote, branch):
    """SHA currently stored on the LOCAL handoff branch, or None when absent."""
    remote = _local_only_git_remote(remote)
    listed = subprocess.run(
        ["git", "ls-remote", remote, f"refs/heads/{branch}"],
        capture_output=True, text=True, check=True)
    line = listed.stdout.strip().splitlines()
    if not line or not line[0].strip():
        return None
    return line[0].split()[0]


def _materialize(remote, branch, dest):
    """Copy the handoff branch's tree. The caller keeps prior candidates from it."""
    remote = _local_only_git_remote(remote)
    scratch = Path(tempfile.mkdtemp(prefix=".nightly-fetch-"))
    try:
        subprocess.run(["git", "init", "-q"], cwd=scratch, check=True)
        subprocess.run(["git", "remote", "add", "origin", remote], cwd=scratch, check=True)
        subprocess.run(["git", "fetch", "-q", "origin", branch], cwd=scratch, check=True)
        subprocess.run(["git", "checkout", "-q", "FETCH_HEAD"], cwd=scratch, check=True)
        dest.mkdir(parents=True, exist_ok=True)
        for item in scratch.iterdir():
            if item.name == ".git":
                continue
            target = dest / item.name
            if item.is_dir():
                shutil.copytree(item, target)
            else:
                shutil.copy2(item, target)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def _prior_tree(dirs):
    present = [Path(path) for path in dirs if path]
    if not present:
        return None
    if len(present) == 1:
        return present[0]
    combined = Path(tempfile.mkdtemp(prefix=".nightly-prior-"))
    firmware = combined / "firmware"
    firmware.mkdir()
    for directory in present:
        source = directory / "firmware"
        if not source.is_dir():
            continue
        for package in source.glob("*.fwsc"):
            target = firmware / package.name
            if not target.exists():
                shutil.copy2(package, target)
    return combined


def _overlay(work, site):
    for child in list(work.iterdir()):
        if child.name == ".git":
            continue
        if child.is_dir():
            shutil.rmtree(child)
        else:
            child.unlink()
    for item in Path(site).iterdir():
        target = work / item.name
        if item.is_dir():
            shutil.copytree(item, target)
        else:
            shutil.copy2(item, target)


def _push(site, remote, branch, expected):
    """Commit the site as a child of expected and fast-forward the handoff branch.

    A new root is only the first commit. Later handoffs fetch the remote tip and
    use it as the parent, so a non-force push can move the branch. This does not
    deploy the nightly website repository.
    """
    branch = _destination(branch)
    if not remote:
        raise NightlyPublishError("nightly remote is not set; production pages are not a nightly destination")
    remote = _local_only_git_remote(remote)
    site = Path(site)
    if not site.is_dir():
        raise NightlyPublishError("local Git fixture source directory is missing")
    # DnBOX Rule 8: never stage firmware/product binaries or private headers.
    forbidden_suffixes = {".fwsc", ".img", ".vst3", ".component"}
    forbidden_private = {"state_codec_private.h", "fx_tables.h", "miami_factory_typed.h"}
    for entry in site.rglob("*"):
        if entry.is_symlink():
            raise NightlyPublishError("symlink source is forbidden in DnBOX Git handoff")
        if entry.is_file():
            if entry.suffix.lower() in forbidden_suffixes or entry.name in forbidden_private:
                raise NightlyPublishError("DnBOX forbids binary or private source in Git handoff")
            try:
                entry.read_text(encoding="utf-8")
            except (UnicodeError, OSError) as exc:
                raise NightlyPublishError("DnBOX Git handoff permits UTF-8 text files only") from exc
    work = Path(tempfile.mkdtemp(prefix=".nightly-commit-"))
    try:
        tip = _remote_tip(remote, branch)
        if tip != expected:
            raise NightlyPublishError("remote handoff moved; the descendant was not pushed")
        subprocess.run(["git", "init", "-q"], cwd=work, check=True)
        subprocess.run(["git", "remote", "add", "origin", remote], cwd=work, check=True)
        if tip:
            subprocess.run(["git", "fetch", "-q", "origin", branch], cwd=work, check=True)
            fetched = subprocess.check_output(
                ["git", "rev-parse", "FETCH_HEAD"], cwd=work, text=True).strip()
            if fetched != tip:
                raise NightlyPublishError("remote handoff moved; the descendant was not pushed")
            subprocess.run(["git", "checkout", "-q", "-B", branch, "FETCH_HEAD"], cwd=work, check=True)
        else:
            subprocess.run(["git", "checkout", "-q", "--orphan", branch], cwd=work, check=True)
        _overlay(work, site)
        subprocess.run(["git", "add", "-A"], cwd=work, check=True)
        message = f"Nightly handoff {branch}"
        # Use the executing account's existing default Git identity.
        # DnBOX Rule 3 forbids synthetic publish authors or email overrides.
        subprocess.run(
            ["git", "commit", "-q", "--allow-empty", "-m", message],
            cwd=work, check=True)
        if tip:
            parent = subprocess.check_output(["git", "rev-parse", "HEAD^"], cwd=work, text=True).strip()
            if parent != tip:
                raise NightlyPublishError("handoff is not a descendant of the remote tip")
        head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=work, text=True).strip()
        # No --force. A non-fast-forward update is rejected and the remote SHA stays.
        subprocess.run(["git", "push", "-q", "origin", f"HEAD:{branch}"], cwd=work, check=True)
        published = _remote_tip(remote, branch)
        if published != head:
            raise NightlyPublishError("remote handoff SHA is not the descendant commit")
        return head
    finally:
        shutil.rmtree(work, ignore_errors=True)


def _handoff_record():
    """State that this directory is not the live nightly website."""
    return {
        "schema": 1,
        "result": "HANDOFF",
        "live_deploy": False,
        "pages_repository": LIVE_REPOSITORY,
        "pages_ref": LIVE_REF,
        "note": (
            "Not a deploy of gobbajogger/x0x37-for-Loopy-Pro_nightly gh-pages. "
            "This does not run that repository's publisher."
        ),
    }


def publish_valid_nightly(repo, elite_record, pro_record, elite_fwsc, pro_fwsc, out,
                          elite_ctl=None, pro_ctl=None, remote=None, push=False,
                          branch=BRANCH, merge=None, ledger=None):
    """Record a validated local nightly preview, never push firmware to Git.

    DnBOX RULES 3, 8 and 1m retire the legacy --push entry point because page
    outputs include .fwsc packages. Source delivery uses connected GitHub API
    on the canonical repository; local preview with push=False remains supported.
    """
    if push:
        raise NightlyPublishError("DnBOX legacy Git push disabled: no firmware binaries in Git; use connected GitHub API for text-only source delivery")
    merges = [Path(merge)] if merge else []
    prior = None
    try:
        commit, verified = _verify_records(
            repo, elite_record, pro_record, elite_fwsc, pro_fwsc, elite_ctl, pro_ctl, ledger)
        version, catalog, copies = _catalog(commit, verified, merges)
        prior = _prior_tree(merges)
        out = Path(out)
        if out.exists() and not out.is_dir():
            raise NightlyPublishError("nightly output exists and is not a directory")
        out.parent.mkdir(parents=True, exist_ok=True)
        staging = Path(tempfile.mkdtemp(prefix=".nightly-", dir=str(out.parent)))
        backup = None
        recorded = False
        try:
            try:
                make_pages.write_site(staging, catalog, copies, prior, banner=BANNER)
                (staging / "handoff.json").write_text(
                    json.dumps(_handoff_record(), indent=2) + "\n", encoding="utf-8")
                if out.exists():
                    backup = out.with_name(out.name + ".replaced")
                    if backup.exists():
                        shutil.rmtree(backup)
                    out.rename(backup)
                staging.rename(out)
                recorded = True
            except subprocess.CalledProcessError as error:
                raise NightlyPublishError("nightly handoff was not recorded") from error
        finally:
            if staging.exists():
                shutil.rmtree(staging, ignore_errors=True)
            if not recorded and backup and backup.exists() and not out.exists():
                backup.rename(out)
            if recorded and backup and backup.exists():
                shutil.rmtree(backup)
    finally:
        if prior and prior not in merges:
            shutil.rmtree(prior, ignore_errors=True)
    print(f"nightly handoff {version} ({commit[:12]}) -> {out}")
    print(f"not a live deploy of {LIVE_REPOSITORY} {LIVE_REF}")
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
    parser.add_argument("--remote", help="legacy parameter: Git network delivery is disabled")
    parser.add_argument("--branch", default=BRANCH)
    parser.add_argument("--push", action="store_true", help="retired: always refuses Git publishing")
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
