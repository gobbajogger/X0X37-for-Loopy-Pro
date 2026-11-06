# Chicago/Miami carry-forward — local isolated preparation

Delegation scope: exact proposal tree 2e38c67d3dbf1b905085580229e395433aefd92d. Reconstructed from PR698 48e92b76 + c76228c5 v2/v3 patches + 29dd62fa rearm; git write-tree MATCH. Main rules read at 1a6bc3fe; maintained workflow at nightly 66073c68dd80d51a7053432c2c161a8b720a1341.

Existing PR698 sole integrator remains receiver. This directory is an isolated clone; no shared branch, remote write, merge, publication or device operation. Prior QA does not qualify this candidate.

1. Compare e0931311, 2ad1b517, 6319ffec, 544e7f75 and PR813 a41651e9 / PR814 dcc9833f against exact proposal; record per-hunk reuse and conflicts.
2. Adapt only engine spans, scheduler skip, exact FMA shortcut, fused Miami output and coefficient/glide caches. Preserve newer EQ, VCA, control tables and idle paths. Exclude X0X_FAST_FMA, restored inserts, unrelated engine/UI/build policy changes.
3. Reuse donor cache/control/audio identity tests, preserving old expressions as reference. Run affected current host checks and size/pool assertions. Record actual errors. No native speedup or memory acceptance without measured exact candidate receipts; pool_free >=8192 remains required.
4. Provide minimal patch versus exact proposal plus source map, executed logs and remaining native/independent QA blockers to existing integrator.

Overlap: experiments/original_synth/{chicago_voice,chicago_mp4,chicago_scheduler,miami_ms20,miami_pitch,miami_selected_voice,miami_voice_owner,oscillator_motion,master,miami_output}; matching headers; firmware/src/dsp/original_{chicago,miami}_backend.c. Incremental recompile overlaps control owner paths; reconcile separately. Reuse donor tests without replacing tests/run_tests.sh wholesale.
