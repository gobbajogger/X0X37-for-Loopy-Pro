# Recovered UI reference — historical source, not active firmware

This archive preserves locally available work for the existing one-build owner. It does not replace the live firmware tree, activate legacy engines, or establish current-head acceptance. No firmware/product binaries, private headers, raw device captures, SDK or toolchain files are included.

## Reference identity

The combined UI reference is local/owner-dev-window-20261029 at `0e58b960faa25a3c43d342cd923f5f14425ab2ba`, tree `aa9273145360c1e9583ec293240a1ecb4f58e65c`. The fixed Settings sections are MIDI + CLOCK, BLUETOOTH, AUDIO, KEYS + SCREEN, PROJECT and SYSTEM. Its archived UI source includes later common-base edits and preserves the reviewed e2 drawing/glyph functions. Native evidence is historical and unreviewed as a combined candidate.

The earlier scoped reviewed UI source is `e2f881b0b0d5fbde8b4e352ab03c8ae2adc46058`; its firmware inputs match native-built `2c8b1572b013180725d7302e9337e7376bbf97b7`, differing only in one test repair. Neither package establishes device acceptance.

## Contents and use

`MANIFEST.json` lists original paths, source pins and SHA256. Source and test files are unmodified historical bytes. Historical test harnesses and mock generators may refer to old includes or absolute build paths; this is not a standalone build or a request to recreate a private workspace. Read their expectations, then have the existing one-build producer reuse the proven UI selectively against its actual current dependencies. Preserve Chicago/Miami, project revision13, active ownership and all current safety gates. Never copy legacy engine implementations or private headers to make historical tests compile.

The PNGs are synthetic design mocks, not raw device captures. Settings design round3 and glyph design round2 received independent approval; this is distinct from blanket human approval. Earlier text inside historical documents may carry superseded delivery states; the provenance record distinguishes these from current authority.

## Acceptance and handoff

The next consumer is the existing one-build integrator/UI owner, not a replacement writer. Compare current Settings/glyph/render callers against the pinned reference, reuse exact proven behavior, preserve later feature edits, and report actual input-specific consumer action. Independent current-candidate review, affected renders and owner Mac builds remain separate stages. Do not treat this reference PR or historical build receipt as an installed result.

### Dependency boundary

Exact historical gfx.c, ui_theme.h, font generator, text BDF font sources with original licenses, and shared host UI harness/runner are included. Binary TTF inputs are deliberately omitted and listed by original filename/hash. Other firmware/DSP dependencies are not an exhaustive closure. Historical x0x project revisions and legacy includes are provenance only, never current integration instructions. This archive cannot be represented as a complete standalone firmware build.
