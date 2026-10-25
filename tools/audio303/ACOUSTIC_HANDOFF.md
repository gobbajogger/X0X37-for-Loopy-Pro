# Chicago gate amplitude and velocity acoustic experiment

State PLANNED; fresh Cursor successor requested, not continuation of completed PR226.
One sole Cursor implementation writer; local coordinator supplies licensed measurements
and acceptance readback only. All changes confined to original host tooling/data/docs
under tools/audio303. No firmware or production defaults change.

## Purpose
Chicago Gate output was stationary while actual bass303 amp output decayed about
2.43 dB over .125–.425 seconds. Velocity40 vs100 changed level by -15.943 dB in
Chicago versus -11.689 dB in source. Improve an explicitly experimental host-render
amplitude model using actual source, preserving its spectral/pitch limitations.
This is substantive acoustic matching, not the completed import-safety/synthetic task.

## Exact inputs and ownership
Completed tooling head 823b327fc72be270639358c3f729067775437662, code
6fb4c6319be94f8169324254b5bfb1044c494e2d, PR226. Prior Cursor
bc-f6529365-5378-4a0a-8ccb-38569cd67245 stopped, no successor assigned.
Production bass303.c SHA256
321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f.
Historical experiment d15f6eb0a40fd64610ea4beb366c69ec74172bf9 improved one slide
setting/interval across two octaves and one spectral anchor, not broad equivalence.
New derived reference file accompanies this packet. Raw private captures and licensed
AU stay Mac-only. Derived values are original measurements, not synthetic training.
Use actual render.c and bass303 source; controls are explicit readbacks, not presets.
Reference AU Chicago3.1.6, public APIs, fresh instance every trial, 44100 Hz native,
mono average of stereo, fixed MIDI/block boundaries. Reset/reinit is not fresh-state
substitution. Factory names are metadata only. No copyrighted preset redistribution.

## Work and acceptance
1. Post real agent ID/URL, exact input head and packet/data blob readback, placement,
   sole-writer acceptance and continuation/review capability. Request STANDARD/MEDIUM
   when exposed; report unavailable controls. Do not retry the completed task.
2. Implement a bounded opt-in host amplitude experiment using actual bass303 output.
   Simple low-dimensional time/velocity correction preferred; no per-sample target
   reconstruction, per-heldout fit, arbitrary lookup or synthetic-reference tuning.
   Keep default render byte-identical. Explicitly handle bounded note-off/retrigger
   history; avoid presenting post-render diagnostic compensation as production DSP.
3. Freeze split before fitting: train note48 velocity100 across all five measured
   cutoff values plus note48 velocity40 at cutoff .6. Heldout note60 all cutoffs,
   note48 velocities80/127, fresh note60 velocities40/80/127. No heldout selection.
   Keep repeat/accent-repeat and overlap history as separate regression diagnostics,
   never optimize their labels against a different Chicago setting. Existing source
   velocity alters filter cutoff as well as gain; amplitude-only fit cannot hide this.
4. Primary metric: temporal envelope RMSE in dB over bins 2..8 (centers
   .125..425 s), each temporal contour normalized at bin2; plus relative-to-velocity100
   level transfer RMSE at cutoff .6. Report absolute envelope RMSE separately: use
   only one fixed training gain at cutoff .6, never independently normalize heldout
   absolute levels or fit a new cutoff curve. Across-cutoff absolute mismatch remains
   a reported limitation. Record baseline,
   selected model and all train/heldout rows, worst-case regressions, note/cutoff
   coverage, gain/shape/pitch separately, history first/second note, silence/tail,
   clipping/nonfinite outcomes. Predetermine handling of low-energy bins and report
   exclusions, do not remove bad rows after selection. Require measurable heldout
   improvement, otherwise reject model honestly. No universal sound-equivalence claim.
5. Run actual host renders and analysis in agent environment, not Actions; use local
   relay requests only if raw/reference-dependent metrics missing. Request must give
   exact source SHA, command/manifest and needed derived metrics. Local coordinator
   remains active and returns approved derived data via this PR. No raw data upload.
6. A different reviewer checks final exact source, split integrity, independently
   recomputes measured results from approved numerical data, default identity and
   regressions. Cursor owns ordinary repairs and at most one bounded verification
   successor through outcome; local creator/relay can launch a fresh PR successor if
   supported self-creation absent. Internal reviewer is QA, not owner-visible successor.
7. Push source/tests/nonsecret metrics/docs on this isolated branch; keep PR draft.
   Report code/data hashes, commands/exits, measured changes and honest failures.
   Stop at independently reviewed bounded acoustic result or precise capability gate.

Progress reader: this draft PR conversation and pushed code/results. Local coordinator
reads admission and actual results; root is not routine stage-start permission gate.
Current Miami coordinator owns integration/capacity; UI/note-lock, firmware and website
owners are independent. Do not edit their files or operate devices.

## Boundaries
No GUI, installations, new credentials/subscriptions, permissions changes, vendor
binaries/SDKs/raw captures/private paths, decompilation, hardware, Actions execution,
merges, publication, production promotion or force-push. Source/test/docs isolated
pushes and draft PRs are authorized. Current instructions override historical repo
merge/install/loop text. Do not run unbounded firmware release loops.
Canonical trial contract 218dbf80cc61384947d6bb2b02e41fc1f3556048,
SHA256 c41776551668119ce561c9e03fe9eb55d4fa03e52e0eda198e32c43fde75a9df.
