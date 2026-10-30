# DnBOX UI completion proposal

This is a proposal, not an implemented render. Reuse the approved matrix B geometry in `docs/notes/2026-10-26-ui-matrix-b.md` and the reviewed #647/#650 shared material. The original-engine matrix has four rows per bank, a filled active row, sparse unused cells, physical K5–K8 captions and unchanged row navigation. All 22 Chicago and 18 Miami controls remain reachable, including read-only performance indicators in MORE.

Use the same shallow panel edges, wells and shared parameter glyphs for HOME, BREAK, FX, lists, sequence, arrangement, mixer and settings. Preserve real values, units, step overlays, meter travel and interaction. Original-engine parameter names must come from their metadata, never the replaced bass303 or wobble parameter enums.

Add LIGHT after the five existing theme choices. Keep their stored indices. The Light palette uses a warm off-white base (#ECEFE8), white faces (#FAFCF6), grey-green edges (#B4C1B4), dark text (#18291F) and a pale green filled active row (#CDDFC4). Foreground accent and part colours use darker Light variants; selection text must contrast with its filled row. Dark themes retain the existing Night material. Persist the selection through the existing palette byte in revision 13. No project format change is needed.

The static default boot screen uses the exact mixed-case DnBOX wordmark centred above `909 808 CHICAGO MIAMI` and a small `BREAK` caption. Use the existing fonts and strip renderer; no video or animation player. Preserve the custom still-picture record and restore-default behavior. Match installer-facing product captions to DnBOX; keep protocol identifiers, repository URLs and copyright intact.

Factory names must come from the original program tables. Keep the original compact 55/30 selection and pending/edited indicators. Preset adoption waits for source/voice ownership, then fades any residual insert output over 512 samples and requires 512 consecutive quiet output samples; validate the actual adoption boundary and do not label a block-boundary check as audible proof.

Validation must use the production renderer pinned to the candidate, including every theme, matrix bank, settings, HOME/BREAK/FX, sequence, arrangement, drum MIX and the static boot screen. Check contrast, clipping, active-row distinction and nonmutation. Run host regressions and one original-engine Docker build, report memory, and leave hardware validation to perf-test.

Independent design review accepted the labelled SVG proposal after identifying palette admission, lowercase font coverage, original-control aliases and rendered FX-tail quiescence as required repairs. The candidate must prove those repairs. No approved bilingual font or raster logo is present in the supplied scope; item 71 must retain that limitation.
