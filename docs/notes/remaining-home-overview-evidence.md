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

## Exact-header host recheck

The host test source `tests/host/home_pattern_glyph_test.c` and helper header are identical to the pushed Git blobs (`237b2bb6a921cf4c46a5d4c23c6da75dd1319d65` and `065933fc65074b196aa70c97cd24d937fe1ec515`). GitHub's live `nightly` headers `pattern.h` (`19dc003c8bc3e43d94dd4b4e9c1c068030a84cd2`) and `motion.h` (`157b43e9ef0dbbbbd18fbf3ec205963814bb2cfd`) were reconstructed locally and confirmed by `git hash-object` before running:

```sh
CC=gcc HOME_GLYPH_OUT=build/host/home-glyph-exact-headers-gcc sh tests/host/home_pattern_glyph_test.sh
CC='clang -fsanitize=undefined -fno-sanitize-recover=all' HOME_GLYPH_OUT=build/host/home-glyph-exact-headers-clangubsan sh tests/host/home_pattern_glyph_test.sh
```

Both returned exit 0 and printed `PASS home_pattern_glyph`. The test reads only those exact headers and uses the actual new helper. This is **scoped host PASS** for the data projection, not proof of the full current firmware, current-HEAD UI framebuffer output, animation or physical rendering. Those are still **NOT_RUN**.
