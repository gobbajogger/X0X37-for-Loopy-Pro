# Unselected integration proposal

Exact source tree: f333f4f0491f45324794f8a04cdf085b77eda1f5.
Base proposal: 2e38c67d3dbf1b905085580229e395433aefd92d.
CLI revision2 donor tree:20c60779d0bb3c086bd6f40a3348df44dfaac9a8; packet2078b7c89db56770d588a4a7eda8af6df3c4ad04. Parent reports CLI-MODEL-01 closed by original reviewer.
Settings donor revised tree:3f8eb61bcba4915eef02f0b7fddb3bba23a6787a; packet a8ede83514445a78ad602fcc57057b9f2304ac13, prior packet95f76cafcfeef7ec2964aa5bf933b12058e65a73. Parent reports P2 closed by original reviewer.

Each donor was independently reconstructed from authenticated exact patch bytes onto2e38c67d and matched its pinned tree. Composition is exactly the union:3 CLI paths +8 settings paths, no shared paths and no manual resolution. Whole runner,252 registrations, existing panic fixes/tests, M7a interfaces, sequencer, engine/UI, version plumbing and pinned baseline argument preserved. Independent scoped composition reviewer found no issue. All five affected test files already have runner selectors. Engine and LCD additions remain excluded while under review.

## Receiving integrator procedure

1. Preserve existing sole shared writer. PR698 freshly rechecked draft at48e92b76c2f476cf40771e00a04e1838136bb55d, nightly66073c68dd80d51a7053432c2c161a8b720a1341. No shared branch was changed here.
2. Reconstruct prior unselected2e38c67d proposal from existing v2/v3/rearm evidence. Apply combined-v4.patch; SHA256 d10cdf90abc041f18bba2a0cbce506f8537eaab9935f08f5f4f5c4792ee46295. Fresh-index reconstruction must equal f333f4f0491f45324794f8a04cdf085b77eda1f5. Do not apply this delta blindly to current shared HEAD.
3. Consume scoped actual results and independent composition review. Preserve donor findings and external reviewer status; do not substitute packet author statements for reviewer closure. Return any newly found interaction to existing owners. Any future engine/LCD additions require exact input selection and new composition checks.
4. Resolve source selection/ownership and remaining gates before any ordinary nightly merge: full affected combined proof, required baseline/cross-firmware evidence, native/DEV/audio/resources, accepted scope and ROADMAP enforcement, UI provenance and installer qualification. No merge or publication follows from this proposal alone.

## Actual scoped results and limits

Eight CLI scripts exit0: install, controller, slots, loader binding, identity integrity, backup snapshot, finish ACK and restore status. The install script explicitly SKIPS logical-image comparison against fm1pkg.js because build/x0x-smk37.fwsc is absent. No hardware access; fixtures are simulated.
Project staging26864 checks PASS normal andASan/UBSan; save preflight PASS; boot roles PASS28 plain+28UBSan; project/SMC runtime PASS; expanded panic-route sanitizer suite PASS. Part-OFF runtime cases PASS with explicit NOT_RUN baseline equality/cross-firmware store; no claim transfers old donor cross-firmware proof to this changed composition. Web E2E result in settings-results.json.

The preexisting legacy format2 migration fixture fails on donor and prior proposal according to authenticated donor evidence; left open, not rerun or relabeled here. Full host/native/device qualification NOT_RUN. Prior package/native evidence remains scoped to its actual inputs.

Files: identity.json, cli-results.json, settings-results.json, review.json, logs/. No shared-source writes, merge, firmware publication or device operation.
