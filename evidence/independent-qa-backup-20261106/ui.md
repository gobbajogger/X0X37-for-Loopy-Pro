# Independent UI carry-forward review

Result: no new blocking source or host-render defect found in the scoped delta. This is not release acceptance, native memory qualification, a UI provenance approval, or approval of integration into a different combined tree. Existing LX-REQ-016 / related LX-REQ-023 ownership, finding identities and review budget are preserved; budget UNKNOWN/UNCHANGED. No prior review was reset.

## Exact source

Packet commit `d9a7b3aa7d21f733ad725090a9bd59afaa438542`, directory `evidence/ui-carry-forward-40c9273a`.

- Verified base commit `c295af1554f9e80a29864cd013995eccad66cd2c` has tree `d98072ba1d689cb99bd16e4a6e1e682b22b04c70`.
- Independently verified patch SHA256 `2fcecab3530e019bc3618e782acdf2b09f2b5f129b818a401f4dd82d9bf3ac4b`.
- Applied only to an isolated Git index; resulting tree exactly `40c9273a6bfbf99c4e799ad26abc5b4ecb68013e`.
- All 16 changed-file SHA256 values match the packet manifest. All 5,113 extracted tracked leaves match the candidate Git tree, including symlinks. No candidate production source or shared checkout was edited.
- Current nightly policy pin remains `66073c68dd80d51a7053432c2c161a8b720a1341`.

This candidate is based on c295, not the earlier independently reviewed combined-v4 tree f333. This report does not transfer qualification to a future union of those candidates.

## Source review

Verified six GLOBAL sections preserve the existing settings/action rows, local section selection/scroll, confirmation behavior, and SMC navigation. Held section-entry/back arrows consume repeat ownership through matching production/host callers. Settings navigation does not introduce project-persistence changes.

Peer UI uses a bounded two-name snapshot and 43-byte joined text buffer, sanitizes nonprinting name bytes, and clips/marquees within the value region. Snapshot output is zeroed before connected names are copied; no transport pointer escapes. State/role sampling occurs before the guarded name copy, so this is a display snapshot, not an atomic transaction guarantee across every BLE state transition. Bounds and transition fixtures pass; no transport ownership change was introduced.

Waveform projection restores native sound programs into local temporary state, uses bounded integer geometry, and caches cell/voice/preview-mask input. Lock/motion previews do not modify stored sound. Reviewed Chicago oscillator mix and Miami carrier/modulator/wobble routing, cache invalidation and invalid-input handling. This is a raw oscillator cue, not a visualization of filtered/effected audio.

DSP, sequencer, project persistence, smc_mixer.c and the original seven-effect policy are unchanged. `ORIGINAL_CHICAGO_FX_CHOICES` remains 7; actual UI tests exercise all seven effect readouts and their fitting fonts. The main_fm1.c delta only consumes held menu arrows. Existing bounded LCD scheduling/dispatch and drawing cadence are retained; affected production-helper tests pass.

## Independent execution

Actual selected runner execution: exit 0, all thirteen requested checks PASS:

- ui-settings-sections: 494 checks.
- ui-peer-names: 61 checks; separate ASan+UBSan execution also PASS, 61 checks.
- original-release-ui: 23,176 checks in both normal and ASan+UBSan runs; all 40 normal/sanitized PNG pairs byte-identical.
- ui-perf-identity: 25,876 checks.
- play-question-ui and sound-reset-ui.
- smc-multi-peer: 5,769 checks.
- smc-ui, smc-ui-bounds, smc-record-feedback.
- lcd-bounded and lcd-dispatch-reuse: exact pixels, idle behavior, ownership, deferred DMA lifetime, <=1536-byte transfers and key/pad/clip dispatch.
- ui-waveform-truth: 32,768 exact evaluator samples, max error 0.

Additionally, the new production BLE peer-name bounds/snapshot test passed under its ASan+UBSan script. A separate unchanged-base SMC UI build/test passed for host memory comparison.

The old `smc_peer_names_test.sh` FAIL was independently reproduced on BOTH exact base and candidate: redefinition of `plat_ble_name` and missing `bt_peer_text`. It remains an unresolved baseline compile failure; it was not repaired, deleted, waived, or counted as PASS. See `peer-baseline-base.log` and `peer-baseline-candidate.log`.

Runner package tests SKIPPED due missing app/loader/AC79_SDK inputs. Full unfiltered suite NOT_RUN. No hardware tests, MIDI device operations or native firmware build were performed.

## Visual evidence

All 27 named PNGs in the packet manifest were independently regenerated and match their SHA256 values; see `capture-comparison.json`. Inspected actual GLOBAL index, peer marquee middle position, reset confirmation, Chicago waveform and Miami wobble waveform renders. These samples have legible labels and bounded drawing with no observed overlap/clipping regression. Expected peer text clipping is confined to its value field.

Producer raw captures were not present in the public packet; comparison is against its recorded hashes. Reproducing those hashes and inspecting candidate-pinned renders is independent render evidence, not proof that earlier mockups/design selection were formally approved. Prior #859/#860 selection/provenance questions remain with existing UI/QA/integrator owners; no historical preview waiver is generalized.

## Memory and remaining gates

Independent GCC host `nm -S` measurements reproduce the packet:

- ui: 568 -> 584 bytes (+16).
- original_wave: +156 bytes.
- peer_scroll: +68 bytes.
- Total additional named static state: +240 bytes. Source marks +172 in X0X_POOL and +68 ordinary BSS.

Independent host stack-usage output: ui_original_wave_project 352 bytes, ui_original_wave_update 48 bytes, original_wave_view 128 bytes. These are individual host frames, not target call-chain totals or runtime high-water. The program restoration and draw path require target stack assessment.

Native recompilation NOT_RUN. Native code size, BSS/alignment, pool free >=8192, Elite/Pro budget checks, stack high-water and timing remain NOT_MEASURED. Historical 8,600-byte pool headroom from another tree cannot be reduced arithmetically to qualify this candidate. There is no current-tree model/package/CRC/controller qualification here. DEVICE_NOT_TESTED.

The native Build owner must measure exact candidate outputs. Existing integrator must resolve formal UI selection/provenance and qualify any composed tree. Prior ROADMAP/distribution/release gates are not closed by this UI review. No merge, publication, source mutation or device action occurred.

Evidence: packet/, delta.diff, integrity.json, focused.log, bounds.log, peer-san.log, base-smc.log, baseline failure logs, and actual extracted-source build/host plus build/ui-theme/original-ui logs/captures.
