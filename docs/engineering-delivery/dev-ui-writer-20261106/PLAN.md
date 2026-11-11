# DnBOX dev UI next stage

Request: lane/dev-ui-writer-20261106, owner-authorized recut/build/test/PR delivery.
Writer and executor: this Codex task. Integration remains with the existing owner.

Pins: base 48e92b76c2f476cf40771e00a04e1838136bb55d, lane 564c8f32005f64348ae29a67a43a145c25c556d8.

1. Recut the eight UI writer commits (9c1f044b through 564c8f32) onto the exact base. Include only required helper source/tests; preserve base optimization and roadmap work. Adapt changed APIs to this base. Preserve old lane ancestry in the final commit so delivery is a normal fast-forward without rewriting history.
2. Run the existing behavioral tests before and after repairs. Run Elite and Pro development builds with SDK V1.2.1, then the complete host suite here. Fix actual failures and recover missing inputs; never count missing or skipped checks as passes.
3. Inspect the final diff for forbidden paths, private-header changes, binaries and secrets. Deliver only source and sanitized evidence to the existing lane. Read back the remote ref/tree and open or update exactly one PR into wip/one-build.

Stop condition: verified source delivery and PR with full cloud evidence, or a precise unresolved prerequisite with all independent authorized delivery completed. No main/nightly writes, .github changes, release builds, publication or device operations.

Implementation: PLAY dismisses a question before one transport toggle; HOME+PLAY stays redo. HOME+part SELECT confirms an undoable sound-only reset while mixer edits remain separate. Header/arc/wave optimizations retain pixels, pattern badges show saved content, and carried SEQ key releases finish before EDIT live-key dispatch. The recut adapts the lane to the base APIs without importing its unrelated ancestry.

Exploratory validation found and repaired SMC fixture declarations and pending Pause unlock cancellation during questions. Final acceptance runs against the committed candidate, using the source-bound host runner and exact V1.2.1 SDK assets. Independent read-only review found no other blocking source issues; direct fixed-pixel badge checks cover content mapping.
