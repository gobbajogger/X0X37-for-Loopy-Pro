# ui-theme progress

Lane: `codex/ui-theme`. Base: `e888d046`. Scope: local source commits, host checks and one `X0X_ORIGINAL_SYNTH=1` Elite Docker build. No push or device access.

## Current checkpoint

- Read the feature audit and pr-merges record. No supported project-chat reader is available; lane records provide the available ownership evidence. Other lanes remain read-only.
- Fetched #647 (`e6eb8267`) and #650 (`a51b030d`). #650 is a clean trial in pr-merges but has no result commit yet. Recheck before porting it.
- Reuse the approved matrix B presentation and the reviewed #647/#650 material. Prepare the missing Light/global material and static DnBOX boot proposal for independent review before implementation.

## Feature outcomes

- 56: done for the requested agreed presentation — #647 shared glyph/drum MIX material and #650 sequence/list/song/mixer material, reconciled with the original-engine matrix. Existing HOME/BREAK/FX/settings use the common surfaces. Actual candidate renders are tested and independently reviewed; this does not approve unspecified future layouts.
- 68: done — filled active row, four rows per bank, sparse unmapped cells, original parameter names and physical SMK K5–K8 captions. All original page targets and read-only performance indicators are checked.
- 70: done — six persisted palettes including Light, complete material/part colours and revision-13 admission/save/load. No format migration.
- 71: done for the current spelling/style request — mixed-case DnBOX on default boot, About/crash captions and installer; lowercase n retained in generated fonts; common visual material. No supplied bilingual-font or raster-logo reference is claimed as implemented; the code-native wordmark is shown in the reviewed proposal and actual render.
- 72: done — one static default boot frame and validated custom still picture/text rendering; no video/player. Actual startup selects Amber; Light boot is a renderer variant.
- 3: done for the original panel contract — 22 Chicago / 18 Miami controls/indicators, native selector states, musical rates, native unitless tune and exact neutral values. Legacy enum/ADSR/wave aliases cannot relabel original controls. No old engine is reintroduced.
- 6: done — original compact 55 Chicago / 30 Miami factory selection, real names and browser, both independent slots. Exact external input hashes are verified.
- 10: done in host proof — source/voice-owned notes block replacement; residual insert output fades for 512 samples, followed by 512 quiet samples below 1/65536 before factory adoption. Actual tested boundary is within one 16-bit LSB. Cancellation ramps back; typed history retains its prior contract. No audible hardware result is claimed.

## Validation

Host tests: NOT_RUN. Target build: NOT_RUN. Flash/RAM free: not measured. Local commits: none.

## #650 result integration

pr-merges completed #650 as `298af033f992b0430174fd4cb6b7365f9bf1b01f`. Cherry-picked its first-parent UI delta with `git cherry-pick -m 1 --no-commit 298af033`; excluded its lane-owned PR-MERGES.md. Reconciled the missing inherited surface helper from #650's reviewed head. Preserved the corrected whole-808 mute fixture. The temporary uncommitted #650 port was stashed before this operation.

`PATH=/opt/homebrew/bin:$PATH CC=cc CXX=c++ X0X_HOST_DEFS=-D_DARWIN_C_SOURCE sh tests/host/ui_main_material_test.sh`: PASS, 3,235,627 checks. Log: `build/ui-theme/pr650-main-material.log`. This is host renderer proof, not original-engine target or device proof. Final original-engine validation follows the remaining changes.

## Implementation and independent review

Local #650 result port: `6ba2085e`. #647 is ported on top of that result with its shared dispatcher and drum MIX geometry. The labelled proposal is `docs/ui-theme-proposal-20261030.svg`; independent design review accepted it after requiring full Light material/admission, original-control mapping, mixed-case font coverage and actual tail quiescence. No additional aesthetic approval gate is pending.

Implemented all six palettes including Light, matching material/part colours and revision-13 Light admission. Original matrix rows use original names/values and real factory names, physical SMK K5–K8 captions, sparse cells and filled active rows. Legacy ADSR/wave aliases cannot reinterpret original controls. The static default/custom still renderer uses the existing boot record and has no video player. Installer captions/defaults use DnBOX. The font generator retains lowercase n, including About/boot branding.

Original preset adoption now requires 512 consecutive rendered samples below 1/65536 in both channels in addition to source ownership. A numeric regression observed seven tail blocks after voice activity ended, then a tested adoption boundary within one 16-bit LSB. Original 55/30 tables and revision 13 remain intact. All twelve external headers were copied untracked from chicago-opt and hash-verified against EXTERNAL-INPUTS.json.

Validation checkpoint: original-release UI normal + ASan/UBSan PASS (22,949 checks, 36 equal PNGs). Initial SMK theme suite normal + sanitizer PASS (23,775 checks, 127 equal PNGs), before subsequent font and glyph-bounds checks. Actual Light, matrix, settings and HOME renders were inspected; that review found the generator's lowercase folding, now repaired.

The first wider runner is FAIL, not PASS: original integration assumed every tail ended within twelve blocks; ui-theme correctly rejected stale generated font data. Logs: `build/ui-theme/host-suite-initial.log`, `original-integration-initial-fail.log`, `theme-initial-fail.log`. Integration now has a bounded 4,096-block wait for the actual quiet boundary. Fonts were regenerated. A supported Python 3.12 plus Pillow was prepared entirely under `build/ui-theme/`; the first runner's host-build was NOT_RUN for missing Pillow. Corrected affected checks are running in `build/ui-theme/host-suite-final.log`.

Target build remains NOT_RUN. No device or publication action occurred.

## Tail-switch follow-up

The wider integration still found a held small insert residual after a 4,096-block wait. That check remains FAIL in `build/ui-theme/host-suite-tail-investigation.log`; it caused a product repair. Audio-owned gain now fades residual insert output over 512 samples only after note/source ownership ends; cancellation or renewed activity ramps back over 512 samples. DSP histories continue advancing. Miami source/voice activity is separated from retained decimator output, while its existing general activity predicate keeps the same semantics.

After that repair, `tests/run_tests.sh host-build original-release-integration ui-theme project-stage` is PASS (`build/ui-theme/host-suite-final2.log`). Original integration: 210,508 checks, normal/sanitizer equality, all 55/30 factories in both parts, seven Chicago FX, audio rendering, revision-13 project round trip, keys/MIDI/Stop and motion restoration. SMK UI/theme: 14,854,730 checks, 127 PNGs with normal/sanitizer equality. Project-stage invalid palette now uses 6; Light 5 is valid. Actual final THEME/LIGHT and mixed-case DnBOX boot screenshots were inspected.

Additional cancellation and Miami-tail regression coverage is running before the target build. Simulated boot-record test initially had a missing compiler include path; that wrapper is repaired. Web boot tests PASS. No target build has run yet.

## Source candidate checkpoint

Independent source and actual-render review has no remaining blocker. The factory fade is deliberately separate from typed undo/redo, whose baseline admission guard is restored; the regression covers immediate redo with zero quiet samples and no factory fade owner.

Final affected runner: PASS, `build/ui-theme/host-suite-final5.log` (`original-release-integration ui-theme boot-screen`). Integration: 210,508 checks, normal/ASan/UBSan equality. SMK theme/UI: 14,855,263 checks, 127 identical normal/sanitized PNGs. Chicago tail: three post-voice blocks, nine total switch blocks, smooth cancellation verified. The chosen Miami factory had no post-voice tail: zero natural tail blocks, four switch blocks; a separately seeded real decimator-held state proves signal history is not note ownership. The adoption output bound is at most one 16-bit LSB for both tested transitions. `project-stage` and `host-build` PASS in runner final2; `part-order` PASS in the initial runner. Web boot tests PASS. Earlier failed attempts remain failures in their logs; they are superseded only by the repaired affected checks.

Target build is the next authorized action. Source commit will identify the candidate; flash/RAM figures remain unmeasured until that build completes.
