# UI carry-forward technical evidence

This evidence-only branch retains an applyable public-source patch and sanitized test receipts for the existing #698 integrator. No firmware source path is changed by this evidence commit. Raw captures, SDK/toolchain inputs, private data and binaries are excluded.

Read REPORT.md for exact sources, candidate tree, scope, measured results and limitations. Apply ui-carry-forward.patch only to its pinned base; verify the resulting tree against manifest.json. Existing independent QA remains responsible for combined-candidate review and design-provenance disposition.

Native RAM, stack and pool headroom are NOT_MEASURED. Host symbol sizes increased by 240 bytes; this is not a target budget pass. The legacy smc_peer_names_test.sh compile failure was reproduced on both the untouched base and candidate. Full host and native qualification are not claimed.
