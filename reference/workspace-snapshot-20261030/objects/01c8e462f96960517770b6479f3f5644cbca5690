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
BOARD=smk37 MODEL=mke sh build.sh --release "$VERSION"

PRO="build/x0x-smk37-${VERSION}.fwsc"
ELITE="build/x0x-smk37-elite-${VERSION}.fwsc"
MKE="build/x0x-smk37-mke-${VERSION}.fwsc"
[ -f "$PRO" ] || { echo "missing Pro package: $PRO" >&2; exit 1; }
[ -f "$ELITE" ] || { echo "missing Elite package: $ELITE" >&2; exit 1; }
[ -f "$MKE" ] || { echo "missing MKE package: $MKE" >&2; exit 1; }
python3 tests/smk_pkg_test.py "$PRO" "SMK-37 Pro_"
python3 tests/smk_pkg_test.py "$ELITE" "SMK-37 Elite_"
python3 tests/smk_pkg_test.py "$MKE" "MKE-P37_"
python3 tools/make_pages.py --version "$VERSION" --out build/pages \
  --pro "$PRO" --elite "$ELITE" --mke "$MKE"

site="build/pages"
archive="build/x0x37-installer-$VERSION.zip"
cat > "$site/INSTALLER_README.txt" <<EOF
X0X SMK-37 installer $VERSION

Unzip this folder, open a terminal in it, and start a local web server:
  python3 -m http.server 8000

In Chrome or Edge, open http://localhost:8000/install/ with the SMK-37 connected by USB.
Pick your keyboard model (Pro / Elite / MKE-P37). The sample importer is at /breaks/.
Firmware packages are under firmware/ for command-line installation.
EOF
rm -f "$archive"
(cd "$site" && zip -qr "../$(basename "$archive")" .)

SUMS="build/x0x-smk37-$VERSION-SHA256SUMS"
shasum -a 256 "$PRO" "$ELITE" "$MKE" "$archive" > "$SUMS"

echo "Pro firmware:   $PRO"
echo "Elite firmware: $ELITE"
echo "MKE firmware:   $MKE"
echo "Site:           build/pages/  (install + breaks sample importer)"
echo "Installer ZIP:  $archive"
echo "SHA256 manifest: $SUMS"
