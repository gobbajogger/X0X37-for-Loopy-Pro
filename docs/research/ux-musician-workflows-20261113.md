# Musician workflow evidence for the DnBOX UX plan

Research date: 2026-11-13. Status: early research draft, not an approved design or firmware qualification.

## Scope and ownership

This document preserves external research for the existing U1 planning owner in [PR #891](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/891). It adds no implementation, mockups, runtime changes or new firmware writer. The research-only branch starts at nightly commit 66073c68dd80d51a7053432c2c161a8b720a1341. That is a documentation base, not a claim about the latest DnBOX runtime.

Plan: preserve sources and limitations; extract musician needs and tradeoffs; give U1 reproducible audit questions; retain rejected ideas and unknowns. Stop this bounded task when the report is pushed, read back and linked to #891. The existing U1 owner then reconciles it with current firmware behavior and the concrete overhaul plan.

The latest direction is plan-first. A researched plan and proposed mockups are not permission to implement a redesign. This report makes no finding that a DnBOX feature is missing: the current feature registry, source and actual interactions must be checked before that judgment.

## Evidence and decision language

- Primary documentation describes intended behavior; it does not prove hardware performance.
- Reviews are reasoned experience, not controlled comparative measurements.
- Forum/Reddit posts are attributed qualitative reports. Versions, context and personal familiarity matter; votes do not establish consensus.
- Search-only or inaccessible material stays a lead, not verified audiovisual evidence.
- ACCEPT FOR AUDIT means retain the question/principle in planning. It does not mean an owner-approved implementation.
- REJECT means reject the stated inference or copying strategy, not dismiss the musician.
- UNKNOWN means not tested against current DnBOX or not independently verified.

No device testing, firmware builds, user interviews or audiovisual playback were performed for this research.

## Findings and dispositions

### R01. Safe recovery enables creative risk

[Resident Advisor, Digitakt review, 2017](https://ra.co/reviews/21197) describes a focused instrument whose quick pattern experimentation, variation saving and restoration support improvisation. It also acknowledges the learning burden.

Why: a dependable way back makes radical musical changes less risky. A constrained, coherent tool can be more creative than a larger feature set.

ACCEPT FOR AUDIT: play a pattern, make extreme edits, restore, preserve a good variation. Record exactly which objects are restored.
REJECT: copying Elektron's whole architecture or treating a 2017 review as a claim about every current Elektron model.
UNKNOWN: DnBOX recovery scope, memory cost and present interaction quality.

### R02. Save, switch and undo need one understandable mental model

[Analog Four owners: saving-project confusion](https://www.elektronauts.com/t/saving-projects-confusion/18233) describe confusion between projects, patterns and kits, including lost sound edits. This historical discussion is not a claim of a current Elektron bug.

[Deluge community controls manual](https://delugecommunity.com/manual/device_overview/controls/) explicitly limits undo: certain editing operations are covered, but entering the sound editor or changing a preset clears history. Subsequent saves propose incremented variants; overwrites require confirmation. Loading during playback is described as switching at the loop boundary.

[FM1 workflow complaint](https://www.reddit.com/r/MVaveFM1/comments/1xd7syq/which_groovebox_firmware_is_actually_usable_for/) reports difficulty with pattern/song/project hierarchy and autosave. Exact firmware behavior was not reproduced.

Why: users mean “save my music,” while software may persist differently scoped objects. Silent scope differences destroy confidence.

ACCEPT FOR AUDIT: edit A, switch B, return A, undo, save, restart; ask users to predict each outcome. Test blank-pattern creation and duplication without damaging the original.
REJECT: adding hierarchy merely because competitors have it; promising unlimited undo without proving storage cost and boundaries.
UNKNOWN: which DnBOX objects persist together and which operations invalidate recovery.

### R03. Consistent control grammar reduces relearning

[Baud Girl SMK quick start](https://baudgirl.com/work/SMK-37%2BVA/quick-start) documents consistent navigation/edit roles, explicit pad modes, configurable knob banks and fader pickup. It distinguishes pattern preset storage from retaining the current preset.

[Makkuro37 README](https://github.com/genkigenki/Makkuro37) describes an eight-column/eight-knob correspondence reused across menus.

Why: stable spatial and gesture relationships transfer learned actions between pages.

ACCEPT FOR AUDIT: inventory selection, edit, back, fine adjustment, long press, bank switching and target feedback across all DnBOX pages.
REJECT: copying graphics or repurposed printed button labels without addressing discoverability.
UNKNOWN: current inconsistencies and which can be simplified within the actual controls/display budget.

### R04. Arrangement quality must be judged across musical transitions

[Circuit Tracks versus Electribe owners](https://www.reddit.com/r/synthesizers/comments/v9av1r/electribe_2_vs_novation_circuit_tracks/) disagree about preferred workflows. One wants familiar multi-bar chunks and a screen; another praises Circuit's speed. Several report Electribe transition tail cuts. These are version-specific user reports, not independent measurements.

Why: loop improvisation and structured composition impose different demands. A successful state switch can still fail musically if it cuts sustained sound.

ACCEPT FOR AUDIT: create eight bars, make a variation, queue/cancel a transition, preserve notes/tails, return to the original and arrange a short piece. Separate novice learnability from expert familiarity.
REJECT: declaring one universal best workflow based on anecdotes.
UNKNOWN: actual DnBOX tail, held-note, clock and queued-transition behavior.

### R05. Good defaults remove work before creativity begins

[Digitakt workflow discussion](https://www.elektronauts.com/t/tips-for-speeding-up-workflow/241264) recommends prepared kits, stable track assignments and purposeful small sound selections to reduce repeated setup and choice paralysis.

Why: setup decisions consume attention before the musical idea becomes audible.

ACCEPT FOR AUDIT: fresh start to a useful beat and bass line without a tutorial; record actions, time, wrong turns and prerequisite preparation. Evaluate existing defaults before adding functions.
REJECT: forcing one genre or immutable track layout.
UNKNOWN: whether current DnBOX starting states serve both beginners and experienced players.

### R06. Pads and navigation are part of performance

[SMK pad/sequencer discussion](https://www.reddit.com/r/MvaveSmk/comments/1xtrj18/the_pads_sequencer/) values direct sequence editing or distinctive performance pad roles over duplicating the keyboard.

[FM1 live-set report](https://www.reddit.com/r/MVaveFM1/comments/1xv7ohk/one_week_into_exploring_firmwares_need_advice/) describes hurried mode changes between selecting patterns and playing instruments, plus reported lag/underruns in another firmware. These claims were not reproduced.

Why: navigation competes with the player's timing and working memory.

ACCEPT FOR AUDIT: edit steps, mute, change patterns and play notes without stopping; check active target visibility, entrances and held-note continuity.
REJECT: treating a small sample as consensus or the other firmware's reported fault as a DnBOX fault.
UNKNOWN: DnBOX modal conflicts and whether existing shortcuts already solve them.

### R07. Responsiveness and smooth sound are UX requirements

[Baud Girl manual/release notes](https://baudgirl.com/work/SMK-37%2BVA/manual/102) describe prioritizing key events over drawing, faster scanning and fewer display redraws. Its timing numbers are author-reported and were not independently measured.

[Saadet discussion](https://www.reddit.com/r/MvaveSmk/comments/1xqgi9q/saadet_10_for_the_mvave_fm1_ppg_wave_22_hydra/) contains a report of audible stepping during slow filter sweeps. The suggested internal cause is not established by that report.

Why: a musical instrument fails at the interaction level when input timing or continuous control feels unreliable, even if the screen is attractive.

ACCEPT FOR AUDIT: reproduce reported Chicago second-parameter-page lag and dropouts with demanding sounds, simultaneous parameter sweeps and page changes; distinguish audio deadline misses, input delay, redraw latency and discontinuities.
REJECT: using average CPU or emulator success as hardware qualification.
UNKNOWN: current root causes, actual worst configurations and hardware margins.

### R08. Existing functionality can feel broken when its target is hidden

[OG Synth release discussion](https://www.reddit.com/r/MVaveFM1/comments/1xpjsz1/og_synth_122_is_out_thank_you_and_heres_what_you/) includes a user unable to identify/select the drum target until the author explains the gesture.

Why: having a feature is not the same as making it discoverable during music making.

ACCEPT FOR AUDIT: ask a new user to find and change one drum/voice without a manual. Record target visibility and accidental edits.
REJECT: solving every discovery problem by adding text to a manual.
UNKNOWN: equivalent ambiguity in DnBOX.

### R09. Recovery and data scope belong to the experience

Independent FM1 posts describe missing/broken presets after restoring stock firmware:
[report A](https://www.reddit.com/r/MVaveFM1/comments/1xqkuiw/back_to_original_firmwarepresets_missing/),
[report B](https://www.reddit.com/r/MVaveFM1/comments/1xus1ny/what_have_i_done_wrong/),
[report C](https://www.reddit.com/r/MVaveFM1/comments/1xosufd/fm1_semi_bricked/).

Why: the user judges success by playable sound and preserved work, not by a flashing progress bar.

ACCEPT FOR AUDIT: explain firmware versus preset/project/complete-backup scope and supported host paths; define recovery completion as boot, sound and promised data preservation.
REJECT: adopting community force-flash/recovery remedies without verified safety and separate authority.
UNKNOWN: whether any comparable DnBOX issue exists.

### R10. Honest platform and simulator boundaries preserve trust

[Felucca SMK README](https://github.com/ChanceTheMaker/felucca-smk) describes one-level undo, separate browser/device storage scopes and remapped controls. It explicitly says its simulator is not cycle-accurate hardware emulation and distinguishes device verification levels.

Why: a believable promise needs a precise boundary. Similar-looking software surfaces may store different data or support different capabilities.

ACCEPT FOR AUDIT: document each scenario's actual model, transport, storage and verification level; show unavailable capabilities clearly.
REJECT: equating build success, simulator output, browser UI state and physical-device qualification.
UNKNOWN: current DnBOX evidence closure for each offered configuration.

## YouTube and discovery register

The following direct video URLs were recovered from linked pages, but YouTube fetching was throttled. No playback, transcript or audiovisual behavior is claimed:

- [NatLife Sounds, FM1+VA tutorial](https://www.youtube.com/watch?v=jfuoEBIUsEE). Indexed chapters identify macros, arp and sequencer topics.
- [Darren Lock, SLOOP demonstration](https://www.youtube.com/watch?v=yedcHP3jy5s).
- [Charles Vestal, X0X demonstration](https://www.youtube.com/watch?v=cZLYrZaLDUk).
- [Digitakt arrangement video linked by a forum participant](https://www.youtube.com/watch?v=Kh3Mevf1c_k). Title/content not independently verified; do not infer a recommendation.

Discovery/provenance: [Studio Return article](https://www.studioreturn.nl/journal/m-wave-fm1), [Matrixsynth 2026-09-29 archive](https://www.matrixsynth.com/2026_09_29_archive.html), and the R05 forum.
Other leads, not accepted evidence: Stanley Gurvich's BaudGirl overview; ElektroTV's installation demonstration; DECIMA1's new-firmware demo; an extended “if...?” performance. Search titles do not prove the content.

[Sound On Sound Digitakt II review](https://www.soundonsound.com/reviews/elektron-digitakt-ii) appeared in search but opening returned HTTP 403; not used as primary evidence.
[Maru directory](https://maru.audio/community/firmware/fm-1/baud-girl-fm1-va) redirected and differed from indexed content; use author docs instead.
A guessed Matrixsynth article path failed; it is not treated as a verified citation.

## Next research and acceptance checkpoint

U1 should map each finding onto current exact DnBOX source, registry and observable interactions. For each journey retain:
current behavior; right/wrong/missing/unclear; musician impact; source or reproduction; accepted/rejected/unknown decision and reason; hardware cost; acceptance test.

Prioritize sound/data-loss failures, then performance-breaking ambiguity, then repeated creative friction, then visual refinement. Preserve successful existing behavior. Do not turn this priority suggestion into an implementation approval.

Required follow-up evidence: watch accessible demonstrations; compare official manuals for exact versions; conduct task-based observation rather than preference voting; establish current DnBOX baseline; measure hardware timing through the authorized owner. This draft does not claim a complete competitive survey or exhaustive configuration coverage.

## Checks for this documentation stage

- Content/source-boundary review: PASS; user reports and author claims are labelled.
- Public-content review: PASS; no private logs, credentials, binaries or proprietary source.
- Main source URLs: retrieved through web search/open during research; access failures and search-only leads labelled above.
- YouTube audiovisual review: NOT_RUN, fetch throttled.
- DnBOX behavioral validation, hardware testing and builds: NOT_RUN, outside this research stage.
- Independent review and design approval: PENDING.

## Shared website planning use

The existing website owner in [PR #890](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/890) should use the same evidence IDs, not maintain a contradictory capability story. Website research can feed new questions into firmware planning; a page claim must follow verified firmware behavior.

- R01/R05: demonstrate a short real musical journey and recovery, rather than claim effortless creativity without proof.
- R02/R09: make saved-data scope, prerequisites and recovery boundaries understandable before installation.
- R03/R06/R08: show controls and the playable target in context; a static beauty shot cannot prove discoverability.
- R04/R10: distinguish standalone, phone/tablet, computer and external-gear scenarios by verified model, connection, routing and storage capabilities. Do not imply wireless support from a proposed showcase.
- R07: use measured, qualified reliability claims, not emulator success or competitor figures.

These are planning implications, not approved copy or a new implementation assignment. Both owners should preserve accepted/rejected/unknown decisions and reasons against these IDs. Outstanding video review and current-firmware audit remain explicit gaps.
