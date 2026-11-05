# Commands and acceptance

All commands ran here on Linux x86-64. No owner Mac, CI badge or historical output was used as current proof.

The environment used the official 20260730.1 JieLi Linux toolchain (clang 4.0.1), the three byte-pinned AC79NN SDK V1.2.1 packaging files, and reference commits 9W9 10cbe5c1e1c9e65b0818bb34644ab1a13092082c, 8W8 94aa2716d4c2e5851fc7c5dbb4c17adfd133c997 and TB3PO f49d81d05d1e29ae37728a08e30f193892be3bc9. input-manifest.json records hashes.

For both the exact base and the candidate, each model was built with the same inputs:

```sh
X0X_GEN_CACHE=/workspace/work/recut/cache \
JIELI_TOOLCHAIN=/workspace/work/recut/inputs/toolchain/jieli-linux-toolchains-20260730.1 \
AC79_SDK=/workspace/work/recut/inputs/sdk X0X_DEV_PADCAL=1 MODEL=elite sh build.sh
# Repeated with MODEL=pro in the same checkout.
```

The candidate full suite used:

```sh
X0X_STRICT_PREFLIGHT=1 X0X_GEN_CACHE=/workspace/work/recut/cache \
X0X_GATE_LOCK=/workspace/work/recut/gate.lock \
JIELI_TOOLCHAIN=/workspace/work/recut/inputs/toolchain/jieli-linux-toolchains-20260730.1 \
AC79_SDK=/workspace/work/recut/inputs/sdk \
TB3PO_REF=/workspace/work/recut/refs/schwung-tb3po/src/dsp/tb3po.c \
python3 -B tests/host/run_host_with_evidence.py \
  --outdir /workspace/work/recut/full-suite-final --jobs 8
```

The 9W9 and 8W8 references were available through the runner's default sibling paths, each pointing to its verified pinned reference tree. The supported per-command network grant was used for native execution and the final host suite; the default syscall sandbox made Node child creation and LeakSanitizer fail. No checks or sanitizers were disabled.

The structural runner is registered in tests/run_tests.sh. It runs the full-state/four-bus comparisons and the exhaustive binary32 check at -O0 and -O2, without its old skip switch. Every table read was also checked separately under ASan/UBSan.

Full suite: 333 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN; exit code 0. Transcript SHA-256: e21ab381796e309584596b87af0a87addb38dbd067e7fb863d1aea93e69f84c5. See the pristine run-record.json and the post-run formatting equivalence receipt.

Portable auxiliary checks (run from repository root):

```sh
cc -O2 -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math -fno-builtin -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen evidence/recut-build-25-struct-20261106/tanh-bench.c -lm -o build/tanh-bench
build/tanh-bench
cc -O1 -g -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math -fno-builtin -fsanitize=address,undefined -fno-sanitize-recover=all -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen evidence/recut-build-25-struct-20261106/pair-bounds.c -lm -o build/pair-bounds
build/pair-bounds
```
