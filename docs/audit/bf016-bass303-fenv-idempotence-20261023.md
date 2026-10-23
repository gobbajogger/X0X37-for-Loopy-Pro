# BF-016 — repeated TB-303 filter-envelope mode restarts live ADSR

**2026-10-24 producer retest, revision BF-016/v2: SOURCE COMPLETE / PRODUCER HOST CHECKED / READY FOR INDEPENDENT REVIEW.** Source is NOT independently QA-approved, NOT target-built for this PR head, NOT installed, and NOT a live NIGHTLY package. Owner physical observations and model-specific results remain UNKNOWN. The previous BF-016/v1 record was incomplete because only an isolated setter projection had run; this revision runs and links the entire exact source file with the full regression under both host compilers.

## Identity and actual source boundary

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`; producer PR #99, isolated lane `lane/bugfinder-bf016-bass303-fenv-idempotence-20261023-1455`; unchanged PRODUCER SOURCE HEAD before this evidence-only change: `93d0851f2430714eede3851561b40d5d166356c6`.
- Immutable firmware `nightly` base commit: `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`; **base full root tree** `71b759dd206d844cceed9e40ae68966142720e9c`; distinctly **base firmware/ subtree** `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`. Neither base tree denotes the PR head tree.
- GitHub PR-head producer source `firmware/src/dsp/bass303.c` blob: `66c9851e3bc538e6b9932d6eb65c97229a79b308`; complete regression `tests/host/bass303_lite_test.c` blob: `8aacb0b31b20b5912dddc7e950b80bb8bebdb97a`. Both local tested inputs matched `git hash-object` exactly.
- Unchanged included `firmware/src/dsp/bass303.h`: `085b8521f7410ef03bd53aa82961ace51eaf5b60`; `firmware/src/dsp/fastmath.h`: `7d2e41bf70590bafa8af0a7376bb912bf763b098`. Both matched the corresponding GitHub producer HEAD blobs.
- Baseline `bass303.c` `379981bfc39c1bb368d14b5614e258145192224a` (derived from the exact producer source by removing only the two-line equal-FEnv guard) and original test `7826da8641e32ea7de6f58fde9e58befab8784a8` were separately Git blob-verified against NIGHTLY. Negative runs deliberately use baseline source with the newly added, exact-PR regression.
- Starting archive: Library `LoopyX0X37-Project-Sources-Bootstrap-20261023.zip`, SHA-256 `ea173ec3afd6adea9219622652ed0dcafe48f8a6cba43395c4dab62895928319`; historical full checkout at `7537a46`, NOT live PR HEAD. Reconstructed/replaced exactly four needed inputs and verified their blob identities against live Git before compiling. **This is actual full `bass303.c` execution with exact matched dependencies/test, NOT an exact full-root checkout, whole firmware-tree test, or firmware target build.**
- Source/test delta remains unchanged by this revision: skip FEnv mode handler only when clamped requested mode equals stored mode. Duplicate 1 and clamped duplicate 127 preserve filter-envelope stage/samples, true 1→0 transition resets it, duplicate 0 preserves the state. Existing audio, ADSR, lite/oversampled, velocity and toggle behavior remain exercised by the complete host regression.

## Actual native compile/run matrix (eight executable runs)

Linux x86_64; GCC `14.2.0`, Clang `17.0.0`; C99. From reconstructed matched-input directory, each command is exactly:
```sh
gcc -std=c99 -O0 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize=shift -fno-omit-frame-pointer -fno-sanitize-recover=all -Ifirmware/src/dsp firmware/src/dsp/bass303.c tests/host/bass303_lite_test.c -lm -o testlogs/gcc_O0_fixed.bin
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 testlogs/gcc_O0_fixed.bin
```
For each other configuration, substitute GCC/Clang, O0/O2, `fixed` source or `testlogs/bass303-baseline.c` and corresponding `testlogs/CC_OPT_STAGE.bin`. Identical actual compile flags and invocation are recorded for each variant; no hosted CI.

| Compiler | Opt | Source | Compile exit | Run exit | Local complete log SHA-256 |
| --- | --- | --- | ---: | ---: | --- |
| GCC | O0 | baseline | 0 | **1, expected** | `d4fd9f5e0d7705c67b3b8724d9237be4404e5453feb4b495be691b2d529594f4` |
| GCC | O0 | producer fixed | 0 | **0** | `e45b981eeb5383e884fa262d334ab71cf47e7ae1208a0f4d878816b6f3872512` |
| GCC | O2 | baseline | 0 | **1, expected** | `aac96f033f0baaf0fd5545168bfa689e8a23edcc2d409fb3dc08a2ef8748951d` |
| GCC | O2 | producer fixed | 0 | **0** | `76a9d29c2a073b4b18fb29dfec8571f6856a23161a97a8339d32b9e37f658d61` |
| Clang | O0 | baseline | 0 | **1, expected** | `19cc0c07722ebfe28b4810e6015bcf1ac14f7e01cbd2439c5a509ae5249c8620` |
| Clang | O0 | producer fixed | 0 | **0** | `5ef98a613bb4af7071244966666011c77c8ac790b332197347f0026a96867475` |
| Clang | O2 | baseline | 0 | **1, expected** | `6aa0cace9c9150b99d45efae9f71094538a1171202b9c08206214e5cf802dd76` |
| Clang | O2 | producer fixed | 0 | **0** | `66dd993a6f4a2dc21eccc58db03604e0dff68d4f09078339ba00d212c9ee1c7b` |

In each baseline run the complete test printed **14 failing assertions**, including identical/clamped FEnv restarts and downstream held/legato attack and sustain. Each producer-fixed run completed with `bass303 lite: ok` and no detected AddressSanitizer or enabled UndefinedBehaviorSanitizer violations. This is not a claim that the entire firmware passed sanitizers.

**Deliberate sanitizer boundary / separately owned defect:** Strict `-fsanitize=address,undefined` without `-fno-sanitize=shift` compiled but FAILED at unchanged `firmware/src/dsp/fastmath.h:57:14` (`left shift of negative value -2`) for GCC/Clang O0/O2. That is an existing separate issue owned by PR #28; this producer neither hides it nor edits its owner files. The four fixed runs above deliberately disabled only shift UB instrumentation while retaining ASan and all other UBSan checks. Strict combined sanitizer PASS is **NOT** claimed and must be rerun against integration that includes #28 or equivalent independently accepted correction.

## Current disposition and next named owners

- **Producer Hourly Bugfinder**: source + full linked, exact affected Git blobs + positive/negative regression PASS under scoped sanitizer; source-complete and ready for Independent QA. Historical v1 source-window 4× baseline fails and 4× corrected passes remain recorded in the existing manifest and are not conflated with these new real-engine tests.
- **Independent QA** (next): independently retest exact current PR source/test HEAD, verify the original negative assertions, confirm FEnv mode behavior on correct source, address strict `fastmath` shift UB via separately owned PR #28 and perform full affected/combined source review. Ready for review is NOT QA approval.
- **Hourly Build** (in parallel): compile exact pinned Elite/Pro DEV and matching controller **BUILD_ONLY** immediately from this producer's immutable source SHA; do NOT wait for QA, merge, Pages, catalog, or physical device. Record model, compiler/pins, artifacts, CRC/SHA, budgets; do not present outputs as safe/installable based solely on compilation.
- **Safe Merge** (only after review): combine accepted source with current NIGHTLY through its own isolated integrated PR, independently check conflicts and gates, never specialist direct `nightly` write. **Catalog/publisher** only after independent prehardware package/model/controller/backup/license/integrity/HTTP gates; keep prior verified builds.
- **NOT RUN**: exact entire Git PR root checkout/combined quickcheck, full firmware subtree/target builds, pinned Elite/Pro/controller, memory/pool budgets, model/mismatch/OTA/backup negative suite, independent QA, lawful firmware hosting, immutable LIVE HTTP checksum and actual physical-device testing. NIGHTLY Elite and Pro status: **NO INSTALLABLE YET**, physical feedback **UNKNOWN** (no attributable served per-model candidate).

No production `main`, firmware `nightly`, website, `gh-pages`, device, sample, vendor SDK or binary was altered by this producer.
