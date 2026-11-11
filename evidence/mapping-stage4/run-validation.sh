#!/bin/sh
# Run the scoped DEV checks and complete package validation for both models.
set -eu
cd /Users/mac/Documents/x0x37/smc_mapping_stage4_20261021
export CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain AC79_SDK=/sdk
export X0X_REFS=/Users/mac/Documents/x0x37/validation_refs
export NINEW9_DIR="$X0X_REFS/schwung-9W9" EIGHTW8_DIR="$X0X_REFS/schwung-8W8"
export SCHWUNG303="$X0X_REFS/schwung-303" TB3PO_REF="$X0X_REFS/schwung-tb3po/src/dsp/tb3po.c"
export PYTHONDONTWRITEBYTECODE=1 X0X_DEV_PADCAL=1
for model in elite pro; do
 python3 tools/dev_validate.py run --commit 4ff0cd468e8afa746deca5840afb7f2890eedb9c --model "$model" --plan evidence/mapping-stage4/dev-plan.json --output "evidence/mapping-stage4/$model"
done
