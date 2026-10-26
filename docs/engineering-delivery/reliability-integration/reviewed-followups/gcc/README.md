# Independent GCC portability review

PASS for the bounded unit at source `4e20d3fa69a08c77bc36a741b0ae4b37e3df8cbb`, tree `d917d2172f872161553819e7d5d229537daf12f9`, atop `43d078dd384dd9d6544d368f5081da985dd27698`. Author evidence-only suffix `a8ddbf84c5e3cb721f7398ae110a9ca370043f73` adds no production/test source changes. Reviewer isolated clone `gcc-review` stays clean on frozen source.

The GCC path uses static inline math bodies and supported `optimize("Os")`/`noinline` attributes. The native Clang weak/gnu_inline/minsize path and all math expressions remain unchanged. Ensemble edits split chained lines without changing conditions or order. No warning flags are weakened. The OTA mock now checks framing length before indexing, caps packed bytes74 to decoded buffer64, and refuses decoded lengths below3 before reading command. Independent sanitizer probes cover maximum decode and the75-byte packed rejection boundary.

Independent Linux GCC strict execution passed all9 targeted selectors: fastmath, fastmath-ubsan, bass303-sustain, chicago-arp, chicago-ens, smk-clip-keys, editor-ota-dispatch, project-protocol and project-stage. Mac GCC15 passed8; its UBSan link failed due missing libubsan, preserved rather than called passing. The actual mock under Clang ASan/UBSan passed13 boundary/valid cases. No full-suite repeat.

Verified29 author proof digests and7 source pins. Inspected actual local JieLi compiler-output files: all4 old/new target-object pairs are byte-identical with matching receipt hashes; all8 named weak-helper preprocessed function bodies are byte-identical; whole preprocessed differences normalize to whitespace only. Compiler binary hash and archived changed-source inputs match. These are independent verification of author native outputs, not new native compilation or full firmware qualification.

Source diffcheck passes. Full evidence suffix flags only preserved raw `proof/mock-wire-bounds.log:8` trailing output whitespace. Existing preflight random-invalid fixture failure remains with its separate owner; no production validation weakened here. No native package/controller/budget, hardware, install, device, GUI, recovery, push, merge or publication claim.

`receipt.json` records exact results and setup limits. `author-native-verification.json` and `verify-author.py` bind the author evidence. `linux-internal-focused.log` is the completed Linux selector run; the first bindmount run failed FIFO setup and is not counted passing. `mock-bounds.c/.json` are reviewer-only synthetic probes.
