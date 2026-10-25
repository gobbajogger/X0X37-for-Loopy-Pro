# Chicago host analysis continuation

State: READY FOR CURSOR; acceptance pending. One bounded package, not a firmware
cycle. The completed local Chicago experiment has stopped writing. The sender
will make no further tooling edits after this packet is pushed. Cursor becomes
the sole continuation writer only after explicit acceptance and input readback.

## Problem and result

Private-file loading currently runs when analyze.py, contour.py and
slide_experiment.py are imported. Separate pure numerical functions from workflow
execution and add reproducible synthetic diagnostics. This lets maintainers test
experimental sound comparisons without a proprietary plugin or private captures;
musicians retain the existing firmware sound by default.

Inputs: base bd489eda7c4ce4b56770fd24d6eec228d997c778, completed original tooling
d15f6eb0a40fd64610ea4beb366c69ec74172bf9. The 12-file diff is source/docs only.
Production bass303.c SHA256 remains
321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f.
Prior local independent review accepted the bounded host experiment. It found
held-out slide trajectory RMSE 150.535 to 5.471 cents and one spectral anchor
4.391 to 2.616 dB; these are historical measurements, not synthetic-test results
or general Chicago equivalence. General wave/envelope/resonance/accent matching
remains unvalidated. No raw references accompany this handoff.

## One package and acceptance

1. Read this packet and the exact PR head. Post Cursor agent identity, execution
   placement, input SHA, accepted scope and continuation capability. If unavailable,
   report the actual blocker; Bugbot review quota is not coding-agent capacity.
2. Keep edits within tools/audio303 and its synthetic tests. Extract import-safe
   feature, distance and contour routines; preserve valid-input numerical behavior.
   Add an explicit main entrypoint to private-reference workflows.
3. Test known harmonic spectra, insufficient-partial rejection and JSON nulls,
   pitch-search boundary reporting, zero/nonfinite inputs, gain independence and
   separate train/holdout selection. Use original synthetic fixtures only. Run
   relevant commands in the agent runtime, never GitHub Actions.
4. Have a different reviewer verify final source and test behavior. Record exact
   SHA, commands/exits, observed results and remaining limits. Keep this PR draft.
   No DSP/default changes or claims of fresh licensed-reference verification.
5. Own ordinary repairs and at most one fresh independent verification successor
   needed to finish this package. Before transferring writes, checkpoint final
   SHA, stop/release the previous writer, create one supported successor and read
   its acceptance/results back. Record its real ID/link; a prompt alone is not
   acceptance. If no supported creator exists, report that precise capability gap.
   No repeated audits or routine root stage-start replies.

Request STANDARD/MEDIUM wherever exposed; report unavailable selectors honestly.
Use the reviewed engineering-delivery trial at
218dbf80cc61384947d6bb2b02e41fc1f3556048, contract SHA256
c41776551668119ce561c9e03fe9eb55d4fa03e52e0eda198e32c43fde75a9df.
Current explicit restrictions below override contrary historical repository text.

## Authority and stop boundaries

Reviewed source/tests/docs may be pushed to isolated non-main branches and this
draft PR updated. No source merge, NIGHTLY merge, publication or production
promotion is authorized here. NIGHTLY actions require action-time owner approval.
No GUI, credential changes, purchase, denied-action rerouting, duplicate writer,
SDK/toolchain uploads, vendor binaries, raw captures, secrets or broader sharing.
Keep licensed AU/reference runs and hardware on the Mac. Cursor may perform local
work only through an actually supported qualified runtime; provider name does not
prove device access. Do not interrupt active controls, multi-SMC, presets, scheduler,
Miami or firmware Stage 7 owners. Hardware is outside this package. Any future
hardware stage needs sole ownership, exact model/artifact/data checks and owner
notification before flashing; do not duplicate an existing flash.

Stop after reviewed synthetic diagnostics are delivered and accepted, or at a real
scope/capability/permission blocker. Report locally retained reference obligations
separately. Progress reader: this PR conversation and pushed source/evidence.
