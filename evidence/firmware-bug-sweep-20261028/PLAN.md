# Firmware bug sweep, 28 October 2026

## Requested outcome

The owner requested: “Full send sol loop firmware for all obvious bugs and issues”.
Find reproducible defects in the current firmware, make the smallest complete
repairs, prove each repair, obtain independent Sol approval, and deliver the
candidate through the existing integration owner and applicable gates.

## Inputs and ownership

- Firmware baseline: `7e4bbfd95da1d9ad86c80541b9bceae48231d006` (`nightly`).
- Canonical request/ownership snapshot: `12f3b3cd32851f55c4635c697568cbbbb9a29300`
  (`coordination/merge-queue`).
- Isolated source producer: this owner-requested ChatGPT execution, branch
  `fix/firmware-obvious-bugs-20261028`.
- After discovery, the protocol helper received exclusive ownership of an
  isolated `fix/midi-overflow-order-20261028` worktree for the three MIDI queue
  paths and their covering fixtures. Root owns the disjoint Chicago engine
  changes. The protocol commit is integrated locally before combined review.
- Existing sole integration owner remains `dot existing sole PR monitor`.
  This task does not transfer shared integration or ledger ownership.
- Existing request identities remain authoritative: LX-REQ-008/009/022 for
  sequencer/transport, LX-REQ-017 for host tooling, and LX-REQ-016 for exact
  candidate build/QA. Findings will be mapped to their actual affected scope.
- Current UI, adaptive BREAK, sample admission, multi-SMC and strict-host PRs
  retain their owners. No duplicate implementation of their known changes.
- The selected repairs are saturated MIDI arrival order, BLE counter rollover,
  Chicago BPM Delay tempo, effect history on parameter edits, and insert tails
  after oscillator idle. The missing clip-knob fixture symbol is repaired only
  to restore its existing covering check. No additional discovery is queued.

## Bounded execution plan

1. Inspect current callers, existing tests, recent source and existing findings.
   Independent discovery helpers are read-only; they may run isolated temporary
   reproductions but cannot edit shared source or claim integration ownership.
2. Select definite failures with an executable reproduction and confirm that
   an existing repair does not already cover the affected source.
3. Make minimal fixes on this isolated branch and add focused regressions.
   Preserve established features, data, model identity and audio quality.
4. Execute fail-before/pass-after, affected boundary/negative checks and the
   relevant native build with verified compiler/SDK inputs. Keep actual failures,
   missing inputs and unrun checks explicit.
5. Freeze the candidate for one actual GPT Sol reviewer. Default budget is three
   rounds; later rounds cover changed code and open findings only. Keep the ledger.
6. Deliver approved source through a normal branch/PR with full remote readback.
   Route the exact candidate to the existing sole integrator and verify a real
   receiving action where supported. Do not take over shared refs.

## Acceptance and boundaries

Each repair needs a demonstrated original failure, a passing regression and
applicable covering proof. Sol approval must cover the final behavior diff.
Review approval, host proof, native BUILD_ONLY, merged source, publication and
hardware acceptance are distinct outcomes. No device probing/flashing, production
promotion, force push, restricted-input publication or scheduler changes are in
this execution. Existing action-specific denials remain binding for their scope.

Stop when the bounded evidenced repairs are delivered through their authorized
outcome, or a precise unavailable capability/authority/input prevents the next
required action. Preserve exact evidence and the existing receiving owner for
any remaining stage; do not claim all possible bugs are eliminated.
