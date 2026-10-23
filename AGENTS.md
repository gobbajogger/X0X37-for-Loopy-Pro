# Nightly installer preview — repository rules

- This repo contains **public GPL-3.0-only** browser-installer source and a source-only, no-hosted-firmware preview. Retain author notices and license.
- Never add vendor JieLi SDK/toolchains, DSP reference trees, user samples/backups, credentials, firmware `.fwsc` images or `ctl.img` artifacts.
- The preview catalog must stay empty unless the owner separately authorizes an exact validated nightly firmware candidate and its distribution checks.
- After UI changes run `npm run build`, `npm test`, and an applicable responsive browser preview before pushing.
- The production `gobbajogger/X0X37-for-Loopy-Pro` repository, its `main`, and `gh-pages` remain separate destinations; do not modify them from here.
- Never force-push, publish stable firmware or flash hardware. Main updates here require explicit owner approval.

## Two-channel build catalog policy (owner, 2026-10-23)

- NIGHTLY is the complete development **status/history** inventory, including actual build generations, failed/unbuilt source checkpoints, superseded and hardware-untested revisions. Maintain `build-status.json` and its exact generated public read-only copy `docs/build-status.json`; record source SHA, model, package digest, status, physical evidence, and provenance truthfully. Do not expose flash/download links for unapproved items.
- The **installable** nightly catalog (`preview-catalog.json`, `docs/versions.json` and future `gh-pages/versions.json`) remains empty until a **specific** candidate passes all exact-byte source/package/controller/model/CRC/data-preservation/distribution gates and receives owner authorization. Read-only history does not override that rule or grant permission to host .fwsc or ctl.img.
- Production/public installer has **only development builds proven to work on the corresponding physical model**. A passing compilation, DEV CHECKED validation record, source recency or unsubstantiated `hardware_tested` value is not enough. Require attributable exact SHA/model/controller hardware PASS, all applicable non-hardware gates and explicit production publication approval per model, including an individually approved rollback. Empty production eligibility is valid.
- The nightly source branch is not a published site. Only the designated nightly Pages publisher may fast-forward validated status files to nightly `gh-pages`; only the designated production publisher may modify production `gh-pages`. Preserve historical binaries; hide excluded production choices instead of deleting archives, and do not rewrite history/force-push.
