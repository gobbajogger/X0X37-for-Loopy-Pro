#!/usr/bin/env bash
# Run in a fresh Linux x86-64 cloud checkout after saving its network settings.
# Usage: bash firmware-setup-check.sh /workspace/X0X37-for-Loopy-Pro FULL_COMMIT
# This validates development setup only; it does not install or publish firmware.
set -euo pipefail
repo=${1:?Pass the fresh cloud repository directory}
expected=${2:?Pass the exact approved 40-character commit}
[[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || {
  echo "Requires Linux x86-64 in the intended cloud environment." >&2
  exit 2
}
[[ $expected =~ ^[0-9a-f]{40}$ ]] || { echo "Expected a full commit SHA." >&2; exit 2; }
cd "$repo"
[[ $(git rev-parse HEAD) == "$expected" ]] || {
  echo "Checkout does not match the expected commit; select it before setup." >&2
  exit 2
}
[[ -z $(git status --porcelain --untracked-files=all) ]] || {
  echo "Use a clean isolated checkout; existing changes were preserved." >&2
  exit 2
}
for tool in cc c++ python3 git curl tar xz sha256sum; do command -v "$tool"; done
python3 -c 'import PIL; print("Pillow", PIL.__version__)'
export CC=cc CXX=c++
export X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export X0X_JOBS=4
deps=${X0X_CLOUD_DEPS:-/workspace/x0x-deps}
export JIELI_TOOLCHAIN="$deps/jieli/toolchain"
export AC79_SDK="$deps/fw-AC79_AIoT_SDK"
sdk_tag=AC79NN_SDK_V1.2.1_2023-12-13
mkdir -p "$deps" build/cloud-setup
if [[ ! -d "$deps/jieli" ]]; then
  stage=$(mktemp -d "$deps/jieli-download.XXXXXX")
  # pipefail also catches a failed download in the repository's curl | tar helper.
  bash -o pipefail tools/get_toolchain.sh "$stage"
  mv "$stage" "$deps/jieli"
fi
[[ -x "$JIELI_TOOLCHAIN/pi32v2/bin/clang" ]] || {
  echo "Existing toolchain is incomplete; inspect it without overwriting it." >&2
  exit 2
}
if [[ ! -e "$AC79_SDK" ]]; then
  stage=$(mktemp -d "$deps/ac79-download.XXXXXX")
  git clone --depth 1 --branch "$sdk_tag" \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git "$stage"
  mv "$stage" "$AC79_SDK"
fi
[[ $(git -C "$AC79_SDK" rev-parse HEAD) == \
   "$(git -C "$AC79_SDK" rev-parse "$sdk_tag^{commit}")" ]] || {
  echo "SDK checkout does not match the required tag." >&2
  exit 2
}
[[ -z $(git -C "$AC79_SDK" status --porcelain --untracked-files=all) ]] || {
  echo "SDK checkout has changes; inspect them before building." >&2
  exit 2
}
# Match the three input hashes pinned in tools/build.py. Fail on a mismatch.
python3 - <<'PY'
import hashlib
import os
from pathlib import Path
base = Path(os.environ["AC79_SDK"]) / "cpu/wl82/tools"
expected = {
    "uboot.boot": "4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4",
    "cfg_tool.bin": "276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b",
    "cfg/eq_cfg_hw.bin": "41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d",
}
for rel, digest in expected.items():
    actual = hashlib.sha256((base / rel).read_bytes()).hexdigest()
    if actual != digest:
        raise SystemExit(f"SDK hash mismatch: {rel}")
    print(actual, rel)
PY
{
  git rev-parse HEAD
  git -C "$AC79_SDK" rev-parse HEAD
  readlink -f "$JIELI_TOOLCHAIN"
  sha256sum "$JIELI_TOOLCHAIN/pi32v2/bin/clang"
  "$JIELI_TOOLCHAIN/pi32v2/bin/clang" --version
  python3 --version
  cc --version
} > build/cloud-setup/dependency-identity.txt
MODEL=elite bash build.sh --board smk37 2>&1 | tee build/cloud-setup/elite-build.log
test -s build/x0x-smk37-elite.fwsc
sha256sum build/x0x-smk37-elite.fwsc > build/cloud-setup/package.sha256
git diff --exit-code
git diff --cached --exit-code
echo "Elite development build completed. This is not install, release, or device evidence."
