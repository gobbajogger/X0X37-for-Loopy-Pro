# SMK-37 APC40 mkII / Loopy Pro implementation notes

## Implemented in source

This section shows the current behaviour (`1.8-apc40`). The version sections below are history. Some statements in them are no longer true.

- Added a Universal MIDI Device Inquiry response with Akai manufacturer ID `0x47` and APC40 mkII product ID `0x29`, so Loopy Pro can auto-identify the SMK as its supported APC40 mkII controller.
- Added the APC40 mkII introduction-message response, with complete-message queue reservation so a full MIDI queue cannot emit a truncated SysEx reply.
- Replaced the local simulated clip state with host-driven LED feedback. The source retains all 40 APC logical pad IDs (`0x00..0x27`) and uses the APC velocity palette (128 colors) for static RGB rendering. Clip colors are accepted on MIDI channels 0–15; the channel does not change a clip cell's color.
- The firmware trims empty outer APC margins and keeps internal empty cells in their positions. A footprint of 3 or 4 rows and 4 or fewer columns folds onto one 8×2 view (pad order: see `0.9-apc40` and `1.7-apc40`). Other layouts show one local 8×2 view at a time.
- The physical pad rows are vertically flipped in clip mode. Columns stay left to right. A KNOB BANK press toggles clip mode.
- In clip mode, `<` and `>` do not send APC LEFT or RIGHT. They step the selected field of an open SCALE / CHORD / ARP page (BEHAVIOR.md CLP-76). KNOB BANK exits on press. FADER BANK switches the fader bank. The firmware sends no APC navigation messages.
- OCT− and OCT+ move the local 8×2 view down and up by 2 rows. The move stays in the latched footprint. To see clips outside the Loopy APC box, change the Loopy page.
- Pad hits do not trigger X0X pad functions. Keybed input stays available.
- The layout latches 250 ms after the first feedback burst. It cannot shrink, so cue and play color changes cannot move pads. It grows only to complete a 4×4 fold (`smk_apc40_layout_grows` in smk_apc40.h). 8-wide layouts stay as latched. A page snapshot clears old-page cells (rules: see `0.9-apc40` and `1.0-apc40`).
- The USB receive interrupt stores APC clip and transport feedback at all times, also before clip mode. Thus the pads show the correct colors at clip mode entry.
- APC clip notes (0x00-0x27) and transport feedback notes go to the sound engine only when clip mode is off. In clip mode the firmware keeps them from the sound engine. Before `1.8-apc40` follow-up, the firmware also kept them from the sound engine after APC host identification.
- Pads: a lit pad shows the Loopy color at half level. There is no brightness floor on the pads. The off phase of a queued blink is fully off.
- LCD clip map: a lit cell shows the LCD color for its velocity (`APC40_LCD565` in apc40_lcd565.h, made by tools/apc_pad_colors.py from the Akai palette tools/apc40mk2_akai_protocol_v1.2.csv). Each entry is the Akai color as plain RGB565 (no lift). An empty cell is a small grey dot. (Before 2026-10-16: `SMK_CLIP_LCD_FLOOR` mapped the brightest channel from 0-255 to 128-255.)
- In clip mode, X0X does not echo a MIDI Start, Continue or Stop from Loopy back to the host. With clip mode off, the transport behavior does not change.
- Preserved the normal pad row-swap and LED-chain index mapping outside clip mode.
- Seq PLAY and Seq REC send APC40 PLAY (`0x5B`) and RECORD (`0x5D`) momentary MIDI notes while clip mode is active. Their LEDs follow Loopy feedback for playback and recording/overdub state. Outside clip mode, the original sequencer play and record behavior remains active. The active Loopy Pro Record binding controls the target and overdub behavior.
- Extended the web installer generator to accept release identities for FM-1, SMK-37 Pro, SMK-37 Elite, and MKE-P37. Generated SMK installer pages use the target model name and hide the FM-1-only stock restore/recovery controls.
- Added `tools/build_smk_release.sh VERSION` to build both model-specific release packages and generate a matching installer site for each when the required toolchain and SDK are installed.

## Verification performed

Earlier verification for release `0.2-apc40`:

- `sh tools/build_smk_release.sh 0.2-apc40` built Pro (`SMK-37 Pro_9000200`) and Elite (`SMK-37 Elite_9000200`), generated both installer sites and ZIPs, and passed the package integrity checks.
- The SHA-256 manifest verified both firmware packages and installer ZIPs. Each ZIP's embedded firmware was byte-for-byte identical to the corresponding model package.
- `python3 tests/smk_apc40_test.py`
- `python3 tests/make_pages_test.py` and `python3 tests/gen_font_test.py`
- the C APC feedback fixture and mapping tests (`tests/smk_apc40_map_test.c`), covering 8×4, 4×4, sparse cells, feedback removal, viewport refresh, row reversal, and page boundaries
- the firmware target build checks: RAM use, no soft-double routines, RAM-text restrictions, and register-access checks
- `sh tests/run_tests.sh`: the SMK APC map, installer targets, and all other runnable tests passed. It returned three failures because the sibling 9W9, 8W8, and schwung-303 reference source checkouts are absent. FM-1 update-path checks and stock SMK package tests were skipped because this task built SMK release packages rather than `build/x0x.fwsc`.
- `git diff --check`

The build ran on macOS through its running Colima Linux x86_64 Docker guest. The host Pillow package lacks `libraqm`, so a tested fallback centers digits in tabular cells while retaining the Barlow font set.

## Corrected release `0.3-apc40`

- Built Pro (`SMK-37 Pro_9000300`) and Elite (`SMK-37 Elite_9000300`) with the macOS Colima Docker path. Both passed package checks; installer ZIPs and SHA-256 manifest were created and verified.
- Installed the Elite package to the connected SMK-37 Elite. `fm1_install.py` completed and verified the running identity as `SMK-37 Elite_9000300`.
- Sent a Universal MIDI Device Inquiry to the installed keyboard. It returned the APC40 MkII identity (`0x47` / `0x29`, version 1.2).
- Sent a synthetic batch of all 40 APC clip-cell color Note On/Off messages over MIDI channels 0–15 to the connected device. The input endpoint remained active and emitted no extra MIDI messages during the fixture.
- Loopy Pro is open to an 8×4 workspace, but no Loopy-to-keyboard feedback was observed during a passive CoreMIDI capture. The test batch exercises the actual USB-MIDI input path; visual LED colors and live clip launches remain unverified because the current workspace did not provide live clip feedback to observe.

## Protocol reference

The layout assumptions follow the [Loopy Pro manual](https://loopypro.com/manual/), which documents APC40 mkII support and customizable variable-size canvas layouts, and the [Akai APC40 mkII Communications Protocol v1.2](https://cdn.inmusicbrands.com/akai/attachments/apc40II/APC40Mk2_Communications_Protocol_v1.2.pdf), which defines 40 clip note IDs and the four navigation controls. The APC feedback does not report project dimensions, so this build treats the reported cells as the current host viewport.

## Spatial mapping, transport, and cue stability follow-up

- Replaced color compaction with coordinate-preserving maps. The 8×4 fixture maps to two 8×2 pages with the original APC notes. The 4×4 fixture maps to one page, with source rows 0-1 on the physical left block and rows 2-3 on the right block.
- Black feedback cells remain in their positions and pad presses on those cells are ignored. Sparse cells no longer shift neighboring clips.
- Viewport movement now requires at least one colored clip in the current feedback view. Once the view has no colored clips, further movement outward is blocked. Movement back toward the view where the navigation began stays enabled.
- The APC protocol has no project-size or empty-slot metadata. The firmware infers its active footprint from nonzero clip colors. It recognizes a 4-column layout when clips appear in four rows and no active cell appears past column four. Empty outer rows or columns cannot be distinguished from project bounds.
- A 250 ms latch after the first feedback message prevents clip-state changes from shrinking the inferred footprint. Viewport changes clear the latch before new host feedback arrives.
- Seq PLAY sends APC40 Play (`0x5B`), and Seq REC sends APC40 Record (`0x5D`), as momentary messages in clip mode. The firmware sends Note Off on button release and leaves the ordinary X0X sequencer behavior in place when clip mode is off.
- Release `0.5-apc40` built Pro (`SMK-37 Pro_9000500`) and Elite (`SMK-37 Elite_9000500`) with target memory, RAM-text, soft-float, and register-access checks passed. Both firmware packages passed `smk_pkg_test.py`.
- Both installer sites and ZIPs were created. SHA-256 checks passed for both firmware packages and installer ZIPs. A ZIP inspection confirmed that each installer embeds the matching model firmware byte-for-byte.
- `tests/smk_apc40_test.py` and the C mapping fixture passed. The full `tests/run_tests.sh` suite passed all available mapping, installer, package, and scenario checks. Three audio reference tests could not run because sibling `schwung-9W9`, `schwung-8W8`, and `schwung-303` sources are absent. FM-1 update-path tests were skipped because this build contains SMK packages only.
- The Elite package was installed on the connected unit. `fm1_install.py --info` reports `SMK-37 Elite_9000500`. A MIDI Universal Device Inquiry returned APC40 MkII identity `0x47/0x29`, protocol version 1.2. A synthetic fixture sent Note On and Note Off for all 40 clip IDs on channels 0–15, then confirmed that the keyboard still answered inquiry.
- Loopy Pro emitted no MIDI feedback during a passive 1.5-second capture. Hardware LED colors, clip launches, and live Loopy clock/overdub behavior therefore remain unverified end-to-end. The code and mapping fixtures cover their MIDI messages and pad positions.

## Viewport and transport LED correction (`0.6-apc40`)

- Removed local 16-cell pages. The four APC direction controls now own Loopy viewport navigation; 8-wide grids show the current viewport's first two rows, and a 4×4 grid folds into a single 8×2 pad view.
- The layout records the first occupied APC row and column. This trims empty viewport margins so an 8×4 project that begins one row down still has four relative rows. Empty cells inside that area stay in place.
- Navigation stages incoming feedback while keeping the last complete map lit. The new viewport is committed as one snapshot after the feedback burst settles, preventing blank pads and cue-time row shifts.
- APC Play (`0x5B`) and Record (`0x5D`) feedback now drives the physical PLAY and REC LEDs in clip mode. The normal X0X LED state resumes outside clip mode.
- Loopy Pro documents same-message MIDI state feedback for bound controls; the APC40 MkII protocol defines its 40 clip note IDs, Play/Record LEDs, and four viewport buttons. References: [Loopy Pro MIDI state feedback](https://loopypro.com/manual/#midi-state-feedback) and [Akai APC40 MkII protocol v1.2](https://cdn.inmusicbrands.com/akai/attachments/apc40II/APC40Mk2_Communications_Protocol_v1.2.pdf).

## Clip entry, local views, and transport LEDs (`0.7-apc40`)

- The firmware stores APC clip and transport feedback at all times. Clip mode entry latches the layout at once. The pads show the stored colors.
- The firmware sends no APC navigation notes. Local 8x2 views replace Loopy viewport movement.
- An 8x4 footprint has two views: rows 0-1 and rows 2-3. An 8x5 footprint has three views. The last view shows rows 3-4.
- Physical `<` and `>` select the previous or next view. The move stops at the first and last view of the clip footprint.
- A short KNOB BANK tap does nothing in clip mode. FADER BANK pins the clip keys page (CLP-76). A long KNOB BANK hold still toggles clip mode. OCTAVE buttons do nothing in clip mode.
- A footprint of 4 rows and 4 or fewer columns folds onto one view. Rows 0-1 go on pads 1-4 and 9-12. Rows 2-3 go on pads 5-8 and 13-16.
- A clip-mode press of PLAY or REC toggles its LED at once. Loopy feedback for notes `0x5B` and `0x5D` replaces the local state.
- Limit: a sparse 8-wide project with clips in 4 or fewer columns and 4 rows folds as a 4x4 grid. APC feedback has no project size data.

## Active page, 2D viewport, and clock blink (`0.8-apc40`)

- Root cause of stacked clips: Loopy sends the colors of the new page on a page change. It does not send "off" for cells of the old page. Release 0.7 stored all feedback, so old-page cells stayed lit.
- The firmware now detects a page snapshot. A feedback burst with gaps below 40 ms that touches 8 or more cells is a snapshot. Cells that the burst does not touch are cleared. The layout then latches again.
- Small bursts (cue, play, record) change only their cells. A page with fewer than 8 clips does not clear old cells.
- The project file shows the active page as a 12x4 canvas with a 4x4 clip grid at columns 8-11. Old 8-wide page cells made this grid look 8 wide. The 4x4 fold then failed and only pads 1-4 and 9-12 showed clips.
- A footprint of 3 or 4 rows and 4 or fewer columns folds onto one view. The latched layout can grow when late feedback arrives. It cannot shrink, so cue changes cannot shift pads.
- OCT- and OCT+ move the view up and down in steps of 2 rows. The move is clamped to the footprint. An 8x4 grid has 2 views.
- `<` and `>` send APC LEFT (`0x61`) and RIGHT (`0x60`) to Loopy. RIGHT is legal only when APC column 8 holds a clip. LEFT is legal only back to the start position.
- A short KNOB BANK tap does nothing in clip mode. FADER BANK pins the clip keys page (CLP-76).
- MIDI Clock and Start now drive the clip-mode LEDs. PLAY blinks on the first quarter of each beat. Pending clips (APC channels 6-15) blink on and off with the clock. Without clock they blink at 2 Hz. Static clips stay steady.

## Half swap and local arrows (`0.9-apc40`)

- 4x4 fold: rows 1-2 go on pads 5-8 and 13-16. Rows 3-4 go on pads 1-4 and 9-12.
- `<` and `>` do nothing in clip mode. They do not send APC LEFT or RIGHT to Loopy.
- Root cause of the 0.8 arrow faults: the firmware guessed a legal move from APC column 8. Loopy does not report clips outside its 8x5 box. A move to an empty area gave mixed old and new cells, and the layout latched from that mix.
- Root cause of breaks without a move: the 0.8 page snapshot cleared all cells that a burst did not touch. A cue or scene-launch burst of 8 or more clips cleared the other clips.
- A burst is now a page snapshot only when it touches 8 or more cells, holds no cue state (channels 6-15), and touches at least half of the lit cells.
- Limit: horizontal movement cannot reach clips outside the Loopy APC box. Change the Loopy page to see them.

## Restore 0.7 drawing (`1.0-apc40`)

- Only a footprint of exactly 4 rows and 4 or fewer columns folds. The 0.9 4x4 mapping does not change. All other footprints use the 0.7 8-wide drawing.
- The latched layout can grow only into a 4x4 fold. 8-wide layouts stay as latched, as in 0.7.
- A page snapshot does not reset the OCT view.
- A page snapshot must now also cover 3 or more rows and 3 or more columns. A launch of one or two rows or columns cannot clear clips.
- Limit: old-page cells stay lit after a change to a page with fewer than 3 rows or 3 columns of clips.

## 4x4 fit and queued blink (`1.1-apc40`)

- The 0.9 fold rule is back. A footprint of 3 or 4 rows and 4 or fewer columns folds. A 4x4 with a late or empty edge row still fills one view. 8-wide layouts keep the 0.7 drawing.
- Only APC blinking channels 11-15 blink. Loopy uses them for queued clips. Pulsing (6-10) and one-shot (1-5) states draw steady.
- Queued clips blink once per beat (24 clock ticks). Without clock they blink 250 ms on and 250 ms off.
- The APC rate table now follows the protocol: 4, 6, 12, 24, and 48 ticks. The old table used 1 tick for the 1/24 rate.

## Clip mode entry and beat blink (`1.2-apc40`)

- Capture: a virtual "APC40 mkII" port on the Mac received Loopy Pro output. Loopy sent the APC introduction, then about 240 events in 5 ms.
- The burst holds "off" for all 40 clip notes, track LEDs on channels 0-7, and all clip colors on channel 0.
- Loopy flashes a queued clip itself. It toggles the color and velocity 0 about every 100 ms on channel 0. This rate does not follow the BPM.
- Cause 1 (confirmed by capture): the fast pad blink is the Loopy 100 ms toggle. The old firmware drew each toggle.
- Fix 1: the firmware holds an "off" for 256 ms. An "on" in that time marks the cell queued. A queued cell blinks locally, one cycle per beat (24 clock ticks), on for half a beat.
- Cause 2 (assumed): the MIDI input queue holds 64 events. A 240-event Loopy burst can overflow it. Dropped events left old colors on the pads.
- Fix 2: the USB receive interrupt applies APC clip and transport feedback at once. These events do not go through the queue.
- Clock: Loopy Pro has "MIDI Clock Output" set to X0X FM-1. The clock count adds one per 0xF8 event from the single USB cable. Without clock, queued cells blink 250 ms on and 250 ms off.
- The unused staged-feedback arrays were removed. This gives RAM for the cell state.

## Filled, playing, and queued clips (`1.3-apc40`)

- Captures of Loopy Pro output show filled clips as the darkest shade of an APC palette group: 0x1F, 0x27, and 0x37 (index % 4 == 3). All were on channel 0.
- A queued clip that then started used the full shade 0x39 (index % 4 == 1). The firmware treats a full shade as playing. This rule comes from one clip only.
- Empty: pad off. Filled: dim breathing in the full shade of the clip hue. Playing: steady full color. Queued: one blink per beat.
- Breathing: triangle shape, brightness 12 to 72 of 255. One cycle is 2 beats (48 clock ticks) with live clock, else 2048 ms. All filled pads share one phase.

## 1.4-apc40

- The last color that Loopy sends for a cell is the truth. Velocity 0 or note off clears the cell at once.
- Loopy flashes a queued clip itself (color, then 0, about every 100 ms). Two off-to-on toggles within 256 ms each set the queued state. A queued cell keeps its color and blinks once per beat.
- The queued state stops 384 ms after the last flash message. If the last message was off, the cell clears.
- A Loopy refresh (note off for all 40 cells, then all colors) gives only one toggle. Thus it does not set the queued state.
- The firmware draws each cell in the color that Loopy sent. The 1.3 breathing is removed. Loopy sends the same dim shade for empty slots and for filled clips, so the firmware cannot find filled clips from APC feedback.
- Cell stores, cell aging and burst close occur only in the USB interrupt (usb_poll). The main loop only reads the cell state. The engine does not store clip feedback a second time.

## 1.5-apc40

- In clip mode, the LCD main area shows the 8x5 APC clip grid. A lit cell shows the color from Loopy. An unlit cell shows a dim dot. A queued cell shows a white center square. A white frame marks each cell that is on the 16 pads now (the OCT view, or the full 4x4).
- The main loop draws the grid (ui.c draw_main calls smk_clip_lcd_draw). The interrupt does not draw. The grid redraws when the cells or the view change, at most every 64 ms, and once per second. It uses the existing canvas. It adds no RAM buffer.
- When clip mode stops, the normal X0X view draws the full main area again at the next frame.
- The queued blink uses only the MIDI clock count: on for ticks 0-11, off for ticks 12-23 of each beat. MIDI Start sets the count to 0. Song Position (0xF2) sets the count to position x 6. If no clock came in the last 500 ms, a 250 ms timer is the fallback.
- Filled clips: capture on 2026-10-15 against Project.sqlite scene 8149. APC note = (4 - Loopy row) x 8 + column. Playing clips are 0x60. Filled clips that do not play and empty slots get the same dim column color (for example, filled cell note 24 and empty cell note 25 are both 0x0B). Thus the firmware cannot find filled clips. The dim breathing stays removed.
- Loopy sends the MIDI clock to the port "X0X FM-1" (preference "MIDI Clock Output"). The APC40 mkII port gets no clock.

## 1.6-apc40

- 4x4 layouts: the two pad rows of each half are swapped. The left and right halves do not change. Source rows count from the lowest APC row of the layout (source row 0 = the bottom Loopy row of the 4x4).
  - Pads 5-8 = source row 0. Pads 13-16 = source row 1.
  - Pads 1-4 = source row 2. Pads 9-12 = source row 3.
  - In Loopy rows (row 1 = top row of the 4x4): pads 9-12 = row 1, pads 1-4 = row 2, pads 13-16 = row 3, pads 5-8 = row 4. Loopy columns 1-4 go left to right in each group of 4 pads.
- LCD clip map: the map shows only the area that the pads use (the latched layout), not the full 8x5 grid. Empty cells in the area stay in their positions. A smaller area gets bigger square cells. The top Loopy row is on top. The white frame marks the cells on the pads now (the full grid for 4x4). With no clips, the map shows the full 8x5 grid.

## 1.7-apc40

- The 4x4 pad mapping is the same as in 1.5 again (the 1.6 pad row swap is removed). The user reported the swap on the screen, not on the pads.
- LCD map: the screen row comes from the source row only (smk_apc40_lcd_screen_row). The highest APC row (the top Loopy row) is on top. The 4x4 pad fold does not change the screen. Thus the screen shows the Loopy layout for 4x4 and 8-wide layouts. In 4x4 mode, the white frame is around the full grid.
- The 1.5 screen drew the highest APC row at the bottom. That was the upper and lower swap that the user saw.

## 1.8-apc40

- Loopy Undo and Redo in clip mode:
  - PATCH (X0X HOME) held + SEQ REC sends 9F 70 7F (note 0x70, channel 16). The release of SEQ REC sends 8F 70 00.
  - PATCH held + SEQ PLAY sends 9F 71 7F (note 0x71, channel 16). The release of SEQ PLAY sends 8F 71 00.
  - In these cases the firmware does not send the APC transport note (0x5B or 0x5D).
  - The release of PATCH does not change the X0X view.
  - The screen shows "LOOPY UNDO" or "LOOPY REDO".
- When clip mode is off, PATCH + SEQ REC and PATCH + SEQ PLAY are the X0X undo and redo, as before.
- Notes 0x70 and 0x71 are not APC40 mkII notes. The Loopy APC40 auto-map does not use them (assumed: the auto-map is in Loopy's code and has no public list).

### Loopy setup (one time)

1. In Loopy Pro, open Control Settings.
2. Select a profile. Use a global profile for all projects, or the project profile for this project only.
3. Start MIDI Learn. Tap the Undo button in Loopy. On the SMK-37, enter clip mode, hold PATCH and push SEQ REC.
4. Tap the learned message. Set the trigger to "On".
5. Do steps 3 and 4 again for Redo with PATCH + SEQ PLAY.
6. Stop MIDI Learn. Test: hold PATCH and push SEQ REC once. Loopy must undo one step.

## 1.8-apc40 follow-up (2026-10-15)

- Idle brightness floor. Loopy sends dim palette shades for idle clips and empty slots (for example 0x0B). At half level these shades were almost not visible on the pads. The firmware now lifts the brightest channel of a lit pad to 48 of 255 (`SMK_APC40_PAD_FLOOR` in main_fm1.c). The ratio of the channels does not change, so the hue stays the same. Colors that are already brighter do not change. Brightness 0 (the off phase of a queued blink) keeps the pad off. (Removed in follow-up 2: the user reported the LCD, not the pads.)
- Note drop only in clip mode. The USB receive interrupt still stores all APC feedback. It keeps APC notes from the sound engine only while clip mode is on. Before this change, it also kept them after APC host identification. Then host MIDI notes 0x00-0x27 (for example the GM drum notes 36-39) did not reach the X0X parts with clip mode off. Now, with clip mode off, the original MIDI input path does not change (agent rules, 10).
- `smk_apc_host_is_active` is removed. Nothing reads the host state now. usb.c does not change: it still calls `smk_apc_host_seen` and `smk_apc_host_lost`. `smk_apc_host_lost` still clears the stored clip state.

## 1.8-apc40 follow-up 2 (2026-10-15)

- The user reported that the dark problem is on the LCD clip map, not on the pads. Idle clips on the LCD were too dark to identify their color.
- Cause: the LCD drew the Loopy palette color without a change. Idle shades (for example 0x0B = 0x271B00, shown as 32,24,0) were darker than the grey dot of an empty cell (74,74,82).
- LCD fix: the firmware maps the brightest channel from 0-255 to 128-255 (`SMK_CLIP_LCD_FLOOR` in main_fm1.c). The ratio of the channels does not change. Examples: 0x0B gives 148,101,0. 0x27 gives 0,89,139. 0x60 (playing, 0xFF7F00) does not change.
- Pads: the pad floor of the first follow-up (`SMK_APC40_PAD_FLOOR`) is removed. The pads show the Loopy color at half level again, as in 1.4-apc40 to 1.8-apc40. The user reports that this pad brightness is good.
- Transport echo: a Loopy Start, Continue or Stop in clip mode sets `seq.midi_req`. Then the sequencer does not send 0xFA, 0xFB or 0xFC back to the host. `seq_advance` clears the flag after each transport request. With clip mode off, the flag stays 0 and nothing changes.
- Limit: with more than one clock source on the USB port, each tick counts (BEHAVIOR.md LIM-10). Send clock to "X0X FM-1" from one source only.
