#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the GitHub Pages site and push it to the gh-pages branch (classic hosting).
# Keeps previously published firmware versions (merge), so the install page can pick any of them.
#
#   tools/publish_pages.sh --shell
#     hostable installer + sample importer (drop any .fwsc; Releases fill versions later)
#
#   tools/publish_pages.sh 0.2
#     uses build/x0x-smk37-0.2.fwsc (+ elite/mke when present), or build/x0x-0.2.fwsc for FM-1
#
#   tools/publish_pages.sh 0.2 --pro PATH --elite PATH [--mke PATH]
#
#   tools/publish_pages.sh --nightly --elite-record ... --pro-record ... --out DIR
#     nightly installer page. Valid Elite and Pro development builds only.
#     Does not push production gh-pages. See tools/publish_nightly.py.
set -e
cd "$(dirname "$0")/.."
if [ "${1:-}" = "--nightly" ]; then
  shift
  exec python3 tools/publish_nightly.py "$@"
fi
SHELL_ONLY=0
V=""
if [ "${1:-}" = "--shell" ]; then
  SHELL_ONLY=1
  shift
elif [ -n "${1:-}" ] && [ "${1#-}" = "$1" ]; then
  V="$1"
  shift
fi
PRO=""; ELITE=""; MKE=""; FM1=""
while [ $# -gt 0 ]; do
  case "$1" in
    --shell) SHELL_ONLY=1; shift ;;
    --pro) PRO=${2:?}; shift 2 ;;
    --elite) ELITE=${2:?}; shift 2 ;;
    --mke) MKE=${2:?}; shift 2 ;;
    --fm1) FM1=${2:?}; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 1 ;;
  esac
done

if [ "$SHELL_ONLY" = 1 ]; then
  ARGS="--shell --out build/pages"
else
  [ -n "$V" ] || { echo "usage: tools/publish_pages.sh VERSION|--shell ..." >&2; exit 1; }
  # Defaults: SMK packages from build_smk_release.sh / build.sh, else a single FM-1 package.
  [ -n "$PRO" ] || { [ -f "build/x0x-smk37-$V.fwsc" ] && PRO="build/x0x-smk37-$V.fwsc"; }
  [ -n "$ELITE" ] || { [ -f "build/x0x-smk37-elite-$V.fwsc" ] && ELITE="build/x0x-smk37-elite-$V.fwsc"; }
  [ -n "$MKE" ] || { [ -f "build/x0x-smk37-mke-$V.fwsc" ] && MKE="build/x0x-smk37-mke-$V.fwsc"; }
  [ -n "$FM1" ] || { [ -f "build/x0x-$V.fwsc" ] && FM1="build/x0x-$V.fwsc"; }
  [ -n "$PRO$ELITE$MKE$FM1" ] || {
    echo "no packages for $V: run tools/build_smk_release.sh $V (or ./build.sh --release $V), or use --shell" >&2
    exit 1
  }
  ARGS="--version $V --out build/pages"
  [ -n "$PRO" ] && ARGS="$ARGS --pro $PRO"
  [ -n "$ELITE" ] && ARGS="$ARGS --elite $ELITE"
  [ -n "$MKE" ] && ARGS="$ARGS --mke $MKE"
  [ -n "$FM1" ] && ARGS="$ARGS --fm1 $FM1"
fi

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
MSG="Site for X0X ${V:-shell} (from $SHA); versions kept"
git commit -q -m "$MSG"
git push -q -f "$REMOTE" gh-pages
rm -rf .git
REPO="$(printf %s "$REMOTE" | sed -E 's#(git@github.com:|https://[^@]+@github.com/|https://github.com/)##; s#\.git$##')"
gh api -X POST "repos/$REPO/pages/builds" >/dev/null 2>&1 || echo "(enable Settings → Pages → Deploy from branch → gh-pages if the site is not live yet)"
echo "pushed to gh-pages → https://gobbajogger.github.io/X0X37-for-Loopy-Pro/"
echo "  install:  …/install/   (version picker + drop any .fwsc for upgrade/downgrade/stock/custom)"
echo "  samples:  …/breaks/"
echo "  projects: …/projects/"
