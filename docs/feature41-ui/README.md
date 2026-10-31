# Feature #41: SMC FX momentary strip

Status: proposed **draft** implementation. This branch is isolated from `wip/one-build`, based on `5bff28caba24568cd5070acb99aaf37729cd8323`. It is not installed or merged.

## The approved two-page mapping

Every SMC strip has four independently assigned held momentary FX buttons, in physical note order `REC, SOLO, MUTE, SELECT`. Page and button indexing is `page*4 + button` into the existing `SMCFX_SUIT[bus]` tables. Missing slots are `--` and produce no FX. The display shows them top to bottom `MUTE, SOLO, REC, SELECT`. The strip order follows the real firmware's `part_at` physical bus assignment (909, 808, BREAK, Chicago, Miami, Reverb, Delay, Master). Button presses play the selected FX only while held; release stops them. Per-bus latest-held precedence is deterministic; peer zero keeps its existing precedence.

The three PNGs in this directory are **diagrammatic reference previews**, not verified real-firmware framebuffer renders:

- [Page 1 before/after](smc-fx-page1-before-after-reference.png)
- [Page 2 before/after](smc-fx-page2-before-after-reference.png)
- [Held 303A before/after](smc-fx-held-303a-before-after-reference.png)

They are intended to be replaced or supplemented by genuine before/after native framebuffer captures before approval. The owner-supplied screenshots are the authoritative pixel/layout reference.

## Behavior and preservation

The existing page-arrow gesture is retained with two pages. HOME+one key cycles only that key's effect. SELECT main-bus tap navigation and held activation delay remain; REC, SOLO and MUTE play momentary FX on the FX view. Short taps for the existing overlapping HOME/LIST/GLOB/SONG/PART/MIX keys still invoke their original navigation; other views retain original button behaviors. Existing engine DSP, Chicago/Miami voices, storage revision, USB and BLE transport are not edited.

## Test evidence

The isolated `smc_fx_strip.h` and mapping test passed **164 assertions on GCC** and **164 on Clang with AddressSanitizer and UndefinedBehaviorSanitizer**, using a limited suitability-map fixture. This is a scoped verification, not a full firmware test. The new `smc-fx-two-page-strip` suite selector and updated production-function host harness are supplied, but **exact-commit full host suite, Elite SDK V1.2.1 build, Pro SDK V1.2.1 build, and genuine framebuffer captures are NOT_RUN** for this draft. No device flash or firmware package was produced.

## Required before integration

1. Recover exact current source and the approved SDK V1.2.1, then run the complete host suite with zero failures and zero skips, fixing regressions in this branch.
2. Build both SMK-37 Elite and Pro from the exact draft commit.
3. Render real framebuffer page 1, page 2 and held 303A before/after PNG comparisons and compare with the owner's provided reference pixel/layout.
4. Recheck preserved navigation, held-key ownership, HOME+button remapping, per-peer release, and SMC page LED behaviors on hardware.
5. Keep this draft open until all gates pass. Never publish the installer or flash protected regions.
