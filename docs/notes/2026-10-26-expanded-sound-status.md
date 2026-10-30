# Expanded SOUND status — final owner layout

The owner consolidated the readout direction after the earlier Chicago preview:
keep WAIT (not SWAPPING), bring each status icon close to its text, use the freed
CANCEL column as part of a proper right-half STATUS block, and put appropriate
contextual actions at the bottom. This supersedes the interim proposal to put
WAIT: SWAPPING PRESET or bare wait reasons in the footer.

ENGINE and PRESET remain on physical K5/K6. The right half is display-only;
K7/K8 still have no parameter references or actions. The divider before STATUS
moves to the start of that shared half; there is no line through its centre.
Existing ARP/value bars and all other glyph locations are unchanged.

Status uses a compact glyph/text group based on the actual text advance and
primitive ink width with a two-pixel layout gap. Actual pixels verify the ink
gap and cell bounds in selected and unselected states. ACTIVE, EDITED, USER and
UNKNOWN are distinct; no idle PRESET token duplicates the visible name.

While pending, STATUS keeps WAIT and places the explanation below it:

- Next audio boundary: WAIT FOR AUDIO.
- Held note or release tail: WAIT FOR NOTES/TAIL. The existing public reason
  combines these; the display must not falsely instruct the player to release
  a note that has already ended. No engine query or state was added.
- Pitch or modulation wheel ownership: RETURN WHEELS. This covers pitch centre
  and modulation rest, without incorrectly telling the player to centre both.
- Sound lock ownership: SOUND LOCKS ACTIVE.
- Previously accepted queued input: WAIT FOR INPUT.
- An unstable publication snapshot: WAIT FOR UPDATE, not an applied/failed sound.

The bottom shows PATCH: CANCEL plus row navigation while a selected SOUND
request waits; otherwise normal row/PARA navigation and PATCH: BACK. Existing
held-step hints take priority. Cancellation eligibility, publication/edit hold,
DSP, storage, wheel routing, knobs and the wait state machine are unchanged.

The focused `chicago_readout_test.sh` uses actual engine reasons and framebuffer
pixels for both voices/engines, known/user/unknown identities, live-note and
release-tail waits, publication interruption, compact glyph spacing, longest
explanations and unchanged K7/K8 mapping. It checks the complete displayed status
region and action footer against an independent expected layout, and verifies
rendering does not mutate sound/save state. A prior-source replay must reject
the old single-column status. Other behavior remains covered by the frozen
4f5c5c5 source/host/native evidence; it is not relabeled as this new native build.

The change needs only targeted host/sanitizer and affected native compile/link
qualification, then the same Sol reviewer's changed-delta round. Actual host
pixels are not device readability, hardware proof or rollout permission.
