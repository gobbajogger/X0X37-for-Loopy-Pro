#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the GitHub Pages site and safely overlay checked site contents on gh-pages.
# Only the designated publication owner may run this AFTER all required
# firmware, installer, model, data-preservation and license gates pass.
# This script is NOT a gate, release authorization or device test.
#
#   tools/publish_pages.sh --shell
#   tools/publish_pages.sh 0.2 --pro PATH --elite PATH [--mke PATH]
#   tools/publish_pages.sh --nightly ...
#     Uses the separate qualified nightly route; never pushes production gh-pages.
set -eu
cd "$(dirname "$0")/.."
if [ "${1:-}" = "--nightly" ]; then
    shift
    exec python3 tools/publish_nightly.py "$@"
fi
# Accident prevention: merely running `--shell` or specifying a version must
# never publish. Publication is a distinct, owner-controlled action after the
# external complete DEV test/build/artifact and legal gates have passed.
[ "${X0X_PAGES_CHECKED_DEV_PUBLISH:-}" = "YES" ] || {
    echo "refusing publish: set X0X_PAGES_CHECKED_DEV_PUBLISH=YES only after the DEV candidate publication gates pass" >&2
    exit 1
}
SHELL_ONLY=0
V=""
if [ "${1:-}" = "--shell" ]; then
    SHELL_ONLY=1
    shift
elif [ -n "${1:-}" ] && [ "${1#-}" = "$1" ]; then
    V=$1
    shift
fi
PRO=""; ELITE=""; MKE=""; FM1=""
while [ "$#" -gt 0 ]; do
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
    set -- --shell --out build/pages
else
    [ -n "$V" ] || { echo "usage: tools/publish_pages.sh VERSION|--shell ..." >&2; exit 1; }
    [ -n "$PRO" ] || { [ ! -f "build/x0x-smk37-$V.fwsc" ] || PRO="build/x0x-smk37-$V.fwsc"; }
    [ -n "$ELITE" ] || { [ ! -f "build/x0x-smk37-elite-$V.fwsc" ] || ELITE="build/x0x-smk37-elite-$V.fwsc"; }
    [ -n "$MKE" ] || { [ ! -f "build/x0x-smk37-mke-$V.fwsc" ] || MKE="build/x0x-smk37-mke-$V.fwsc"; }
    [ -n "$FM1" ] || { [ ! -f "build/x0x-$V.fwsc" ] || FM1="build/x0x-$V.fwsc"; }
    [ -n "$PRO$ELITE$MKE$FM1" ] || {
        echo "no packages for $V: build and validate approved model packages first" >&2
        exit 1
    }
    set -- --version "$V" --out build/pages
    [ -z "$PRO" ] || set -- "$@" --pro "$PRO"
    [ -z "$ELITE" ] || set -- "$@" --elite "$ELITE"
    [ -z "$MKE" ] || set -- "$@" --mke "$MKE"
    [ -z "$FM1" ] || set -- "$@" --fm1 "$FM1"
fi

# Snapshot exactly the previous Pages branch before generating the new site.
# Never treat a failed fetch as an empty site or drop older firmware versions.
git fetch -q origin refs/heads/gh-pages:refs/remotes/origin/gh-pages || {
    echo "gh-pages fetch failed; refusing site generation/publication" >&2; exit 1;
}
PAGES_BASE=$(git rev-parse --verify refs/remotes/origin/gh-pages)
rm -rf build/pages-prev
mkdir -p build/pages-prev
git archive "$PAGES_BASE" | tar -xf - -C build/pages-prev
if [ -f build/pages-prev/versions.json ] || [ -d build/pages-prev/firmware ]; then
    set -- "$@" --merge build/pages-prev
fi

python3 tools/make_pages.py "$@"
# The shared helper checks old firmware bytes, model headers, catalog hashes,
# and an unchanged expected gh-pages SHA; it pushes only fast-forward history.
sh tools/publish_pages_ff.sh build/pages "$PAGES_BASE"
echo "Pages publish operation finished; inspect output for no-op vs pushed ref, and verify live deployment/download hashes."
