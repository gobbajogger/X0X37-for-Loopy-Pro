#!/usr/bin/env bash
# Push C: one fast-forward of gh-pages (nightly repo) 3c427179 -> 97299556 (tree 93d06623). NOT RUN by Build.
# Must be run by an agent/person that holds the publish authorization. Needs push credentials for the nightly repo.
set -euo pipefail
REPO=https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git
OLD=3c427179774016fc29d7baf7748e635c5423a11b
NEW=972995560eed516df39fcd532b85836993374f28
TREE=93d06623c73cf2eeeba54182b209ebd20d35b631
OLDTREE=2e43d9dd6cd175b9502efb6ac3e1c8d9649c2174
BR=evidence/pushc-dryrun-ce278129-devchecked-20261105
W=$(mktemp -d); cd "$W"
git init -q . && git remote add origin "$REPO"
git fetch -q origin gh-pages "$BR"
[ "$(git rev-parse origin/gh-pages)" = "$OLD" ] || { echo "ABORT: gh-pages is not $OLD"; exit 1; }
[ "$(git rev-parse origin/$BR)" = "$NEW" ] || { echo "ABORT: branch tip is not $NEW"; exit 1; }
[ "$(git rev-parse $NEW^{tree})" = "$TREE" ] || { echo "ABORT: tree mismatch"; exit 1; }
[ "$(git rev-parse $NEW^)" = "$OLD" ] || { echo "ABORT: parent is not $OLD"; exit 1; }
git merge-base --is-ancestor "$OLD" "$NEW"
[ "$(git diff --name-only $OLD $NEW | sort | tr '\n' ' ')" = "install/index.html versions.json " ] || { echo "ABORT: unexpected files"; exit 1; }
# fast-forward only, plain push (no --force)
git push origin "$NEW:refs/heads/gh-pages"
# verify
git fetch -q origin gh-pages
[ "$(git rev-parse origin/gh-pages)" = "$NEW" ] && [ "$(git rev-parse origin/gh-pages^{tree})" = "$TREE" ] && echo "OK gh-pages=$NEW tree=$TREE"
echo "Then check: raw versions.json status of nightly-ce278129-dev == dev-checked; install page lists it; Pages build finished."
# Rollback (only on instruction): create a revert commit whose tree == $OLDTREE, fast-forward push, never force.
