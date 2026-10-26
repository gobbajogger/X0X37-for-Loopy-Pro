# Reduce duplicate development checks

## Trigger and cause

The owner requested this change in chat `01a15a99-75d7-7921-9d65-f4b120566904` on 2026-10-21.
The owner requested omission of duplicate Pro checks after Elite passes.
The owner then requested other simple reductions.
The previous rule required checks for both affected targets without an explicit shared-development exception.

## Effect

Rule 9 now selects Elite for shared development checks.
Pro-specific changes and packages selected for installation or distribution retain their checks.
The defined coverage of an authorized full gate remains applicable.
A passing Elite build does not give a Pro artifact or device PASS.
Rule 11 links to this target selection.
Rule 9 also makes cross-stage reuse and one host-check run explicit.
Web-only and host-tool changes need no firmware build unless firmware inputs or package compatibility can change.

## Evidence

In `smc_mapping_stage4_20261021/evidence/mapping-stage4/artifact-verification.json`, both models have passing development records.
Elite has 1912 B code free. Pro has 1928 B code free.
Both have 524 B RAM free and 8364 B pool free.
Both controller artifacts have SHA-256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
The complete Elite package has 651396 B. The complete Pro package has 651392 B.
Package padding means that package sizes do not show the full application code difference.
These measurements apply to candidate `4ff0cd468e8afa746deca5840afb7f2890eedb9c` only.
`tools/build.py` uses the model selection for identity, loader identity, output paths, and package identity.

## Other reductions

1. Run model-independent host checks once.
   The mapping stage-4 script loops over both models with the same two regression commands.
   These shell tests do not read `MODEL`.
   Keep checks whose compile definitions or inputs differ.
   This reduction is explicit in rule 9.
2. Reuse stage-3 test evidence in stage 4 when the checked inputs and scope match.
   Reuse stage-4 artifacts after a merge when the resulting checked inputs match.
   A combined source change still needs affected checks.
   Rules 9 and 17 already permit evidence reuse. Rule 9 now names the stage condition directly.
   `tools/dev_validate.py verify` can verify existing development records and artifacts.
   Current records couple tests, application builds, and controller builds.
   Independent reuse of these parts needs a record-format change.
3. Build the controller once for the same controller inputs.
   `ble/ctl/build_in_container.sh` does not select a keyboard model.
   `tools/dev_validate.py run` rebuilds the controller for each complete validation.
   A separate controller record must include its source, generated RF tables, pinned libraries, ABI header, flags, scripts, and toolchain.
   Keep exact controller and package-insertion checks.
   This is a tool-change candidate. It is not implemented here.
4. Hash each dependency directory once per input snapshot.
   `tools/validation_inputs.py capture` hashes toolchain and SDK contents.
   It then hashes each configured reference path independently.
   A configured reference parent and its child directories cause repeated reads.
   An in-memory cache can remove duplicate reads within one capture.
   Preserve separate captures before and after validation. Do not use an mtime-only cache.
   This is a tool-change candidate. Its elapsed-time benefit is not measured.
5. Use the existing selected-test mode for local changes.
   `tests/run_tests.sh` accepts test names and `X0X_TESTS`.
   Avoid a full suite when the diff and dependencies need only specified tests.
   Do not remove distinct tests because their names are similar.
6. Generate test headers without a full unused host application build.
   Both mapping regression scripts call `host/build_host.sh` before they compile their own probe.
   Neither script runs the host application produced by that call.
   A shared header-generation step can replace these preliminary host builds.
   Keep each probe compile and execution. Generate fresh headers when their inputs change.
   This needs a small test-harness change. It is not implemented here.
7. Consider a content-addressed application build cache after the simpler changes.
   `tools/build.py generate` regenerates assets on every invocation.
   `tools/build.py build_app` submits all application units for compilation.
   The quickcheck comment calls this incremental, but that path has no object-reuse check.
   A correct cache must include headers, generated data, flags, model, configuration, and toolchain contents.
   It must retain link and budget checks for the resulting image.
   This requires more work than the preceding reductions.

`CTL_NO_CACHE=1` prevents writes to the shared installer cache. It does not control compilation reuse.
Keep that isolation flag. Its removal would not remove a controller build.

## Checks and limits

The task changed policy documents only.
No firmware tool, gate coverage, validation record, package, or device state changed.
`python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` passed for all 26 current entries.
`python3 tools/check_project_skills.py` passed for all ten skills.
`git diff --check` passed. All local links in the changed policy files resolve.
Rule 9 changed from 3498 B to 4214 B (504 to 626 words).
Rule 11 changed from 3909 B to 3977 B (554 to 560 words).
The additional text states the omitted work and its conditions.
No elapsed-time savings are claimed.

## Independent review

The bounded independent review found one incorrect package-size statement.
The statement now gives the recorded size for each model.
The review found no policy defect in the seven representative cases.
These cases included shared development, model-specific identity, full gate, web-only changes, stage reuse, combined merges, and changed controller ABI.
The review used source and saved artifact evidence. It did no test or device operation.
