# X0X37 roadmap

This file tells what we build, what we finished, where we are and what we must still do. Update this file when a step ends or a plan changes. USER_PREFERENCES.md gives the preferences and rules. x0x37/BEHAVIOR.md gives the intended behavior. x0x37-progress-log.md gives the detailed history.

## The goal

We make X0X firmware for the M-Vave SMK-37 (Pro and Elite). The firmware has these properties:

1. It has a clip mode that acts as an Akai APC40 mkII for Loopy Pro. The pads start clips, and the pads and the screen show the clip colours.
2. It keeps all the X0X features (sequencer, synth engines, BREAK mode) at full quality: 44.1 kHz, with no length cuts.
3. End users can import their own loops for BREAK mode, and the loops stay safe across updates.
4. It has Bluetooth MIDI (MIDI only), and it stays freely shareable under an open license.
5. End users install it from a web page, with no toolchain.

## Done

| Area | What | Where |
|---|---|---|
| Clip mode | APC40 mkII emulation for Loopy Pro. The pads send and receive MIDI and show the clip colours. The screen shows the clip view. OCT- and OCT+ move the view down and up. | smk37 branch |
| Clip mode | Fixes: the firmware drops notes only in clip mode. Loopy Start does not echo back. The screen clip view is not too dark (LCD floor). | 0fbfe07 ... be6fc1d |
| Docs | BEHAVIOR.md (intended behavior, 200+ items), agent-rules.md, BREAK mode guide, README, licences | 4046fe9, 7269951, c24899f |
| Colours | Colour tool that calculates the real pad and screen colours from the APC40 palette | 07eb709 |
| Step 1 | The installer backs up the loop slots and restores them automatically | 39eafa9 ... bd6f9f3 |
| Step 2 | The firmware stores the 909 ride, crash and hi-hat as IMA ADPCM at 44.1 kHz (no audible difference, about 166 KB free after the change) | 241bd15 ... 936f155 |
| Step 3 | Factory breaks: 2 bars at 44.1 kHz. The import tools use 44.1 kHz | d3f23c5 ... 25c87bf |
| Step 4 | New flash layout: one 64 KiB Bluetooth block, 3 loop slots (about 11 s of import space at 44.1 kHz) | 62fc644 ... 81d5f74 |
| Step 5 | Bluetooth MIDI code: the JieLi controller runs as a separate image on core 1. The end user builds it on a computer. GLOBAL > BLUETOOTH MIDI settings | 07159b6 ... 7972fbb |
| Installed | The keyboard ran 81d5f74, and all 5 loops of the owner stayed intact | 2026-10-15 |
| Safety | The installer refuses to add the Bluetooth image to old firmware (the image can overwrite code) | ab44241 (flash_review, ble-stage3) |
| Safety | Bluetooth start guard: a failed Bluetooth start shows FAILED, and the keyboard starts normally (no crash loop into safe mode). The DIAG command reads the crash and start details | bad7b95, 70a9a5f (flash_review, ble-stage3) |
| Bluetooth | Bluetooth starts: STATUS shows READY at power-on (6 of 6 automatic rounds) | f0bcb28 (a52) |
| Bluetooth | The Mac connects over Bluetooth MIDI. MIDI goes in both directions | 79eb477 (a59), 4e6ae9b (a60) |
| Bluetooth | The iPhone connects over Bluetooth MIDI, and the link holds | 94285b5 (a67) |
| Clip keys | Knobs 2-8 send CC 48-54 when no clip keys mode is on | 9984344 |
| Bluetooth | BLE-16: when the Bluetooth link ends, X0X releases the notes that came from Bluetooth (host test tests/scenarios/blenotes.x0x). In the installed build 89f680e | 355f920 |
| Clip colours | The LCD clip colours are the plain Akai colours as RGB565, from the Akai CSV (CLP-05). In the installed build 89f680e | 453e9c8, 9a0b2c2 |
| Pad colours | Pad colour calibration (CLP-79, dev builds only). The build option X0X_DEV_PADCAL (default 1) removes it from a release build | d303e4e (in 89f680e), bd254a0 (integration be9a097) |
| Pad colours | The owner calibrated the pad colours (export of 2026-10-17, 89 entries with owner values). The clip pads use this table APC40_PAD_OWNER in the dev build and in the release build (CLP-13) | 48892fe, 4113ce0 (in the installed build c6bf44a) |
| Gate | The gate builds and tests the dev configuration and the release configuration. The install guard checks the configuration | 7cef9e4, 2012283 (in f9bcc46) |

## Where we are now (2026-10-19)

- Branches:
  - Local integration HEAD = 692c935 (batch 11 open). It adds a one-line dev PAD COLOURS waiting-screen redraw fix to f752007. Both Elite local builds and the focused and reference tests pass. The formal batch 11 gate and device test have not run. This local fast-forward has not been pushed to the box; the last box integration record in LIVE_BRANCHES is f752007.
  - Installed = batch 10 (c6bf44a), release configuration (X0X_DEV_PADCAL=0). Batch 10 contains the batches before it. It remains the installed build.
  - The gate on c6bf44a gave PASS at 2026-10-17 02:19 (tree b220466e91d0). It built and tested the dev and the release configuration (116 ok, 0 FAIL, 0 skip in each).
  - Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18).
  - The side branches ble-a68-opt (a1d8ea1), ble-a69-readback (75c42e1), ble-a70 (77fa067) and ble-a71 (9ef7bc2) have builds. Nobody installed them.
- Keyboard:
  - The keyboard runs integration c6bf44a (batch 10) in the release configuration. The install time is 2026-10-17 02:25 (INSTALLED.tsv). It has no PAD COLOURS page, and the owner pad colours are in the firmware.
  - The fwsc CRC32 is c48b5c6e. The ctl.img read-back is eb742dd2 (the same as the file). Slots 1 and 2 did not change, and slot 3 is empty. DIAG shows READY.
  - Free bytes of the Elite release build at c6bf44a: app code 3864 B, static RAM 5824 B, pool 8196 B.
  - Device test: PENDING (batch 10). Regression (owner 02:49): in clip mode, the knobs do nothing on the installed build. The batch 11 host fix is in local integration 692c935; the device has not been retested. Tag last-good-hw stays 94285b5.
- iPhone:
  - The iPhone works on a67 (owner test PASS 19:18). On a73, the link carries the Loopy clock and Start (20:37 PASS).
  - History: a65 iPhone test 1 (18:23): the iPhone connected (interval 7.5 ms, MTU 185). Then the link ended with reason 0x08 (connection timeout).
  - a62 and a62b: the link held for about 2 min at 26.25 ms with 0 connection updates, reason 0x00. a63-15 (d9844a4): iOS dropped the link before any GATT step.
  - Oct 15: Loopy used the keyboard as its APC40 over Bluetooth (test 30). But some colour notes made sounds, some pads stayed empty and the colours were slow.
  - A connection parameter request made the iPhone drop the link (test 31) and gave a large MIDI delay (test 34).
- Mac: the Bluetooth GATT connection works, and MIDI goes in both directions (a59 79eb477, a60). The a59 fix: the host must register its event handler after btstack_task_init, because btstack_task_init removes it.
- Bluetooth start: the supply must have the stock values (DCDC13, VDC14 1.60 V, SYSVDD 1.38 V) before the RF init (test 27, 56a7452). The controller init runs on core 0 before core 1 starts (83cd4dc). The clock stays at 360 MHz.
- Release package 0b80994 (SMK-37 Elite only, a64 files: .fwsc CRC32 5ec8b21a, ctl.img CRC32 e9c05144):
  - The free pool is 4096 B. The free app code is 144 B. The free app RAM is 0 B.
  - Nobody built the Pro package. Nobody pushed the package. It is for tests only, it is out of date, and it is not for release.
- Clip keys:
  - clip-keys-a65 is at 5a89b76. Its changes are in integration (cherry-picked) and in the installed build 89f680e.
  - It has the KEYS panel and the SCALE, CHORD and ARP pages. Knobs 2-8 send CC 48-54. It has 24 scales, 30 chords, swing, Euclid rhythm, HUMAN and strum.
  - The host tests pass. The owner did not test it on the hardware.
- Pool limit: build.sh and the gate require at least 8192 B of free pool (enforced 2026-10-16 21:53). USER_PREFERENCES.md keeps the older 4096 B owner entry.
- Tools: tools/ble_autotest.py runs the Bluetooth test over USB with no user action. The attempt ledger is BLE_ATTEMPTS.md in the Bluetooth branch (one row for each attempt). The JieLi SDK Bluetooth design compared with our host, with the step plan for the iPhone link, is in ~/Documents/x0x37/ble_split/SDK_DESIGN_COMPARE.md (Mac).

## Left to do

1. Mac: test the clock and Start/Stop over Bluetooth, and check for audio dropouts.
2. Clip mode with Loopy Pro:
   - Open: CLP-50 FAIL. X0X PLAY does not start or stop Loopy Pro (a73, owner test 20:37-20:39). Capture the outbound MIDI from X0X PLAY, then test the pads, overdub and tempo with Loopy Pro.
   - Open: CLP-13 FAIL (a74 owner test 1, 21:18). A violet clip from Loopy Pro shows blue on the pads. The installed build c6bf44a has the owner pad table (48892fe). In c6bf44a, an idle pad sends the full owner table level (4113ce0). Current host tests pass; a device retest is still needed.
   - Open: CLP-18 FAIL (a74 owner test 1, 21:18). When Loopy Pro disconnects, the pads go dark, but the clip colours come back immediately. The fix (a76) is in the installed build c6bf44a. In c6bf44a, a link end clears only the clip cells of that link (30a08fc, 79de009). Device test: pending.
   - Open: the owner test of batch 10 (c6bf44a).
   - Open: regression on c6bf44a (owner 02:49). In clip mode, the knobs do nothing. The batch 11 fix is in local integration 692c935; the local builds and tests pass. It needs the formal gate and a device retest. At 03:20, the owner said "Just fix them". The intended result is that knobs 2 to 8 edit the keybed ARP, CHORD and SCALE again, so that Loopy Pro records the keybed (owner 03:26).
   - Batch 11 (owner 02:49, Second Builder): in clip mode, a tap on MODE latches the MODE UI open, and a second tap closes it. A hold works as before.
   - Batch 11 (owner 02:50, Second Builder): show a "waiting for Loopy Pro to connect" screen instead of the 8x5 grid.
   - Batch 11 (owner 02:52, Second Builder): a tap on the stage latches it open, and a tap closes it. FADER BANK also closes it. The screen has no top bar, and it shows a help line at the bottom. The stage is the MODE UI of the 02:49 item. The owner said at 02:51: "The SCALE, CHORD or ARP button" (Second Builder confirmed).
   - Open: the owner did not test the a74 clock source (MID-13) and PLAY note (MID-14) items yet.
3. Turn Bluetooth on and off in GLOBAL without a restart. OFF: stop advertising and disconnect. ON: start the controller while the keyboard runs, with no audio dropouts (the radio calibration is the hard part).
4. Run the formal batch 11 gate on integration 692c935 or its later final hash. After the finished set is installed and the owner reports a device PASS, bring that integration hash into the main tree (x0x37, smk37).
5. Owner checks by ear and eye: the ADPCM cymbals, the 44.1 kHz factory breaks and the idle clip colours on the screen. Also check that the patterns and settings stayed the same.
6. Web installer: let the page get the JieLi files and build the Bluetooth image. Or let it guide the end user. If possible, end users need no toolchain.
7. Release: Merge Integrator builds the Pro and Elite packages from a hash with a gate PASS and a device PASS. It checks the installer hashes and publishes only when the owner asks. The release build has X0X_DEV_PADCAL=0. It has no PAD COLOURS row, and it uses the pad table that the owner calibrated. Pad calibration is only for development (owner, 2026-10-17 00:01). The build option is in the code (bd254a0, BUILDING.md c3cdb5a).

## Open decisions

- Keep spare flash for growth, or give all of it to the loop slots.
- Pairing: keep no pairing, or add pairing and a "forget pairing" row (BEHAVIOR.md OQ-11).
- Installer: add ctl.img to an SMK-37 install by itself, or only with --ctl (BEHAVIOR.md OQ-12).

## Not planned

- FM-1 build. We do not maintain it, and we accept its overflow.
- Bluetooth audio.
- Ship JieLi Bluetooth files with the firmware.

Last update: 2026-10-19.


## Rules

- **Removals need the owner's own words (owner rule, 2026-11-12 03:17 UTC+10).** Every removal or disabling of a feature must carry a written reason sourced from the owner's own words (his chat or his own comment, quoted with date and time). An agent commit message, PR body or audit note that only says "per the owner" is not a reason. A merge never removes or disables a feature: if a conflict seems to need it, keep both versions and report it.

## 0.0.25 Classic continuation (owner request, 2026-11-08; checkpoint 2026-11-10)

The October 19 narrative above is retained as history, not the current delivery status.
This section reconciles the existing M1–M13 identifiers in this sole ROADMAP;
it does not create a second backlog or change existing feature/review owners.
Scope pin: `5f578fc0642a1200a5163802bcd69277c511bfc7:docs/ROADMAP.md`, lines 286–298,
PR #812. Owner requests at 2026-11-08 21:09 UTC+10 (integrate/test selected built
0.0.25 features, fixes and optimizations) and 21:16 UTC+10 (continue/help Dot)
authorize this continuation. Later owner messages select SDK V1.2.14 and ask for
actual Elite/Pro files, with the owner handling the push. These are source/build
instructions, not a waiver of the release gates or permission to flash devices.

Local composition retains the verified `8be16486` UI/PARA/LCD delta and ports
bounded donor changes rather than replacing production files wholesale. Remote
PR #698 remains at `48e92b76`; this local branch is a candidate, not a claim that
those remote PRs were merged. SHA-256 manifests and the actual final local Git
commit identify the exported packages. Existing acceptance is not transferred
from donor trees to the combination.

| Existing row | Selected source / disposition | Current local status and remaining gate |
|---|---|---|
| M1 Global PANIC: keyboard FX+SEQ; same-peer SMC BANK L+R in DAW only; none in MMC | #853 `fc0669cc`; actual engine/MIDI cutover and physical-chord repairs | Implemented locally. The engine regression reproduced 42 failures before repair; final focused checks cover fresh note admission, recording, full queues and real SMC ownership. An uncombined DAW BANK action commits on release, so a chord has no earlier navigation side effect. Device validation and independent final review remain pending. |
| M2 Muted fader takes its level on unmute | Existing production behavior | Preserved; no replacement feature. |
| M3 Instant tempo | Existing `R_TEMPO` behavior | Preserved; no replacement feature. |
| M4 Reset selected part saved sound using SMC HOME+SELECT | #860 `c97587c8` | Sound-only reset, existing undo, part/RESET SOUND/NO SAVED SOUND feedback included. Mixer level/pan/sends/mute retained. |
| M5 Unknown item | No recorded definition | QUEUED. Nothing invented or implemented. |
| M6 SMC FX effect selection/throw and proposed drum fill | Approved effect presentation; existing source ownership retained | Unbuilt final effect-presentation work remains QUEUED under M9. Drum-fill proposal is not admitted. Existing FX behavior is retained. |
| M7 Per-part settings ON/OFF | #854 `8a382001`, M7a core | Core note admission and checked saved-state/header support included; all-ON legacy bytes preserved. Full settings UI remains QUEUED, not claimed delivered. |
| M8 PLAY during a question answers NO then normal PLAY; HOME+PLAY redo | #860 (overlaps #845; not applied twice) | Included. Question polling preserves physical HOME/PLAY ownership. Real-path tests cover redo priority, one ordinary toggle and held edges. |
| M9 Approved integration of every selected already-built feature and selected fixes | #853, #860, #861 `ae42441a`, #857 `9435f07b`, #855 `d6b34361`, #854 and current optimization stack | Combined locally: PANIC, reset/PLAY, USB retry/909 lookup, presets, longest chain and part-off core. All 20 production optimization deltas from `a49605bc` are retained/ported, including the exact LZ optimization once. #850/#856/#858 are evidence, not duplicate production patches. Unaccepted engine revision-2 `ca165812` is not silently admitted; independent acceptance remains required. |
| M10 Both-model free pool at least 8,192 B | Exact current native builds, SDK V1.2.14, PADCAL=0 | Both current integrated builds measure 8,328 B free. Rebuild after any firmware-input change; artifact receipts carry exact final hashes and measurements. |
| M11 Automatic passing NIGHTLY; no agent install/flash | Owner now handles push | No publication performed. Model/controller/package checks do not waive M12, performance, full qualification or owner-only device operation. |
| M12 Pinned candidate/ROADMAP completeness and uploader enforcement | Existing #822 owner and scoped acceptance retained | NOT QUALIFIED on this composition. No new independent reviewer or review-budget reset. No source-only acceptance is promoted to final release approval. |
| M13 Straight 1/8 and 1/4 SEQ rates | #824 scoped timing core only | QUEUED beyond existing core; no additional UI/BREAK/editor/storage/protocol stages or downgrade claims admitted. |

### 0.0.25 real build status (2026-11-12 UTC+10, branch `lane/025-zero-dropout`, firmware `1ec5d683`)

Owner 2026-11-11 19:31-19:37 UTC+10: "need to produce a real combined 25 build that is
installable through nightly. and it must be up to roadmap and be a real update",
"without any regressions", "newer sdk is better". Source = recovered 0.0.25 (`8984a297`,
superset of 0.0.24.5 `8be16486`) merged onto `42de2893`, with the items below. Firmware images and the official emu360 gate are from [`1ec5d683`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/1ec5d683cd5d70ef99415509ff3460c33152bcca) (test-only `engine-brk-mute` extract on top of `cd3128e45`; ELFs are byte-identical to `cd3128e45` after swapping the 8-character GIT string). Elite 11,496 µs, Pro 11,488 µs, 0 late during the test, both PASS under 11,609 µs. Host suite on that tip: 413 ok, 1 FAIL (`ctl-compile` stdout/stderr order flake, same on two reruns; not a firmware defect), 1 skip, 0 not-run; `engine-brk-mute` PASS.

| Row | Status in this build | Source |
|---|---|---|
| M1 PANIC | DONE in source. One implementation (recovered `4e036296`): notes-off runs before fresh onsets, so a Note On accepted after the request plays and records; presses queued before it are dropped before they can record. 42de `notes_off_suppress_rec` (d3510b82) and `midi_seq_take`/`cq_pick` not ported: they fix a same-drain silence this design does not have. Confirmed by `panic-clear` (d3510b82 test, ported; prefix-0 case now expects the same-drain press to sound and record, MIDI pushed through the epoch-stamping host queue): 0 failures, both Chicago and Miami builds. 42de queue accept order on overflow ported. | `4e036296`, merge `a08cbeda` |
| M2 muted fader | DONE (unchanged) | 0.0.24.5 |
| M3 instant tempo | DONE (unchanged) | 0.0.24.5 |
| M4 sound-only reset | DONE | #860 via `4e036296` |
| M5 | QUEUED: no definition in ROADMAP history (only "unknown item", 94af36cd). Nothing invented. | - |
| M6 SMC FX effect selection/throw | DONE for the approved effect mockup: held FX key + < > steps that key's effect (existing), approved footer panel (#815 item6-after-effect). Drum fill APPROVED by the owner 2026-11-12 02:29 UTC+10 ("Yes, build the drum fill per the #815 mockup in 0.0.25"): BRK strip REC (SMC note 2) on the FX page, per `item6-after-fill.png`. DONE: hold BRK REC (note 2) on the FX page; from the 909's next bar a 909 snare on every step, 50%..100% over the bar (pattern hits keep their own); release stops at once; stop, panic and leaving the FX page clear it; mute and part-off respected; panel per mockup. Test `drum-fill`. | `311897d7`, this commit |
| Restored lost features (engine swap, bass303 `c4ecd248` -> Chicago) | DONE (lane/025-small-a, merged). Owner 2026-11-12 02:36 UTC+10: audible accent and 303-style slide on Chicago. Owner 02:38: velocity-to-cutoff SKIPPED (not ported). Owner 02:40: one global Tune for Chicago and Miami, GLO > AUDIO > TUNE, default 0 = concert pitch (bit-exact). Tune and QUEUE MUTES share the spare settings byte: bits 0-6 Tune, bit 7 QUEUE MUTES (settings_t stays 40 B). Integration fix [`4044db77e`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/4044db77e53c680538903e01a2b622cc66a15231): the accent output gain was clamped to the Chicago gain stage's valid 0..1; unclamped, an accented note faulted the bass part (cq-reclaim, audio-stress). Owner 2026-11-12 07:31 UTC+10: "Tone accent down to about +6 dB" (x2.0, `BASS_303_ACCENT_GAIN` 2.0f in `bass_303_adapt.h`; was x3.47, about +10.8 dB). In this combined build via [`e29664d3`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/e29664d3f) merge [`f7280228`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/f7280228b). Test `bass-303-adapt`. | [`465e8bbf6`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/465e8bbf6913f8470ec9ceb673d3e3b23d54a7e2), merge [`b28a35957`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/b28a359577d48e47acd38b4925296d860e62f710), [`8d667efcb`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/8d667efcb6c50d53666b9fb5774c350b1daddea7) |
| Per-pattern preset recall | DONE. Owner 2026-11-12 02:42 UTC+10. SEQ > PRESET page > PAT SOUND per bass part (its own page after SEQUENCE, so the SOUND page keeps its right-half STATUS cell, [`50c96d2d8`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/50c96d2d84929662e3c48fcaf111f0a5b991c4e9)) per pattern (OFF, or a Chicago/Miami factory preset). Stored in pattern_t.rsv[0..1]; kits have no presets, so dpart_t is unchanged. Applied when that pattern becomes the playing one, through the existing preset switch (it waits for a quiet voice, so a long legato line can delay it). Unset = bit-exact. Stream format 14 only when used; a project without it saves as 13 and still loads on 0.0.24.x; a rev-13 project loads with these bytes 0. Test `pattern-preset-notelock`. | [`29296cbd7`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/29296cbd72a7b0a9e764e06ec55bf98e0c389331), merge [`ccd1a646b`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/ccd1a646bf5778d988548584e8ccb8af3318f0b0) |
| Per-note bass sound lock | DONE. Owner 2026-11-12 02:43, 03:12 UTC+10. Capture restored from `c4ecd248` (`279df014c`/`76f9f88fd`, deleted in `18481a63`) into motion lanes (target 0xfd); new playback applies the locked sound at that note-on and restores the base sound on the next onset or stop. GLO > AUDIO > NOTE LOCK turns capture on (runtime, not saved). Off = bit-exact. Shares the stream-14 bump. | [`29296cbd7`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/29296cbd72a7b0a9e764e06ec55bf98e0c389331), merge [`ccd1a646b`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/ccd1a646bf5778d988548584e8ccb8af3318f0b0) |
| M6 drum-bus FX suit | Owner decisions 2026-11-12 02:33 UTC+10: "Let drums have similar fxs" and "It doesn't sound right that 808 and 909 have different fxs from break". 909, 808 and BREAK share one suit: REV, DLY, STUT, SHUF, GATE, HP (page 1: REV DLY STUT SHUF; page 2: GATE HP). Supersedes the #370 split (no GATE on 909/808). BREAK stutter is real: one mono ring channel (13 in total). | this commit |
| M7 part ON/OFF | DONE: core + saved state (#854 via `4e036296`); GLOBAL > AUDIO toggles per part (#815 item 7). | `4e036296`, `336eaa4f` |
| M8 PLAY answers question, HOME+PLAY redo | DONE | #860 via `4e036296` |
| M9 integration | DONE for built items on lane/025-small-b: the owner-approved #737 SMC mixer page peek (PICKUP hint and knob ACTIVE, plus a short locked Pause peek) is ported from `08e8d8573`. Peek state is a small byte buffer (no L14 paint cache). The heavy L14 snapshot (`9ffe5d6d`, `13b43154`, `fb6b5827`, +1,888 B .bss) is still not merged. The default build has one SMC peer. | this branch |
| M10 pool >= 8,192 B | DONE: 8,320 B free on Elite and on Pro (pool 335,744 of 344,064), measured on `1ec5d683` release images | [`1ec5d683`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/1ec5d683cd5d70ef99415509ff3460c33152bcca) |
| M11 NIGHTLY | Images built from [`1ec5d683`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/1ec5d683cd5d70ef99415509ff3460c33152bcca) (GIT `1ec5d683`, `0.0.25`). Official emu360 gate: Elite 11,496 µs and Pro 11,488 µs, 0 late during the test, both under 11,609 µs, PASS. Catalog id `nightly-1ec5d683-dev`, DEVICE NOT TESTED. Elite package SHA-256 `11b0795fce853bd2ca648a8afe399ce84bf25c572796840ebc9b08581528039a`, Pro `f8b2aa2e1f27ea0fc8a1a4f818bffa2bd188f7a1eaa62788c1787c67c0c75d1b`. | publish |
| M12 completeness gate | PARTIAL: #822 not adopted. It needs a pinned proof for every required ROADMAP row (2 PRESENT / 144 MISSING at its last audit) and an independently accepted release selection; not populated. | #822 |
| M13 1/8, 1/4 rates | DONE on lane/025-small-b: timing core and drum/bass selectors retained. BREAK SEQ RATE is 1/16 (default), 1/8 or 1/4; the generator stays on a 16th clock and groups those 16ths. Downgrade policy: a project file that uses RATE_8 or RATE_4 is format 2, so 0.0.24.5 refuses it instead of playing 1/8 as 1/16 and 1/4 as 1/16T (`rate & 3`). Files that only use rates 0-3 stay format 1. Device settings stay format 1. | `2a81b7ff`, `336eaa4f`, this branch |
| SMC CLIP KEYS tag | DONE: ported from 42de2893 | `03c8f007` |
| Queued-mutes toggle | DONE (lane/025-small-b, merged): GLO > AUDIO > QUEUE MUTES (default off, bit-exact). On, mute/unmute waits for the next 909 bar; a second press cancels; Stop applies a pending change; stopped presses are immediate. Saved in bit 7 of the spare settings byte (bits 0-6 are Tune). | [`e2ecdb045`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/e2ecdb04578a59187ca29aa04e12cc98227defee), merge [`8d667efcb`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/8d667efcb6c50d53666b9fb5774c350b1daddea7) |
| SMC ECHO page | DONE (lane/025-small-a, merged): the third SMC FX page ECHO SELECT (ROOM/TIGHT/ECHO) is back per the owner (03:12). | [`aeb69f71a`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/aeb69f71adaadcc1478f826adb02335a88f8e09b), merge [`b28a35957`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/b28a359577d48e47acd38b4925296d860e62f710) |
| SMC page peek (#737) | DONE (lane/025-small-b, merged): see M9. | [`1b17a7ef0`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/1b17a7ef07c008e913a874c6e12f5c5dff80bd94), merge [`8d667efcb`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/8d667efcb6c50d53666b9fb5774c350b1daddea7) |
| M13 BREAK rate | DONE (lane/025-small-b, merged): see M13. | [`420746f31`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/420746f3198977f5a21213912ffaeff701754bc9), merge [`8d667efcb`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/8d667efcb6c50d53666b9fb5774c350b1daddea7) |
| Third imported-loop sample slot | DONE. Owner 2026-11-12 03:23 UTC+10: restore it; its cut (`e758e195` / #351) had no reason in the owner's words. Adapted port of `bb32802e`: slot 3 is 0xB3000-0xBAFFF, 32 KiB (not 44 KiB, because the sector-0 shadows now use 0xB0000-0xB2FFF). Slots 1 and 2 do not move. Installers keep slots 1 and 2 and back up slot 3 on a downgrade. At the slot-3 merge, RAM was 98,172 of 98,304 B. Later bit-exact cuts reduced `.data+.bss` to 95,556 of 98,304 B (2,748 B free) on the `1ec5d683` Elite and Pro release images; pool stayed 8,320 B free. | [`70ccf9723`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/70ccf97234bb4eea3ff59dc35880e55af47a8f3f), [`a6f59c103`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/a6f59c1038fae5cc0dcfd64a6c9ae64d6d819208), merge [`acad8006a`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/acad8006aba95a27694e67894a49d6784aab383d) |

### Preserved packaging requirements

Models `SMK-37 Elite_900` and `SMK-37 Pro_900`; `X0X_DEV_PADCAL=0`.
Owner-selected SDK: V1.2.14; build inputs are hash checked, not accepted on archive
filename alone. Controller remains 61,552 bytes, SHA-256
`a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`, embedded CRC32
`564a8f55`. No `BTIF`, `USRTRIM`, efuse, key_mac or bootloader operation is
performed. No `main`, `.github`, nightly catalog or iPhone path is changed.

Full host results are reported with baseline failures and explicit focused
reruns; old receipts never qualify this new tree. Optimizer audio validation
compares 17 scenarios / 50 audio-state files before versus after the optimization
port. Host ABBA15/A-A measurements are separate from native/device timing and
must report noise or slowdown honestly. Device tests remain NOT_RUN. Exported
BUILD_ONLY candidates are not a passing NIGHTLY release or a safe-install claim.

## 0.0.26 (owner requests, 2026-11-12 UTC+10)

Source for each row is the owner's own chat words. Status is from the box at 2026-11-12 12:50 UTC+10. No 0.0.26 code is written yet.

| # | Item | Owner source | Status | Link |
|---|------|--------------|--------|------|
| 26.1 | Drone synth engine: lightweight, full-featured, majestic sound, runs smoothly under heavy load, so DnBOX can be fully standalone. Search other custom firmware for the best one. | 03:32 "search other custom firmware to add a drone engine and chords support for it in seq for 26build ... best lightweight but feature full synth ... run it smoothly at hard load and will sound majestically ... fully standalone device" | RESEARCH DONE. Pick: Mutable Instruments Plaits String Machine (MIT), about one Chicago voice per sample on the host (36-38 ns/frame); device cost NOT MEASURED. | `/workspace/drone-engine-research-026.md` (box) |
| 26.2 | The drone is a third synth engine, selected in a synth slot like Chicago and Miami. No 6th part, no song-format change; it uses that slot's level, mute and FX. | 03:49 "Drone engine is just another synth engine don't you get it?" | DESIGN AGREED. | research file, "OWNER CORRECTION" |
| 26.3 | Chords in the sequencer from held keys recorded to steps; no one-finger auto-chords. Live checks: chord changes land on the step; held chords sustain across steps and release cleanly on mute, Stop and PANIC; pattern switches leave no hanging notes; predictable voice stealing; CPU safe at full load. | 03:32 "chords support for it in seq"; 03:49 "it shouldn't play chords with 1 note I think, or it can but there's a lot of things to be considered for a live performance" | DESIGN AGREED. | research file, "OWNER CORRECTION" |
| 26.4 | Juno-6-style (or similar classic polysynth) slot engine. | 06:10 "Maybe there's something like a juno6?"; "Or smth cool like it" | AGREED (owner 12:50 "I asked for it"). Plan: Juno-60-style engine on the Plaits analog oscillator/filter model (MIT), 6 voices, about 160 B per voice, host 32-39 ns/sample per voice; still needs osc mix, HPF, envelope, LFO. Ruled out: AMY full library (size), Junox/KR-106/OB-Xd (GPL). | `/workspace/juno-engine-research-026.md` (box) |
| 26.5 | Cheap chorus for that engine, ideally without a big delay line. | "Maybe there's a cheaper way to achieve chorus without delay line" | AGREED (owner 12:50, part of the Juno plan). Plan: one shared 16-bit short delay, 512 B, Juno-60 chorus I/II range 1.7-5.4 ms, about 11 ns/sample on the host, cost does not grow with voices. Delay-free options sounded like detune or phaser. | `/workspace/juno-engine-research-026.md` (box) |
| 26.6 | fm1-emu (MIT, 360 MHz, SMK-37 adapter) is the device-accurate CPU gate for every build from 0.0.26 on. | 03:50 owner approved the plan (thumbs up) | DONE for 0.0.25: emulator fixed to 360 MHz and pushed (branch `emu/fm1-emu-360mhz` 8c8fa82); gate `/workspace/zdprobe/dnbox360/gate.sh`, PASS = worst block < 11,609 µs and 0 late (owner 07:26). | `emu/fm1-emu-360mhz` |

Every 0.0.26 item still has to pass all standing rules: no slower than the previous build, zero dropouts at < 11,609 µs per core under heavy load, no added latency, pool reserve >= 8,192 B, nothing removed without the owner's words.
