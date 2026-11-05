# 909 optimizer recut on a06e20189

Reapply only the five 909 optimizer tip patches from `lane/optimizer-909-20261106-v2` onto `a06e20189141ff5fb8766e2c6152be460f4356c3`. They remove repeated settled-pan, held-pitch, rim and clap work so playing these voices does less repeated computation while preserving their sound and persistent state. No newer noise-advance or panic feature is imported.

Source lane tip: `daf1e281a93c581fe564c749d42e3aeac0fdcf2f`. The five-commit range starts after `a18561f17977be8592cef5a3d3cbebf70e4dbf1b`. The checked source commit is `81d038dfdc7a82b4354532df665babd89c3ea913`. The tested four-file source/harness tree is `748c0b168617fa4304f2730821e58ca2fb1e86cf`. Evidence files added afterwards do not change the checked firmware, build or test inputs.

The pinned GitHub base archive matched all 4735 Git blobs and root tree `33690b177cc7c9c8a697eb45b9f7309510d986ec`. SDK input files match all three V1.2.1 SHA-256 pins in tools/build.py. The compiler is JieLi pi32v2 clang 4.0.1 from jieli-linux-toolchains-20260730.1; archive SHA-256 is recorded in native-summary.json. The compiler and SDK were downloaded and used in this Linux x86_64 Codex executor. No SDK, compiler, firmware binary or raw capture is included in this delivery.

The primary agent ran the unfiltered full host suite: **333 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN; exit 0**. Strict setup preflight passed all 16 checks. The new equivalence selector passes at -O0 and -O2, each comparing full 7648-byte state and four PCM buses across 23280 blocks. The original 909/808 and sequencer reference checks use the exact revisions in reference-pins.json. The pinned baseline and recut both build for Elite and Pro in DEV configuration with SDK V1.2.1.

| Metric, both models | Base | Recut | Delta |
| --- | ---: | ---: | ---: |
| App image | 474840 B | 475576 B | +736 B |
| App RAM span, including alignment | 95780 / 98304 B | 95780 / 98304 B | 0 B |
| .ram_text | 2712 B / 844 instructions | 2712 B / 844 instructions | 0 B / 0 instructions |
| Pool used | 335464 / 344064 B | 335464 / 344064 B | 0 B |

App code has 77096 B free; app RAM has 2524 B free; pool has 8600 B free, above the 8192 B requirement. All four .ram_text byte hashes match and contain no calls. Native checks pass for image/RAM/pool limits, no soft-double routines and HAL-only register access. The compiler reports one pre-existing missing-braces warning in miami_decimator_adapter.c.

At 320 MHz and 44.1 kHz the nominal budget is 7256.24 cycles/sample. Target cycles/sample and runtime stack high-water remain unmeasured; source simplification, native size checks and host equivalence do not prove that timing budget. See REVIEW.md for observed local stack-frame increases.

The first baseline build attempt failed when its default cache resolved to the read-only home directory. Setting X0X_GEN_CACHE to a writable workspace path fixed that environment issue; the recorded four successful builds used that path. Tests and native builds were run by the primary agent here; independent review was read-only. No device or protected branch action occurred. The original lane is preserved; delivery uses the owner-approved wip/optimizer-909-a06e20189 branch without force push. PR: none.
