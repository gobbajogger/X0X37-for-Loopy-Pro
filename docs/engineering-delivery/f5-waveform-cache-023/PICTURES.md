# Pictures and glyphs on F5 screens

Owner direction: use pictures, glyphs and small activity-based animations wherever
they communicate the on-screen action clearly. Keep the existing #807 branch and
owner approval gate. No builds or tests in Codex.

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

## Regression and capture work

Extend the registered `original-release-ui` test before changing production:
check literal real-content badge pixels, last-slot bounds, absent/equal cues,
stopped stability, real-step response, transport shape bounds, row/page hint
bounds and state preservation. Add a capture-only fixture using existing host
APIs so the identical scenarios can run on the previous #807 head and candidate.
No test or fixture execution is authorized in Codex.

Source-review the incremental diff through `ce-code-review`, then push a normal
fast-forward update to #807. BEFORE/AFTER 240x240 host PNGs remain NOT_CAPTURED
until the authorized box stage runs the fixture. Keep the PR draft for owner
visual approval, full host/Elite/Pro qualification and RAM measurements.

## Actual source and pending captures

The candidate reuses the existing content and transport painters. Only bounded
primitive strokes for the arrow/row/page badges are added. No persistent state,
timer, framebuffer, image atlas or pool allocation is added by this follow-up.
The original waveform cache's expected 156-byte pool cost is unchanged; target
RAM is still NOT_MEASURED for both Elite and Pro.

`tests/host/original_release_ui_test.c:picture_rule_cases` extends the existing
registered selector with the cases above, dark/Light palettes and all four
direction glyphs. The exact direction value text remains available to lists.
`tests/host/ui_picture_capture.c` is a capture-only fixture, not a new test suite.
It uses APIs that already exist at previous head
`de80f7b67e0a6ec8d97d42b166bb373de4582504`. Copy that identical fixture into a
clean checkout of each source pin on the box; do not use the candidate renderer
to create a purported BEFORE.

For each pin on the authorized box:

```sh
ORIGINAL_UI_TEST_FILE=tests/host/ui_picture_capture.c sh tests/host/original_release_ui_test.sh build/ui-pictures
```

That existing harness writes `build/ui-pictures/{normal,sanitized}-images/`.
Expected 240x240 files, each in both dark and Light variants:

- `home-stopped-queued.png`
- `chicago-oscillator-playing-queued.png`
- `miami-carrier-stopped-queued.png`
- `miami-xmod-playing.png`
- `miami-wobble-playing.png`
- `chicago-envelope-playing.png`
- `sequencer-direction-{0,1,2,3}.png`

The filenames have `dark-` / `light-` prefixes. Actual BEFORE/AFTER PNG paths:
**none yet, NOT_CAPTURED**. No framebuffer execution occurred in Codex, and the
capture paths above are reproduction instructions, not existing evidence.

Run the registered assertions again with physical SMK captions enabled as well
as their default host configuration:

```sh
X0X_HOST_DEFS=-DX0X_SMK_CAPTIONS=1 X0X_TEST_JOBS=1 X0X_TESTS=original-release-ui sh tests/run_tests.sh
```

The tests inspect the composed page/held-step row footer, queued/absent/equal
sound-header cues, and the blank region where direction words used to appear.
Pending-preset back hints and the division-pad-owned header still need composed
framebuffer coverage on the authorized box. These are coverage limits, not PASS.
