# DnBOX feature coverage: candidate bases (read-only, 2026-10-29 PT)

**Limit:** `DnBOX-LoopyX0X37-full-feature-list-20261029.docx` isn't on the Mac (~/Downloads, ~/Documents, lanes) or on the box. The item names below come from the repo split docs: `docs/lanes/orchestrator.md` (wip/orchestrator, 22 items), `docs/remaining-features-20261030.md` (chat/remaining-features, 25 items) and `ui-theme-handoff-20261030/README.md` (box ~/Downloads, 8 UI items). That makes 55 of 122 named items, plus 6 skipped (29, 55, 116, 117, 120, 122) and 1 deferred (119). The other ~60 names are unknown, so they can't be checked.
0e58b960, e66aaa33 and 413364e1 **don't exist** in the Mac repo (after fetch --all) or in any box download.

## Candidates, side by side (counts = git grep hits in firmware/src)
| Candidate | Date (PT) | Chicago/Miami port 9d1cf9fb | Miami tempo f1097d1f | 8 RATE div #643 | glyph #647 / f289fbbd | theme (ui_theme/THEME_) | DnBOX boot/brand | glyph hits | ui_original ctrl | rev13 wip base |
|---|---|---|---|---|---|---|---|---|---|---|
| **a0279524** wip/engines-ui (= codex/ui-theme 4dfac82a snapshot) | Oct 29 7:46 AM | content yes (#684 + e888d046 via 4dfac82a) | Y | content (RATE table ui.c:1159) | partial (shared glyph test) | **25** | **8** | 44 | 33 | from nightly-20261029 |
| ca8299b6 fix/original-chicago-miami-idle-glyphs | Oct 29 2:02 AM | own port | - | yes (ui.c:6243 rate table) | "restored saved animated UI" | 5 | 0 | **74** (103 repo-wide) | **59** | no (nightly + 696 files) |
| ad9c9ddc chat/ui-theme | Oct 29 9:24 AM | **no** | - | - | **f289fbbd Y** | 2 | 0 | 34 | 0 | no (nightly 66073c68) |
| wip/orchestrator 4f039829 | Oct 29 9:31 AM | yes (fmb-guards-slots rev13) | Y | - | - | 5 | 0 | 43 | 32 | **yes** |
| wip/smc-port 7840c039 | Oct 29 9:50 AM | yes | Y | - | - | 5 | 0 | 43 | 32 | yes |
| wip/break-feat 953aa683 | Oct 29 9:17 AM | yes | Y | - | - | 5 | 0 | 43 | 32 | yes |
| chat/remaining-features a5d89b45 | Oct 29 9:23 AM | no | - | - | - | 2 | 0 | 32 | 0 | no (nightly) |
| lane/shared-ui-material-glyph e6eb8267 | Oct 27 2:03 PM | no | Y | Y | **Y (#647)** | 5 | 0 | 44 | 0 | no |
| c287cfe4 codex/perf-test | Oct 29 6:39 AM | Y | - | - | - | 2 | 0 | 11 | 32 | pr684 |
| box zip UI-26ee-reviewed-source (26ee2ace) | Oct 27 | no | ? | ? | glyph semantics reviews r4/r5 | - | - | 55 | - | patch on a51b030d (not in repo) |
| box zip ca8299b6 BUILD_ONLY | Oct 29 | = ca8299b6 + .fwsc | | | | | | | | |
| box 87f12e8 .fwsc | Oct 28 10:08 PM | unknown; binary only, no source or commit found | | | | | | | | |

## Item table
| # | Name | a0279524 | Best elsewhere | Evidence |
|---|---|---|---|---|
| 3 | Correct Chicago/Miami labels, ranges, steps | done | ca8299b6 (ui_original.h) | a0279524 ui.c:1032-1034, 1158-1159; 4dfac82a "finish original-engine matrix" |
| 4 | Pitch bend and damper | done | ca8299b6 | ca8299b6 msg "Keep pitch bend on performance inputs"; ui_original.h DAMPER |
| 6 | Browser of 55 Chicago / 30 Miami presets | partial | ca8299b6 BPRESET_P 80 entries | a0279524 ui.c:462 BPRESET_P 54; ca8299b6 ui.c:536 PRESET 80 |
| 7 | Pattern-bound sound and engine | partial | wip/orchestrator (gap noted) | orchestrator.md: "#7/#8 gaps need design" |
| 8 | Whole-sound locks per note | missing | none | same |
| 10 | Real preset names, click-free switching | partial | - | factory_banks.c names; no click-free proof |
| 11, 12, 14, 15, 17, 18 | Long notes, held duration, RANDOM, cutoff locks, recording lifecycle, clocks | done (inherited nightly) | wip/orchestrator confirms | orchestrator.md "existing, needs acceptance" |
| 13 | Drum-step probability | done | wip/orchestrator | orchestrator.md "#13 found in source"; bass_probability_midi_test.c |
| 21 | Miami tempo divisions and pad selection | partial | lane/sb-miami-stock-eight (11383f21), ca8299b6 | rate[] 8 divisions ui.c:1159; f1097d1f in a0279524. **Pad wobble rate/shape: missing everywhere** |
| 22 | Legato becomes slide | partial | wip/orchestrator | source exists, proof NOT_RUN |
| 23 | 909/808 identity | done (source) | chat/remaining-features test 4710774f | drum_kit_identity_test |
| 30, 38 | SMC eight strips; second Select opens drum mixer | done | all | firmware/src/app/smc_mixer.c, smc_select_mode_test.c |
| 43 | Silent linked SMC context | done | wip/orchestrator | orchestrator.md |
| 47-50 | Save, Load, Clear/Reset, export/import | done (inherited) | - | orchestrator.md |
| 52 | Multi-client atomic ops | partial | - | orchestrator.md |
| 54 | rev13-only policy | done on rev13 bases | wip/* (fmb-guards-slots) | orchestrator.md; a0279524 is nightly-20261029, rev13 to verify |
| 56 | Agreed HOME/MIX/SOUND/BREAKS/FX/settings layouts | partial | a0279524 + f289fbbd + UI-26ee | 4dfac82a ui.c +848; f289fbbd glyph strips; drum MIX material ad9c9ddc |
| 57-62, 64-66, 69 | Pattern overview, readable UI, envelopes, knob map, navigation, grouped settings, animation | partial | chat/remaining-features (queue only), UI-26ee (settings sections, action headers) | remaining-features doc: "not a claim of completion" |
| 63 | Navigation wrap | partial | wip/orchestrator 4f039829 | ui_page_wrap_test.py |
| 68 | Active row highlight, semantic cards | partial | a0279524 / ad9c9ddc | shared cards (#647 content) |
| 70 | Themes including high-contrast Light | **done** | only a0279524 | firmware/src/ui_theme.h, tests/host/ui_theme_test.c |
| 71 | DnBOX spelling, logo, glyphs | **done** | only a0279524 | 8 DnBOX hits; boot |
| 72 | Remove boot video, static DnBOX boot | **done** | only a0279524 | 4dfac82a "static DnBOX boot" |
| 73, 74 | Recovery loader; dead-pad diagnosis | partial | all (INLOG arg 6) | main_fm1.c, tools/inlog_fmt.py |
| 76, 83, 86, 88 | Site song transfer, controller, screenshots, variants | partial | wip/web-site 9fe416a3, chat/web-site f4b0d6a9 | web-site lanes |
| 110, 114, 115, 118 | Usability acceptance, readiness states, exact-input proof, main/nightly split | process | remaining-features tools/remaining_feature_proof.py | |
| 112, 113 | Elite/Pro packages, installable build | missing for a0279524 | ca8299b6 BUILD_ONLY .fwsc | ~/Downloads LoopyX0X37-Chicago-Miami-UI-ca8299b6-BUILD_ONLY |
| 119 | Community library | deferred | - | |
| 121 | M-VAVE source evaluation | done (doc) | chat/remaining-features | |
| user | Engine/preset selector | done | ca8299b6, all #684 | a0279524 ui.c:461-462 BENGINE_P/BPRESET_P |
| user | Freeplay pad wobble rate selector | **missing everywhere** | - | only R_FREEPLAY toggle (ui.c FREEPLAY_P); no pad wob code in git, lanes, stash, Codex logs or zips |
| user | Wobble shapes on pads | **missing everywhere** | knob-page SHAPE only | ui.c W_WOBBLE_SHAPE "WOB SHAPE" |
| ~60 | Unnamed items | unknown | | docx not found |

## Ranking
1. **a0279524** (wip/engines-ui): has Chicago/Miami, selector, original controls, Miami tempo, and the only themes, DnBOX boot and static boot. It's the newest UI lane.
2. **ca8299b6**: the most glyph and animated-UI work, 80 presets, a built .fwsc. But it's a divergent tree (696 files vs 9d1cf9fb) and lacks themes.
3. **wip/orchestrator / smc-port / break-feat** (rev13 fmb-guards-slots base): newest feature work (#13, #63, SMC, BREAK), but no theme or DnBOX UI.
4. **ad9c9ddc** chat/ui-theme: newest glyph port (f289fbbd), but on old nightly with no Chicago/Miami.
