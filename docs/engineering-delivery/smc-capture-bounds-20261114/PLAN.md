# Retained SMC capture bounds repair

Base: 94197fc839b00aff47b98ca02f3c6396e4a6eabf.
Owned lane: chat/smc-capture-bounds-20261114. Sole integration owner remains
Firmware Main Builder / #869. Roadmap writer #887 and UX owner #891 unchanged.

Fix only invalid pattern indices in tests/host/smc_remainder_test.c capture_scenes.
First run a bounds-instrumented capture regression against unchanged source;
then select the last valid pattern using NPAT and validate the same capture path.
Keep the 64-step last-page scene. No production source or UI behavior changes.
Independent read-only review must inspect the exact diff and sensitivity evidence.
Deliver reviewed nonsecret text/source on the isolated lane and routine fix PR.
No merge, build, device or release action. Do not rerun prior passing F4 suite.

Artifact intake: supplied qualifier ID 6af8078c-4fd8-8191-bf80-8b75dce4c0e1
and reported prior counts are context, not consumed raw evidence. Requested
REPORT.md, NEXT_STAGE.json and f4-host-qualification-evidence.zip are unavailable
in this environment. Exact-title Library search found no matching packet;
user-message search found no qualifier history. No supported thread reader or
creator is exposed. Proceed under the explicit source-inspection fallback.
The creator retains preservation ownership; do not replace or delete old evidence.

Ownership reconciliation: no matching smc-capture branch; open PR searches for
SMC, F4, capture and bounds reveal no competing fixture repair. The shared
coordination/merge-queue ref returns 404, so no claim of global worker inventory.
Current nightly policy read at 66073c68dd80d51a7053432c2c161a8b720a1341;
current task authority explicitly permits this bounded host validation in Work.
