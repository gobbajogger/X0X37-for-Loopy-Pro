# Glyph polish: real framebuffer comparisons

BEFORE is PR [#743](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/743), commit `e5a75c7c8e7f1b21baa1476b5fbf635b29017480`. The reference mockups are PR [#744](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/744), commit `51b2180e3eecd57b7e3a40900aca6ad7519c2223`.

AFTER comes from the changed production UI and engines. The exact consumed source-file SHA-256 values are recorded in [render-inputs.json](render-inputs.json). The render harness is identical for both phases. All 1,987 sampled frames per phase are represented by the sheets below: 47 series, 24 animated control families and 23 individual original-engine controls. Raw PNG frames and executables remain in scratch and are excluded from the PR.

The host exports the actual SMK-37 **240 × 240 RGB565 framebuffer** as RGB PNGs. The production SOUND material retains its native green tint, even with the MONO palette selected. Native images preserve those pixels. The separate, labelled [1-bit readability masks](mono-role-atlas-01.png) threshold actual pixels at Rec.709 luma **128/255**; they do not redraw the glyphs or alter production colours.

## Reading the comparisons

Each GIF shows BEFORE above AFTER, with representative values across the columns. Five-value comparisons use exact pots **0, 32, 64, 96, 127**, labelled approximately **0, 25, 50, 75, 100%**. The single-setting Chicago LEVEL/BITE and Miami LEVEL/WOBBLE examples use pot **100** (about 79%), grouped under the nominal 75% label; native readouts retain the actual value. Discrete controls retain their actual quantized switch positions.

For Chicago ENV these pots span the bipolar depth: the centre pot **64 is exactly zero**; lower settings move the knee left and higher settings move it right. Native quantization makes the positive 75% example read **+51%**. PEAK sets height; ENV moves the curve horizontally with the real filter envelope. Zero ENV remains stationary.

Frame `f00` is idle. A note or drum hit is requested immediately afterwards. Bass note-off is requested at **500 ms**. The source samples are at **0, 8, 16, 24, 40, 60, 90, 120, 160, 220, 300, 420, 580, 800, 1100, 1500, 2000 and 2600 ms**. Chicago live sequences explicitly use ENV mode and disable ARP so the real VCA stages are visible.

GIF playback follows the sampled time intervals, with **160 ms idle dwell**, **300 ms end dwell**, and a **20 ms minimum frame duration** for the earliest samples. Thus the first few milliseconds play slightly slower; source timestamps remain labelled. The complete cycle is 3,080 ms. Identical GIF frames may be consolidated into a longer hold; the every-frame sheets retain all 18 source samples. The engine envelopes themselves are neither stretched nor synthesized.

The `*-frames-*.png` sheets include **every source frame** as nearest-neighbour glyph crops, split to retain legibility. `*-values.png` files show native full screens at representative values, sampled at 90 ms. The original-control atlases consolidate all static frames and label each engine, control name and parameter ID.

## Gallery

| Family | Animated BEFORE / AFTER | Native value sheet | Every-frame sheets |
| --- | --- | --- | --- |
| 808-accent | [GIF](808-accent-before-after.gif) | [Values](808-accent-values.png) | [1](808-accent-frames-01.png) · [2](808-accent-frames-02.png) |
| 808-attack | [GIF](808-attack-before-after.gif) | [Values](808-attack-values.png) | [1](808-attack-frames-01.png) · [2](808-attack-frames-02.png) |
| 808-decay | [GIF](808-decay-before-after.gif) | [Values](808-decay-values.png) | [1](808-decay-frames-01.png) · [2](808-decay-frames-02.png) |
| 808-level | [GIF](808-level-before-after.gif) | [Values](808-level-values.png) | [1](808-level-frames-01.png) · [2](808-level-frames-02.png) |
| 808-shell-tone | [GIF](808-shell-tone-before-after.gif) | [Values](808-shell-tone-values.png) | [1](808-shell-tone-frames-01.png) · [2](808-shell-tone-frames-02.png) |
| 808-snappy | [GIF](808-snappy-before-after.gif) | [Values](808-snappy-values.png) | [1](808-snappy-frames-01.png) · [2](808-snappy-frames-02.png) |
| 808-sound | [GIF](808-sound-before-after.gif) | [Values](808-sound-values.png) | [1](808-sound-frames-01.png) · [2](808-sound-frames-02.png) |
| 808-tone | [GIF](808-tone-before-after.gif) | [Values](808-tone-values.png) | [1](808-tone-frames-01.png) · [2](808-tone-frames-02.png) |
| 808-tune | [GIF](808-tune-before-after.gif) | [Values](808-tune-values.png) | [1](808-tune-frames-01.png) · [2](808-tune-frames-02.png) |
| 909-accent | [GIF](909-accent-before-after.gif) | [Values](909-accent-values.png) | [1](909-accent-frames-01.png) · [2](909-accent-frames-02.png) |
| 909-attack | [GIF](909-attack-before-after.gif) | [Values](909-attack-values.png) | [1](909-attack-frames-01.png) · [2](909-attack-frames-02.png) |
| 909-decay | [GIF](909-decay-before-after.gif) | [Values](909-decay-values.png) | [1](909-decay-frames-01.png) · [2](909-decay-frames-02.png) |
| 909-level | [GIF](909-level-before-after.gif) | [Values](909-level-values.png) | [1](909-level-frames-01.png) · [2](909-level-frames-02.png) |
| 909-pitch | [GIF](909-pitch-before-after.gif) | [Values](909-pitch-values.png) | [1](909-pitch-frames-01.png) · [2](909-pitch-frames-02.png) |
| 909-pitch-depth | [GIF](909-pitch-depth-before-after.gif) | [Values](909-pitch-depth-values.png) | [1](909-pitch-depth-frames-01.png) · [2](909-pitch-depth-frames-02.png) |
| 909-snappy | [GIF](909-snappy-before-after.gif) | [Values](909-snappy-values.png) | [1](909-snappy-frames-01.png) · [2](909-snappy-frames-02.png) |
| 909-tone | [GIF](909-tone-before-after.gif) | [Values](909-tone-values.png) | [1](909-tone-frames-01.png) · [2](909-tone-frames-02.png) |
| 909-tune | [GIF](909-tune-before-after.gif) | [Values](909-tune-values.png) | [1](909-tune-frames-01.png) · [2](909-tune-frames-02.png) |
| chicago-adsr | [GIF](chicago-adsr-before-after.gif) | [Values](chicago-adsr-values.png) | [1](chicago-adsr-frames-01.png) · [2](chicago-adsr-frames-02.png) · [3](chicago-adsr-frames-03.png) · [4](chicago-adsr-frames-04.png) · [5](chicago-adsr-frames-05.png) · [6](chicago-adsr-frames-06.png) |
| chicago-bite | [GIF](chicago-bite-before-after.gif) | [Values](chicago-bite-values.png) | [1](chicago-bite-frames-01.png) · [2](chicago-bite-frames-02.png) |
| chicago-env | [GIF](chicago-env-before-after.gif) | [Values](chicago-env-values.png) | [1](chicago-env-frames-01.png) · [2](chicago-env-frames-02.png) |
| chicago-level | [GIF](chicago-level-before-after.gif) | [Values](chicago-level-values.png) | [1](chicago-level-frames-01.png) · [2](chicago-level-frames-02.png) |
| miami-level | [GIF](miami-level-before-after.gif) | [Values](miami-level-values.png) | [1](miami-level-frames-01.png) · [2](miami-level-frames-02.png) |
| miami-wobble | [GIF](miami-wobble-before-after.gif) | [Values](miami-wobble-values.png) | [1](miami-wobble-frames-01.png) · [2](miami-wobble-frames-02.png) |

All original-engine controls: [1](original-controls-atlas-01.png) · [2](original-controls-atlas-02.png) · [3](original-controls-atlas-03.png) · [4](original-controls-atlas-04.png) · [5](original-controls-atlas-05.png) · [6](original-controls-atlas-06.png).

1-bit role atlases: [1](mono-role-atlas-01.png) · [2](mono-role-atlas-02.png) · [3](mono-role-atlas-03.png). These use the 90 ms sample and discard dim ghosts below the stated threshold.

## Renderer verification

Both real host render runs completed with **124 assertions passed**. The final composition has **108 checks passed, 0 failed**: nonzero live-control motion, stationary zero ENV and distinct signed ENV depth sequences. Zero level and zero snappy are allowed to remain silent. [Motion measurements](motion-measurements.json) contain glyph-region frame hashes, per-step changed-pixel counts and unique-frame counts. [Value differences](value-differences.json) compare adjacent representative settings. [Render validation](render-validation.json) records each check separately.

The primary agent inspected every sampled source frame across all 58 every-frame sheets/static atlases; [visual-inspection.json](visual-inspection.json) pins the reviewed image hashes. Motion and representative values are visibly distinct, with intentional stationary zero ENV and silent zero-level/noise exceptions.

[Verification results](verification.json) record all 10 passing host UI checks, successful native Elite/Pro SDK V1.2.1 development builds, exact source hashes and the resolved independent code review. This is host and build evidence; no device or hardware-test claim is made.

To reproduce, run the same harness in a source checkout at the BEFORE revision and in the changed checkout, then compose the scratch output directories:

```sh
sh tests/host/ui_glyph_polish_render.sh /absolute/scratch/before
sh tests/host/ui_glyph_polish_render.sh /absolute/scratch/after
python3 tools/render_glyph_polish.py --before /absolute/scratch/before \
  --after /absolute/scratch/after --output docs/mockups/glyph-polish
```

The harness is added by this PR: copy its two test files into the unchanged BEFORE checkout for the first command. Both runs use the production SMK-37 UI and `engine_render`; only hardware interfaces use the existing host fixture substitutes. No FM-1 target is built or tested.
