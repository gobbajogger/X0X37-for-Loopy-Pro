# PR #715 exact-scoped cloud checks

Candidate base: `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`
Source HEAD before evidence-only delivery: `0ea379dce60f8d584657f64c54e15435af44d2be`

**Full PR host suite, Elite build, Pro build: NOT_RUN** (exact Git source not yet materialized in native runtime).

## New current-source-extracted C sanitizer reruns

### clang-rate0-address+undefined.log

```text
Compiler: clang; X0X_DEV_WOBBLE=0; sanitizers=address,undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: clang-rate0-address+undefined
```

### clang-rate0-undefined.log

```text
Compiler: clang; X0X_DEV_WOBBLE=0; sanitizers=undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: clang-rate0-undefined
```

### clang-rate1-address+undefined.log

```text
Compiler: clang; X0X_DEV_WOBBLE=1; sanitizers=address,undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: clang-rate1-address+undefined
```

### clang-rate1-undefined.log

```text
Compiler: clang; X0X_DEV_WOBBLE=1; sanitizers=undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: clang-rate1-undefined
```

### gcc-rate0-address+undefined.log

```text
Compiler: gcc; X0X_DEV_WOBBLE=0; sanitizers=address,undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: gcc-rate0-address+undefined
```

### gcc-rate0-undefined.log

```text
Compiler: gcc; X0X_DEV_WOBBLE=0; sanitizers=undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: gcc-rate0-undefined
```

### gcc-rate1-address+undefined.log

```text
Compiler: gcc; X0X_DEV_WOBBLE=1; sanitizers=address,undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: gcc-rate1-address+undefined
```

### gcc-rate1-undefined.log

```text
Compiler: gcc; X0X_DEV_WOBBLE=1; sanitizers=undefined
PASS: exact 1eb remote Free Play rate-pad handlers and mocked ownership
PASS: gcc-rate1-undefined
```

## SDK validation

```text
Kit checksums and all 11 pinned SDK inputs verified.
```

## Source SHA-256

```text
26a7fec8d3d7b305d68ab322c0e96e9a9c1efb04fd7fc6d26ebf54004da43681  /mnt/data/x0x-work/pr715-stage-evidence/rate_pad_exact.c
ccf855d9e545be49ffa72fa66b8604d60d0ebca29c2212abf1db10d918150d57  /mnt/data/x0x-work/pr715-stage-evidence/updated-test_freeplay_rate_pad_port.py
```

## Exact repository regression script (fresh runs)

The script is byte-identical to GitHub blob `50f3c477bbdfd7330a88b20cd99037b297e8995d`. Its source-inclusion files contain the six verified GitHub-extracted production routines from `smk_input.h@4baf29c88f38a08f4eeff3d8d963c776e2341ebb` and `ui.c@2e51cc1ffa6026c98eeaba76774b041982f5e7ac`. The extraction-only staging is not a complete repository checkout.

### GCC (exit 0)

```text
PASS: physical Free Play rate pads: eight rates and opt-out note behavior
PASS: physical Free Play rate pads: eight rates and opt-out note behavior
```

### Clang (exit 0)

```text
PASS: physical Free Play rate pads: eight rates and opt-out note behavior
PASS: physical Free Play rate pads: eight rates and opt-out note behavior
```

**Full host suite and Elite/Pro builds: NOT_RUN on this complete PR tree.**
