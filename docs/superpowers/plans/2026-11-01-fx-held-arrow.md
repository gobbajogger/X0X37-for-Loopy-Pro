# Held FX arrow mapping implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation; independent design and Compound Engineering code review remain separate.

**Goal:** Hold one FX button and press `<` or `>` on the SMC or keyboard to change only that button's mapping; show its effect glyph while held.

**Architecture:** Extend the existing suited-effect cycle and strip ownership. Intercept arrows before their existing page/SELECT routes. Keep the current momentary engine publication and framebuffer renderer.

**Tech stack:** C unity firmware, production UI/engine host fixture, existing registered `smc-remainder`, `smc-select-mode`, `smc-fx-preset-preserve` selectors.

**Spec:** Owner's row 41 instruction in this thread; supersedes HOME+FX-button remapping and the main SELECT activation delay. Quick main SELECT taps retain their existing navigation after playing the momentary effect; an arrow chord suppresses that navigation.

## Global constraints

- Same draft PR #800 and branch, based on af1a3a5e6ab9d4442977ed283072ded3a034ddee; author gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- No builds, test execution, framebuffer driver execution or compilers in Codex. Write tests first; RED/GREEN, actual 240x240 before/after PNGs, SDK V1.2.1 Elite/Pro X0X_DEV_PADCAL=0 and static RAM are NOT_RUN here.
- No main_fm1.c, FM-1 target, .github, Actions, protected storage, identity or ADSR changes. Owner approves real visuals before merge.
- Existing source is reused: smcfx_cycle, strip press/release, arw_frame, smc_ui_draw_peer. No replacement router or renderer.

## Review focus

- Empty slots remain silent until mapped; Left starts at the last suited effect, Right at the first.
- Multiple held FX keys consume arrows without bulk editing or navigating; keyboard checks all peers.
- Release, overflow and context exit clear owned FX even after a chord suppresses navigation.
- Modal/help/project/performance/clip input retains priority. No new arrow repeats edit the effect.
- Held bus/page ownership survives a later context change. Glyph and right-hand strip indicator cannot collide.

### Task 1: Held mapping and arrow priority

- [x] Extend `tests/host/smc_remainder_test.c` first: live quick taps including main SELECT, forward/back/wrap, independent mappings, HOME no longer remaps, SMC and real `arw_frame` paths, ambiguity and context guards.
- [x] Adapt existing SELECT and extracted FX preservation checks to the owner's new immediate activation and mapping gesture; retain release/peer/long-hold coverage.
- [x] Add `smcfx_step(bus,fx,d)` preserving `smcfx_cycle` forward compatibility. Add a per-note page bitmask to `smcfx_strip_t` to pin the original page, including empty slots (4 B per peer; written RAM reason: prevent held edits changing a different page). Preserve key_bus and live-FX order.
- [x] Add held-target selection and one-note stepping in `smc_mixer.c`; consume SMC arrows before HOME/page routes. Keyboard `arw_frame` uses an optional unity-build hook, so main_fm1.c stays unchanged. Remove HOME+button remapping and main SELECT activation delay; retain short-tap navigation eligibility until a chord.
- [ ] Box only: run `sh tests/run_tests.sh smc-remainder smc-select-mode smc-fx-preset-preserve smc-fx-two-page-strip smk-arrows`, then the complete suite. Codex records NOT_RUN.

### Task 2: Held-effect glyph and delivery

- [ ] Proposal: reserve x182..199,y1..18 of the 240x20 SMC header for an 18x18 effect glyph. Use room waves, echo blocks, filter curves, gate rhythms, stutter chunks, shuffled beats, and an explicit unassigned mark; existing glyph style and renderer. Context text ends by x178; control glyph remains x209 and strip number at235. No new animation clock.
- [x] Independent static design review before implementation: original bus/page, empty ownership, multi-key safety and header bounds reviewed; no unresolved blocker.
- [x] Add real-framebuffer assertions and deterministic before/after capture scenes to the existing registered fixture.
- [ ] Run captures on af1a3a5e and the candidate on the qualified box; attach actual PNGs to PR #800 before owner approval.
- [x] Compound Engineering review before commit; resolve retained defects, record static-only limits and exact source hashes.
- [ ] Push the reviewed source/tests/docs to PR #800, verify remote author, parent, tree, head and draft status. Do not merge.
