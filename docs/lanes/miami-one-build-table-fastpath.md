# Miami filter-table fast path on the latest integrated build

## Source and scope

- Canonical repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Immutable integrated base, verified on GitHub before implementation:
  `wip/one-build` at `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`,
  tree `ce0b0b9555e31ddd8d8ac54f6da385052e3565af`.
- Owner-authorized scope: preserve original Miami PCM, level and characteristics;
  improve active-voice CPU only where exact; no Chicago or UI/pad/selector edits.
- No gain/output/codec/USB/preset/project revision changes. No reintroduction of
  legacy `wobble.c` or `bass303.c`, no private #684 header, SDK, controller,
  firmware binary or raw audio capture is part of the Git commit.

The latest integrated source still calls the complete software FMA for four
table lookups in `miami_ms20.c` and the selected voice's inline MS20
preparation. The guarded `miami_interpolation.h` helper from the isolated Miami
optimization branch performs binary32 round-to-nearest-even table interpolation
in its proven positive-normal descending-table domain and delegates all other
cases to the original `original_rne_fmaf`. Under native FMA, `fmaf` remains
unchanged. Only three coefficient-table lookups and one resonance lookup in
each preparation path use it. Filter topology, table data, phase and oscillator
work, master level and state updates are unchanged.

## Executed cloud proof, October 30 2026

The exact two modified source blobs were reconstructed from GitHub's
`wip/one-build` contents, and the corresponding local compiled files have
the identical Git blob IDs `514a055c99fbb4fd8828198f8d3cb57cb6a1cbb5`
and `a7214c145e023a131103d8dc7ac2b65577c8d4e0`.

- Exact helper unit: `tests/host/miami_table_interpolation_test.c`,
  408,412 binary32 comparisons against the 18-limb reference per
  configuration. GCC and Clang, both `-Os` and `-O2`: 4 PASS.
  Both compilers under ASan/UBSan: 2 PASS.
- `tests/host/run_miami_table_interpolation.sh`: GCC and Clang
  `-Os` and `-O2` builds PASS, 408,412 cases per build.
  `X0X_TEST_JOBS=1 X0X_TESTS=miami-table-interpolation
  CC=clang sh tests/run_tests.sh`: PASS in reconstructed checkout.
- Selected voice: `original_miami_selected_voice_test.sh`
  with Clang: all nine ownership/arithmetic/sanitizer builds PASS,
  including complete sample/state byte equivalence. A baseline using
  **the exact unoptimized latest-base Miami DSP blobs** produced
  identical `.raw` output and state bytes to the optimized candidate;
  a matching baseline/candidate SHA256 for the selected-voice raw trace
  was `775b8322368f676f954d261a407b0832bb03ebc68512b0d11da061c993798317`.
- Joined original Miami stereo graph: native/software FMA,
  sanitized/normal Clang variants PASS; baseline and optimized
  binary outputs identical in both normal variants.
- `original_miami_backend_test.sh`: 30 factories, 18 endpoints,
  314,580 stereo frames and 1,470 blocks PASS in the reconstructed
  candidate checkout, including sequenced keys and state operations.
- `miami_ms20_transaction_test.sh`: 260,632 exact
  output/state/status/FP-flag cases per configuration PASS, four
  native/software/sanitizer combinations.
- GCC's neighboring selected-voice/stereo-graph scripts FAIL TO COMPILE
  under `-Werror=misleading-indentation` on pre-existing one-line
  statements in `miami_selected_voice.c` and `voice_gain.c`.
  Clang compilation and all corresponding execution checks PASS.
  This is not silently marked as GCC PASS.

The cloud compiler was GNU GCC 14.2/Clang 17 for host and genuine JieLi
Clang 4.0.1 for firmware. Private SDK kit `x0x37-private-build-inputs-20261021.zip`
SHA256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`
verified all eleven pinned V1.2.1 SDK files; the JieLi toolchain ZIP
SHA256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`
verified and ran. Ten active private original-engine header hashes matched
the latest `EXTERNAL-INPUTS.json` manifest. The other two are reference-only.

**Pro and Elite native DEV compile/link completed successfully**
with `X0X_ORIGINAL_SYNTH=1`, `X0X_DEV_PADCAL=1`, V1.2.1 SDK
and the tested changed Miami source blobs in the recovered **local**
`6ae0928fe3264ee759c25c4f879eff4dc5dc3b20` source snapshot.
Both produced local 651,455-byte `.fwsc` packages and passed
the linker code/RAM/pool checks; the current configuration printed
an approximately 463,608-byte app and 89,064 bytes free in the
Elite/Pro app code area. **These packages are not built from the
complete GitHub `094bb1a5` tree**, are not current-HEAD target
qualification, and must not be installed, published, committed or
transferred as finished artifacts.

## Remaining qualification on the actual GitHub branch

The GitHub API resolved the exact latest base and the changed DSP blobs,
but the complete source archive of `094bb1a5` was not available as a
downloadable/mountable cloud file in this classic session. Networked
`git` is prohibited; the container's `api.github.com` DNS resolution
failed. The available source bundle was a distinct older complete tree.
Therefore the **complete latest-base native build, full host suite,
15 internal golden renders, all-30-program state/PCM battery and active
and idle Callgrind counts are NOT_RUN at this branch HEAD**.
Do not combine the historical complete-tree build receipts with
the current source changes into a fabricated target PASS.

After obtaining a complete source checkout at this exact GitHub commit
through an authorized source-delivery channel, run:

```sh
python3 tools/check_original_inputs.py --scope active
X0X_TEST_JOBS=1 X0X_TESTS='miami-table-interpolation original-release-integration' CC=clang sh tests/run_tests.sh
X0X_ORIGINAL_SYNTH=1 X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model pro
X0X_ORIGINAL_SYNTH=1 X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model elite
```

Then run the original-synth 30-program PCM/state and the 15-file
internal golden render checks on the same pinned input closure,
plus both active and idle CPU counts in the qualified Valgrind
environment, and compare against matching original Miami reference
events. Mark device parity, integrated CPU improvement, and
installer readiness **PENDING** until that exact evidence exists.

## Follow-up: strict GCC compile repair

The initial GCC selected-voice/stereo-graph tests failed on five pre-existing
misleading-indentation warnings. Five formatting-only statement splits
in miami_selected_voice.c and voice_gain.c remove the warning without
changing any DSP operations. All nine strict GCC selected-voice variants
and four strict GCC stereo-graph variants now PASS; Clang also PASS.
Reference versus optimized selected-voice and stereo-graph binary output/state
traces remain identical after the correction. Pro/Elite native DEV rebuilds
in the same archived-source compatibility checkout PASS after these edits.
Exact latest-tree target build and complete golden/CPU gates remain NOT_RUN.


## Latest integrated FMA oracle retest (2026-10-30)

The recovery snapshot's FMA source was 22 bytes shorter than the
current GitHub integrated FMA source. We patched ONLY the local
untracked build checkout to match the actual `wip/one-build` file
`experiments/original_synth/fused_reference.c`. Git blob hash was
verified **exactly**:
`625899e0c818a295a9eb673a9ff25087c425393d`.
The recovered local compiled Miami source blob IDs also match the
PR's current tested files exactly:
- `miami_ms20.c` `514a055c99fbb4fd8828198f8d3cb57cb6a1cbb5`
- `miami_selected_voice.c`
  `0f53534be461b5c30b192079a922df64f6dc9bdf`
- `voice_gain.c`
  `f64e3277ae02d6f02f7ee2f892f9e2ed7d99224d`

**Fresh retest with those exact components, not previous oracle
results** on the recovered source checkout:
- `CC=gcc|clang sh tests/host/run_miami_table_interpolation.sh`:
  **PASS**, each at `-Os` and `-O2`, 408,412 bit-for-bit
  comparisons against the newest complete 18-limb oracle.
- Separate GCC and Clang `-fsanitize=address,undefined` builds:
  **PASS**, 408,412 cases each; ASAN leak scanning disabled and
  UBSAN halt-on-error.
- `X0X_TEST_JOBS=1 X0X_TESTS=miami-table-interpolation
  CC=clang sh tests/run_tests.sh`: **PASS**.
- `CC=gcc|clang` selected voice host suite: **PASS** all nine
  reference/native/sanitizer cases per compiler.
- `CC=gcc|clang` original Miami joined stereo graph suite:
  **PASS** all four cases per compiler. A combined initial rerun
  timed out before the final Clang graph test; that one was rerun
  independently and **PASS**. Do not report timeout as a pass.
- `original_miami_backend_test.sh`: **PASS** all 30 factories,
  314,580 stereo frames, 1,470 blocks, reported hash
  `cbc19169bf6599d4`.
- `miami_ms20_transaction_test.sh`: **PASS** all four
  reference/native/ASan configurations, 260,632 exact output,
  state, status and FP-flag cases each.
- Native JieLi Clang 4.0.1 SDK V1.2.1 DEV **Pro and Elite
  compile/link/package PASS** with this corrected FMA source and
  exact modified Miami blobs overlaid onto the older archived
  `6ae0928` surrounding source. Each model: 463,560-byte app,
  89,112 bytes free in app area, .data+.bss 95,408/98,304 and
  pool 324,212/344,064. The Pro local package SHA256 was
  `17ddca709244c655513f0ae2bbb006ea747cf439155419494a60357f5817a6e9`
  and Elite
  `418522d3c55e091e11e0475df8e03cbd4d8ea3af26a958f37a82d058c2f77027`.
  These are **local compatibility artifacts**, not firmware
  binaries of the complete GitHub HEAD; they are **not uploaded
  to GitHub**, not installed and not distributed.
- The archived checkout lacks `tools/check_original_inputs.py`,
  so its direct script invocation was **NOT_RUN**. The already
  verified ten active private input files have exact SHA256s
  matching the integrated manifest, and the pinned JieLi SDK
  input-kit verification passed separately.

**Remaining precise boundary:** Only the changed public DSP files,
shared FMA source and focused independent tests are byte-identical
with the GitHub latest-base inputs. The *whole* surrounding source
checkout remains archived `6ae0928`, not `wip/one-build` or this
PR HEAD. Therefore the complete latest-tree Pro/Elite DEV
qualification, full host suite, golden 15/15, 30-program current-head
PCM/state trace, Valgrind active/idle instruction count and
physical hardware proof remain **NOT_RUN**, not fake PASS. No sound
boost or output-level change was made. The user explicitly requested
source and regression delivery without waiting for the owner;
the prepared branch and draft PR contain the exact GitHub-source
patch and honest proof scope.
