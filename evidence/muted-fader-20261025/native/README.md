# Native app size evidence

Both final normal Elite and Pro DEV links **FAIL**: XIP overflow 748 bytes.
This is source/test draft evidence and cannot qualify a package or device install.

| Measurement | b4 baseline | Final Elite | Final Pro |
| --- | ---: | ---: | ---: |
| App bytes | 552636 | 553420 | 553420 |
| Code headroom | 36 | -748 | -748 |
| Static RAM headroom | 300 | 284 | 284 |
| Pool headroom | 8324 | 8324 | 8324 |

Final image sizes come from separately labeled private diagnostic relinks after
the normal failures. Diagnostic links use a larger private linker MEMORY length
to expose exact image/static/symbol deltas, while the unchanged real board budget
check fails. `installable` is false. No diagnostic binary is included here or used
for packaging.

Baseline Elite recompiles all app TUs and exactly reproduces the accepted b4 app
hash. Candidate Elite recompiles the three affected units; Pro recompiles its
model-specific unity TU and reuses source-closure-matched model-independent
objects. Baseline Pro's accepted same-size b4 receipt is reused; it was not newly
compiled here. `native-size.json` records compiler/image, flags, object hashes,
normal outcomes, diagnostic outcomes and the executed private script hashes.
`source-closure.json` records the seven runtime before/after hashes and all local
quoted-include closures. System headers remain pinned by the unchanged compiler.

`reproduce.py` is a portable all-app-TU measurement helper supplied after the run,
not the cached-object orchestration that produced the receipts. Give it the exact
source checkout, four generated headers matching the receipt, and private output
inside the pinned Linux container. Use network disabled and a read-only toolchain
mount. It builds neither loader nor controller nor package. Normal mode retains
the linker failure; optional `--diagnostic` reports the oversized bytes with the
real budget still failing. Binary outputs must remain private.

Sanitized normal logs replace container paths with `<workspace>` and `<toolchain>`.
Full logs and binaries stay in the isolated private workspace. No user-local path,
SDK content, vendor binary, credential or device output is included here.
