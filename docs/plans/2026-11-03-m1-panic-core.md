# M1 global PANIC: isolated chord core

## Goal and authority

Prepare the owner-approved M1 gesture core for global all-part notes-off while
transport continues. L6 is an alias of M1, not another feature. This packet is
partial: it does not connect keyboard, SMC, audio or MIDI ingress/consumers.

Source: canonical `nightly` at `66073c68dd80d51a7053432c2c161a8b720a1341`.
Latest ROADMAP owner: PR #812 at `496c87269349b70f9a900bd95b996d765aa51a80`.
Coordination snapshot: `4a35a6cdaa0f112db6b3f577542138afd170c20d`.
The delegated parent owns integration handoff and ledger updates. Existing M4,
M8 and held-note writers retain their paths. This stage owns only the new helper,
dedicated tests and this plan/evidence document on `wip/m1-panic-core-20261103`.

## Design

Use a dependency-free C99 header so no build manifest or shared source needs an
edit. One zero-initialized state belongs to each physical keyboard/SMC peer.
The caller supplies a two-bit snapshot of the approved chord and an explicit
source mode. Keyboard means FX+SEQ; SMC DAW means BANK L+R (0x2E/0x2F).
SMC User and MMC are ineligible. Channel arrows (0x30/0x31) are not BANK.

Return FIRE once when both members are held, plus CONSUME for the recognized
chord and its remaining releases. Single members pass through. Require both
members released before rearming; this is a conservative implementation choice
to prevent repeated panics from repeats or partial release/repress. Disabled
mode while either member is held inhibits firing until both are released, so
switching back to DAW cannot convert an old hold into a panic. Only the two
declared chord bits count; unrelated held controls do not affect the chord.

This helper recognizes gestures only. FIRE is a global request, never a part
selection or transport command. No dispatcher with mock note-off callbacks is
added: it would not prove the five real engines or MIDI owners are cleared.

## Steps and acceptance

1. Write executable tests against the intended API; observe missing-feature RED.
2. Implement only the chord helper. Verify both orders, single keys, repeats,
   release consumption, both-up rearm, mode changes, User/MMC rejection, peer
   isolation and unrelated-key masking with literal expectations.
3. Run C99 warnings-as-errors, sanitizers and a mutation negative control. Obtain
   independent Sol review of immutable scoped files; keep its finding ledger.
4. Push reviewed text to the isolated canonical branch, read the remote SHA,
   parent/tree and blobs, and open a draft PR for the integration owner.

No UI render changes occur. Native Elite/Pro/controller builds, full host suite,
performance measurements, hardware, publication and all-parts release acceptance
remain NOT_RUN here. No benchmark/device reservation is taken.

## Integration handoff and stopping condition

Before hooks, one current integration owner must confirm ownership of shared
keyboard/SMC paths. On the selected final base, map real controls and pass exact
DAW eligibility (exclude User mode even for raw NoteOn); keep independent peer
state and update/reset on mode changes, disconnect and consumed release edges.
The first member can already have caused ordinary navigation before the second
arrives; any rollback/delay policy belongs to integration and its real-path tests.

Implement audio-owned global notes-off separately: clear all five internal parts,
live/arp/damper/queued notes and external output ownership without stopping clocks,
rewinding playheads, losing future notes or changing sound/mixer/project state.
Do not reuse `engine_project_flush()` (sends STOP), `seq_stop()` (stops transport),
or assume two `engine_bass_off()` calls cover all parts and ownership.
Test the real hooks and all engine/MIDI boundaries, then run applicable complete
host/native/model/controller gates before claiming M1 complete. No merge or
publication authority transfers with this packet.

This stage ends with reviewed remotely verified source and an exact handoff, or
a precise dependent blocker. Whole-feature completion remains pending integration.
