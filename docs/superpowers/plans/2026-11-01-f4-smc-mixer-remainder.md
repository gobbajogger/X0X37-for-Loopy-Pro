# F4 SMC/mixer remainder implementation plan

Later owner amendment: [held-FX arrows](2026-11-01-fx-held-arrow.md) supersedes this plan's HOME-remapping, activation-delay and no-new-state assumptions for row 41 only. The original port plan below remains historical context.

**Goal:** Port only uncovered rows 20, 30, 31, 32, 36, 37, 38, 41, 44 and 67 onto #789; deliver one draft PR into build/0.0.23.

**Architecture:** Keep #789's existing popup, independent peer context and descriptor cache. Reuse the source from #688 and #788 for the remaining clock and strip-8 navigation gaps. No new renderer, timer, connection protocol, FX assignment or persistent state.

**Tech stack:** Existing C firmware, production host UI/engine fixture and shell host-suite selectors.

**Spec:** The owner's F4 brief in this conversation. Existing visual proposal: #688 `docs/mixer-ux-proposal.md` at 7b2d1726f3515b319d5edcab34529007d33ab3a8. No new mockup is required for these agreed items.

## Global constraints

- Base: #789 02c5224d12c7a96c0061fea71f13de04de58741b. #788 5b5974b667eaaacb904ef69391877f6779021543 is open, diverges at bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2, and is not an ancestor.
- One owned branch `chat/f4-smc-mixer-remainder-023`, one draft PR into `build/0.0.23`. Author gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- No main, .github, Actions, main_fm1.c, FM-1 targets, BTIF, USRTRIM, efuse, key_mac, bootloader, USB identity or Miami ADSR changes.
- AGENTS forbids builds/tests in Codex. Write/register tests first; RED/GREEN execution, framebuffer captures, full suite and SDK V1.2.1 Elite/Pro with X0X_DEV_PADCAL=0 are delegated to the Orchestrator's box. No result may be claimed before execution.
- Latest owner instruction (2026-11-01): no previews and no visual-approval wait for this change. The Orchestrator verifies on the box; draft status reflects pending box qualification.
- Preserve every existing test assertion. No new static state or RAM allocation in production; measured static RAM remains NOT_RUN until both model builds.

## Review focus

- Drum strip-8 precedence, including five empty bank-2 positions, must remain intact.
- FX SELECT press/release and HOME modifiers must keep their current owned effects; short taps still navigate.
- Clip-stage navigation may change the clip stage; knob-bank navigation must not do so or change the keyboard page.
- Held/repeated/released inputs and modal guards must not acquire unintended actions.
- Both header paths must follow accepted clock phase through STOP/WAIT, Start/Continue and track playhead changes.

## Task 1: Audit overlap and write regression evidence

Files: `docs/engineering-delivery/f4-smc-mixer-remainder/README.md`, `tests/host/smc_remainder_test.c`, `tests/host/smc_remainder_test.sh`, `tests/host/smc_remainder_capture.c`, `tests/run_tests.sh`.

- [ ] Record row-by-row current-source coverage and named commit disposition.
- [ ] Port focused production-queue regressions for strip-8 modes, clip stage/banks, clock icons and existing lock/queue/hold contracts. Register `smc-remainder`.
- [ ] Provide a separate capture driver using the production framebuffer without relaxing assertions. Capture execution is NOT_RUN here.

## Task 2: Port only uncovered source

Files: `firmware/src/app/smc_mixer.c`, `firmware/src/app/ui.c`.

- [ ] Reconcile #788's clip strip-8 navigation, pickup blink, Pause LED phase and both header play icons. Up/Down already banks in clip mode; retain the existing code and HOME modifiers.
- [ ] Reconcile #688's repeated SELECT8 MIX-page cycle and matching MIX caption.
- [ ] Preserve #789's keyboard independence and context cache. Exclude #788 row45 unmute and keyboard-focus restoration; those belong to other scope.
- [ ] Retain already-present rows and archive implementations unchanged.

## Task 3: Review and draft delivery

- [ ] Independent visual/source review before implementation; Compound Engineering code review before PR.
- [ ] Inspect exact diff and protected paths. Source review is separate from test PASS.
- [ ] Deliver draft PR, full head SHA, registered tests, capture commands/expected paths and explicit unrun gates. Verify remote ref and commit author.

## Verification contract

On the qualified box, run `X0X_TESTS='smc-remainder smc-ui-popup smc-lock smc-lock-beat smc-select-mode smc-mixer-page smc-mode-page smc-fx-two-page-strip smc-fx-preset-preserve ui-peer-names' sh tests/run_tests.sh`, then the full host suite. Record individual failures; #789's reported 9 failing SMC selectors are historical receipts, not current-head results. Build both models from the same full head with SDK V1.2.1 and X0X_DEV_PADCAL=0; report free static RAM against the exact same-config base and the requested 2352 B reference.

The Orchestrator verifies the changed behaviour on the box. The separate capture driver remains available for internal diagnostics; previews, image attachments and owner visual approval are not delivery gates under the latest instruction.
