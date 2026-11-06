# CLI installer reuse lane

Task: delegated CLI safety lane from parent 01a1af1b-2207-7017-b4fb-4e7f6c2c90ea.
Base proposal tree: 2e38c67d3dbf1b905085580229e395433aefd92d, reconstructed from pinned PR698 plus v2/v3/rearm patches. Donor: PR806 62cf11e9897aadd468c55ac73485a1ea8470e0d2. Policy nightly: 66073c68dd80d51a7053432c2c161a8b720a1341.

Writer: this delegated CLI lane, isolated /workspace/cli-safety-lane. Owned source: tools/fm1_install.py. Owned tests: tests/install_test.py and tests/install_f02_loader_bind_test.py. Evidence lives outside the candidate in /workspace/cli-safety-evidence. No shared owner branch writes. Parent/existing integrator owns composition; independent QA remains with its assigned reviewer, not this author.

Overlap: donor install_test includes web assertions; omit those because web discovery remains the web owner's scope. Do not replace whole donor files. NIGHTLY-version revision 2, backup/readback/controller code, firmware, web, and runner are unchanged. Loader binding tests share the discovery closure and must retain ambiguity and pre-existing-device refusals.

Steps: verify exact reconstruction; reuse donor implementation and CLI tests; extend existing tests for both Elite/Pro directions and loader identities and renamed loader ambiguity; demonstrate pre-patch failures; apply production delta; run existing synthetic installer/identity/loader/backup/controller/version regressions; produce three-path patch and exact tree receipt. Stop at local reviewable source and evidence. No merge, push, firmware publication, devices or native firmware build.
