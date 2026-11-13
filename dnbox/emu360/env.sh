# Optional native-build env for fm1-live. No secrets.
# Override EMU360_ROOT (or ROOT), DEPS, CARGO_HOME, JIELI_TOOLCHAIN, AC79_SDK as needed.
# On the DnBOX box the usual layout is EMU360_ROOT=/workspace/emu360-test
ROOT="${EMU360_ROOT:-${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}}"
export DEPS="${DEPS:-$ROOT/deps}"
export LD_LIBRARY_PATH="$DEPS/sysroot/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export PKG_CONFIG="${PKG_CONFIG:-$DEPS/sysroot/usr/bin/pkgconf}"
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH:-$DEPS/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$DEPS/sysroot/usr/share/pkgconfig}"
export PKG_CONFIG_SYSROOT_DIR="${PKG_CONFIG_SYSROOT_DIR:-$DEPS/sysroot}"
export CARGO_HOME="${CARGO_HOME:-$ROOT/cargo-home}"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-${HOME}/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-${HOME}/fw-AC79_AIoT_SDK}"
