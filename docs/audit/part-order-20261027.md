# Group the drums before the synths

The owner requested 909, 808, BREAK, CHICAGO, MIAMI in that order, with drums
and synths together. This is a presentation/navigation change, not a project
migration. The default synth engines occupy Chicago B and Miami A, so the
fixed display-to-lane map is `0, 1, 4, 3, 2`. Choosing another synth engine does
not move its strip, MIDI channel, saved pattern, held note or captured control.

## Changed boundaries

- HOME lanes and pattern glyphs; SONG columns, pattern knobs and mute keys.
- MIX and FX overviews, selected MIX strip and global MIDI settings list.
- ALGORITHM navigation, five whole-part black keys and their lamp feedback.
- SMC whole-part faders/knobs, selection, REC, solo, mute and feedback. FX press
  and release both translate the physical strip to the same existing bus ID.
- PERF instrument rows follow the same display order; profiler data retain IDs.

Drum-track banks, engine/source arrays, project/storage layouts, MIDI channels,
clip-editor routing, controller calibration and effect tables are unchanged.
The prior SOUND and held-note Save work is preserved from merged `100b9dd0`.

## Affected proof

`tests/host/part_order_test.sh` checks the explicit order, stable internal IDs,
non-mutating navigation, panel keys/lamps, SMC destinations and held FX release,
captured target identity, HOME glyph positions and actual source renders.
Existing HOME, SONG, SMC UI/SELECT/recording and held-key navigation regressions
cover the affected integration. Scenario click counts now reach the same stable
lanes; musical and persistence assertions are retained rather than renumbered.

Fresh official app-only DEV builds are required because application source
changed. Reuse controller evidence only for unchanged controller inputs. Host
renders and tests do not establish physical-device behavior. Source integration,
preview acceptance, publication and hardware use remain separate stages.

## Inherited test limits

A targeted baseline replay at untouched merged `100b9dd0` reproduced all 33
`rec-live` failure lines and the same failures in legacy `getting_started`,
`patterns`, `plock` and `song` scenarios. Those remain FAIL, not candidate PASS.
Several legacy fixtures still expect hidden recording/editing from EDIT; this
change does not repair that unrelated contract. The maintained tagged tutorial
and the focused visible-SEQ held-step check cover the current UI path separately.

## ORDER-01: onboard fader closure

The same reviewer found one remaining consumer: the keyboard's four-fader bank.
It now uses the same five displayed parts across banks A/B, then the unchanged
reverb return, delay return and master filter. Captions use actual short lane
names. Mute-edge history translates a stable lane ID into its physical bank and
slot before updating pickup state; it never changes the stored destination ID.

The EC026 owner confirmed disjoint main_fm1.c ownership: this patch touches only
smk_faders and its preceding comment, not the OTA/safe-loop dispatch. Root
explicitly extended the same review through round 4 (maximum 5) for ORDER-01's
wrong-destination risk. No unchanged SOUND/Save review was restarted.
