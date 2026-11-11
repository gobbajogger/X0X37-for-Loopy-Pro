# DnBOX work index — updated 2026-10-29 08:00 PDT

Rules: feature work runs in ChatGPT chats (mode Chat, Extra High), one chat per task. Mac Codex is only for installs and perf/hardware tests, at medium effort (`-c model_reasoning_effort=medium`).
Hard rules for all: never commit or push the 12 private #684 headers or binaries; author genkigenki <3080081+genkigenki@users.noreply.github.com>; never push to main; no migration (rev13 only); only Chicago/Miami engines; never touch the bootloader, BTIF 0xC0000-0xC0FFF, USRTRIM 0xC1000, efuse or key_mac; no fm1_rescue.py or --force.
NOTE: all wip/* branches on GitHub are single-commit snapshots on parent 5ed2693d with private headers (incl. state_codec_private.h) removed; =1 builds need the headers on the Mac.
Old WARNING: base e888d046 (from #684 commit 9d1cf9fb) TRACKS experiments/original_synth/state_codec_private.h. Every lane branch below contains it, so none of them is safe to push until that file is removed or verified as not private.

## ChatGPT chats
| Title | URL | Mode | Branch (local Mac lane, base for the chat) | Status |
|---|---|---|---|---|
| smc-port | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae35931-9258-83e9-9b17-6b43457d9cf1 | Chat, Extra High | codex/smc-port @ 2d223fbf (WIP) | started |
| break-feat | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae35949-71d4-83ea-9df5-1222e6945f4e | Chat, Extra High | codex/break-feat @ 83cec78a | started |
| mixer-ux | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae35961-ff70-83ea-90b2-19a09f67e9d4 | Chat, Extra High | codex/mixer-ux @ 6c84db92 (WIP) | started |
| web-site | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae35976-ecd4-83ea-a63e-3c6f0278435d | Chat, Extra High | codex/web-site @ 540330dc (WIP) | started |
| ui-theme | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae35989-e368-83ea-9ca4-e8f5c509c4de | Chat, Extra High | codex/ui-theme @ 4dfac82a | started |
| pr-conflicts | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae359a3-193c-83e9-989c-0101c0685f89 | Chat, Extra High | codex/pr-merges (see PR-MERGES.md) | started |
| web-editor (90-105) | https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e/c/6ae3583d-c468-83ea-8744-462290cc9de9 | Chat, Extra High | — | started |

## Mac Codex lanes
| Lane | Task | Effort | PID | Branch | Status | Result file |
|---|---|---|---|---|---|---|
| perf-test | only installer; real PERF on SMK 37 Pro | default (next: medium) | 56836 | — | done: installed c287cfe4 (CRC ok, slots unchanged), results 186a0449; waiting for on-screen PERF readings | perf-test/PERF-RESULTS.md |
| fmb-guards-slots | guards/slots, migration reverted (rev13 only) | — | stopped 07:58 PDT | wip/fmb-guards-slots 4775ef91 (local 70b87343) | stopped; =1 build PASS 118,512 B free; host 222 PASS/57 FAIL | docs/lanes/fmb-guards-slots.md |
| chicago-opt | Chicago near-0 idle | — | stopped 07:58 PDT | wip/chicago-opt dd735533 (local 230cde40) | stopped; idle 146,978 to 20,638 instr/block, 15/15 bit-exact | docs/lanes/chicago-opt.md |
| miami-opt2 | Miami near-0 idle | — | stopped 07:58 PDT | wip/miami-opt2 c305351b (local 2d523006 WIP) | stopped; last WIP (miami_zero) unverified | docs/lanes/miami-opt2.md |
| pr-merges | PR trial merges | — | done | wip/pr-merges edbf182e (local f3be22be) | done; #677 and #212 conflicts open | docs/lanes/pr-merges.md |
| smc-port, break-feat, mixer-ux, web-site, ui-theme | moved to chats | — | stopped 07:36 PDT | see above | stopped; WIP saved | <lane>/STATUS.md |
| feature-audit | 122-item map | — | — | — | done | feature-audit/FEATURE-MAP.md |
