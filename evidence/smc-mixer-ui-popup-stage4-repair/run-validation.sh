#!/bin/sh
# Stage-4 repair formal DEV package validation for SMC mixer UI popup. No device write.
# Uses a worktree under $HOME because tools/dev_validate.py places scratch in repo.parent.
set -eu
REPO=/home/ubuntu/x0x37-smc-popup-stage4-repair
cd "$REPO"
export CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
export X0X_REFS="${X0X_REFS:-/opt/x0x-refs}"
export NINEW9_DIR="${NINEW9_DIR:-$X0X_REFS/schwung-9W9}"
export EIGHTW8_DIR="${EIGHTW8_DIR:-$X0X_REFS/schwung-8W8}"
export SCHWUNG303="${SCHWUNG303:-$X0X_REFS/schwung-303}"
export TB3PO_REF="${TB3PO_REF:-$X0X_REFS/schwung-tb3po/src/dsp/tb3po.c}"
export PYTHONDONTWRITEBYTECODE=1 X0X_DEV_PADCAL=1
COMMIT=828298d9db1a403919e7e4c8176db83512281bb2
PLAN=/workspace/evidence/smc-mixer-ui-popup-stage4-repair/dev-plan.json
EV=/workspace/evidence/smc-mixer-ui-popup-stage4-repair
for model in elite pro; do
  out="${EV}/${model}"
  rm -rf "${out}"
  echo "=== validating MODEL=${model} COMMIT=${COMMIT} ==="
  python3 -B /workspace/tools/dev_validate.py run \
    --repo "$REPO" \
    --commit "${COMMIT}" \
    --model "${model}" \
    --plan "${PLAN}" \
    --output "${out}"
done
