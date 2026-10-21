# Isometric / native-host verification

2026-10-21. Browser QA remains pending, not passed.

- 27/27 offline model assertions pass.
- 10/10 independent source-focused assertions pass.
- 10,000 seeded C-versus-JS scale/chord cases pass.
- Six DOM-stub offline interaction regressions pass.
- Native dispatch stub verifies middle-key boundary, play/release, SMC encoder
  sign magnitude, User MMC, unsupported-input no fallback and simultaneous arrows.
- Exact 146 control IDs preserved (85 Elite +61 SMC).
- 1,019 Elite +685 SMC projected polygons fit viewBoxes; inverse projection and
  full fader travel assertions pass. Individual1600px SVG renders inspected.
- Actual native C build succeeds; four grouped native tests pass, including
  real SMC queue, pickup, transport and held-Solo releaseAll/disconnect cleanup.
- HTTP state/input/frame succeed; hostile Host/Origin and invalid inputs reject.
- Actual C framebuffer PNG inspected; this is not a browser screenshot.
- No firmware source modified. No radio/USB/audio timing/hardware emulation claim.

Independent review caught three issues before delivery: stale Solo on releaseAll,
combined-arrow release ownership and premature native-active state. Fixed and
regression-tested where applicable. Offline-only panels are disabled/labeled in
native mode; actual state replaces model state and unsupported pads are gray.

The source PNGs and SVGs show hardware geometry. The HTML/CSS iPad scene has not
been visually inspected in a real browser because the available browser routes
could not reach the permitted localhost server. No sandbox bypass was attempted.
