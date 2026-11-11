# Text-only recovered UI and SMC reference

This subset preserves exact historical source, tests, text mock generators, documentation and licenses for selective reuse by the existing source owner. It is not active firmware, a complete standalone build, installed functionality or current-candidate acceptance.

The [full independently reviewed archival reference](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/64575f86375e7daa9a939de25e6eb2f5047e0665/reference/recovered-ui) remains at immutable PR703 commit `64575f86375e7daa9a939de25e6eb2f5047e0665`. Its 76-file local integrity proof is reused. This new package includes only 55 original text files, excluding all19 PNG files and replacing the original README/manifest with subset-specific documentation. Four additional historical SMC source/test/license files are enumerated in this manifest. No binaries are added.

## Provenance and reuse

`MANIFEST.json` enumerates the 59 source/evidence payload files with SHA256 and Git blob identities. The two subset metadata files are additional. Original source, tests and licenses remain byte-identical. The old UI origin pins are recorded as provenance; source recovered from PR703 is anchored to its verified public commit. Additional SMC bytes match local Elite and Pro copies and the existing delivery receipt attributes source `0e58b960faa25a3c43d342cd923f5f14425ab2ba`, tree `aa9273145360c1e9583ec293240a1ecb4f58e65c`; new Git-object proof of that origin was not obtained.

The source includes old includes/paths and generators referencing omitted images/fonts/dependencies. Read expectations and reuse narrowly in the actual current source tree; do not reconstruct private dependencies or copy old engine implementations. Retain original GPL/font licenses. Preserve Chicago/Miami, revision13, static boot, existing ownership, and current safety behavior.

## SMC safety boundary

The added historical `smc_mixer.c` and two Python extractor fixtures preserve held-overlay, FX-chord/lifetime, lock/button and diagnostic-boundary reference behavior. They do NOT establish owner-approved physical FX mappings or current hardware correctness. Current one-build has newer transport/User-preset/reconnect/STOP logic: never wholesale replace it with this historical mixer. Already byte-identical current FX map tables and mapping tests are not duplicated here. The existing SMC/UI producer should consume only relevant deltas and validate all affected paths.

Python syntax parsing of the two added fixtures passed; execution remains NOT_RUN. Historical build evidence and old test documents are reported provenance, not builds/tests executed for this candidate. Mac, firmware, golden, performance and hardware checks remain NOT_RUN. No device operations or publication occurred.
