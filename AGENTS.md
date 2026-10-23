# Nightly installer preview — repository rules

- This repo contains **public GPL-3.0-only** browser-installer source and a source-only, no-hosted-firmware preview. Retain author notices and license.
- Never add vendor JieLi SDK/toolchains, DSP reference trees, user samples/backups, credentials, firmware `.fwsc` images or `ctl.img` artifacts.
- The preview catalog must stay empty unless the owner separately authorizes an exact validated nightly firmware candidate and its distribution checks.
- After UI changes run `npm run build`, `npm test`, and an applicable responsive browser preview before pushing.
- The production `gobbajogger/X0X37-for-Loopy-Pro` repository, its `main`, and `gh-pages` remain separate destinations; do not modify them from here.
- Never force-push, publish stable firmware or flash hardware. Main updates here require explicit owner approval.
