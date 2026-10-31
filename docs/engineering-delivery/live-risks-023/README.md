# Live-risk fixes for 0.0.23

Source candidate for a draft PR into `build/0.0.23`, based on
`bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`. The owner requested Q-P2.1,
Q-P2.5 and instant synth-bus unmute. The attached audit supplies evidence;
its other findings are outside this patch.

## Behavior and recovery

- A stream that fails decoding or its raw CRC is retried with up to 16
  combinations of the four objects' newest and older valid A/B copies.
  This handles saves that skip unchanged objects. Recovery reads flash only.
  Recovered stream hashes are invalidated so the next save repairs the newest
  objects. Unsupported revisions retain the existing rejection policy.
- If stored data exists but no complete project validates, defaults remain
  playable and project writes stay blocked. Ordinary edits, defaults and SAVE
  do not clear the lock. A validated staged load, import or explicit reset does.
  A genuinely erased first boot can still save its defaults. Settings/sound
  damage is distinguished from absence, including when no stream remains.
- Damaged storage has priority over queued/dirty save status: the header uses
  a red `!` chip, or the existing red dot on sound-matrix pages. No damaged-data
  boot banner or recurring autosave message is shown. This is a minimal reuse
  of the existing status presentation, not a new screen.
- Live-key owner, context and generation changes cancel the old physical-key
  source before its inventory is discarded. Held keys are suppressed until
  released; stale key-ups cannot release the other synth or a MIDI source.
- Synth mute gates dry and send gains through the existing short gain glide.
  Voices, envelopes, ARP, inserts and sequence timing continue. Unmute resumes
  the held/sustaining sound without waiting for a new note. Muted sequence
  MIDI output remains suppressed. Existing effect return tails can decay.
  Queued SMC mute and song mute masks reach the same mixer gate. Fader levels
  continue to update while muted, so unmute uses the current level immediately.

The key-owner trace covers `ui_live_key_owner`, both sides of the `input`
wrapper, `project_input_sync`, central/outer physical events, SMC ownership,
clip entry, modal controls, Free Play, recording and performance admission,
and the engine's cancel/preset/base-restore serial retirement paths. Settings
lists and HELP retain the synth keybed. An octave change retains held pitches.
The unchanged physical-fader scanner is inspected and extracted read-only for
the regression; `main_fm1.c` has no diff. No code from draft #788 is imported.

## Evidence status and Orchestrator handoff

All builds, host tests and hardware checks are **NOT_RUN**
by the producer, as explicitly required by the owner. Source inspection and
`git diff --check` are not runtime qualification. This draft is not merge-ready.

The owner explicitly requires no previews or visual-approval wait for this
change. Source delivery is complete without screenshots; the Orchestrator
verifies the candidate on the box.

Independent read-only Superpowers reviews (`storage_review`, `keys_mute_review`)
identified damaged/absent classification loss, two stale PAT0 test expectations,
warning precedence and a key-switch test that hit the part-order clamp. These
were corrected and re-reviewed. The producer then strengthened the recovered
object rewrite assertion, blocked damaged SAVE gestures without a banner, and
extended the test cases. Reviewers performed no builds, tests or edits.

Run on the Orchestrator's box at the PR's verified full head SHA:

```sh
sh tests/host/run_project_stage.sh
sh tests/host/rec_endpoint_storage_test.sh
sh tests/host/storage_warning_test.sh
sh tests/host/freeplay_live_keys_test.sh
sh tests/host/synth_bus_mute_test.sh
```

The NOR regression occupies all four stream objects and cuts before PAT1
(between stream objects 1 and 2). It checks recovery of the preceding project,
no writes during recovery, actual PAT0 rewrite, unrecoverable preservation,
damaged settings/sound-only images, and explicit validated reset. The audio
regression covers both synth banks and both parts with held MIDI notes, pedal
holds, sequence ties, queued SMC mute, SMC fader edits and physical fader input.
The latter uses substituted hardware samples with the real unchanged scanner;
it does not prove radio, ADC or matching-model hardware behavior.

Host `storedump`/`storeload` files keep their legacy single-generation format;
loading one clears unrelated previous in-memory copies. The real NOR fixture,
not those dumps, is the A/B persistence evidence source.

Full host suite and matching Elite/Pro SDK V1.2.1 qualification remain with the
Orchestrator before any integration. No main/.github writes, FM-1 operations,
firmware binaries, merges, release or hardware writes are part of this handoff.
