# Feature #41: mixer FX page, owner-spec recovery and real framebuffer preview

**Status: review-only, awaiting the owner's approval of the specific button map.** This is a stacked documentation/image change on top of [PR #736](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/736) at `dc4109b7dfcddda41168016d0a8c33143606c4dc`. It adds **no firmware changes** and must not enter frozen PR #734 until that PR clears its independently required gates.

![Actual mixer FX framebuffer before and after, with the missing Feature 41 button assignments identified](feature41-fx-button-map-review.png)

## Owner requirements and unresolved assignment

The owner's original **`DnBOX-LoopyX0X37-full-feature-list-20261029.docx`**, item **41: separate mixer effects page**, specifies a distinct mixer FX page, functional context controls with truthful captions, SELECT momentary low-pass **in FX context only**, access to Reverb/Delay/low-pass/high-pass, clear effect-area navigation, and a coloured fader direction/amount indicator. The document says that exact per-button assignments must be recovered from the agreed owner map, **not invented**.

**NOT RECOVERED:** an exact owner-approved FX-specific mapping for the physical REC, SOLO and other button rows, including their desired press/hold/release semantics. The present screenshot does not imply that these buttons are already mapped or working as FX momentaries. This remains a blocker to calling Feature #41 built; a visual mockup cannot resolve it.

## Existing source searched; reuse rather than rebuild

- [#366](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/366): momentary SELECT effects on key 909/808/303A/BREAK buses, ROOM/TIGHT/ECHO presets.
- [#368](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/368) and [#369](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/369): correct bus-suited curated effects. [#372](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/372): extended SELECT FX handling to eight strips.
- [#620](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/620): SELECT ownership across FX and non-FX contexts.
- [#688](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/688) and [#725](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/725): original and integrated FX UI, LPF/RVB/DLY/MIX/CYC captions. Both explicitly describe Feature #41 as **partial**; #725 at `8dfb42be5a1fa9832d5638e65fb85766a5dd99ad` says REC/SOLO assignments were not recovered.
- [#736](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/736) `chat/mixer-fx-page-port@dc4109b7`: the approved-look port and real framebuffer fixture, reused without edits.
- Repository `docs/FEATURE-REGISTRY.md` and relevant FX, mixer and momentary branches/PRs were searched. The recovered evidence establishes **existing SELECT functionality**, not a complete exact REC/SOLO/remaining-row owner map.

Only a source-backed matching implementation should be ported after the actual map has been approved. Do not assign REC or SOLO to a guessed effect.

## Real framebuffer provenance

The two **240 × 240** screen areas come from the real firmware `smc_ui_draw()` host renderer. They are pasted at 2× nearest-neighbour, pixel-for-pixel unchanged; all title, provenance and gaps are **outside** the captures. No SVG substitute or synthesized firmware interface was used.

- **Before:** existing [#736](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/736) baseline capture of `integration/all-in-one@a73882485f867d3ffeaea2c872f312fe4319295a`. Original source PNG SHA-256 `ee868130eafc7dab59ae5b948b29123ffbb23ef404b565a8acf3eb962294b13f`.
- **After:** fresh `#736@dc4109b7` renderer capture. The host checked `firmware/src/app/smc_mixer.c` Git blob `da2d1388b3b4f1ac98cc67490ddc72cf3fe7abb2` against source bytes. Raw framebuffer PNG SHA-256 `7d44c7f9f5fad66dc02fbc8b4b928097c3383ab035a57d4abcaf48b03d070abe`.
- **Review PNG:** `feature41-fx-button-map-review.png`, 1160 × 905 pixels, SHA-256 `288f3fe6c1fe7c090e2f4ec745619a29aa4b55730593efcf4004152eaf8efef1`. Exactly **1,355 of 57,600** source framebuffer pixels differ between the before and after images. The two enlarged source rectangles were checked pixel-for-pixel against their corresponding native captures.

Actual renderer invocation (on connected Mac-mini, source #736 checkout; no device I/O):

```sh
export PATH=/opt/homebrew/opt/python@3.12/libexec/bin:$PATH
cd /Users/mac/Documents/x0x37/pr736-api-proof/target
SMC_UI_TEST_FILE=tests/host/smc_fx_framebuffer_capture.c \
SMC_UI_OUT=/Users/mac/Documents/x0x37/pr736-api-proof/feature41-real-capture \
sh tests/host/smc_ui_test.sh
```

**PASS:** renderer command exit 0; actual `mixer-fx-page.png` captured, with one nonfatal `CK_FN` macro redefinition warning. **NOT_RUN for this review-only change:** complete host suite and SDK V1.2.1 Elite/Pro builds; firmware source was not edited. Never claim screenshot or renderer success as an Elite/Pro link result.

## Review gate and branch boundaries

The owner must first approve/recover the exact Feature #41 REC/SOLO/other-row mapping. **No firmware coding before that approval.** Keep this pull request a draft targeting `integration/all-in-one`, stacked on #736; #734 is frozen until full host suite and Elite plus Pro target builds have 0 fail and 0 skip on one exact head. Do not merge, flash, release, publish installer artifacts or alter `.github/`, sound engines or revision-13 storage.
