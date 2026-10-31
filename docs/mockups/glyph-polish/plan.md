# Glyph polish implementation plan

**Goal:** Make Chicago, Miami and 909/808 control glyphs readable at native size and animate them from live engine state.

**Architecture:** Keep the production framebuffer renderer and control mappings. Add read-only voice telemetry where existing filter/drum views do not expose enough information. Preserve audio processing and parameter laws. Use PR #743 (`e5a75c7c8e7f1b21baa1476b5fbf635b29017480`, source tree `6c97c31d83c34688ab52af008a10016ac479a277`) as the code baseline and PR #744 (`51b2180e3eecd57b7e3a40900aca6ad7519c2223`) as visual reference.

**Tech stack:** C firmware UI and engines, native host framebuffer, Python/Pillow for arranging existing rendered pixels into GIFs and sheets.

## Design

Keep 26 by 19 pixel control cells with one clear contour, a quiet baseline and sparse amplitude marks. Reserve bright ink for current activity. Static outlines explain the parameter; the real envelope, pitch or modulation state explains activity. Avoid dense rapidly oscillating traces that alias at this size.

Chicago ENV uses the actual cutoff and filter EG. Pot 64 is zero, readout -100 to +100. Signed depth moves the knee horizontally; negative moves left, positive right, distance scales with magnitude, and the real filter decay returns it to rest. PEAK alone controls the vertical height. The ADSR graph replaces the A/D/S/R boxes, retains readable stage identification and values, and adds an engine-driven cursor.

Drum decay shows a fading remaining tail; tune shows a falling pitch contour; snappy shows irregular noise; tone shows spectral balance; level uses a segmented meter and accent a peak. Animate only while live engine state changes. Idle and zero-depth views remain stable.

Considered alternatives: oversized pictograms would displace values; dense waveforms obscure meaning at native size. Keep the existing layout and simplify its schematics.

## Constraints

- New branch `ui/improve-glyph-visuals`; one draft PR stacked on #743. Never merge or alter main/nightly.
- Author and committer: gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- No .github/Actions, .env, bass303, firmware binaries, raw captures, bootloader, BTIF, USRTRIM, efuse or key_mac changes.
- SDK V1.2.1; build/test only SMK-37 Elite and Pro.
- Generated GIFs and still sheets are the explicitly requested visual deliverables, not firmware binaries.

## Tasks and verification

1. Capture baseline with real host UI and engine, using the same deterministic fixtures that will render the candidate. Inspect PR #744 reference frames and get independent internal design review.
2. Add focused failing tests for signed ENV direction/magnitude/return/constant height, zero-depth stability, live ADSR/synth activity and drum role/value/motion coverage. Implement in production UI with read-only telemetry. Check bounds and existing UI regressions.
3. Render BEFORE | AFTER GIFs and per-frame sheets for ENV 0/25/50/75/100%, ADSR, Chicago/Miami controls and both drum kits. Check each frame visually and check motion/value differences programmatically. Record fixture timing and actual source identity.
4. Freeze code, run host UI tests and genuine Elite/Pro development builds in cloud with pinned inputs. Independent code/visual review, resolve findings, then repeat only affected checks.
5. Deliver only new changes through the connected GitHub API, verify remote parent/tree/head and commit identities, open one draft PR against `ui/drum-glyphs-for-review`, and post a factual status comment. No merge or hardware claim.

## Review focus

Check cutoff extremes, ENV centre quantization, negative depth, silent/held/released notes, retriggered drums, renderer cache invalidation, polyphonic Miami selection, native pixel bounds and font readability. RGB565 is the real SMK host framebuffer format; any monochrome inset is labelled as a readability view, not a claim about physical hardware.
