# Independent test-only fixture continuation review

No blocking defect or weakened behavioral assertion identified in the scoped repair. Existing review history, finding IDs and budget are preserved (UNKNOWN/UNCHANGED). This is test-repair acceptance within this scope, not full candidate/release acceptance.

## Identity and immutable scope

Packet commit `4f3b9cce82002338dbadf3ae01ae68e44b27dcf2`, directory `evidence/host-validation-0260-20261106/test-only-fixes`.

- Base tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`.
- Verified patch SHA256 `3d276f1ff923d9595b79a709c0e4565793ab301fc7ee020d4ac5d9ab7d35fb5f`.
- Independently reconstructed exact tree `e3bd02f7a4aece4ce96aa0602fb41b990275b629`.
- Exactly 20 files change, all under tests/. All manifest file SHA256 values match. Firmware, host implementation, tools and runner remain byte-identical to reviewed base.
- After checks all 5,120 tracked leaves still match candidate Git blobs. No candidate source edits, merge, publication or devices.

## Assertion and semantics review

All three groups of repair receipts and their before/after logs were inspected, including the two initially still-failing arrow scenarios and final restored immediate assertion. Producer's all-21 RED history is attributed to those receipts. This reviewer independently reproduced two representative original fixtures (arrows and migrate) against the same unchanged production binary; both fail as expected on obsolete flat-menu navigation. Updated versions pass independently.

The three C fixtures locate visible semantic MIDI + CLOCK / PROJECT sections and enter via list_enter before testing their existing channel order/action/reset behavior. Added section-presence and missing-row guards fail closed. Existing behavioral assertions are retained.

Independently audited all 15 changed scenario files as multisets of functional expect statements, excluding only listsel and arrowsteps. Every original functional assertion is retained with multiplicity. New local list selections match the section structure; PROJECT is found relative to final SYSTEM for both category layouts. Clock-source, storage-write, migration rejection/adoption, note probability/count, transport, confirmation/cancellation and performance expectations are not reduced.

Left arrow intentionally exits a GLOBAL section; ALGORITHM now supplies decrement operations where the old scenario expected left-arrow decrement. Right-arrow repeat/upper-bound, section-entry consumption, page/clip/HOME ownership and same-frame fresh-edge behavior remain asserted. The arithmetic 110-33-8+1=70, then +1+2+1+1=75, is consistent with the documented 400/80ms repeat timing and consumed holds; these replace obsolete navigation totals, not functional outcomes. The clip scenario retains immediate clock-value checks plus subsequent stability checks; a swallowed entry would fail its next fresh-press assertion.

The updated source-shape checks retain calibration BOARD_SMK37/X0X_DEV_PADCAL and clip guards, project-hold precedence, input/display ownership and guarded arrow rearming. They are source checks, not device execution.

## Actual independent execution

Built the unchanged production host application, then ran all 21 affected selectors together on the exact assembled tree: exit 0, each PASS:

project-runtime, part-order, song-actions, smk-padcal-wiring, smk-arrows-owner; scenario-arrows, scenario-clip-arrow-menu, scenario-clklock, scenario-clkmove, scenario-clocksrc, scenario-tour, scenario-probcond, scenario-probreset; scenario-autosave, scenario-guard, scenario-migrate, scenario-migrate0102, scenario-perf, scenario-perfsave, scenario-undo; perf-scratch.

The perf-scratch check retains its actual WAV byte comparison. Project runtime retains subsequent queued-route/reset/lock/abort/solo/pickup checks. No test selector was removed or rewritten in the runner.

Four adversarial in-memory source-read probes were rejected by the actual revised static checks: removed arrow rearm, removed calibration clip guard, removed calibration developer guard, and removed project-hold guard. Production files were never changed. These probes verify specific guard sensitivity, not exhaustive mutation coverage.

Probe accounting: the first arrow probe failed from incorrect working directory, and its second invocation did not inject the change because that checker uses builtins.open rather than Path.read_text. Neither is counted as a killed fault. Both logs are retained. The corrected builtins.open probe asserts that the replacement anchor exists and then fails at the intended ownership assertion. The other three probes failed their intended guard checks. Final four-probe result is supported by the actual assertion failures.

## Remaining limitations and gates

Full unfiltered suite NOT_RUN here. Unchanged source/renderer/mute/panic/M7a/native inputs reuse prior scoped evidence; test-only tree identity does not establish a fresh native build result or erase existing native limits.

The packet's inherited smc-fx-peer-view missing popup_knob_full_hash and smc-ui-negative absent mutation anchor remain FAIL; optimization-equivalence remains blocked by missing Clang. External drum808/drum909 reference checks remain NOT_RUN; external TB-3PO subcheck, native package checks and extra legacy smc-peer-names failure remain separate. Package tests explicitly SKIPPED in this run for missing app/loader/SDK inputs.

PASS for scenario-migrate/migrate0102 does not close any separately recorded broad legacy format2 fixture failure. Prior ROADMAP, UI provenance/selection, native/resource, distribution and release gates remain with their owners. No hardware claim or full-suite green claim.

Evidence: packet/, delta.diff, assertion-audit.json, integrity.json, affected.log, host-build.log, old-fixture-results.json and original-fixture logs, guard-probes.json and probe logs, plus actual source/build/host and scenario outputs.
