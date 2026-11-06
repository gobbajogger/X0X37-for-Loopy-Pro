# CLI installer safety reuse

The three-path patch restores exact-model installation admission and name-independent MIDI discovery from PR806, without bulk-merging its branch. The production installer is byte-identical to the donor blob f57d8e2e8a844b4278efa44aabe5b647d5d5e1c4.

Base proposal tree: `2e38c67d3dbf1b905085580229e395433aefd92d`.
Candidate tree: `90b7b2f79196a8840fb174adff39ad0e5edd9a0f`.
Patch SHA-256: `aabf8717bbe77352a6e87795e054175616cf0903734d60b273cfe33b9b22c8e7`.

`identity.json` records fresh-index patch reconstruction and exact changed paths. `PLAN.md` records ownership and overlap. Only tools/fm1_install.py, tests/install_test.py and tests/install_f02_loader_bind_test.py differ from the proposal. Firmware, web, runner and NIGHTLY-version revision 2 files are unchanged. Within the installer, backup/readback/controller/version code and existing loader binding are unchanged. The donor's web assertions were excluded because web is separately owned.

Red evidence: updated existing installer test exits 1 with ten failed assertions on original production code; the renamed-loader ambiguity fixture exits 1 with one failure. Additional refusal cases cover Elite-to-Pro and Pro-to-Elite, running firmware and already-running loaders, requiring exit 6, zero upgrades and zero package bytes served. Donor cases verify renamed ports and misleading names cannot override identity. The original explicit --port restriction remains.

All ten green commands exited 0. Commands and exit codes are in results.json with individual logs. Python 3.12.14. These are synthetic tests; existing fake devices and controller/slot fixtures are reused. install_test.py skips optional logical-image parity against a real build/x0x-smk37.fwsc because that artifact is absent. Synthetic NIGHTLY packages do not qualify native firmware or devices. Independent QA, native firmware builds, firmware publication and devices were not run. No source delivery or merge is claimed.

Local branch: wip/cli-installer-safety-20261106. HEAD remains PR698 base 48e92b76c2f476cf40771e00a04e1838136bb55d; the isolated index contains the full reconstructed candidate. NOT COMMITTED / NOT PUSHED. PR: none. Do not interpret HEAD as the candidate commit.

Next owner: parent/existing integrator and assigned independent reviewer. Apply cli-safety.patch to an isolated exact proposal tree, inspect the three-path delta and reuse the scoped evidence. Compose with other lanes only through their current owners; this patch grants no merge, publication or device authority. No blocker remains for this local patch stage.
