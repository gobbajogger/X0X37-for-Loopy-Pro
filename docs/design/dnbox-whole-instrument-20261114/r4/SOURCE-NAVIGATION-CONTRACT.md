# R4 source navigation contract

Read-only diagnosis of firmware `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`, not execution or acceptance. Source snapshots remain byte-matched to the prior connector blobs. No renderer/runtime/Settings/CP1 edit, emulator, build or test was performed. Effective model/effort is not exposed. There is no change_part() function in this pinned ui.c: normal K3 selection is inline at4575–4608 and uses part_neighbor()108–111.

## Part identity and K3 direction

| Canonical global part | Numeric ID | Display/K3 position | Synth voice | Normal admitted PART/SEQ header |
|---|---:|---:|---|---|
| PART_909 |0|0|—|909|
| PART_808 |1|1|—|808|
| PART_BRK |4|2|—|BREAK|
| PART_303B |3|3|1 / B|Actual engine + B|
| PART_303A |2|4|0 / A|Actual engine + A|

engine.h30 defines canonical IDs [909,808,A,B,BREAK]. ui.c95–111 defines presentation order [0,1,4,3,2], explicitly leaving saved arrays/lanes/physical strip identity unchanged. bidx()346 maps global3→voice1/B and global2→voice0/A; part_display()/part_name()128/142 use part−PART_303A. Do not use presentation position2 as voiceA or presentation position4 as globalBREAK.

For nonzero e, K3 moves **one neighbor by sign(e)**, not abs(e) neighbors. Movement clamps at909 and A; it never wraps: negative sequence A→B→BREAK→808→909→909, positive sequence909→808→BREAK→B→A→A. Actual input consumes e directly only in the held-step velocity/value branches.

K3 dispatch precedence (ui.c4544–4608):

1. O_ASK or active performance test drains/ignores encoders before the K3 branch.
2. O_LIST: turn selected row via turn_ref(...,e,EN_ALGO); no part switch, even if a step is held. A read-only/action row follows its existing turn_ref policy. Settings and SOUND-value details remain incumbent-owned.
3. Only V_SEQ + synth slot (global2/3) + ui.held_step≥0: edit current step velocity by e, clamped1..127; set step_edited/dirty and refresh an existing stopped preview. This is not a K3 part switch. REC is not an additional condition in this branch.
4. Otherwise choose clamped neighboring canonical part. If it changes, run bass_step_cancel() and drum_step_cancel(); assign ui.part; keep the current view except V_GEN→non-synth becomes V_PART; invalidate BREAK outline and rebuild pages. Do not infer an all-note panic or an additional universal held-input cancellation beyond these source hooks.
5. If resulting view is MIX, always set mixsel=ui.part and choose909 MIX page1 for909,808 MIX page2 for808, otherwise PARTS page0. This happens even when the part was clamped and did not change:909/MIX MASTER + K3− stays909 but opens909 MIX.

A held melodic step on V_PART does **not** select the velocity branch. K3 there selects parts and runs the source step-cancel hooks when the part changes. A held drum step on V_SEQ also does not select velocity. There is no source blanket 'any held step means velocity'.

## Page memory

ui.page[NVIEWS][NPARTS] is UI session state (ui.c179–185). page_of(view)1427–1430 returns:

- V_PART, V_SEQ, V_GEN: page[view][canonical global part]. These are separate memories for each view and each part.
- HOME, FX, MIX, GLOBAL, SONG, PERF: page[view][0], one shared memory per view.

PART memory is per synth **slot/part**, not per engine kind. Chicago8, Miami6, Juno5 full pages include SOUND. Engine changes do not allocate a second remembered Chicago/Juno page or move the slot. Preserve the source descriptor rebuild rules instead of inventing per-engine memory. Normal build_pages()1586–1603 resets an out-of-range stored page to0. An already admitted SOUND row follows the new final SOUND index; asynchronous sound-layout preservation has explicit flags at1589–1629. Those remote/pending/unknown transaction details remain a reserved CP1 interface; this contract does not reimplement them or choose a replacement policy.

set_view()2754–2780 does not reset the destination page. Enter PART/SEQ/GEN restores that part/view memory; enter FX/MIX restores the common view memory, subject to descriptor normalization. Only an already-current view's repeated physical page button advances modulo its page count (PARA4066–4069, FX4081–4085, NOTE REPEAT4087–4091, SEQ4101–4105). Returning from another view must not unconditionally set page0.

Examples are source-derived expectations, not executed tests:

- PART A remembered3; PART B remembered6: K3− from A selects B/page6; K3+ restores A/page3. K3 does not zero either memory.
- PART A page3; SEQ A page2: switching SEQ↔PARA restores2↔3, not one shared 'soundPage'.
- FX page1; leave for PART; FX returns page1. A second unused FX tap cycles to0.
- MIX page4; leave for PART; NOTE REPEAT returns page4. Turning K3 on MIX then selects its source part-follow page described above.

set_view() preserves a held drum/melodic SEQ step only on SEQ→PART, enabling p-lock editing. PART→SEQ with ui.held_step or ui.hold_step rejects with RELEASE STEP FIRST and keeps the view. Other view changes run bass/drum step cancellation; set_view clears brk_held and overlay, updates prev_view only when view changes, updates recorder masks, and rebuilds descriptors. This is separate from normal K3's inline branch.

Step-page spage (1–16 through49–64), Home pattern-bank pbank, selected drum track sel[kit], and mixer mixsel are separate state, not page_of(). In particular MIX PARTS black keys change mixsel, not ui.part (ui.c3764–3768). ui_init()8594–8608 zeros UI/page state and starts909 SEQ; navigation does not repeat that initialization.

## Header A/B rule

For normal known engines, PART's dedicated header forces ENGINE A/B from bidx() at ui.c5380–5388; its cache uses the same rule5332–5335. The general PART/SEQ header likewise forces suffixes5423–5434. Thus a Juno on global3 must say JUNO B, even when the other slot is Miami. Global2 is JUNO A. Do not hardcode A for every synth fixture, derive suffix from display position, or rename the stored slot when engines change.

General part_display()/part_name()115–149 and compact part_s()151–166 differ: when only one slot uses an engine they may show JUNO/CHICAGO/MIAMI without A/B; when both use it they add the suffix. HOME/mixer lane labels can therefore be shorter than the PART/SEQ header. Default tagged sound initialization assigns voice0/A=Miami and voice1/B=Chicago (engine.c782–786); current project assignments can differ.

Known-active identity only is described here. Tagged unknown/retained-unavailable SOUND handling stays with its producer. Drone is withdrawn from new active-capability design; source3c41's old Drone branches are historical source facts, not authority to add active Drone pages or renumber stable IDs.

Full machine contract, source pins and explicit state examples are in SOURCE-NAVIGATION-CONTRACT.json. These source expectations should guide the existing reducer; they are not a new runtime implementation or a fresh independent review.
