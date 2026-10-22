#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the GitHub Pages site for release VERSION and push it to the gh-pages branch.
# Keeps previously published firmware versions (merge), so the install page can pick any of them.
#
#   tools/publish_pages.sh 0.2
#     uses build/x0x-smk37-0.2.fwsc (+ elite/mke when present), or build/x0x-0.2.fwsc for FM-1
#
#   tools/publish_pages.sh 0.2 --pro PATH --elite PATH [--mke PATH]
set -e
cd "$(dirname "$0")/.."
V="${1:?usage: tools/publish_pages.sh VERSION [--pro PATH] [--elite PATH] [--mke PATH] [--fm1 PATH]}"
shift
PRO=""; ELITE=""; MKE=""; FM1=""
while [ $# -gt 0 ]; do
  case "$1" in
    --pro) PRO=${2:?}; shift 2 ;;
    --elite) ELITE=${2:?}; shift 2 ;;
    --mke) MKE=${2:?}; shift 2 ;;
    --fm1) FM1=${2:?}; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 1 ;;
  esac
done

# Defaults: SMK packages from build_smk_release.sh / build.sh, else a single FM-1 package.
[ -n "$PRO" ] || { [ -f "build/x0x-smk37-$V.fwsc" ] && PRO="build/x0x-smk37-$V.fwsc"; }
[ -n "$ELITE" ] || { [ -f "build/x0x-smk37-elite-$V.fwsc" ] && ELITE="build/x0x-smk37-elite-$V.fwsc"; }
[ -n "$MKE" ] || { [ -f "build/x0x-smk37-mke-$V.fwsc" ] && MKE="build/x0x-smk37-mke-$V.fwsc"; }
[ -n "$FM1" ] || { [ -f "build/x0x-$V.fwsc" ] && FM1="build/x0x-$V.fwsc"; }
[ -n "$PRO$ELITE$MKE$FM1" ] || {
  echo "no packages for $V: run tools/build_smk_release.sh $V (or ./build.sh --release $V)" >&2
  exit 1
}

ARGS="--version $V --out build/pages"
[ -n "$PRO" ] && ARGS="$ARGS --pro $PRO"
[ -n "$ELITE" ] && ARGS="$ARGS --elite $ELITE"
[ -n "$MKE" ] && ARGS="$ARGS --mke $MKE"
[ -n "$FM1" ] && ARGS="$ARGS --fm1 $FM1"

# Merge prior gh-pages firmware so older versions stay installable.
rm -rf build/pages-prev
mkdir -p build/pages-prev
if git fetch -q origin gh-pages 2>/dev/null && git rev-parse --verify origin/gh-pages >/dev/null 2>&1; then
  git archive origin/gh-pages | tar -x -C build/pages-prev
  if [ -f build/pages-prev/versions.json ] || [ -d build/pages-prev/firmware ]; then
    ARGS="$ARGS --merge build/pages-prev"
  fi
fi

# shellcheck disable=SC2086
python3 tools/make_pages.py $ARGS

REMOTE="$(git remote get-url origin)"
SHA="$(git rev-parse --short HEAD)"
cd build/pages
rm -rf .git
git init -q -b gh-pages
git add -A
git commit -q -m "Site for X0X $V (from $SHA); versions kept"
git push -q -f "$REMOTE" gh-pages
rm -rf .git
REPO="$(printf %s "$REMOTE" | sed -E 's#(git@github.com:|https://[^@]+@github.com/|https://github.com/)##; s#\.git$##')"
gh api -X POST "repos/$REPO/pages/builds" >/dev/null 2>&1 || echo "(could not ask GitHub Pages to build: check the repository's Pages settings)"
echo "pushed the site for $V to gh-pages (https://gobbajogger.github.io/X0X37-for-Loopy-Pro/)"
