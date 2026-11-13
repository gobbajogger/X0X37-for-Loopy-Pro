#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build both SMK-37 model packages and a matching self-contained web installer for each.
# Usage: tools/build_smk_release.sh 0.6-apc40
set -eu
cd "$(dirname "$0")/.."
VERSION="${1:-0.6-apc40}"

python3 tests/smk_apc40_test.py
python3 tests/make_pages_test.py
python3 tests/gen_font_test.py
MAP_TEST="${TMPDIR:-/tmp}/x0x-smk-apc40-map-test-$$"
trap 'rm -f "$MAP_TEST"' EXIT HUP INT TERM
cc -std=c99 -Wall -Wextra -Werror -Ifirmware/src tests/smk_apc40_map_test.c -o "$MAP_TEST"
"$MAP_TEST"

BOARD=smk37 MODEL=pro sh build.sh --release "$VERSION"
BOARD=smk37 MODEL=elite sh build.sh --release "$VERSION"

PRO="build/x0x-smk37-${VERSION}.fwsc"
ELITE="build/x0x-smk37-elite-${VERSION}.fwsc"
[ -f "$PRO" ] || { echo "missing Pro package: $PRO" >&2; exit 1; }
[ -f "$ELITE" ] || { echo "missing Elite package: $ELITE" >&2; exit 1; }
python3 tests/smk_pkg_test.py "$PRO" "SMK-37 Pro_"
python3 tests/smk_pkg_test.py "$ELITE" "SMK-37 Elite_"

python3 tools/make_pages.py "$PRO" "$VERSION" build/pages-smk37-pro
python3 tools/make_pages.py "$ELITE" "$VERSION" build/pages-smk37-elite

for target in pro elite; do
    site="build/pages-smk37-$target"
    archive="build/x0x-smk37-$target-installer-$VERSION.zip"
    cat > "$site/INSTALLER_README.txt" <<EOF
X0X SMK-37 $target installer $VERSION

Unzip this folder, open a terminal in it, and start a local web server:
  python3 -m http.server 8000

In Chrome or Edge, open http://localhost:8000/install/ with the SMK-37 connected by USB.
The firmware package is also included under firmware/ for command-line installation.
EOF
    rm -f "$archive"
    (cd "$site" && zip -qr "../$(basename "$archive")" .)
done

shasum -a 256 "$PRO" "$ELITE" \
    "build/x0x-smk37-pro-installer-$VERSION.zip" \
    "build/x0x-smk37-elite-installer-$VERSION.zip" \
    > "build/x0x-smk37-$VERSION-SHA256SUMS"

echo "Pro firmware:   $PRO"
echo "Elite firmware: $ELITE"
echo "Pro installer:  build/pages-smk37-pro/"
echo "Elite installer: build/pages-smk37-elite/"
echo "Pro installer ZIP:   build/x0x-smk37-pro-installer-$VERSION.zip"
echo "Elite installer ZIP: build/x0x-smk37-elite-installer-$VERSION.zip"
echo "SHA256 manifest: build/x0x-smk37-$VERSION-SHA256SUMS"
