# DnBOX U3 — bounded startup entry, source-only checkpoint

**Status:** integration primitives and tests, **not production-wired**. This
continues the exact Blender-derived **U2** render at `21261737`, rather than
creating a new visual. Firmware UI is owned separately by #891, website by #890.

## Recovered real assets

| Artifact | Exact value |
|---|---|
| Original `DnBOX-v13r2.blend` SHA-256 | `eb6ab4f5740cf916b65515b8fa8be7fdd3d5c506647f8f769e44bffd6db2cc62` |
| Approved short render commit | `21261737ed2118b73253e974a50d00f8f79b3331` |
| MP4 | `assets/branding/box-entry/DnBOX-box-entry-short.mp4`, 90,324 B |
| First/last PNG | `DnBOX-box-entry-short-first.png` / `DnBOX-box-entry-short-last.png` |
| Duration | 36 / 70 s = 0.514286 s; 36 frames, 70 fps, 480 × 480 |
| Last-frame HOME | exact pixels of the `94197fc8` production HOME, SHA-256 `e847e1d6635f3b85d6e14111ba7622e4cb570dbfd0f3908535c942d54a5cb942` |
| Original scene | 192 frames / 2.742857 s, Blender 4.3.2, saved privately; **not** published by this change |

The existing film is a host preview, NOT supported MCU video format. The
firmware's `boot_screen.h` accepts a **static 240×96, 1-bit** optional image
with text. It cannot play H.264. Keep the source scene private and unchanged.

## Actual source contribution

`firmware/src/app/box_entry_scheduler.h` is a header-only, main-loop-only timing
contract: selects at most one frame per call, fast-forwards dropped frames,
restores the real HOME at 515 ms, and immediately skips on any user input,
reduced-motion or asset validation failure. **No wait/sleep/audio callback,
heap, flash write, or build change**. It does not run on the device until
approved frame data and a verified foreground UI hook are integrated.

`validate_asset.py` rejects mismatched source bytes, wrong resolution, wrong
frame count/fps/duration, absent/mismatched target HOME, or an audio track;
it emits source diagnostics only. Run in a source checkout with ffprobe:

```bash
python3 tools/box_entry/validate_asset.py
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 tests/box_entry_scheduler_test.c -o /tmp/dnbox-box-entry-scheduler-test
/tmp/dnbox-box-entry-scheduler-test
python3 -m unittest discover -s tests -p 'test_box_entry_asset_gate.py' -v
```

## Resource and integration admission

Source `94197fc8` has previously reported **8,440 B Elite** / **8,448 B Pro**
application flash free. The firmware minimum is **8,192 B** free on **both**;
only **248/256 B** is spare over that reserve at this checkpoint. The H.264
film is **90,324 B**. It cannot be embedded as-is, and a decoder would add
still more code. An uncompressed RGB565 240×240×36 series is **4,147,200 B**.
These are static capacity calculations, **not** new exact native builds.

The only valid future device integration is: (1) obtain the *current* #891
accepted HOME framebuffer/renderer and verify last-frame pixel identity;
(2) prove a genuine bounded native frame representation, including decode,
flash ≥8192 B and pool ≥8192 B on both models; (3) connect the scheduler
inside the ordinary UI foreground after audio/USB/input initialization, never
before them; (4) immediate input/reduced-motion skip, restore the live HOME,
no audio/encoder/MIDI blocking and no extra startup delay; (5) exact device
load/dropout/UI responsiveness testing. Skip animation and render HOME directly
when any admission check fails. Safe mode, recovery and updater are excluded.

The existing public short film may be used for **website** entry only with
#890's explicit ownership, loading and reduced-motion review. No installer
HTML/CSS/JS or existing U1 UI file is changed by U3.

## Acceptance boundary

- **Verified existing evidence:** Blender asset was already rendered, linked
  and checked into the branch at `21261737`; exact sizes and hashes above are
  preserved from its source `plan.md`.
- **Local focused checks:** pure C foreground scheduler and Python gate
  synthetic-input tests (record output per actual run).
- **Not run here:** native compile, firmware sizing, integrated framebuffer
  proof, dynamic asset gate against the MP4 (if unavailable), hardware/audio.
- **Owner-review decision:** preserving the exact Blender film on the website
  versus authorizing a new compact firmware-specific representation. These are
  separate product surfaces; neither requires shipping an MCU H.264 player.
