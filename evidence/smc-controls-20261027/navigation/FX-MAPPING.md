# SMC source mapping and installation-history audit

This is a source/host replay, not a device or installed-byte readback. The owner identifies the installed Pro source as `3e7fed606ec3b2858811c080a39a976c87d1b117`; current integration base is `504edff4ae49481b2d7c54e3c914d0b305e81358`. PR620 is already integrated there. This lane does not merge or flash.

## Actual ordinary FX-page mapping

These are the retained assignments in `smc_protocol.h`, `SMC_VIEW`/`SMC_PAGE` and `smc_fx_map_tables.c`. FX means the actual FX view without a list/confirmation or clip owner. A temporary SMC control card is not a menu overlay.

| Physical row / DAW notes | Strips 1–5 | Strip 6 | Strip 7 | Strip 8 |
| --- | --- | --- | --- | --- |
| Record `00..07` | Select 909/808/303A/303B/BREAK and toggle recording | List / activate row | Home / back; FX hold is assignment modifier, plain FX tap navigates on release | Pattern workspace; repeated presses cycle HOME, SEQ pages, SONG pages |
| Solo `08..0F` | Solo corresponding source while held; release clears original target | MIX | Global / back | Sound workspace; repeated presses cycle PART pages, then 303 generator pages when applicable |
| Mute `10..17` | Toggle corresponding part mute | Reverb return zero/restore | Delay return zero/restore | Master FX output zero/restore |
| Select `18..1F` | Hold assigned source effect | Short tap: reverb page; hold 180 ms: reverb-bus effect | Short tap: delay page; hold 180 ms: delay-bus effect | Short tap: MASTER MIX; subsequent mode-cycle presses advance/wrap MIX pages; FX hold 180 ms: master-bus effect |

FX ROOM defaults across Select1..8 are reverb throw, delay throw, high-pass, low-pass, decay gate, reverb throw, delay throw, sixteenth gate. Home+SELECT cycles only the existing per-bus suitable-effect list. Home+Left/Right chooses the existing ROOM/TIGHT/ECHO assignment page; no effect assignment was invented. Up/Down now changes only knob mappings, including while Home is held. The old Home+Up/Down cutoff-save/reset bindings are superseded by this explicit knob-bank-only requirement; stored cutoff preset data is preserved.

Source SELECT LEDs1–5 follow held source FX inside FX and ordinary selection outside it. The three main SELECT indicators retain their existing destination-page indication. Record/Solo/Mute LEDs follow their actual actions. Captions come from the same key/FX descriptors; the grid and touched-control value use the actual parameter getter. The existing approved grid uses lane-title selection instead of a separate SELECT row; an FX press shows its bus/effect in the lower readout. Navigation readouts identify SELECT, MENU, PAGE or the pending PATTERN instead of incorrectly naming transport/mute/record actions.

Stock User buttons are six-byte MMC, not button CCs. Commands 3/7/11/15/19 are Record1–5;23/27/31 are Record6–8. Solo uses2/6/10/14/18/22/26/30; Mute1/5/9/13/17/21/25/29; Select4/8/12/16/20/24/28/32. Direct DAW notes use channel1. Hosted channel messages require explicit User mode and channel16. CC16–23 are relative knobs; User CC40–47 are faders. No saved button-CC assignment was found or added.

MMC has no releases. Existing source FX use latch fallback. The three main MMC SELECT commands now execute their established short-tap navigation once per command instead of waiting forever for an unavailable release. Note-based main buttons retain tap/hold ownership. The held Home assignment modifier requires a Note-based or panel Home input; stock MMC has no held modifier event. Help keeps its existing R7 dismissal priority on FX.

## Proven FX modifier regression

A production replay of Home press followed by Select1 gives:

| SMC source | View after Home press | View after Select1 | 909 assignment |
| --- | --- | --- | --- |
| `3e7fed60` | HOME | HOME | DLY, assignment succeeds through the old global FX predicate |
| `504edff4` | HOME | PART | REV unchanged; SELECT falls through to ordinary source selection |
| This candidate | FX | FX | DLY; assignment succeeds while remaining exclusive to FX |

PR620's required actual-FX guard exposed an older immediate Home navigation. This lane defers only the FX Home tap. A consumed modifier release cannot trigger late Home navigation; a plain tap navigates once on release. All 32 physical DAW row actions are replayed through real dispatch and release, with state changes plus captions and emitted LED bytes. Overlay/clip ownership remains guarded. A captured drum lock now permits the actual visible FX owner while preserving captured fader destinations.

## Page/knob/lock contract and remaining physical ambiguity

The three strip 8 buttons retain drum Record/Solo/Select on direct drum MIX/SEQ entry, including inactive second-bank positions. To preserve that existing assignment, a shortcut that entered a mode owns repeated page cycling through that workspace, including drum MIX pages. Keyboard navigation or context change ends that shortcut context. There is no invented button chord.

Consequently a direct drum entry still needs a decision if strip8 must ALWAYS act as global mode keys: that would replace existing drum8 Record/Solo/Select functions. The reachability fixture labels its keyboard HOME setup boundary and does not claim an SMC-only exit from direct drum entry. This unresolved physical-role choice must not be presented as a complete global fader 8-navigation fix.

Up/Down has a separate bank cursor and never also changes the visible view/page. Main knobs cycle cutoff/pan/reverb/delay dimensions; drum knobs cycle pan/reverb/delay; parameter views use their registered parameter-page banks. Menus retain list-row ownership. A clip editor retains its own single exposed bank and Up/Down no longer cycles clip stages. Intentional bank changes while locked rebind the knobs only; captured fader targets stay pinned. Ordinary keyboard association changes leave both captured target arrays stable.

Physical button holds own the temporary card independently of its presentation. Last release starts the1200ms grace; real knob/fader movement renews it, including pickup-pending movement. Duplicate samples, zero relative packets, background target notices and MIDI clock do not renew/open it. Link, guard, project and dropped-release cleanup end stale ownership. MMC commands have normal timed grace because no release exists.

## Earlier installed version evidence

No retained record establishes an exact known-working Pro page-lock/button/overlay package. The owner's report is preserved; missing attribution does not invalidate it.

- `evidence/f14-stage6/tip-install/install-record.json`: Elite source `b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a`, package SHA256 `4dc49bad6e8b555aefaf3a70f0ccb1579638d2b0e35c30a5f5fee043124f7376`, controller SHA256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`. This proves installation, not the requested Pro behavior.
- `docs/scheduler-handoffs/smc-fader-pickup-20261025.md` records owner-installed source `e75fb2a584aa22978f352697a5e36fc9f886c3c5`, with pickup symptoms; no complete known-working Pro artifact tuple. Its SMC blob is `5748a91151e2e3ddfe25830d34bf84e8c8f6be23`.
- `docs/scheduler-handoffs/smc-spatial-lock-select-20261025/README.md` identifies flashed source `58e2c5205a02836df678083eaa8db9994b6192e8`, before physical lock implementation.
- The fixed 1200 ms/context-cancel overlay functions were unchanged from `58e2` through `e75`, `3e7` and `504`; the implementation originated in `0e00c262348a1574c37f27e37a60531faef44cb4`.
- Blanket locked navigation/SELECT parking was unchanged since `7138b999b8b0ab72fe9b554362d3ba1f9977330b`, including `e75`, `3e7` and `504`. Immediate Pause-toggle behavior was likewise retained. Those missing semantics are not newly introduced by PR620.

The Home+FX modifier regression above is distinct and has a direct old/new production replay. The absence of an exact earlier Pro working pin prevents a broader hardware-root-cause claim.
