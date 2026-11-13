# Box-entrance startup integration plan

**Status: PENDING.** This is a read-only plan against firmware base `94197fc8`. The owner-approved shortened original entrance is an asset only, commit `21261737ed2118b73253e974a50d00f8f79b3331` on `assets/dnbox-box-entry-preview-20261113`. It is not in a device build. It stays PENDING until it is.

**Recommended option (summary):** design the player now, do not merge, and do not place the film in any current flash hole. A faithful copy of the 36-frame clip does not fit anywhere the firmware may write. The decoder alone, measured at **462 B** of JieLi `-Os` code, already exceeds the Elite app-flash headroom of **248 B**. Frame data needs **747 KiB** lossless or **41–194 KiB** for a visibly lossy subset — there is no allow-listed region of that size. Play can be arranged so it does **not** slow boot (hold the closed box during the existing ~0.6 s BLE init, finish a SPI-budgeted subset in the existing 330 ms post-TIMER5 window, skip on a key, last picture = live UI). That player stays on paper until the Optimizer frees the decoder bytes and the owner grants a data region large enough for a quality they accept.

Every number is labelled **HOST MEASURED**, **HOST ESTIMATE**, or **MISSING**. No JieLi firmware image was built. No flash, install, or device write was done. The box was kept light: one 36-frame host measurement (19 s) and two single-file `-c` compiles.

---

## 0. Sources

| Item | Source |
|---|---|
| Firmware tree | Read-only export `/workspace/box-entry/fw-94197fc8` → `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (blob hashes of `fm1_flash_map.h`, `boot_screen.h`, `main_fm1.c`, `storage.c` match `git rev-parse 94197fc8:…`) |
| Asset | `21261737`, files also at `/workspace/box-entry/render/DnBOX-box-entry-short.mp4` (36 frames, 70 fps, 0.514286 s, 480×480, sha256 `566ee16ad25c8268…`, **HOST MEASURED**) and `/workspace/box-entry/work/short-frames/0001.png`–`0036.png` |
| Device panel | 240×240, ST7789, RGB565 big-endian, SMK SPI **12 MHz** (`firmware/src/lcd.c` lines 11–14, 131) |
| Flash free at 94197fc8 | Elite **8,440 B**, Pro **8,448 B**, pool free **8,340 B**, floor **8,192 B**, headroom **248 / 256 B** (`/workspace/kept-026-94197fc8/inputs-provenance.json`, PADCAL=0, SDK V1.2.14) |
| Host measurements | `/workspace/box-entry/check/frame-budget.json`, `frame-budget.txt`, `frame-budget-extra.txt` |
| Decoder objects | `/workspace/box-entry/check/decoder/dec_dirty_rle.o`, `dec_pal_xor.o` — pi32v2 clang **4.0.1** `-target pi32v2 -Os -ffunction-sections -fdata-sections -fno-builtin -mcpu=r3` |

Downsample used for every size: 2×2 area average from 480×480 RGB888, then RGB565. Frame 36 after that downsample is pixel-identical to `/workspace/box-entry/work/HOME-94197fc8-240.png` (**HOST MEASURED**, 0 differing pixels). Frames 26–36 are identical to each other and to that HOME.

---

## 1. Current boot path

### 1.1 How the boot screen is drawn

There is **no video player**. STATUS.md item 72 is a static still. Historical NS025 (the 192-frame 3D entrance) is `SUPERSEDED_FOR_FIRMWARE_BOOT`.

1. `fm1_cstart()` (`firmware/src/app/main_fm1.c` 2591–2636) halts core 1, starts TIMER4 (`fm1_time_init`, `firmware/hal/fm1_time.h` 20–28), arms the watchdog and boot-guard, clears `.bss` / pool, copies `.data`, then calls `fm1_main()`.
2. `fm1_main()` (2352–2588) first talks to flash: JEDEC check, `fl_plain_window_init`, `st_owner_check()` (SMK), user-sample scan, OTA cleanup (2358–2371). The LCD is still off.
3. `lcd_init()` (`firmware/src/lcd.c` 161–180): hardware init, `lcd_spin(2000000)` (**HOST ESTIMATE ~80 ms**, from the comment that `lcd_spin(3000000) ~ 120 ms` at line 168), SWRESET wait **150** units and SLPOUT wait **120** units at “~1 ms per unit” (lines 126–129, 168), then a full-screen `lcd_fill` and `DISPON`. The fill is 115,200 B on a 12 MHz SPI = **76.8 ms HOST MEASURED (computed)**. **HOST ESTIMATE** for the whole `lcd_init` on SMK: **~427 ms**. Not measured on the device (**MISSING**).
4. `palette_set(1)` (Amber), then `boot_shown()` + `draw_boot_screen()` (main_fm1.c 2374–2377).

`boot_shown()` (`firmware/src/app/editor.c` 661–670) XIP-maps the two 4 KiB sectors `BOOT_SEC0` / `BOOT_SEC1` (`0xAB000`, `0xAC000` on SMK, `boot_screen.h` 49–55) and calls `boot_pick()` (`boot_screen.h` 396–406). The valid copy with the higher sequence wins. If neither is valid, it returns 0 and the factory words are used.

`draw_boot_screen()` (`firmware/src/gfx.c` 281–300) is **one static frame**:

- A custom record may include a **240×96 1-bit** picture (`BOOT_PIC_BYTES` = 2,880, `boot_screen.h` 28–33). Maximum record 2,924 B, which fits one 4 KiB sector (`boot_screen.h` 67).
- Two 240×120 strips are drawn through the existing canvas `cv_px` and blitted (`gfx.c` 289–294). Factory default is the words `DnBOX` / `909 808 CHICAGO MIAMI` / `BREAK` (`boot_screen.h` 61–62, `gfx.c` 297–299). There is no picture unless a custom record sets `BOOT_F_PIC`.

The installer can change this still while the app runs (BEHAVIOR FLS-19; `ed_boot` in `editor.c` 678–). `BOOT_MS` (20,000) is the **upload timeout**, not a display duration (`boot_screen.h` 36, 274, 316).

### 1.2 When audio, USB, key scan and MIDI come up

After the still is on screen, `fm1_main` continues **synchronously** on core 0, with the still held:

| Step | Lines (`main_fm1.c`) | What becomes live |
|---|---|---|
| Optional crash caption | 2385–2398 | Extra **`fm1_delay_ms(2500)`** if a new crash record is present. Leave this path alone. |
| `fm1_input_init`, `fm1_adc_init`, `smk_trim_load` (read-only USRTRIM), `panel_init`, `led_pos_init`, `plat_perf_init` | 2401–2409 | Pins and tables only. **Keys are not yet scanned.** Scanning is `fm1_input_tick()` from the TIMER5 ISR (`firmware/hal/smk_input.h` 20, `main_fm1.c` 149). |
| `engine_init`, `project_load` / `project_defaults`, `engine_apply_sound`, `project_apply_io`, `engine_brk_loops` | 2410–2417 | Engine and last project in RAM. Duration **MISSING**. |
| `ble_prepare(1)`, `ble_start()` | 2418–2423 | BLE controller init on core 0 **with IRQs off**, **“about 0.6 s”** (BEHAVIOR BLE-03; `ble.c` 273, `fm1_wdt_feed` comment “init takes about 0.6 s of the 8 s”). The screen cannot be updated here. `ble_guard.init_ms` is recorded from TIMER4 (`ble.c` 297) but a typical device value is **MISSING**. If Bluetooth is not started this boot, this returns quickly (`ble.c` 264–270). |
| `plat_cpu2_set(1)`, `ble_attach()` | 2425–2430 | Core 1 starts; controller moves to core 1. |
| `audio_init()` | 2431, 116–121 | ALNK0 audio DMA starts. Audio is live from here. |
| `usb_start()` | 2432 | USB peripheral is started. **Traffic is not polled yet.** |
| `uart_midi_init()` | 2433–2435 | TRS UART programmed. Must be **before** TIMER5 (comment: PORTH RMW). Poll is later, from TIMER5. |
| `fm1_timer5_start(isr_timer5, 4)` then `fm1_irq_enable_all()` | 2436–2442 | TIMER5 is 10 kHz (`firmware/hal/fm1_timer.h` 7). The ISR (`main_fm1.c` 133–200) runs `fm1_input_tick()` every 100 µs, and every 5 ticks (`usb_due`, 2 kHz) `usb_poll()`, `uart_midi_poll()`, `ble_service()`. **This is when keys, USB MIDI and TRS MIDI become live.** `fm1_ms` also starts advancing here (it is incremented in this ISR, line 190). Before this, delays use TIMER4 directly (`fm1_delay_ms`, `fm1_time.h` 42–46). |
| `fm1_delay_ms(30)` then optional `panel_setup()` (both OCTAVE keys held) then **`fm1_delay_ms(300)`** | 2443–2451 | **Forced 330 ms** with the boot still showing, keys and MIDI already running. This is the existing “show the still” window, not a measurement of init. |
| `lcd_fill(0,0,240,240,0)` (black, another **76.8 ms HOST COMPUTED** SPI), `ui_init()`, then the main loop | 2452–2588 | Live UI. First `ui_frame()` runs on the first loop iteration (`fm1_ms - last_frame >= 16`, 2565–2572). |

`ui_init()` (`firmware/src/app/ui.c` 8585–8629) sets **`ui.view = V_SEQ`** (line 8604: “start on the visible step grid”), not `V_HOME`. The approved clip lands on the current HOME (909 / 808 / BREAK / CHICAGO / MIAMI, PERFORM). See §5.

Safe-mode (`safe_main`, 2286–2350) is a different path: black screen, text, USB only, no audio, no animation. Do not change it.

### 1.3 Current boot time

**Total time from reset to live HOME/SEQ, on the device: MISSING.** The 94197fc8 packages record `hardware_boot_restart: NOT_TESTED` (`inputs-provenance.json`).

Documented or computed pieces, not a total:

| Piece | Value | Label | Source |
|---|---|---|---|
| `lcd_init` | ~427 ms | HOST ESTIMATE | `lcd.c` comments + 12 MHz SPI fill |
| `draw_boot_screen` SPI | ~77 ms + text | HOST ESTIMATE | two 240×120 blits |
| BLE controller init | **about 0.6 s** | documented | BEHAVIOR BLE-03; `ble.c` 273 |
| Forced still hold after TIMER5 | **330 ms** (30+300) | documented | `main_fm1.c` 2443–2451 |
| Crash caption (rare) | 2,500 ms | documented | `main_fm1.c` 2396 |
| `engine_init` + `project_load` + sample scan | — | MISSING | — |
| Time until keys / USB MIDI / TRS MIDI | after TIMER5 start, itself after BLE + audio | relative, not a millisecond figure | `main_fm1.c` 2431–2442 |
| USB host enumeration after `usb_poll` starts | host-dependent | MISSING | — |

The still is already visible for the whole of engine/project/BLE/audio plus 330 ms. The entrance can use that same window. It must not lengthen it.

---

## 2. Frame data budget

Device format: **240×240 RGB565 big-endian**. One raw frame = **115,200 B**. 36 raw frames = 4,147,200 B. The last picture is not stored (live UI, §5). After downsample, frames 26–36 **are** that HOME, so stored motion is frames **1 and 6–25** (21 unique frames; 2–5 and 26–36 are holds). **HOST MEASURED.**

Unique RGB565 colours in the whole clip: **1,484** (does not fit a 256-colour lossless palette). Frame 1 has 526 colours.

### 2.1 SPI is the pace, not 70 fps

SMK `LCD_BAUD 4` = 12 MHz (`lcd.c` 11–14). A 240×72 strip at that rate is **23.04 ms HOST COMPUTED**, which matches the comment “took 23 ms” (`lcd.c` 17). A full 240×240 frame is **76.8 ms HOST COMPUTED**. 70 fps is a 14.3 ms budget: a full (or nearly full) blit cannot keep 70 fps. Mid-opening frames dirty 37–54 k pixels (49–72 ms of SPI if only dirty pixels are sent).

`lcd_data` requires the **source buffer in RAM**, not XIP (`lcd.c` 6). Compressed bytes can live in flash; the decoder must write pixels to RAM, then DMA.

### 2.2 Lossless encodings (HOST MEASURED, actual frames)

All sizes include a keyframe for frame 1. “Drop last” also drops 26–35, which equal HOME.

| Encoding | 35 frames (1–35) | 21 unique (1, 6–25) | 18 frames (every other of 1–35) |
|---|---:|---:|---:|
| Raw RGB565 | 4,032,000 B | 2,419,200 B | 2,073,600 B |
| Independent per-frame pixel RLE | 1,219,854 B | — | — |
| Keyframe + XOR-RLE | 863,156 B | 828,551 B | 491,861 B (18 kept frames, XOR vs previous kept) |
| Dirty-span RLE (skip unchanged, RLE the rest) | 756,690 B (1–35) | **747,418 B** | 393,515 B (18 kept frames, spans vs previous kept) |
| Firmware LZ77 on XOR/raw, chunked at 57,600 B (codec rejects inputs ≥ 64 KiB, `lz.c` 108, `LZ_NONE`) | 842,668 B | 807,932 B (22 holds) | 474,463 B |
| zlib -9 on concatenated XOR (HOST reference only; **not** in the firmware) | 454,340 B | 450,942 B | 259,496 B |

Best lossless that a device decoder can implement with no previous-frame RGB565 buffer: **dirty-span RLE, 747,418 B** (~730 KiB) for the 21 unique frames. Firmware LZ does not beat dirty-span RLE here, and it needs a 64 KiB-class output buffer.

### 2.3 Lossy palette + XOR-RLE (HOST MEASURED)

Median-cut palettes, 8-bit indices, XOR against the previous **kept** index map, then byte RLE. PSNR is on RGB565 expanded to 8-bit.

| Subset | 4 colours | 8 colours | 16 colours |
|---|---:|---:|---:|
| 21 unique frames | 107,805 B (PSNR min 18.2) | 120,846 B (20.4) | 193,661 B (25.5) |
| 11 every other | 65,257 B (18.5) | 71,558 B (20.8) | 106,704 B (23.9) |
| 6 keyframes (1,6,10,14,18,25) | 41,102 B (18.8) | 50,584 B (21.5) | 67,492 B (24.9) |
| 3 keyframes | 25,137 B (19.2) | 39,812 B (23.7) | 41,155 B (25.5) |
| First frame only (a still) | 6,968 B (22.2) | 7,849 B (24.2) | 10,287 B (27.6) |

4-colour PSNR ~20 dB is clearly posterized. Even the smallest *animation* in this table (3 keyframes, 4 colours, 25 KiB) is larger than every available hole in §3. A 4-colour first-frame still (6,968 B) would fit the 8 KiB spare, at the cost of the presets reservation and of not being the approved film.

1-bit 240×240 first frame, threshold 80: packed 7,200 B, RLE 2,533 B, firmware LZ 1,930 B. A 240×96 crop (the existing `BOOT_PIC` geometry) is 2,880 B raw and **463–1,570 B** RLE depending on `y0`. That fits the current boot record. It is a still, not the entrance.

### 2.4 Decoder code size (JieLi clang, not a HOST estimate)

Single-file `-c` with the same flags as `tools/build.py` (`-Os`, `-ffunction-sections`, `-mcpu=r3`, pi32v2 clang 4.0.1). No link, no firmware image.

| Decoder | `.text` | Static RAM in the object | Notes |
|---|---:|---:|---|
| Dirty-span + pixel-RLE `boxent_decode_dirty_rle` | **462 B** (`0x1ce`) | 480 B line + 4 B sink | Line can be `lcd_fillbuf` (already 240×uint16, `lcd.c` 84). |
| 16-colour index XOR-RLE `boxent_decode_pal_xor` | **292 B** (`0x124`) | 57,600 B index map + 480 B line + 32 B palette | The 57,600 B map **fits in existing `cv_px`** (66,240 B) for the boot window only. |

Host gcc `-Os` (labelled **HOST**, not used for the budget): 559 B and 319 B `.text` respectively.

Player glue (timing, skip, `lcd_blit` / `lcd_sync`, replacing the 300 ms delay, branching around `draw_boot_screen`): **HOST ESTIMATE 200–500 B**, not compiled. Together with the 462 B decoder, **about 650–1,000 B of app flash**, which is already more than the 248 B Elite headroom.

`objsizedump -dump-stack-size` reported 0, which is not credible. Stack is **MISSING** as a measured figure; **HOST ESTIMATE** a small frame (tens to ~128 B of pointers and counters).

Existing firmware LZ (`firmware/src/app/lz.c`) is already in the image. Reusing it would save a new decompressor, but not the player, and the lossless LZ size is worse than dirty-span RLE. The decompressor writes a whole output buffer in place and rejects inputs ≥ 64 KiB, so a full RGB565 frame has to be two 120-row bands.

### 2.5 RAM

There is **no full 240×240 framebuffer**. `gfx.c` 3–25: “Small-canvas renderer (no full framebuffer).” `cv_px` is one strip, **`CV_ROWS` 138**, 240×138×2 = **66,240 B** in `.pool` (link-time, always reserved). `lcd_fillbuf` is **480 B**. After `engine_init`, pool free is **8,340 B** (94197fc8 provenance) — a 115,200 B RGB565 previous-frame buffer **does not fit**, even temporarily, because other `.pool` symbols (delay, undo, FX, …) are already placed by the linker.

Recommended RAM for the player:

- Reuse `cv_px` and `lcd_fillbuf` during boot, **before `ui_init`**.
- Dirty-span RLE: **0 extra RAM** (decode a line, DMA it, no previous frame).
- Palette XOR-RLE: 57,600 B previous indices inside `cv_px` (allowed), 32 B palette, 0 extra pool.
- Do not add a 115,200 B framebuffer.

Animation must finish (or be abandoned for the live UI) before the first `ui_frame()`, which owns `cv_px`.

### 2.6 Decode CPU cost per frame (HOST ESTIMATE)

AC79 360 MHz. A straightforward RLE loop is **HOST ESTIMATE ~10–20 cycles per emitted pixel** (not measured on the device; **MISSING** on-device). Worst unique frame has 57,600 pixels (frame 1) or 54,238 dirty (frame 16): **about 0.6–1.2 ms** at 20 cycles, 360 MHz. SPI at 12 MHz is **77 ms / 72 ms** for those frames and dominates by a large factor. After TIMER5 is running, `lcd_sync` polls in the main loop with IRQs on, so audio, key scan and USB MIDI keep running during the wait (`lcd.c` 7–8, 33–42). **Do not blit during `ble_start` (IRQs off).**

---

## 3. Where the data could live

Protected ranges that this plan will **never** propose to edit (`fm1_flash_map.h` 26–46, 114–116): head / SPL / isd_config `0x00000–0x03FFF`, BTIF `0xC0000–0xC0FFF`, USRTRIM `0xC1000–0xC1FFF`, stock USR `0xF4000–0xFDFFF`, key_mac `0xFF000–0xFFFFF`, and the bootloader. `FL_PROTECTED` / `FL_GUARD` already refuse them.

### 3.1 App flash (app.bin / XIP)

App code window `0x04120–0x8AFFF`, `FL_APP_CODE_MAX` = 552,672 B (`fm1_flash_map.h` 28, 72). At 94197fc8: Elite free **8,440 B**, Pro **8,448 B**, floor **8,192 B**, headroom **248 / 256 B**. Putting either the decoder (~462–1,000 B) or any frame data here goes through the floor. The Optimizer’s in-flight savings (a few hundred to a few thousand bytes) can cover a **decoder**, not a **747 KiB** or even a **41 KiB** film. **Not a home for the frames.**

### 3.2 Boot-image sectors `0xAB000` and `0xAC000`

Two **4 KiB** sectors, 8 KiB together. Today they hold A/B copies of the still record, max **2,924 B** used, **1,172 B** leftover per sector (`boot_screen.h` 33, 45–50, 67; `storage.c` 45–47). A foreign-store clear erases both (`storage.c` 62–63, 93–102, through `ST_OWNER_SECTOR` at `0xAF000` except in the vendor-test profile).

Reusing them for the film would:

- at most free **8,192 B** if A/B and the still were both given up — still far short of 25–747 KiB;
- break FLS-19 (custom still from the installer) and the torn-write A/B rule, unless the still is kept in one sector (then only 4 KiB remains).

**Do not evict the custom still without the owner’s own words (R3).** This region is the right place for a *factory 240×96 1-bit still* of the closed box (it fits `BOOT_REC_MAX`), which is **not** the approved animation. See open questions.

### 3.3 Spare `0xAD000–0xAEFFF` (8 KiB)

Two free 4 KiB sectors inside `FL_DATA`, already in `FL_STORE_OK`. At 94197fc8 nothing programs them (`storage.c` 47: “0xAD000..0xAEFFF stays spare.”). The presets lane has **reserved this 8 KiB** for a user-preset bank (`docs/plans/2026-11-13-preset-format-v1.md`, ROADMAP 26.8). 8 KiB is also far short of any animation subset above. **Conflict: do not take this spare for the entrance.** A foreign-store clear would wipe it anyway.

### 3.4 `0xFE000–0xFEFFF` (4 KiB, unused)

Commented “free, unused (4 KiB: too small for a slot)” (`fm1_flash_map.h` 44; BEHAVIOR FLS-01). It is **not** in `FL_STORE_OK` / `FL_RANGE_OK` today, so the RAM driver will not program it until the allow-list is extended. Extending the allow-list to this window would **not** touch a protected range (`FL_WIN_SAFE` would accept `[0xFE000, 0xFF000)`). It is still only 4 KiB — too small for the film, tight even for a 4-colour first-frame still (6,968 B).

### 3.5 Other options — rejected

| Place | Why not |
|---|---|
| User sample slots (`FL_SMP` / `FL_SMPV`, slot 3, 32–120 KiB) | User data. Still too small for lossless, and it steals sample memory. |
| OTA staging `0xBB000–0xBFFFF` (20 KiB) | Needed for M-UPGRADE. 20 KiB still too small. |
| Bluetooth block leftover (~4 KiB in 64 KiB at `0x8B000`) | Encrypted app-area controller image. Do not put animation there. |
| Stock USR `0xF4000–0xFDFFF` (40 KiB) | **Protected**, kept for rollback. Never. |

**There is no allow-listed, unused region large enough for a faithful — or even a 3-keyframe 4-colour — copy of this film.** That is the primary blocker.

---

## 4. Boot-time cost — it must not slow boot

### 4.1 Can it play in parallel?

Yes, in part. The right timeline, cited against the current `fm1_main`:

```
lcd_init                              ~427 ms HOST ESTIMATE, no picture yet
draw / DMA frame 1 (closed box)       76.8 ms SPI; DMA can run while
                                      engine_init + project_load start
engine / project / trim / panel       duration MISSING; IRQs still off
                                      until TIMER5; can blit more frames
                                      here only as busy-wait on core 0
ble_start()                           ~0.6 s, IRQs off (ble.c 273–297).
                                      NO blit. Frame 1 holds. This hold
                                      is longer than the clip’s 5-frame
                                      closed-box hold (71 ms). It does
                                      not add wall-clock beyond today.
audio_init, usb_start, uart_midi_init short
TIMER5 + IRQs on                      keys, USB MIDI, TRS MIDI LIVE
                                      (main_fm1.c 2436–2442)
existing 30+300 ms window             PLAY remaining frames here,
                                      IRQs on, lcd_sync does not
                                      stall audio or the key matrix.
                                      Skip on a key (fm1_input already
                                      ticking). Cap total SPI in this
                                      window to ≤ 330 ms so ui_init
                                      is not late.
ui_init + ui_frame                    LIVE last picture. Skip the
                                      black lcd_fill (2452) so the
                                      landing does not flash black.
```

Skippable on key **before** TIMER5 would need a polled `fm1_input_tick` in the player. During `ble_start` IRQs are off for ~0.6 s, so a skip cannot be serviced then anyway. Practical skip: from TIMER5 onward.

MIDI and keys become ready at the same point as today (TIMER5, after BLE and audio). The player after that point must not call anything that masks those IRQs for long. `lcd_sync` is a main-loop poll and is fine.

### 4.2 SPI budget vs the 330 ms window (HOST MEASURED)

All 21 unique dirty-pixel blits: **791.95 ms** of SPI (76.8 ms for frame 1 + 715 ms for 6–25). Frame 1 can (and should) go up **before** BLE, overlapping engine/project. That leaves **715 ms** after BLE, but the existing forced window is only **330 ms**. Playing every motion frame after BLE would add about **385 ms** to time-to-UI — **not allowed**.

To not slow boot: **drop frames** so remaining SPI ≤ 330 ms (plus any leftover of engine/project if used before BLE). That is a sped-up opening, at whatever rate 12 MHz SPI allows, ending when today’s `ui_init` would have run. A 6–11 frame subset with dirty spans is in the right SPI order of magnitude (to be re-measured against the *kept* previous frame, not the skipped neighbours, before implementation).

If Bluetooth is OFF this boot, `ble_start` returns at once and the pre-TIMER5 window is shorter; the same cap still applies: never finish later than the current `ui_init` moment.

### 4.3 Boot-time impact of the recommended player

**Target: 0 ms added** to time-to-keys, time-to-MIDI, and time-to-live-UI, versus 94197fc8. Evidence when this is built: a host (and later device) log of TIMER4 ticks at `usb_start` / TIMER5 / `ui_init`, compared with the same points on the base image. Until then the impact is a **design constraint**, not a measured zero. Device boot time itself is **MISSING**.

---

## 5. Last frame = live UI

Do **not** store frames 26–36. After RGB565 downsample they **are** the current HOME (**HOST MEASURED**, 0 pixel difference vs `HOME-94197fc8-240.png`).

At the end of the player:

1. Do not `lcd_fill` black (`main_fm1.c` 2452) — that would flash.
2. Call the existing `ui_init()` then `ui_frame()` (or let the main loop do the first `ui_frame` immediately).
3. That draws whatever the firmware actually boots into, from live project state, not a canned bitmap.

**Mismatch, needs an owner choice:** the approved asset lands on **HOME**. `ui_init` sets **`ui.view = V_SEQ`** (`ui.c` 8604). Drawing “the live UI” without a view change will land on the 909 step grid, not the HOME the clip was rendered to meet. Changing the first view to `V_HOME` is a behaviour change and needs the owner’s words. Until then, either the landing jumps, or the last held 3D frame (already HOME pixels) is replaced by a different live page.

Factory / custom still: if a **custom** boot record is present (FLS-19), recommended default is **show the custom still as today and skip the animation**, unless the owner says otherwise.

---

## 6. Recommended option

**Pending integration, no merge, no new flash use.**

1. **Player architecture** (implement only when §3 and the decoder bytes are solved):
   - Bitstream: dirty-span + pixel-RLE (lossless) if a ~750 KiB region is ever granted; otherwise a **named, owner-accepted** lossy subset (palette XOR-RLE), never a silent quality cut.
   - Decode into `lcd_fillbuf` / `cv_px` lines, DMA via existing `lcd_blit` / `lcd_sync`.
   - Frame 1 before BLE; hold during `ble_start`; remaining frames in the existing 330 ms window, **dropped to fit**; skip on a key once TIMER5 runs; last picture = `ui_init` + `ui_frame`.
   - Keep FLS-19. Keep `panel_setup` on both OCTAVE keys. Keep the crash caption and safe mode unchanged.
2. **Flash:** do not put frames in app flash, `0xAD000` (presets), `0xAB000`/`0xAC000` (custom still A/B), sample slots, OTA, or any protected range. Report the film as **blocked on a granted data region that does not exist today**.
3. **Code flash:** decoder **462 B MEASURED** + glue **200–500 B HOST ESTIMATE** waits on Optimizer headroom above 8,192 B.
4. **RAM:** 0 extra if `cv_px` and `lcd_fillbuf` are reused and released before `ui_init`.
5. **Boot-time:** 0 ms added, by construction of the SPI cap. Device confirmation **MISSING** until a device build exists.
6. **Optional interim, only if the owner asks:** a factory **240×96 1-bit** closed-box still in the existing boot record (fits 2,924 B). Program it as a sector (installer / first-boot write), not as 2,880 B of app code. This is **not** the approved entrance and must not be presented as done.

---

## 7. Risks

| Risk | Why it matters | What to do |
|---|---|---|
| Flash: no legal hole for the film | Primary blocker. A “ship it in the leftover 248 B / 8 KiB” plan would fail the floor or collide with presets / FLS-19. | Leave PENDING. Do not take `0xAD000`. Do not evict the custom still. |
| Decoder also exceeds 248 B headroom | Even a player with no frames will not link against the 94197fc8 floor. | Wait for Optimizer savings; report a real JieLi Elite+Pro delta. |
| SPI-limited playback | 70 fps is impossible on 12 MHz for this film. All 21 unique frames need 792 ms SPI and would slow boot. | Cap SPI to the existing window; drop frames; evidence the cap. |
| BLE init freezes the picture for ~0.6 s | Longer closed-box hold than the 71 ms in the clip. | Accept as “use the time we already spend”; it does not add boot time. Tell the owner. |
| `ble_start` with IRQs off | A blit or a key poll here would stall or be ignored. | No LCD and no skip during that call. |
| Audio already running in the 330 ms window | A future decoder that masks IRQs, or a flash read through a parked core, could drop audio. | Main-loop only, IRQs on, no flash write, no `printf`. Source pixels from RAM. |
| First live view is SEQ, asset is HOME | Visual jump, or an unasked behaviour change. | Owner question (§9). |
| Custom still vs animation | Replacing FLS-19 without a quote violates R3. | Skip animation when a custom record is present, by default. |
| Foreign-store clear | `st_owner_check` erases `0x9D000–0xAEFFF`, including boot sectors and the 8 KiB spare. | Any data in `FL_DATA` must be restorable (factory in the package, or accept loss like today’s still). |
| `lcd_fill` black at 2452 | A black flash between the last 3D frame and the live UI. | Drop that fill on the animation path. |
| Pool / `cv_px` lifetime | Using `cv_px` past `ui_init` corrupts the first UI frame. | Player ends first. |
| Quality cuts | 4-colour ~20 dB is not the approved original. | No silent lossy default. |
| Elite vs Pro | Same LCD rate and same player; flash headroom 248 vs 256 B. Both must be measured. | Linked Elite and Pro size on the real build, when there is one. |

---

## 8. Exact files and functions to touch later

When (and only when) the flash gates pass. One new unit, smallest changes to existing boot.

**New**

- `firmware/src/app/box_entry.h` (and a `.c` if not a header unit): bitstream walk, `boxent_decode_*`, `boxent_play(deadline_ticks, skip_fn)`. Host packer under `tools/` from `work/short-frames/*.png`.
- Host test: decode every stored frame into a 240×240 RGB565 buffer, compare with the downsample used here; SPI-budget check; skip; “`ui_init` tick is not later than base.”

**Existing, player and boot**

- `firmware/src/app/main_fm1.c` — `fm1_main()`: after `lcd_init`, start the player (frame 1), leave `ble_start` as a hold, replace `fm1_delay_ms(300)` (and the black `lcd_fill` on this path) with a deadline-capped remainder; keep `uart_midi_init` before TIMER5; keep `panel_setup`; then `ui_init()`.
- `firmware/src/gfx.c` — `draw_boot_screen()` stays for custom/factory still and for safe/crash paths; the player uses `cv_blit` / `lcd_blit` / `lcd_sync`.
- `firmware/src/lcd.c` — reuse `lcd_init`, `lcd_blit`, `lcd_window`, `lcd_sync`, `lcd_fillbuf`. No baud change (12 MHz is the stock SMK rate).
- `firmware/src/app/ui.c` — `ui_init()`, `ui_frame()` for the live last picture; `ui.view` only if the owner chooses HOME.
- `firmware/src/app/boot_screen.h` — `boot_pick`, `boot_image_state`, `boot_lines`, constants. New kind only if a still and a film must share a sector (not recommended at these sizes).
- `firmware/src/app/editor.c` — `boot_shown()`, `ed_boot*`: FLS-19 unchanged.
- `firmware/hal/smk_input.h` — `fm1_input_tick` / `fm1_in` for skip, already running from `fm1_timer5_irq`.
- `firmware/src/app/lz.c` — only if the chosen bitstream is firmware LZ.

**Map and storage — only if a new allow-listed window is granted**

- `firmware/hal/fm1_flash_map.h` — comments and `FL_STORE_OK` / `FL_RANGE_OK` for a **new non-protected** window. Never `FL_PROTECTED` ranges.
- `firmware/src/storage.c` — `st_owner_check` comments and erase span, if the region is inside `FL_DATA`.

**Docs and evidence, same change**

- `BEHAVIOR.md`, `docs/ROADMAP.md` (a row, PENDING until a device build), host tests under `tests/`.
- Linked Elite and Pro size deltas, real framebuffer before/after, TIMER4 ticks at TIMER5 and at `ui_init` versus 94197fc8.

**Do not touch**

- Bootloader, SPL, BTIF, USRTRIM, efuse, key_mac, stock USR, `.github`, `main`, device flash from an agent.

---

## 9. Open questions for the owner

1. **Flash.** A faithful film needs ~750 KiB; a clearly lossy 6-frame 4-colour cut still needs 41 KiB. No allow-listed unused region is that large. Is the entrance to stay PENDING (recommended), or is a quality and a region to be granted explicitly? Taking sample slots or protected flash is not offered.
2. **First live page.** The approved clip ends on **HOME**. The firmware starts on **SEQ** (`ui.c` 8604). Change the first view to HOME so the live last frame matches the asset, or keep SEQ and accept a jump?
3. **Custom boot stills (FLS-19).** Recommended: if a custom record is present, keep today’s still and skip the animation. Agree?
4. **Closed-box hold during BLE (~0.6 s).** Longer than the clip’s 71 ms, but it uses time already spent. Acceptable?
5. **Interim 1-bit factory still** of the closed box in the existing 240×96 boot picture, not presented as the entrance — wanted or not?

---

## 10. Blockers and status

| Gate | State |
|---|---|
| Frame data flash | **Blocked.** 747,418 B lossless / 41,102 B smallest 6-frame 4-colour animation; largest unused legal holes are 8 KiB (presets) and 4 KiB (`0xFE000`). |
| Decoder app-flash | **Blocked** on Optimizer: 462 B MEASURED + 200–500 B HOST ESTIMATE glue > 248 B Elite headroom. |
| `0xAD000` 8 KiB | Reserved for presets. Do not use. |
| Boot time | Design can be 0 ms added (SPI cap + parallel hold). **Device confirmation MISSING.** |
| RAM | Not a blocker (reuse `cv_px`, 0 extra). |
| Last frame | Live UI is the right mechanism; HOME vs SEQ is an owner question. |
| Merge / publish / device | **PENDING** until the film is in a device build. This plan does not change firmware. |

**Short budget for the Orchestrator:** recommended option = pending player as in §6; frame flash **747,418 B** lossless (or 41–194 KiB lossy subsets) with **nowhere to put them**; decoder **462 B MEASURED** + glue **~200–500 B HOST ESTIMATE**; RAM **0 extra**; boot-time impact **0 ms by design**, **MISSING** on device; blockers = **flash (data and decoder headroom)**; open questions in §9.
