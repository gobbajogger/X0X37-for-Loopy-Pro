# HOME content and assignment

The HOME thumbnail keeps its existing first-16-step, eight-column contract: each
column represents a pair of steps. Drum rows combine voices; break rows use their
step mask. A genuinely dense or alternating pattern can therefore fill all eight
columns. This is not a new overview layout or an all-64-step summary.

An empty first-16 row no longer receives a full-width assignment placeholder.
This also applies when stored content exists only after step 16. Current assignment
uses a separate two-pixel-high part-colour tick on the thumbnail's left border,
outside the eight content columns. Nonempty content brightness and the separate
cue/chain frames retain their existing meaning.

Bass content follows the piano roll's stored forward note-origin semantics. An
orphan TIE or all-TIE row is blank, including an all-TIE row created by the normal
REC/LFO/OCT input route. Valid NOTE-plus-TIE and holds wrapping from step 64 remain
visible. The HOME bass lanes use the same content helper. This is stored-note
visualisation, not a claim of audible output under every playback direction,
chance, mute or transport state. Rendering does not modify the pattern.

`tests/host/home_overview_test.sh` uses the production renderer and actual HOME
key selection. It checks the two content-pixel rows, assignment ticks, sparse,
dense, alternating, later-only, cue, orphan/valid/wrapped holds and pattern
nonmutation. `tests/host/home_tie_semantics_test.sh` runs the production sequencer
sink: NOTE/held-TIE positives, both-voice orphan silence, and a wrapped hold through
the final TIE's normal half-gate. These host checks are not hardware validation.
