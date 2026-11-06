# Baseline UI restoration on combined v6 + RAM reduction

Source-only isolated proposal. No shared branch, merge, firmware publication, device operation or SDK/helper access.

- Input source tree: `0260cd3f78a1b3717ef988d54aca767d65b6a2bb` (combined v6 `fba9b1f2df4ca7c075a0f757e60319d04c3a02f9` plus the separately delivered 47-byte UI storage reduction).
- Baseline donor commit: `d8086cc09203945919c70d3c805f8ca3e6acbe5a` (0.0.24).
- Candidate source tree: `bc822c0a12a65df4cfbf8e2b051f93825b79facb`.
- Incremental patch: `ui-baseline-restore.patch`, SHA256 `f01a5595a9a7edab266ffd0ad6103b8f5af5d0ddd995a7707ff20419ec410abe`.
- Reapplied patch to a separate index initialized at the input tree: reconstructed the candidate tree exactly. Diff whitespace check passed.

## Scope and provenance

Parent explicitly requested restoration of the missing baseline visuals. Reviewed recorded donor `docs/engineering-delivery/f4-smc-mixer-remainder/UI_GLYPHS.md` and `docs/engineering-delivery/f5-waveform-cache-023/PICTURES.md`: reuse content pictures and audio-clock phase, preserve exact names/values and physical legends, no new buffers/timers. This is implementation against that scope, not a new owner approval or complete release claim.

Production change is only `firmware/src/app/ui.c`. Reuses donor selected GLOBAL section title, fitted part/mode XS font, audio-owned `seq.metro_view` transport pulse, NEXT pattern glyph toast with bounds handling, long-toast XS fallback, physical action-key cue, About label placement, and sound-header pending cue. The existing header cache now keys on metro phase using its existing field. Retains newer normal pattern badges, header commit path, bounded LCD dispatch, visual cadence mechanism, settings/navigation, SMC/input/reset/panic behavior, waveforms, seven-effect Chicago policy and the earlier RAM saving. No sequencer/mute/audio-engine changes.

The five navigation functions `global_index`, `global_section`, `global_back`, `open_global`, `list_sub_arrow` are byte-identical in donor, input and candidate (hashes in navigation-hashes.json).

Six test/support files accompany production change: frozen donor renderer reference for specified regions, dedicated details harness/runner, updated independent performance raster reference and phase/cache coverage, and test selector registration. The donor oracle deliberately excludes the newer normal pattern badge region; existing identity tests cover it. Full toast, sound-header and list-band comparisons remain. Synthetic long-toast input deliberately exceeds large-font width; it is a boundary fixture, not a real device message.

## Verification

- Baseline details: 160,347 pixel/behavior checks; repeat with UBSan passed.
- Performance identity/cache: see perf.log; includes independent metro phase changes, existing arc/wave cadence and pattern badge expectations.
- Focused suite: settings sections, Bluetooth peer names, play-question UI, sound-reset UI, bounded LCD, LCD dispatch reuse all pass.
- Original-release UI: normal and ASan/UBSan runs pass 23,176 checks each; 40 PNG captures byte-identical between modes.
- Before and after images are actual production-renderer host framebuffer PNGs, not mockups. Captured all six sections, clip action mode, fitted labels, long toast, NEXT patterns 1/32 and invalid999, nine sound-header phases for drum/synth, and pending cues. Before/after images are retained privately in the existing Library artifact; no captures are included in this GitHub packet. These are not hardware captures. The capture-only harness reports only one width-fixture check; it is not a functional regression suite.

## Memory and limits

Matching capture-only host executables contain identical B/b/D/d writable-symbol sizes (1,242,647 bytes each including host-only fixtures); no symbol-size changes. Header snapshot remains32 bytes, UI584, original waveform156, peer scroll68, LCD transfer buffer1536, blit hashes16. See memory.json. This supports zero added static state; it is not a native RAM/link measurement. No allocation or pool increase introduced. Rendering code and stack/flash costs require native measurement.

Native RAM: NOT_RUN. Native fit remains UNVERIFIED. Parent reported original combined v6 native link 98,324/98,304 bytes, 20 over, pool8,268. The prior 47-byte reduction is retained, but subtracting that from a failed link is not evidence of a successful native link or runtime margin. No native binary was built here.

Existing `smc_peer_names_test.sh` baseline compile failure (duplicate plat_ble_name plus obsolete bt_peer_text) was previously reproduced on untouched base; this restoration does not fix it. Working ui-peer-names and settings/interaction tests pass. No hardware timing/cadence, stack, native framebuffer, native RAM or on-device validation claim.

An initial original-release runner invocation omitted its required output argument and exited before testing; rerun with the required directory passed. Compile logs include existing unused-variable/macro-redefinition warnings. No release completeness claim.
