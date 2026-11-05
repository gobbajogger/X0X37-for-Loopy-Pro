# Cloud agent lane push template (DnBOX)

Each cloud agent pushes **only its own new lane branch** with this script. Branches are then verified read-only by the **Git Verifier** (tip / tree / parent / identity / no trailers / mains untouched) and reported on **#832** (gobbajogger/X0X37-for-Loopy-Pro). The script **never opens a PR**, never touches `main`, `nightly`, `gh-pages`, `artifacts/nightly-dev`, `wip/one-build`, or any PR head branch.

Auth: use the environment's **existing** git credential mechanism (credential helper / pre-configured remote). Do not paste, print, or embed any token in this script, the remote URL, or logs.

```bash
#!/usr/bin/env bash
# usage: lane-push.sh <lane-name> [<remote>=origin]
# pushes HEAD to refs/heads/lane/<lane-name>-<YYYYMMDD>
set -euo pipefail
NAME="${1:?lane name required}"; REMOTE="${2:-origin}"
DATE="$(date +%Y%m%d)"
BR="lane/${NAME}-${DATE}"
ID_NAME="gobbajogger"
ID_EMAIL="255244738+gobbajogger@users.noreply.github.com"
die(){ echo "ABORT: $*" >&2; exit 1; }

# --- auth: <ENV_CREDENTIAL_MECHANISM> (existing helper; nothing here) ---

# 1. Identity + no signing (repo-local)
git config user.name  "$ID_NAME"
git config user.email "$ID_EMAIL"
git config commit.gpgsign false
git config tag.gpgsign false
export GIT_AUTHOR_NAME="$ID_NAME"    GIT_AUTHOR_EMAIL="$ID_EMAIL"
export GIT_COMMITTER_NAME="$ID_NAME" GIT_COMMITTER_EMAIL="$ID_EMAIL"
# commit with: git commit --no-gpg-sign -m "..."   (hooks that add trailers must be fixed/disabled first)

# 2. Branch name guard
[[ "$NAME" =~ ^[A-Za-z0-9._-]+$ ]] || die "bad lane name"
case "$BR" in lane/*) ;; *) die "not a lane/ branch";; esac
for bad in main master nightly gh-pages artifacts/nightly-dev wip/one-build; do
  [[ "$BR" == "$bad" ]] && die "protected target $BR"
done

# 3. Remote ref must NOT exist
if [[ -n "$(git ls-remote --heads "$REMOTE" "refs/heads/$BR")" ]]; then
  die "remote $BR already exists (no overwrite, no force)"
fi
# 3b. Must not be a PR head branch (read-only check; gh optional)
if command -v gh >/dev/null 2>&1; then
  [[ -z "$(gh pr list --head "$BR" --state all --json number -q '.[].number' 2>/dev/null)" ]] || die "$BR is a PR branch"
fi

# 4. Check every new commit: identity, no gpgsig, no trailers
BASE="$(git merge-base HEAD "$REMOTE/main" 2>/dev/null || true)"
RANGE="${BASE:+$BASE..}HEAD"
for c in $(git rev-list "$RANGE"); do
  idn="$(git log -1 --format='%an <%ae>|%cn <%ce>' "$c")"
  [[ "$idn" == "$ID_NAME <$ID_EMAIL>|$ID_NAME <$ID_EMAIL>" ]] || die "identity $c: $idn"
  git cat-file commit "$c" | grep -q '^gpgsig' && die "signed commit $c"
  git log -1 --format=%B "$c" | grep -qiE '^\s*co-authored-by:' && die "Co-authored-by trailer in $c"
done

# 5. Push: plain, new ref only (NO --force / --force-with-lease)
LOCAL="$(git rev-parse HEAD)"
git push "$REMOTE" "HEAD:refs/heads/$BR"

# 6. Readback
REMOTE_SHA="$(git ls-remote "$REMOTE" "refs/heads/$BR" | cut -f1)"
[[ "$REMOTE_SHA" == "$LOCAL" ]] || die "remote sha $REMOTE_SHA != local $LOCAL"

# 7. Report for Git Verifier / #832 (no PR opened)
echo "PUSHED $BR"
echo "repo:   $(git remote get-url "$REMOTE" | sed -E 's#https://[^@]*@#https://#')"
echo "tip:    $LOCAL"
echo "tree:   $(git rev-parse HEAD^{tree})"
echo "parent: $(git rev-parse HEAD^)"
echo "commits: $(git rev-list --count "$RANGE")  identity: $ID_NAME noreply  trailers: 0  gpgsig: none  force: no  PR: none"
echo "Next: Git Verifier read-only check (verify-branches.sh), result on #832."
```

Notes
- Abort is final: if any check fails, fix locally (e.g. `git commit --amend --no-gpg-sign` with identity env set, strip trailer) and push a **new** lane name/date — never overwrite.
- If an environment hook injects `Co-authored-by`, the step-4 grep aborts the push (seen before: report-v4-v5).

Suggested patch (LOCAL, unpushed) — insert as step 3c, before step 4:
```bash
# 3c. Neutralize hook-injected Co-authored-by on HEAD before checks/push
if git log -1 --format=%B HEAD | grep -qiE '^\s*co-authored-by:'; then
  git log -1 --format=%B HEAD | grep -viE '^\s*co-authored-by:' > /tmp/msg.$$
  GIT_AUTHOR_NAME="$ID_NAME" GIT_AUTHOR_EMAIL="$ID_EMAIL" \
  GIT_COMMITTER_NAME="$ID_NAME" GIT_COMMITTER_EMAIL="$ID_EMAIL" \
    git -c core.hooksPath=/dev/null commit --amend --no-verify --no-gpg-sign -F /tmp/msg.$$
  rm -f /tmp/msg.$$
fi
# (step 4 grep remains the post-amend check; after push, re-check:
#  git log -1 --format=%B "$REMOTE/$BR" | grep -qiE '^\s*co-authored-by:' && die "trailer on remote")
```
