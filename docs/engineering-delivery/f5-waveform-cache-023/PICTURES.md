# Pictures and glyphs on F5 screens

Owner direction: use pictures, glyphs and small activity-based animations wherever
they communicate the on-screen action clearly. Keep the existing #807 branch and
same PR. Latest owner instruction: no previews and no visual approval wait for
this change. Finish and push; the Orchestrator verifies on the box. No builds or
tests in Codex.

## Design and implementation plan

Reuse the existing HOME content projection, `pat_glyph_row`, `play_icon`, matrix
arrows and sequencer direction glyphs. The chosen approach keeps the current
screen strips, font assets and input routing; no new image atlas or animation
buffer is needed. Engine/control names, values, units and physical button names
remain where pictures alone would be ambiguous.

- Replace the normal header's P-prefix with a framed, eight-column real-content
  pattern badge and slot number. Use a drawn arrow and the queued pattern badge
  instead of text/punctuation for the pending cue. The sound-matrix header gets
  the same queued badge in its unused status area when pads do not own that area.
- Reuse the play triangle and show a square when stopped. Keep external-clock
  waiting feedback. A queued arrow may pulse only with actual playing steps;
  stopped headers stay still as wall time and UI frame count advance.
- Replace ROW/NEXT/BACK words in sound-matrix hints with row-stack/page/arrow
  pictures. Keep physical panel legends and numeric bank ranges. Held-step and
  pending-preset ownership retain their existing routing and precedence.
- The sequencer already draws forward, reverse, ping-pong and random direction
  pictures. Suppress their duplicate FWD/REV/PING/RND value text on the normal
  knob strip; retain precise text in lists and Help. Do not invent REW/FWD inputs.

## Verification handoff

The registered `original-release-ui` selector includes the real-content badge,
last-slot, absent/equal cue, stopped stability, actual-step response, transport,
composed row/page hint, state-preservation and direction-picture assertions.
No tests or builds have run in Codex; no runtime PASS is claimed.

The candidate reuses the existing content and transport painters and adds no
persistent state, timer, framebuffer, image atlas or pool allocation. The original
waveform cache's expected 156-byte pool cost is unchanged. Elite/Pro RAM remains
NOT_MEASURED and is part of the Orchestrator's exact-head box verification.

The prior preview-image production and owner visual-approval steps are removed
for this change by the latest owner instruction. Do not wait for previews or
visual approval. The existing capture-only utility is not a delivery prerequisite.

The Orchestrator verifies on the box, including the physical-caption configuration
(`X0X_SMK_CAPTIONS=1`), pending-preset/back hints, division-pad-owned headers and
composed layout bounds. Keep the existing source review and test checks intact.
