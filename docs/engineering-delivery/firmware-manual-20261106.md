# Firmware manual delivery plan and receipt

Build: documentation only; no firmware build or installation.
Replaces: the unqualified SMK37.md control/install summary; preserves FM-1 history.
Checked against decisions: main AGENTS.md, nightly AGENTS.md and task/docs/writing/check
skills at 66073c68dd80d51a7053432c2c161a8b720a1341; current delegated manual request.

## Plan recorded before manual edits

The delegated outcome is a practical end-user and maintainer manual, grounded in
existing documentation and current candidate source. Stop at reviewed documentation
on an isolated branch and a draft PR; no merge, website or firmware publication.

- Documentation parent: nightly 66073c68dd80d51a7053432c2c161a8b720a1341.
- Owned branch: chat/firmware-manual-20261106. Sole writer: delegated manual task.
- Behavior reference: wip/one-build 48e92b76c2f476cf40771e00a04e1838136bb55d,
  observed PR #698 head on 2026-11-06. This is an immutable documentation reference,
  not a declaration that it is selected, released or device qualified.
- Read coordination 4a35a6cdaa0f112db6b3f577542138afd170c20d: request records,
  delivery records, role registry and runtime contract. No dedicated manual request
  or active manual writer was identified. Parent inventory agrees. The existing
  documentation-consistency role retains workflow/rule maintenance.
- Reviewed both pages of open source PRs. No dedicated manual PR was found;
  #826/#740 own SMC FX design references, #843 status reporting, #841/#842 rules.
  Do not edit those owned paths or infer their features have been integrated.
- Intended files: SMK37.md, docs/MANUAL.md, generated docs/manual.html,
  docs/FIRMWARE_REFERENCE.md, and this receipt. No firmware, ROADMAP, shared runner,
  SMC reference, build configuration, private inputs or binaries.
- Reuse: existing SMK guide, FM-1 manual/generator, SMC_MIXER.md, PROJECT_FILES.md,
  BEHAVIOR.md, ROADMAP.md and APC40_IMPLEMENTATION.md. Preserve old manual content
  with a prominent scope notice, replace the misleading SMK summary in place.
- Checks: exact API tree/blob verification of both archives; source tracing;
  applicable source-excerpt/pad-map tests; local links/anchors; HTML regeneration
  parity; changed-path and secret/binary review; one independent documentation review.
- Review/integration order: hold docs fixed for independent review, fix findings,
  publish only the reviewed five-file delta by GitHub API, open a draft against
  nightly, verify remote parent/tree/blobs. Existing sole firmware integrator
  receives the PR through the originating coordinator. No queue writer takeover.
- Before integration: compare the final selected firmware commit with the cited
  source. Newer unselected combined tree 2e38c67d3dbf1b905085580229e395433aefd92d
  was reported by the coordinator; it is not substituted for the inspected commit.
  In particular, panic must remain candidate-only until traced at the selected head.

## Results

Source inspection and scoped verification completed on 2026-11-06:

- PASS: API archive verification, 4,935 candidate and 4,021 base blobs, no mismatch.
- PASS: `python3 tests/host/test_freeplay_rate_pad_port.py`, production excerpts,
  enabled eight-rate/disabled note route; no firmware link or physical-pad claim.
- PASS: `python3 tests/pad_placement_doc_test.py`, source/document table consistency.
- PASS: `python3 tests/host/smc_fx_preset_preserve_test.py`, 502 assertions against
  real handler excerpts; confirms assigned-effect publication despite stale captions.
- PASS: 37 new-document/scope-notice links checked for local files/anchors or
  immutable candidate paths. Existing upstream manual links were not requalified.
- PASS: regenerated HTML byte-identical to a second generator invocation; scope
  notice exists in output. No new visual design or website deployment.
- PASS: delta limited to the three declared existing documents and two new Markdown
  files. Bounded credential-pattern scan and manual content review found no secret,
  proprietary input, firmware binary or device capture in the payload.
- Independent read-only review identified stale K4, part-order and SMC mappings.
  Corrections now follow actual SOUND-page override, PART_ORDER, encoder banks,
  context-specific navigation and eight FX-view buses. Final review closure is
  recorded in the PR; author checks are not described as independent approval.
- NOT_RUN: full host suite, firmware/controller builds, hardware, installation,
  Loopy UI and publication. No firmware inputs changed and no such PASS is claimed.

The source appendix records exact scope and separates inspected behavior from
planned features. Reproduction: obtain the two pinned API archives, compare each
blob's SHA-1 over `blob <length>\0<bytes>` with its recursive API tree (read symlink
text for mode 120000), run the three named candidate scripts, regenerate
`python3 tools/manual_html.py`, and check the new Markdown links against their
local or pinned destinations. Delivery uses the connected default author, one
parent and the reviewed five-file delta. Remote commit and PR identity are in the
PR/delivery response rather than a self-referential commit field.
The originating coordinator owns request-ledger intake and the integration handoff;
this branch does not allocate a competing request ID or modify the shared ledger.
