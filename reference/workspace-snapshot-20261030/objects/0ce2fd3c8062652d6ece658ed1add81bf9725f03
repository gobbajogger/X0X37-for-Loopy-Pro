#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Install tools/update.hook and tools/pre-receive.hook as the server hooks of a bare repository.
#   tools/install_server_hooks.sh [bare repository, default /workspace/x0x37.git]
# Each file goes to a temp file in the hooks folder, gets mode 755 and replaces the old file with "mv -f".
# It also sets receive.advertisePushOptions=true (for git push -o skip-quickcheck="<why>").
set -u
SRC=$(cd "$(dirname "$0")" && pwd); R=${1:-/workspace/x0x37.git}
for f in update.hook pre-receive.hook; do bash -n "$SRC/$f" || { echo "install_server_hooks: NOT INSTALLED: syntax error in $f"; exit 1; }; done
[ "$(git -C "$R" rev-parse --is-bare-repository 2>/dev/null)" = true ] || { echo "install_server_hooks: NOT INSTALLED: $R is not a bare repository"; exit 1; }
git -C "$R" config receive.advertisePushOptions true || { echo "install_server_hooks: NOT INSTALLED: git config failed"; exit 1; }
for p in "update.hook update" "pre-receive.hook pre-receive"; do
    set -- $p; t="$R/hooks/.$2.new.$$"
    cp "$SRC/$1" "$t" && chmod 755 "$t" && mv -f "$t" "$R/hooks/$2" && cmp -s "$SRC/$1" "$R/hooks/$2" ||
        { rm -f "$t"; echo "install_server_hooks: FAILED: $2"; exit 1; }
done
echo "install_server_hooks: INSTALLED $(sha256sum "$R/hooks/update" | cut -c1-12) update, $(sha256sum "$R/hooks/pre-receive" | cut -c1-12) pre-receive in $R/hooks (receive.advertisePushOptions=true)"
