# Settings/project safety carry-forward

Scope: isolated adaptation on exact reconstructed proposal tree 2e38c67d3dbf1b905085580229e395433aefd92d. Existing parent remains integrator; independent QA is a later receiving stage. No shared source branch writes, merge, publication or devices.

Donor: PR810 45a7285e536d8c24e0c57bf512393c804a6aad5c, which already reconciles PR790 314614c011c3b8bb119f6d298c558288f99aed11 / settings fix 16ba9e239d116f99c5ed9428058ce9264fc45c69. Reuse real NOR/LZ tests.

Plan: compare donor with proposal; apply storage return classifications and previous-copy support; adapt fallback-save latch and stream recovery into current project.c; preserve current M7a live/staged OFF, settings tail and export header; run complete-save rejection, A/B recovery, runtime and OFF regression tests; deliver patch plus exact tree/test receipts.

Paths: firmware/src/app/{project.c,plat.h,plat_fm1.c}, firmware/src/storage.c, host/x0x_host.c, tests/host/{project_stage_test.c,project_save_preflight_test.c}. No sequencer/SMC/UI or runner edits. Potential overlap: project.c callers and project-stage fixtures; plat.h shared platform interface; storage returns consumed by SMC boot roles. Sequencer owner retains mute/OFF behavior; SMC owner retains role/UI ownership. Integrator must union these paths, not replace files wholesale.

Observed conflicts: five project.c donor hunks reject because proposal already has settings_read_failed/read_settings_payload plus M7a tail and OFF state; keep existing versions and add only missing settings_load_failed/recovery state and boundaries.
