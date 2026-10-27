# Shallow UI surfaces and truthful Miami shape cues

Renderer-only change on the combined stock8 + dual-SMC base. Screen rectangles, native fonts, knob mappings, data and audio remain unchanged. Shared panels have clipped four-pixel chamfers, local upper highlights and inset lower edges. Active rows receive a shallow face gradient; inactive faces stay flush. Parameter captions use a one-pixel inset-light treatment. Miami keeps four rows; the approved Chicago second screen keeps three.

Miami's old ideal-square/inverted-pulse icon did not match the selected DSP form. The new cue samples the unchanged raw periodic evaluator: triangle, eight-term Fourier square, or positive10% BLEP pulse with negative1/9 low level. A fixed1/128 phase increment defines this normalized shape reference; it is not the played note frequency, final oscillator gain, modulated voice, or audio scope. One cycle occupies the existing29×9 area. Four-subcolumn raster coverage smooths the display, not the signal or intended transitions. TRI/SQR/PUL identify the real selected form; no DSP code changes. MIX/WAVE remains the post-master signal view.

## Focused checks

- `sh tests/host/ui_depth_wave_test.sh`:32,768 exact comparisons against unchanged private DSP functions; display bounds, fractional coverage and renderer nonmutation; actual native240 host screenshots.
- `sh tests/host/night_ui_test.sh`: retained content, cells, centering, states and labels. Pattern-centering test now identifies12-pixel cell edges rather than counting a short same-colour panel chamfer as a cell.
- `sh tests/host/miami_stock_rates_test.sh`: all30 stock presets, all8 labels, internal/external tempo and synthetic project/sample/panel preservation.
- `sh tests/host/smc_ui_test.sh`: retained controller mappings/navigation/overlays.
- Fresh controller, Elite and Pro builds are BUILD_ONLY evidence; no hardware or publication.

No migration, custom-preset system, layout redesign, recovery, device access or boot animation. Geometry manifest and before/after renders are handed to the existing coordinator; preview acceptance remains separate from source review.
