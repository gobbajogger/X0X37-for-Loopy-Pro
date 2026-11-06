# Chicago/Miami carry-forward preparation

Prepared and tested locally. No source commit, shared-branch write, push, PR, merge, publication or device operation. Existing PR698 integrator remains the receiver. Independent QA has not run on this candidate.

## Exact identities and deliverable

- Isolated checkout: `/workspace/engine-carry-forward`, detached HEAD `48e92b76c2f476cf40771e00a04e1838136bb55d`; **NOT PUSHED**. HEAD names the reconstruction anchor, not the resulting candidate.
- Exact proposal base tree: `2e38c67d3dbf1b905085580229e395433aefd92d`. Reconstructed from PR698 plus v2/v3 at `c76228c531eea12ad67e668bf8bbb38ea03f35f2`, then rearm at `29dd62fab31c47bcea4e5325dbb074be72799abe`. Tree MATCH.
- Candidate tree: `01194a7f7fd0043833bf657756939819e23c99ca`.
- `engine-carry-forward.patch`: 29 production paths and 12 test/runner paths; SHA-256 `4a904d439829c9333dfcc02485bed52ba4e0b717f4ae3cc422c0fcad2c4a36b2`.
- Independent-index application of the patch to the proposal reconstructs the candidate tree exactly. `identity.json` lists every changed file; `donor-blob-map.json` records exact proposal/donor/candidate blobs.
- Rules read: main `1a6bc3fe0245277b5ae31ab554c73d74b9ab4d83`; nightly workflow `66073c68dd80d51a7053432c2c161a8b720a1341`; reuse map `ffdfb910d794e4cfcd8e1233ca92da186450d736`.

## Reuse and integration map

| Exact donor | Proposal finding | Prepared closure |
|---|---|---|
| `e0931311b72b347e7c83ab8a01f72feedf4c1a95` | Scalar Chicago render and Miami control-span scheduling exist, but fused kernels and voice-major Miami span are absent. These are not already-equivalent optimized paths. | Added Chicago MP4/voice kernel and Miami MS20 render-add kernel; connected backend spans. Existing exact Miami interpolation retained. |
| `2ad1b5171c8425011a9bdfb24b6f63c3d86986c8` | Existing clock-span and global quiet-frame helpers are reusable; scheduler skip, Chicago voice span, cv_fma and packed lookup cache are absent. | Reused existing helpers; added skip/span, exact finite-product FMA shortcut, packed last-index cache and master cleanup shortcut. |
| `6319ffec183555f521412e854308f214ee5ff4c3` | Per-frame decimator and generic settled mixer exist; corresponding block/settled paths are absent. | Added decimator block loop between resets and fixed-FX settled gain path, without changing effect selection. |
| `544e7f753b303b46ddd3565a496a4a5751a4b9b4` | Miami separately converts FX output then calls master. | Added exact inline output conversion and fused master i32 caller. Shared donor i16 entrypoint retained with its header, but Chicago output ownership remains unchanged. |
| PR813 implementation `a41651e9391012c19f9e09df04226f1f0c438afd` | Per-voice VCF/MG/glide caches absent. | Ported cache fields, invalidation and arithmetic without changing snapshot wire fields. Current PR813 head `196bae8c5b9097cdc3069fc8438f802c8db39638` has identical cache source/tests; its only later change is unrelated insert-test ASan flags, not imported. |
| PR814 final `dcc9833f24b20ec2baf47beb131f937e32d95e11`, compared from `d8086cc09203945919c70d3c805f8ca3e6acbe5a` | Cached selected/full MG and glide routes absent. | Imported final per-part shared cache (not superseded per-voice storage), snapshot/program/note invalidation, cached pitch, exact triangle-zero shortcut and pool sentinel. Used `.base_bits={0}` owner initializer to retain strict GCC14 compilation. |
| `e6aefdf78317a0a27ce32d5137d4b8f55479fac1` | Incremental bend/modulation recompile absent in both owners. | Retained full validation and revision fallback, changed only mapped controls, reused donor differential suite. |
| `0e04067d670dd77bdfdc7a28903d558f6ccc748e` | Miami computes program-only pulse widths in every control tick. | Hoisted identical calculations to compilation; dependency needed by original control-work-count test. Added 16 bytes of compiled controls, included in measured memory delta. |

The proposal already carries newer EQ, VCA, program/control tables, exact Miami interpolation and global/idle helpers. These are reused, not reimplemented: `preserved-inputs.json` records unchanged blobs for Chicago dynamics `.c/.h`, control math `.c/.h`, both program `.c` files, Miami interpolation and Chicago global MG. Existing backend silent paths remain; only active spans and cache invalidation were composed. New EQ, VCA and idle tests pass below.

No approximate `X0X_FAST_FMA`, restored-insert architecture, extra effect types, 81-program selection changes, UI, project/recording source, runtime allocator, build policy or compiler flags are introduced. The donor's Chicago drive/FX2/output rewrites, engine switch optimization, profiler and build flags are outside this bounded closure. Existing input conversion including zero fast paths remains intact. This patch does not claim every line of the four historical multi-purpose commits is carried forward.

Conflicts were narrow: Chicago coefficient hunk context assumed incremental recompile; Miami preparation hunk assumed hoisted widths. Both dependencies are now composed explicitly. Existing newer scalar filter and VCA code was not overwritten. Three runner registrations were added; zero existing literal `run` lines removed. Runner syntax PASS.

## Executed proof

All final commands below completed with exit 0 under GCC 14.2.0 unless identified as native JieLi.

| Check | Result and receipt |
|---|---|
| Private active input closure | PASS, 10 active inputs, manifest `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`; no private files changed or included in patch. |
| `sh tests/host/chicago_coef_cache_test.sh` | PASS, 2,200 ticks, PCM/state equality including snapshot restore (`chicago-cache.log`). |
| `sh tests/host/run_miami_coef_cache.sh` | PASS at O2/Os, hard/soft FMA; runtime-part 5,368 <= 5,368 bytes on host ABI (`miami-cache.log`). |
| `X0X_RECOMPILE_EXACT=1 sh tests/host/control_recompile_test.sh` | PASS, 379,667 checks; test defaults to exact FMA and rejects the approximate mode (`control.log`). |
| `audio_path_identity.py` on exact proposal and final candidate | PASS, 17 whole-engine scenarios / 50 audio-state files identical (`audio-final.log`, both hash manifests). Harness reused from rc1 `c32c54ac8ec68aaca53adccf7af66f155590f2dc`. |
| `synth_peak_test.py --reference-source /workspace/engine-baseline --out /workspace/engine-evidence/peak-final --rounds 3 --exact` | PASS, 20,447,232 PCM bytes per round, SHA-256 `6d9ad20ca43cc3e81ba962df8a922d6d419487666150c69d0fc3c2f6da854404`; all 30 Miami factories, selected FX/tails/edits, requested 1/4-note cases. Original harness from `0e04067d` (`peak-final/result.json`). |
| Same synth probe, `--rounds 1 --exact --sanitize`, ASan/UBSan | PASS, exact baseline/candidate PCM and no reported sanitizer fault (`peak-sanitized/result.json`). |
| Miami idle helper | PASS, normal/sanitized: 300,308 clock + 8,193 zero-store cases (`miami-idle.log`). |
| Chicago clock span | PASS, normal/sanitized: 416,330 exact comparisons each (`clock.log`). |
| Miami table interpolation | PASS, 408,412 cases each O2/Os (`table.log`). |
| Chicago EQ cache | PASS, 37,424 writes, 2,798,928 samples, 49,720 exact states (`eq.log`). |
| VCA sustain | PASS, 10,742,008 comparisons per configuration, four O2/Os hard/soft runs (`vca.log`). |
| Miami owner strict GCC14 compilation | PASS with `-Wall -Wextra -Werror -Wno-unused-function`; no missing-braces suppression. |
| Native Elite and Pro DEV | PASS, actual JieLi 4.0.1 builds with `X0X_DEV_PADCAL=1`, pinned SDK inputs (`native-elite-retry.log`, `native-pro.log`). |

Early incomplete hunk compositions failed compilation and were repaired before final proof. First native attempt failed writing the default cache on read-only `/home/agent/.cache`; retry used supported `X0X_GEN_CACHE=/workspace/engine-private-inputs/gen-cache` and passed. This was an ordinary filesystem error, not an approval denial.

## Measured memory and performance

| Native DEV metric | Exact proposal Elite | Candidate Elite / Pro | Delta |
|---|---:|---:|---:|
| App image | 486,312 B | 493,612 B | +7,300 B |
| Static RAM `.data+.bss` | 95,848 B | 96,372 / 98,304 B | +524 B |
| Pool used | 335,464 B | 335,624 / 344,064 B | +160 B |
| Pool free | 8,600 B | 8,440 B | -160 B |

The required 8,192-byte pool gate passes by **248 bytes**. This is not zero memory growth and must be rechecked after other lanes compose. Static RAM has 1,932 bytes remaining. Miami span scratch contributes 512 static bytes and packed lookup adds 12; two runtime parts grow 80 bytes each in the pool.

Host timing is measured, not a target timing claim. Three alternating rounds pinned by the existing probe show median average ns/sample: Chicago/1 79.84 → 49.80; Miami/1 139.59 → 86.28; both/1 225.76 → 139.04; both/4 225.65 → 136.84. See full `host-timing-summary.json` and per-block CSVs for peaks. No AC79 deadline, IRQ contention, dropout, idle_cpu_probe, or device CPU acceptance is claimed.

## Native reproduction and remaining gates

Compiler ZIP SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`; SDK-input ZIP `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`. Both verified before extraction, ZIP CRC/path/link checks passed, and `kit.py verify` confirmed all 11 pinned V1.2.1 files. No proprietary bytes are in the deliverable patch.

Actual command (repeat for pro):

```sh
X0X_GEN_CACHE=/workspace/engine-private-inputs/gen-cache \
JIELI_TOOLCHAIN=/workspace/engine-private-inputs/toolchain/jieli-linux-toolchains-20260730.1 \
AC79_SDK=/workspace/engine-private-inputs/sdk-inputs/x0x37-private-build-inputs-20261021/sdk-pinned-inputs \
X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model elite
```

Only local unselected DEV binaries were created; they are not deliverables or publishable packages. No release-mode (`X0X_DEV_PADCAL=0`) build, package_nightly/controller/installer qualification, full host suite, native PCM reference execution, independent review or physical device test ran. Hardware testing remains owner-only through the existing authorized route. Current compiler/native PASS does not close PR813/814's original release/CPU acceptance gates.

Next action: existing PR698 source integrator reviews/applies this exact patch to the selected candidate, reconciles overlap in the 29 production paths, and sends the resulting immutable tree to existing independent QA. Recheck native pool after other carry-forward work; only 248 bytes remain above the gate. No new source writer or merger was appointed.
