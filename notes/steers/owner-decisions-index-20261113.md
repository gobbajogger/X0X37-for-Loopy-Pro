# Owner decisions index (2026-11-13)

Bots must read this table and `docs/ROADMAP.md` (branch `docs/roadmap-owner-sync-20261113`, draft PR once open) **before asking the owner anything**. If the answer is here, do not ask again. Only the owner's own words count (messages from machine A694EA5C, widget answers, or quotes Dot relayed from his authenticated ChatGPT, labelled as relayed). A newer owner word wins.

Times are Asia/Vladivostok (UTC+10) unless marked otherwise.

| # | Decision | Owner's words (source) | Date | Status / where |
|---|----------|------------------------|------|----------------|
| R1 | ROADMAP is the sole list. Record every request at once, one short line, no evidence noise. Never re-ask agreed items. | "did you add all my requests to roadmap so you would have to ask me a ton of stupid questions that were already answered"; "Some info is unnecessary"; "can you find all the requests I've made so you can figure out on what's you supposed to work and what am I waiting for" | 2026-11-13 23:23–23:24 | ROADMAP R1, P3 |
| R2 | Removals need his own words, quoted with date and time. | Rule of 2026-11-12 03:17 | 2026-11-12 | ROADMAP R2 |
| R3 | Firmware must only get faster; zero dropouts; pool ≥ 8,192 B. | 2026-11-01 15:49; 2026-10-31 CPU caps | 2026-11-01 | ROADMAP R3 |
| R4 | He installs himself through the site. No agent flashes. Passing nightly publish needs no extra approval. | 2026-11-01 15:42 and later | 2026-11-01 | ROADMAP R4 |
| R5 | Miami is not a 303; never label it 303B. Chicago is the 303-style bass. | 2026-11-12 02:31 | 2026-11-12 | ROADMAP R5 |
| R6 | Non-main source/docs pushes and draft PRs are already in standing scope. Do not invent a new approval. Nothing merges without his OK. | Relayed from authenticated ChatGPT | 2026-11-13 | ROADMAP R6 |
| R7 | Classic ChatGPT threads in desktop browsers, not the Linux app. | "Instead of app, for reliability" | 2026-11-13 22:42 | ROADMAP R7 |
| R8 | Close idle bots' browsers automatically when memory is low; never close a working bot's browser. | Widget: "Yes, close idle bots' browser windows"; "It's should be automatic"; "So work can progress on its own" | 2026-11-13 | ROADMAP R8 |
| R9 | Commits authored as gobbajogger <255244738+gobbajogger@users.noreply.github.com>. | "all commits must be authored as gobbajogger" | 2026-10-30 | ROADMAP R9 |
| R10 | READMEs are professional product docs, not a log of failed builds. Evidence goes in release notes / nightly / evidence docs. | Relayed from authenticated ChatGPT | 2026-11-13 | ROADMAP R10, D2, #885, #886 |
| #50 | Portable project export/import is an existing item, already shipped. Do not add a duplicate. | "And project export was already in the roadmap" | 2026-11-13 23:26 | ROADMAP #50 DONE in 94197fc8, [projects page](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/) |
| 26.8 | Engine-preset **file** import/export (separate files, not #50, not #7). Already requested; do not ask again; build after Optimizer flash savings. User presets only. | Already requested and delegated (relayed); widget: "Yes, add preset file import/export to ROADMAP" | 2026-11-13 | ROADMAP 26.8, `lane/fmb-presets-format` `e199c7e8` |
| 26.1–26.5 | Drone as a slot engine (not a 6th part); held-key chords, not one-finger auto-chords; Juno + cheap chorus. | "Drone engine is just another synth engine don't you get it?"; "I asked for it" (Juno) | 2026-11-12 | IN SOURCE 94197fc8 / #869; flash overflow |
| 26.7 | No extra AMB-1 / other slot engines in 0.0.26. | "Nah let's stick to the previous plan than" | 2026-11-12 13:07 | DECLINED |
| 26.6 | 360 MHz fm1-emu is the CPU gate from 0.0.26 on. | Thumbs up 03:50; PASS rule 07:26 | 2026-11-12 | DONE as gate |
| I1 | 0.0.25.1 on the main installer after QA. | Widget: "Yes, merge #880 and put 0.0.25.1 on the main installer after QA passes" | 2026-11-13 22:51 | #880, Installer QA |
| RN1 | He tested 0.0.25.1 and 0.0.26 on an SMK-37 Elite. Scoped report, not a full qualification. Pro untested. Hash not confirmed. | "Bro I tested a 25,1 and 26 already"; "I used elite"; relayed: "i already recovered to 25.1", "25.1 is good", "and yep 25.1 to 26 is smooth" | 2026-11-13 | Recorded. Live release note still old until he says yes on the existing card. |
| D1/D2 | Manual + both READMEs as draft PRs, nothing merged. | Widget: "Yes, put the manual and both READMEs on GitHub as draft PRs" | 2026-11-13 | #884, #885, #886 |
| W2 | Website keyboard must match the real SMK-37. | "Keyboard layout is wrong" | 2026-11-13 | Website UX |
| W1 | Site: dark only, DnBOX headline, Baud Girl craft, keep iPhone path. | Vision / profile, 2026-11-13 | 2026-11-13 | Website UX, #874 |
| U1 | Firmware UX overhaul (glyphs, chords, piano roll, live states, real SMK opinions first). | Vision, relayed | 2026-11-13 | Firmware UX |
| U2 | Box entrance ends on the current UI, not old 303A/303B. | Vision, relayed | 2026-11-13 | DONE locally `21261737` |
| M6 | Build the drum fill per the #815 mockup in 0.0.25. Shared drum FX. | "Yes, build the drum fill per the #815 mockup in 0.0.25"; "Let drums have similar fxs" | 2026-11-12 02:29–02:33 | DONE |
| Chain | Pattern chain uses the longest playing pattern, not 1 bar. | "Pattern chaining is still only 1 bar instead of using the longest playing pattern" | 2026-11-12 12:33 | DONE in 79280e76 |
| Slot 3 | Restore the third imported-loop slot (cut had no reason in his words). | 2026-11-12 03:23 | 2026-11-12 | DONE |
| Accent | Tone Chicago accent down to about +6 dB. Velocity-to-cutoff skipped. One global Tune. | "Tone accent down to about +6 dB" | 2026-11-12 07:31 | DONE |
| Names | Push 0.0.25 to main once; nightly names like `DN Box 0.0.N`. | "Push 25 to main. And unify the version names so they are formatted same as dnbox 0.24 and not some abstract legacy dev" | 2026-11-12 12:57 | DONE for 0.0.25 |
| Peek | Home knobs peek; locked SMC page-changes peek and the lock stays; batch overlay on every screen. | 2026-10-30 UI decisions | 2026-10-30 | DONE, M9 |
| Autosave | Settings toggle; on = save only in silence; off = manual only. | 2026-11-01 | 2026-11-01 | DONE |
| M1/M8 | PANIC = keyboard FX+SEQ, SMC BANK L+R (DAW only). PLAY during a question = NO then play. HOME+PLAY redo. | Approved 2026-11-01 15:21 | 2026-11-01 | DONE |
| #7/#8 | Per-pattern preset recall; per-note sound lock. | 2026-11-12 02:42–03:12 | 2026-11-12 | DONE |
| M13 | BREAK 1/16, 1/8, 1/4 rates. | In 0.0.25 plan, built | 2026-11-12 | DONE |
| QUEUE MUTES | Settings toggle, default off. | 2026-10-31 | 2026-10-31 | DONE |
| Agreed UI | Agreed items need no new mockups. Ask only genuinely important things he may be missing. | "Да там же все согласовано было"; 2026-11-01 13:02 | 2026-11-01 | STANDING |
| Numbers | Use item numbers with a plain description; never ask him what a numbered item was (including M5). | 2026-11-01 | 2026-11-01 | STANDING; M5 stays QUEUED, undefined |
| USB name | Never identify or filter a device by USB/port name; hardware and firmware fields only. iPhone install path must keep working. | 2026-10-31 15:48 | 2026-10-31 | STANDING |
| Backup | A failed slot backup must not block install (model and BT checks stay). | "This shouldn't be an issue" | 2026-11-01 | STANDING |
| Slots | 0.0.25 3-slot map (slot 3 at 0xB3000, 32 KiB) is accepted by the nightly slot-map check. | Approved 2026-11-12 12:17 | 2026-11-12 | DONE on nightly |
| Vision | Firmware first: best in class, proud, recommendable, more than seemed possible on a low budget. Website and installer support that; they are not an equal goal. | Relayed: "I want our firmware to be best in class"; "It must be reliable tool i can recommend…" | 2026-11-13 | Aspiration; ROADMAP goal |
| Glyphs | Use existing glyph code, don't redraw; ship agreed glyphs. Miami ADSR graph on hold. | 2026-10-30 / 10-31 | 2026-10-31 | STANDING for UX |
| Process | Follow the plan and dnbox-work-gate; do what he asked, don't launch unrequested work (engine opts were requested). | 2026-11-01 13:14 | 2026-11-01 | STANDING |

Open decisions (do not re-ask unless work is blocked): spare flash vs loop slots; Bluetooth pairing (OQ-11); installer `ctl.img` only with `--ctl` (OQ-12); RN1 live release-note PATCH (existing card, not a new question).
