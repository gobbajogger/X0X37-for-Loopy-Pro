Bounded LCD reuse preparation, delegated 2026-11-06

Scope: isolated patch against reconstructed proposal tree 2e38c67d3dbf1b905085580229e395433aefd92d; do not apply to a shared branch. Existing UI/SMC owners retain writes and sole integrator retains source selection. Stop at reviewable patch, scoped test receipts and handoff.

1. Reconstruct exact proposal from PR698 plus pinned v2/v3/rearm patches; verify tree.
2. Trace final donor dispatch/painter boundaries and current header cache/SMC ownership.
3. Reuse only bounded commit helpers, lcd_tiles.h and call sites. Retain proposal painters, cadence, header snapshots, held-note, peer and panic paths; no #809 bulk restore.
4. Test fresh-index application, donor tile regressions and actual production dispatch/framebuffer helpers. Run applicable SMC host tests if their inputs are available.
5. Deliver overlap/order/source identities, actual PASS/FAIL/NOT_RUN and timing/resource risks to parent. Later owner applies after UI/SMC context and waveform work, then reruns final-painter proof and independent QA/native qualification.
