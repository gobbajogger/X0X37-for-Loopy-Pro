# DnBOX box-entry startup — plan

Prepare a short startup from the owner's original saved Blender scene so the box opens and lands on the current 94197fc8 HOME, not the old HOME. Startup stays fast: only the short box entrance, with no review hold, no forced delay, and no firmware boot delay.

Work is on copies under `/workspace/box-entry/work/` only. Originals in `/workspace/box-entry/originals/` are read-only and were not modified. No site page edits, no commits or pushes, no firmware or device writes, no publishing, no main merges.

## 1. Inputs verified

SHA256SUMS in `/workspace/box-entry/originals/SHA256SUMS` all matched:

| File | SHA-256 |
|---|---|
| `DnBOX-original-project.zip` | `2829773373e673811e0b02fc0f01447830d4d96ae7e995ae3b2ae5bfbacd8e13` |
| `DnBOX-short-entry-PROPOSAL-36f.mp4` | `bde6f035c6a09a29fded0f2f47157119ec21cec5ace351922d83647270413f1f` |
| `DnBOX-v13r2.blend` | `eb6ab4f5740cf916b65515b8fa8be7fdd3d5c506647f8f769e44bffd6db2cc62` |

The `.blend` inside the unzipped project is the same hash as the standalone file. It is zstd-compressed and opens in Blender 4.3.2, as the project README requires.

## 2. Current length (confirmed from the scene)

The saved scene and the packed r2 preview are **192 frames at 70 fps = 2.742857 s** (two 175 BPM bars). Confirmed in:

- the opened copy of `DnBOX-v13r2.blend`: `frame_start=1`, `frame_end=192`, `render.fps=70`, resolution 480×480
- `sources/scene-proof/scene-parameters.json` (`fps: 70`, `frames: 192`)
- `evidence/full-render-status.json` and `evidence/video-final-check.json`
- the project README
- the preview file `preview/DnBOX-v13r2-preview.mp4` (192 frames, 70 fps, 2.742857 s, 480×480, H.264 High, yuv420p, plus AAC audio)

The scene custom property `timeline_summary` and `sources/scene-refinement-r2/refine_current_scene.py` line 65 agree:

`8 beats: open1-36; WAVE37-48; SLICE49-72; FX73-96; HOME97-120; logo121-168; final169-192`

The **box entrance is frames 1–36 at 70 fps = 0.514286 s**. In the saved animation the lid starts moving at frame 6–8, is fully rotated and the box is face-on at frame 26, and the lid has folded back by frame 36. That 36-frame opening is the piece to keep.

The original 192-frame film is too long for startup. After the opening it tours WAVE, SLICE and FX, brings in the wordmark, and only finishes landing the old HOME at frame 169, with a final hold through 192.

The proposal file `DnBOX-short-entry-PROPOSAL-36f.mp4` is **92 frames at 70 fps = 1.314286 s**: the 36-frame entrance (about 0.514 s) plus a 56-frame review hold (about 0.8 s). The review hold is not used for startup.

At original frame 36 the landed page is still the old MIX/WAVE view, not HOME. HOME components only start arriving at frame 97 and are settled at 169. Cutting 1–36 without a landing swap would end on the wrong page. The old HOME itself is the earlier host fixture (BRK / CHI / MIA, pattern cells and knobs from source `b57ef24`), not the 94197fc8 HOME.

## 3. Target length

- **36 frames at 70 fps = 0.514286 s**
- Last frame: the current 94197fc8 host HOME
- No review hold, no extra tail, no forced delay, no firmware boot delay
- Audio, if muxed, is only the matching 36-frame opening and is not required for the startup picture

## 4. Frames and sections to cut

Keep only the box entrance into the current UI:

- **Keep** original frames **1–36** (the confirmed opening).
- **Cut** original frames **37–192**: the WAVE / SLICE / FX page tour, the logo phrase, the late HOME arrival, and the final hold.
- **Do not include** the proposal's 56-frame review hold.
- Do not add any new hold after the landing.

The camera itself does not move. The saved camera is `fixed perspective camera` at `(0, 0, 5.4)`, 80 mm, 36 mm sensor, horizontal fit, **no camera keyframes**. The entry is the box opening toward that camera (lid hinge, box scale and squaring-up, walls retracting).

## 5. How the final frame matches the current UI

The current target is the left panel of `/workspace/handoff-dot-20261113/firmware/before-after-home.png` (1040×600), captioned “240 × 240 at 2x · current: 94197fc8 host UI”. That left framebuffer was cropped at pixel origin **(24, 58)** as a 480×480 2× nearest-neighbour copy of the native 240×240 HOME. The 480 and 2×-scaled 240 images are byte-identical.

Canonical work copies:

- `/workspace/box-entry/work/HOME-94197fc8-240.png` — 240×240, sha256 `f8d74170b57756c4655c7fcecd2204bf05d3d576eee2fbd12d419677156d58cf`
- `/workspace/box-entry/work/HOME-94197fc8-480.png` — 480×480, sha256 `e847e1d6635f3b85d6e14111ba7622e4cb570dbfd0f3908535c942d54a5cb942`

That screen is the current production HOME: title HOME, pattern `# 1`, 125 BPM, lanes 909 / 808 / BREAK / CHICAGO / MIAMI, PERFORM with TEMPO 125 bpm, SWING 50 %, FILTER OFF, PUMP 0.0 dB, and the footer `SMC F1-5: PART LEVEL F6: REV F7: DLY F8: FILT`.

The original scene has **no FILE image textures** (the HOME is generated pixel-run mesh with an unlit emission material, “Actual UI source pixels / unlit display surface”). Swapping is done on a work copy by:

1. Hiding collections `02_SHARED_UI`, `03_WAVE`, `04_SLICE`, `05_FX`, `06_HOME_DESTINATION`, and `07_DnBOX` (old page tour, old HOME, later logo).
2. Adding an unlit emission plane at `Z = 5.4 - 2.4×80/36` (the landed HOME depth, 0.066…), size 2.4×2.4, which fills the fixed camera, mapped to `HOME-94197fc8-480.png` with closest interpolation.
3. Leaving the original box, lid and lighting in place so frames 1–36 still open the box onto that screen.

## 6. Render settings (match the original)

From the opened `.blend` and `render_full_preview.py` / `full-render-status.json`:

| Setting | Value |
|---|---|
| Resolution | 480×480, 100 % |
| Frame rate | 70 fps |
| Frame range | 1–36 (was 1–192) |
| Engine | Cycles, CPU, 48 samples, denoising off |
| Motion blur | on, shutter 0.38 |
| Colour | Standard view transform, look None, RGB 8-bit PNG |
| Filter | Cycles BOX, size 0.5, dither 0 |
| Movie | H.264 High, yuv420p, 70 fps (same family as the r2 preview / proposal encodes) |

## 7. Output paths

This prep render goes to `/workspace/box-entry/render/`:

- shortened MP4
- first-frame PNG
- last-frame PNG (must show the current 94197fc8 HOME)

A later checked-in preview, if any, belongs on the existing assets branch `assets/dnbox-box-entry-preview-20261113` (head `c3d52d24de66cb88d6160dc7b94e12adc94f48ca`), next to `assets/branding/DnBOX-box-entry-first-frame.png`. This pass does not commit, push, or open a pull request. DnBOX Website UX owns any site page that might later use the film; do not edit site pages here.

## 8. Repo usage of the existing first-frame preview

Branch `assets/dnbox-box-entry-preview-20261113` of `gobbajogger/X0X37-for-Loopy-Pro`, head **`c3d52d24de66cb88d6160dc7b94e12adc94f48ca`** (13 Nov 2026, 01:50 AM CST), parent `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Versus that parent the branch adds only `assets/branding/DnBOX-box-entry-first-frame.png` (240×240 RGB, 65,191 bytes, blob `28fe787c6f31`, sha256 `5e2a46c23bb184b908fa1528544870b08b61d34b6555b267003d3f9a87e194c0`). Versus default `main` (`035026fe70059af35e25c7c121b40f318b732fe3`) the branch is diverged (ahead 90 / behind 48) because it was cut from `94197fc8`, not because of other asset edits. There is no pull request from this branch.

A tree search of that branch and of `main`, plus a content search of the exact `94197fc8` source, found **no use** of this preview or of a box-entry animation in website pages, the installer, firmware boot, or docs. The filename does not appear in CSS, JS, or boot-splash timing. The 240×240 still is the closed-box first frame of this Blender scene (DnBOX wordmark on the lid), stored for review only.

Firmware `boot_screen` is a **different** system: a static 240×96 1-bit still plus two text lines (`firmware/src/app/boot_screen.h`, drawn once in `main_fm1.c` at the `draw_boot_screen` call). It is not this animation, has no video player (`STATUS.md` item 72), and is not wired to the Blender preview. Historical request NS025 (“DnBOX3D box entrance…”) is marked `SUPERSEDED_FOR_FIRMWARE_BOOT` in `docs/owner-requested-features.md`. This plan does not change firmware or boot timing.

## 9. Blender on the box

Blender was not already installed (`which blender` empty; Debian candidate 4.3.2+dfsg-2 was not used). An official free Linux x64 build was downloaded to `/workspace/box-entry/tools/`, without root, from `https://download.blender.org/release/Blender4.3/blender-4.3.2-linux-x64.tar.xz`.

- Published checksum (`blender-4.3.2.sha256`): `4da1c956673c0485e63054e563ee69198cc8f80d8157dd7592dffc8a6a5592e6  blender-4.3.2-linux-x64.tar.xz`
- Local sha256 of the tarball: **matched**
- Version: **Blender 4.3.2**, build hash `32f5fdce0a0a`, branch `blender-v4.3-release`, 16 Dec 2024, Linux Release

That binary opened a work copy of the `.blend` headless and is used for the shortened render.

## 10. Render receipts (this pass)

All 36 entrance frames were rendered in Cycles on a work copy (`/workspace/box-entry/work/DnBOX-v13r2-short.blend`) with the original settings (480×480, 70 fps, 48 samples, motion blur 0.38). Total render time was 80 s. The last Cycles frame already showed the current HOME, with a tiny filter residual versus the exact crop (48 of 230,400 pixels, max channel error 13, 99.979 % exact). As planned, that last PNG was replaced with the exact 480×480 94197fc8 crop so the delivered landing is pixel-identical. No review hold and no audio delay were added.

| Output | Size | SHA-256 |
|---|---|---|
| `/workspace/box-entry/render/DnBOX-box-entry-short.mp4` | 90,324 bytes | `566ee16ad25c826874f9c536b0c3cf56fa3269be7e32469331864b27b653e7d4` |
| `/workspace/box-entry/render/DnBOX-box-entry-short-first.png` | 177,762 bytes | `814d5729cc2562de1984683222d66dea8faaa906dd81e081c4852aba560dfd52` |
| `/workspace/box-entry/render/DnBOX-box-entry-short-last.png` | 13,218 bytes | `e847e1d6635f3b85d6e14111ba7622e4cb570dbfd0f3908535c942d54a5cb942` |

The MP4 (ffprobe, `-count_frames`): **36 frames**, **70 fps**, **0.514286 s**, 480×480, H.264 High, yuv420p, no audio. Encoder family matches the r2 preview (`Lavc61.19.101 libx264` / `Lavf61.7.103`). The last PNG’s sha256 is identical to `HOME-94197fc8-480.png`. The first PNG is the closed box with the DnBOX wordmark, current HOME already visible around the lid.

## 11. Still open

- No site hookup until Website UX owns that step.
- No commit or push of the new preview onto `assets/dnbox-box-entry-preview-20261113` until the owner asks.
- Originals in `/workspace/box-entry/originals/` were not modified.
