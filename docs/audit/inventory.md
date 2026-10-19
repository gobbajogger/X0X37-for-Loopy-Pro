# Instruction inventory

Snapshot: 2026-10-19, Asia/Vladivostok. This is an audit record, not an instruction source.

The machine-readable [inventory](instruction-inventory.json) lists 35 repository checkouts and 485 instruction or workflow files, with content hashes. It identifies wrapper copies, repo/worktree copies, canonical repair candidates, and historical records. The wrapper is not a Git repository. `rules_audit/docs` holds the repaired canonical files during this task; rollout paths are recorded in the main audit.

Historical backups, dated v1 rules, `repo-bak`, and archived rosters remain evidence. Do not follow them as live instructions. Active agent entry points must resolve to current canonical rules. A file hash records the audited version; it does not claim a code or device PASS.

Checkouts with AGENTS.md but no local docs/agent-rules.md at inventory time:

- `b10/repo`
- `work-303`
- `ux_smc_step_cancel`
- `ux_fader_pickup`
- `ux_note_lifecycle`
- `ux_clip_save_guard`
- `ux_song_mute`
- `flash_review_integration`
- `validation_batch11_local`
- `b3/repo`
- `ux_code_budget`
- `ux_project_link`
- `sb-padcal-wait`
- `integration_final_review`
- `flash_review`
- `batch11_local`
- `ux_copy_semantics`
- `ux_step_preview`
- `validation_batch11_rel`
- `ux_combined_review`
- `smc_ble_work`
- `ux_perf_scratch`
- `ux_save_feedback`
- `b7/repo`
- `validation_batch11_ble`
- `integration_smc_visibility`
- `smc_ui_work`
- `ux_code_budget_latest`
- `ux_apc_midi_owner`
- `clip_ui_work`
- `ux_save_preflight`

[Source archive search](skill-archive-search.json) lists all 46 readable tar/zip/skill archives scanned for `SKILL.md`, `asd-ste`, `caveman`, or `skills-v2` member names. No matching member was present. Git bundles were listed but were not imported or exhaustively traversed for unreachable objects. Checked-out docs contain skill path references, not recovered originals. Git history was not exhaustively searched. No archive was changed.

The doc walk excludes `.git`, generated `build` folders, `node_modules`, `.venv`, Python caches, DSP validation refs, and reference source trees. It includes the active wrappers, repo docs, worktree entry points, and historical rule copies in the workspace.
