#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Publish a PREVALIDATED generated Pages tree without rewriting gh-pages history.
# This does NOT authorize publication, build firmware, or waive any device/data gate.
# Usage: sh tools/publish_pages_ff.sh build/pages EXPECTED_GH_PAGES_SHA
set -eu

fail() { printf '%s\n' "Pages publication refused: $*" >&2; exit 1; }
[ "$#" -eq 2 ] || fail "usage: publish_pages_ff.sh GENERATED_SITE EXPECTED_GH_PAGES_SHA"
SITE_ARG=$1
BASE=$2
[ "${#BASE}" -eq 40 ] || fail "expected branch SHA must be 40 lowercase hex characters"
case "$BASE" in *[!0123456789abcdef]*|'') fail "invalid expected branch SHA" ;; esac
[ -d "$SITE_ARG" ] || fail "missing generated site directory"
[ ! -L "$SITE_ARG" ] || fail "site directory cannot be a symlink"
SITE=$(cd "$SITE_ARG" && pwd -P)
[ -f "$SITE/index.html" ] && [ -f "$SITE/install/index.html" ] && [ -f "$SITE/versions.json" ] || fail "missing required generated-site files"
[ ! -e "$SITE/.git" ] || fail "generated site contains Git metadata"
# A source checkout or private build inputs must never be copied to the public site.
for file in "$SITE"/* "$SITE"/.[!.]* "$SITE"/..?*; do
    [ -e "$file" ] || continue
    case "${file##*/}" in
        .nojekyll|index.html|versions.json|install|breaks|manual|new|projects|firmware) ;;
        *) fail "unexpected top-level site asset: ${file##*/}" ;;
    esac
done
[ -z "$(find "$SITE" \( -type l -o -name .git \) -print -quit)" ] || fail "generated tree contains a symlink or Git metadata"
# Allow only generated public HTML/assets and model-specific firmware.
# This fails closed if a future site change introduces a new asset type: the
# owner must review that addition before publishing.
find "$SITE" -type f -print | while IFS= read -r path; do
    rel=${path#"$SITE"/}
    case "$rel" in
      .nojekyll|index.html|versions.json|install/index.html|breaks/index.html|projects/index.html|projects/x0x_breaks.js|projects/x0x_projects.js|manual/index.html|manual/PROJECT_FILES.md|new/index.html) ;;
      manual/img/*.png|manual/img/*.svg)
        leaf=${rel#manual/img/}
        leaf=${leaf#new/}
        case "$leaf" in ''|*/*) fail "nested manual image is not allowed: $rel" ;; esac ;;
      new/img/*.png|new/img/*.svg)
        leaf=${rel#new/img/}
        leaf=${leaf#new/}
        case "$leaf" in ''|*/*) fail "nested what's-new image is not allowed: $rel" ;; esac ;;
      firmware/*.fwsc)
        leaf=${rel#firmware/}
        case "$leaf" in ''|*/*) fail "nested firmware package is not allowed: $rel" ;; esac ;;
      *) fail "unexpected public-site asset: $rel" ;;
    esac
done

# Fetch without forced remote-tracking ref updates; a non-FF or concurrent
# publication must be explicitly reconciled and revalidated, never overwritten.
git fetch -q origin refs/heads/gh-pages:refs/remotes/origin/gh-pages || fail "cannot safely fetch existing gh-pages"
CURRENT=$(git rev-parse --verify refs/remotes/origin/gh-pages) || fail "no existing gh-pages branch"
[ "$CURRENT" = "$BASE" ] || fail "gh-pages advanced: regenerate and revalidate against $CURRENT"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/x0x-pages-publish-XXXXXXXX") || fail "cannot create scratch worktree"
cleanup() {
    git worktree remove --force "$TMP/site" >/dev/null 2>&1 || true
    rm -rf "$TMP"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
git worktree add --quiet --detach "$TMP/site" "$BASE" || fail "cannot stage history-preserving site"
# Reject malicious or accidental symlinks in the *existing* Pages branch as
# well as new generated assets.  Otherwise cp -R can follow the old link.
[ -z "$(find "$TMP/site" -type l -print -quit)" ] || fail "existing gh-pages contains symlinks"

# Overlay new generated files; NEVER delete old pages or firmware by mirroring
# directory deletions. Reject overwrite of existing binaries and catalog loss.
cp -R "$SITE"/. "$TMP/site"/ || fail "could not copy generated site"
python3 - "$SITE" "$TMP/site" "$BASE" <<'PY' || fail "site preservation/integrity validation failed"
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys

new_root, merged_root, base = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]

def reject(msg):
    raise SystemExit("Pages preservation refused: " + msg)

def catalog(path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict) or not isinstance(data.get("versions"), list):
            reject("invalid versions.json structure")
        return data
    except (OSError, ValueError) as exc:
        reject("unreadable catalog: " + str(exc))

# Read the previous catalog from Git, not the overlaid worktree.
old_bytes = subprocess.run(["git", "show", base + ":versions.json"],
                           capture_output=True, check=True).stdout
try:
    old = json.loads(old_bytes)
except ValueError as exc:
    reject("invalid previous catalog: " + str(exc))
new = catalog(merged_root / "versions.json")
old_entries = {str(v["version"]): v for v in old.get("versions", [])}
new_entries = {str(v["version"]): v for v in new["versions"]}
if len(old_entries) != len(old.get("versions", [])) or len(new_entries) != len(new["versions"]):
    reject("duplicate firmware version identifiers")
if new.get("latest") and str(new["latest"]) not in new_entries:
    reject("latest firmware version has no catalog entry")
old_latest, new_latest = old.get("latest"), new.get("latest")
if old_latest != new_latest:
    # A DEV publication may select a genuinely new DEV candidate but must not
    # silently roll back to an earlier version or clear the default. Both are
    # separate owner decisions with their own approval and regression gate.
    if not new_latest or str(new_latest) in old_entries:
        reject("existing default changed without an approved rollback")
    previous_default = old_entries.get(str(old_latest), {})
    candidate = new_entries.get(str(new_latest), {})
    # A historical hardware-qualified/stable default must never be silently
    # replaced by a DEVICE NOT TESTED candidate. Display DEV separately.
    if (previous_default.get("hardware_tested") is True or
            previous_default.get("status") in ("stable", "release")) and (
            candidate.get("hardware_tested") is not True or
            candidate.get("status") not in ("stable", "release")):
        reject("unchecked DEV cannot replace the stable default")
for entry in new["versions"]:
    if not isinstance(entry.get("models"), dict) or not entry["models"]:
        reject("firmware version lacks model packages")
for version, previous in old_entries.items():
    current = new_entries.get(version)
    if current is None:
        reject("lost existing firmware version " + version)
    # The prior catalog is evidence: a package's digest alone does not attest
    # hardware PASS, stable status, source SHA, validation record or title.
    if current != previous:
        reject("changed existing firmware version metadata " + version)
for version, candidate in new_entries.items():
    if version not in old_entries:
        if candidate.get("hardware_tested") is True or candidate.get("status") in ("stable", "release"):
            reject("new stable/device-PASS publication requires its separate formal gate")
old_history = {str(x.get("id")): x for x in old.get("history", [])}
new_history = {str(x.get("id")): x for x in new.get("history", [])}
if len(old_history) != len(old.get("history", [])) or len(new_history) != len(new.get("history", [])):
    reject("duplicate build-history identifiers")
for identity, previous in old_history.items():
    if new_history.get(identity) != previous:
        reject("previous build-history evidence changed or disappeared: " + identity)

# No archive may silently disappear or change its bytes under the same filename.
old_archives = subprocess.run(["git", "ls-tree", "-r", "--name-only", base, "firmware/"],
                              capture_output=True, text=True, check=True).stdout.splitlines()
for rel in old_archives:
    old_content = subprocess.run(["git", "show", base + ":" + rel],
                                 capture_output=True, check=True).stdout
    current_path = merged_root / rel
    if not current_path.is_file() or current_path.read_bytes() != old_content:
        reject("modified or missing published package " + rel)

# Documentation/media images are retained byte-for-byte just like published
# packages. HTML/JS may legitimately change through an audited site preview.
old_images = subprocess.run(["git", "ls-tree", "-r", "--name-only", base,
                             "manual/img/", "new/img/"], capture_output=True,
                            text=True, check=True).stdout.splitlines()
for rel in old_images:
    old_content = subprocess.run(["git", "show", base + ":" + rel],
                                 capture_output=True, check=True).stdout
    current_path = merged_root / rel
    if not current_path.is_file() or current_path.read_bytes() != old_content:
        reject("modified or missing published image " + rel)

catalog_firmware = set()
for entry in new["versions"]:
    for model, spec in entry.get("models", {}).items():
        package = spec.get("pkg")
        if not isinstance(package, str) or "\\" in package or not package.startswith("firmware/"):
            reject("bad firmware path for " + str(model))
        segments = PurePosixPath(package).parts
        if ".." in segments or not package.endswith(".fwsc") or len(segments) != 2:
            reject("unsafe firmware path " + package)
        catalog_firmware.add(package)
        payload_path = merged_root / package
        if not payload_path.is_file():
            reject("catalog references absent package " + package)
        payload = payload_path.read_bytes()
        expected_hash = spec.get("sha256")
        if not isinstance(expected_hash, str) or len(expected_hash) != 64:
            reject("missing package hash " + package)
        if hashlib.sha256(payload).hexdigest() != expected_hash.lower() or len(payload) != spec.get("size"):
            reject("package integrity mismatch " + package)
        # Mirror the installer's productOf() header decoding. Checking SHA alone
        # cannot detect --elite/--pro accidentally labeling the wrong model.
        if len(payload) < 20 * 48:
            reject("short firmware package " + package)
        decoded = "".join(chr((payload[i * 48 + 47] - i - 1) & 0xff)
                          for i in range(20) if payload[i * 48 + 47] != 0x7d)
        expected_prefix = {"elite": "SMK-37 Elite_", "pro": "SMK-37 Pro_",
                           "mke": "MKE-P37_", "fm1": "FM-1_"}.get(model)
        if not expected_prefix or not decoded.startswith(expected_prefix):
            reject("catalog model does not match firmware identity " + package)
        if decoded != spec.get("product"):
            reject("catalog product does not match embedded firmware identity " + package)

# A valid-looking header or self-consistent SHA is not an admission ticket for
# an unlisted firmware download. Every NEW staged .fwsc must be in the catalog;
# older unlisted files in the historical site remain byte-for-byte preserved.
for payload_path in (new_root / "firmware").glob("*.fwsc") if (new_root / "firmware").is_dir() else ():
    rel = "firmware/" + payload_path.name
    if rel not in catalog_firmware:
        reject("unlisted firmware package in generated site " + rel)
print("Pages preservation, embedded model and package catalog: PASS")
PY

git -C "$TMP/site" add -A || fail "could not stage site"
if git -C "$TMP/site" diff --cached --quiet; then
    echo "Pages site unchanged; no commit or push"
    exit 0
fi
MESSAGE="Pages: verified history-preserving site update from $(git rev-parse --short HEAD)"
git -C "$TMP/site" -c user.name="x0x37-pages" -c user.email="pages@users.noreply.github.com" \
    commit -q -m "$MESSAGE" || fail "could not commit site"
# No --force or '+' refspec: another writer winning the race rejects this push.
git -C "$TMP/site" push -q origin HEAD:refs/heads/gh-pages || fail "non-fast-forward or remote error; site not published"
REMOTE=$(git ls-remote origin refs/heads/gh-pages | awk '{print $1}')
LOCAL=$(git -C "$TMP/site" rev-parse HEAD)
[ "$REMOTE" = "$LOCAL" ] || fail "pushed branch identity could not be verified"
echo "gh-pages source fast-forward pushed at $REMOTE; live deployment/hash verification still required"
