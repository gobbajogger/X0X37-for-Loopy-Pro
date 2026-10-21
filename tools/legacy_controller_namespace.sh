#!/bin/sh
# Host-only compatibility for an unchanged legacy controller builder.
# Called by dev_validate.py through unshare --user --map-root-user --mount --fork.
# Never use this helper without a new mount namespace: fail closed below.
set -eu
WORK=$1
[ -d "$WORK" ] && [ -f "$WORK/ble/ctl/build_ctl.sh" ]
# A new mount namespace must differ from the parent's before any mount operation.
SELF_NS=$(readlink /proc/self/ns/mnt)
PARENT_NS=$(readlink "$2")
[ -n "$PARENT_NS" ]
[ "$SELF_NS" != "$PARENT_NS" ] || { echo 'REFUSED: private mount namespace absent' >&2; exit 3; }
mount --make-rprivate /
mount -t tmpfs -o mode=1777,nosuid,nodev tmpfs /tmp
# The legacy builder hardcodes /tmp/ctlbuild and caches in HOME. Both now private.
mkdir -p /tmp/legacy-controller-home
export HOME=/tmp/legacy-controller-home
export CTL_NATIVE=1
# The original inner builder accepts OUT; never forward an ambient destination.
export OUT="$WORK/ble/ctl/out"
cd "$WORK"
printf '%s\n' 'Legacy isolation: fresh private /tmp tmpfs and HOME; unchanged source builder.'
exec sh ble/ctl/build_ctl.sh
