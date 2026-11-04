# QA2: restore RC1 UI fixture execution

Repair the three stale host fixtures so the preserved release UI, matrix and glyph behavior can be checked before firmware qualification. No production behavior changes.

## Source and scope

Base: c32c54ac8ec68aaca53adccf7af66f155590f2dc; root ac490f7fae9198748aa54bd914d42f99d45444f9.

- tests/host/original_release_ui_test.c: base blob 5da2afc12bbeed86418492649e7e532b27d83fff, also consumed by ui-theme.
- tests/host/sound_matrix_test.c: base blob e606c9e107c429f1097346fdf1fa4b8040674c44.
- tests/host/ui_glyph_polish_test.c: base blob 240d3d5df39d8b2083743cf67a76a3ccd7b4dea4.

Keep production semantics from #807, #788 and #746. Exclude other fixtures, shared runners, firmware, ROADMAP and release artifacts from changes. This is a subset of ROADMAP QA2, not full qualification.

## Steps and evidence

1. Reproduce each existing compile failure at the exact base; read current helper definitions and callers to establish intended behavior.
2. Repair only stale helper calls, coordinates and necessary fixture preconditions. Preserve substantive runtime assertions.
3. Run original-release-ui, ui-theme, sound-matrix and ui-glyph-polish, their available sanitizer modes, meaningful negative controls and adjacent UI checks.
4. Compose the three fixtures, run the complete host command and retain every failure or unavailable check. Do not fix unrelated failures in this change.
5. Independently review the frozen fixture delta, then deliver tests and sanitized technical evidence through an isolated non-main PR.
6. Hand the exact delivered source and remaining results to existing integration and independent QA. No merge, publication or device action.

## Acceptance

The four affected selectors pass; firmware/shared-runner bytes are unchanged; the fixture delta has independent source approval. Full host/native/package/audio/performance/device/release qualification remain separate and must retain failures, skips and unrun coverage.
