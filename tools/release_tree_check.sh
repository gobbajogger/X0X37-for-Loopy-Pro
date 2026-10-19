#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# release_tree_check.sh: check 7 of rules v4 (rule 1: never package refs-oscuro). Merge Integrator runs it in the release
# packaging (/workspace/x0x37-release-packager, step 2 of HANDOFF-2026-10-16.md) before it copies any artifact.
#
#   release_tree_check.sh [--repo GITDIR] <commit>
#
# REFUSED (exit 1) when:
#   - the tree of <commit> has a path with a folder "refs/jieli" or "refs/oscuro" (at any depth, any letter case), or a
#     path with "refs-oscuro" in it;
#   - a refs-oscuro commit is <commit> or an ancestor of it: the tip of the branch refs-oscuro (if the repository has it)
#     and each commit of X0X_OSCURO_COMMITS [ac10280 babd349] that the repository has.
# Output: "TREE OK <short>: ..." (exit 0), "TREE REFUSED <short>: <reason>" (exit 1), "TREE ERROR: ..." (exit 2).
# Read only. GITDIR default: X0X_REPO, else /workspace/x0x37.git.
set -u
REPO=${X0X_REPO:-$(git rev-parse --absolute-git-dir 2>/dev/null || echo /workspace/x0x37.git)}
case "${1:-}" in --repo) REPO=${2:-}; shift 2 || true;; esac
C=${1:-}
[ -n "$C" ] || { echo "TREE ERROR: usage: release_tree_check.sh [--repo GITDIR] <commit>"; exit 2; }
g() { git -C "$REPO" "$@"; }
H=$(g rev-parse --verify --quiet "$C^{commit}") || { echo "TREE ERROR: no commit $C in $REPO"; exit 2; }
S=$(echo "$H" | cut -c1-7)
bad=$(g ls-tree -r --name-only --full-tree "$H" | grep -iE '(^|/)refs/(jieli|oscuro)(/|$)|refs-oscuro' | head -5)
if [ -n "$bad" ]; then
    echo "TREE REFUSED $S: the tree has refs/jieli or refs/oscuro paths: $(echo "$bad" | tr '\n' ' ')"; exit 1
fi
list=${X0X_OSCURO_COMMITS-ac10280 babd349}
tip=$(g rev-parse --verify --quiet refs/heads/refs-oscuro 2>/dev/null || true)
n=0
for o in $tip $list; do
    oh=$(g rev-parse --verify --quiet "$o^{commit}" 2>/dev/null) || continue
    n=$((n + 1))
    if g merge-base --is-ancestor "$oh" "$H"; then
        echo "TREE REFUSED $S: it contains the refs-oscuro commit $(echo "$oh" | cut -c1-7)"; exit 1
    fi
done
echo "TREE OK $S: no refs/jieli or refs/oscuro path, none of $n refs-oscuro commits is in it"
exit 0
