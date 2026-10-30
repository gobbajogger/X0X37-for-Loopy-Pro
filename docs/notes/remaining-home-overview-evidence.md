# HOME pattern overview: full-pattern content and real velocity

Scope: feature-list 57 and 58 only. Reuse the existing eight-column, five-lane HOME
mini-pattern and selected/queued borders; no changes to physical controls, saved
project bytes, audio, fonts or screen geometry. Do not import other UI lanes.

## Layout contract (existing approved footprint)

Each 8-pixel row still belongs to its real 909, 808, BREAK, Chicago or Miami part.
A column represents two sixteenths in a 16-step pattern, four steps in a 32-step
pattern, or eight steps in a 64-step pattern. An event late in a long pattern
must therefore not vanish. Black remains empty. Brightness is the strongest
real event in that column. The assignment tick, cue outline and part colour
remain the existing indications of selected/current/pending state.

Example data projection (not a newly designed screen):

- 16 steps, first beat only: `#.......`
- 64 steps, event at step 17 only: `..#.....`
- 64 steps, final step only: `.......#`

A drum column uses the maximum strike velocity across its real drum voices,
including the stored `T_VEL` automation when present and the normal/accent law
otherwise. A bass TIE inherits the preceding NOTE, including a valid wrapped
hold; orphan/all-TIE patterns stay empty. BREAK retains its existing 16 steps.
No decorative or hypothetical hits are inserted.

## Checks and acceptance

- Pure host regression exercises 909 and 808 separately, 16/64 steps, overlapping
  strong/weak events, per-step drum velocity, another pattern's automation,
  bass held notes and orphan ties, BREAK, and monotonic brightness.
- Existing `home-overview` render regression must show late notes in long
  patterns without claiming they were in the first 16 steps. Other old test
  fixtures still project to their unchanged eight-column masks.
- An actual candidate-pinned HOME framebuffer preview and current-source
  renderer build are required before declaring full visual acceptance. Neither
  historical screenshots nor these pure host tests replace that proof.
