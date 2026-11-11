# SMC drum controls and pad sequencing — 2026-10-21

## Owner's current choice

The owner selected **Individual drums** for Record and Solo on the 909/808 mixer pages: Record selects the visible drum and toggles recording; Solo isolates that drum while held. The owner then clarified, "It can select a drum to seq on smk pads", and requested selection in both SEQ and the 909/808 mixer. The later performance goal is a useful three-layout surface that reduces menu diving: X0X main mix, individual 909 sounds and individual 808 sounds.

This supersedes the earlier per-track Record/Solo deferral. It does not change the earlier approval-history correction: the old whole-part behavior was retained in an approved scope, and calling it unapproved was unsupported. The current choice is a new explicit behavior decision. See [the preference record](USER_PREFERENCES.md#individual-drum-controls-and-smk-pad-sequencing).

## Implemented behavior

The same drum-strip map applies to 909 MIX, 808 MIX, 909 SEQ and 808 SEQ. MIX uses its visible kit; SEQ uses the selected drum part. Each kit retains its selected track and bank. Record, Solo, Mute and SELECT address the same drum as that strip's fader and knob. Bank 1 contains tracks 1–8; bank 2 contains tracks 9–11, with strips 4–8 inactive and unlit.

Record and SELECT focus the actual kit and drum used by SMK pad sequencing. Record toggles the existing global recording mode; SELECT leaves it unchanged. Pads edit the selected drum with recording either on or off. The current MIX or SEQ view stays open. Step lights, playhead, accent display, pad grid and colour follow the selected/visible kit.

On MIX, Left/Right reaches PARTS, 909 MIX, 808 MIX and MASTER without opening a menu. The main layout remains 909, 808, 303A, 303B, BREAK, reverb, delay and master filter. Other sound/effect pages retain the existing main faders and visible-field knobs. This candidate follows X0X pages; it does not introduce an independently pinned SMC layout. Open menus, help, confirmations and clip editors retain their own priority. The complete current map is [SMC_MIXER.md](SMC_MIXER.md).

## Production corrections

The initial problem was more than a lamp mismatch. Drum MIX only mapped Mute and SELECT to individual drums. Record and Solo still addressed whole parts, and several strip positions triggered navigation. SELECT could change the visible drum without changing the actual selected kit. The UI then rejected pad step events on all MIX pages, so a correct selection alone could not enable the requested workflow.

The implementation adds drum context to SEQ, gives all four strip rows priority in drum views, focuses the actual kit and track, and permits drum MIX pad step editing. Held drum gestures retain their original kit, track and pattern. Changing selection cancels the previous gesture before another encoder can edit the newly selected drum. A pattern cue prevents a stale tap from changing the new pattern while still restoring a stopped parameter-lock preview.

Solo uses temporary engine state. Eight physical key targets produce a union of selected parts and drum tracks. Each Note Off releases its original target after a bank, kit or page change; repeated held presses do not activate another target or a navigation shortcut. Disconnect, coalesced reconnect, help, confirmation, performance ownership and project reset clear temporary Solo. Saved part/drum mutes are not rewritten.

The engine publishes the union in one packed word and snapshots it for both audio cores. Drum renderers suppress excluded voices' dry outputs and per-voice effect sends while their voice state advances normally. Release restores remaining tails without retriggering. Whole-part Solo retains its existing gain glide; existing shared effect tails are preserved. MMC has no release event, so it retains a latch fallback: the same target toggles off, and a new target replaces that physical key's prior latch.

## Source and independent regression evidence

- Production commit: `b247ab8c3a85e269e39399a80f42fe423707919a`.
- Test commit: `fd2792b6e2f7edae4fd4461ac208a47473f92db8`.
- Negative-control source: `a1903c052b948a31f093cc8dcbf16e75d9d2939e`.

The production regression sends raw DAW/MMC messages through the SMC queues and dispatch, then uses the production SMK pad mapper and UI input path. It inspects actual pattern writes, emitted SMC lights, pad step lights and rendered engine audio. Hardware and radio I/O are host substitutes.

| Check | Candidate result |
| --- | --- |
| 22 drum destinations in each of MIX and SEQ | 44 passed |
| Five inactive strips per kit and view | 20 passed |
| Pad writes with Record and SELECT | 88 passed |
| Engine/DSP audio cases | 88 passed |
| Assertions, including unchanged destination checks | 124,962 passed; zero failures |

The baseline fails 250 assertions. Ordinary stereo rendering under the regression's fixed sound settings produces the same fingerprints in baseline and candidate: 909 `2b1a2725`, 808 `a0999a81`. The audio checks also cover both dry outputs, both voice sends, already playing tails, release recovery and whole-kit/drum unions.

Independent source review identified the held probability-edit and preview-restoration hazards during development. Both were corrected and covered before the production commit. The review found no remaining blocker within the inspected UI, pad-display, engine and drum-render paths.

Retained logs:

- [Candidate host regression](../evidence/smc-drum-controls-20261021/candidate.log).
- [Exact baseline negative control](../evidence/smc-drum-controls-20261021/baseline.log).

These logs are host evidence. The final installation kit's separate development record identifies the exact integrated source, scoped checks, Elite target build, controller and artifacts. This note claims no physical installation or live Bluetooth/device result.
