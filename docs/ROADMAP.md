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

### 0.0.25 real build status (2026-11-12 UTC+10, branch `lane/build-025-real-20261112`)

Owner 2026-11-11 19:31-19:37 UTC+10: "need to produce a real combined 25 build that is
installable through nightly. and it must be up to roadmap and be a real update",
"without any regressions", "newer sdk is better". Source = recovered 0.0.25 (`8984a297`,
superset of 0.0.24.5 `8be16486`) merged onto `42de2893`, with the items below. Gate numbers
are in the NIGHTLY catalog entry for this source commit.

| Row | Status in this build | Source |
|---|---|---|
| M1 PANIC | DONE in source. One implementation (recovered `4e036296`): notes-off runs before fresh onsets, so a Note On accepted after the request plays and records; presses queued before it are dropped before they can record. 42de `notes_off_suppress_rec` (d3510b82) and `midi_seq_take`/`cq_pick` not ported: they fix a same-drain silence this design does not have. Confirmed by `panic-clear` (d3510b82 test, ported; prefix-0 case now expects the same-drain press to sound and record, MIDI pushed through the epoch-stamping host queue): 0 failures, both Chicago and Miami builds. 42de queue accept order on overflow ported. | `4e036296`, merge `a08cbeda` |
| M2 muted fader | DONE (unchanged) | 0.0.24.5 |
| M3 instant tempo | DONE (unchanged) | 0.0.24.5 |
| M4 sound-only reset | DONE | #860 via `4e036296` |
| M5 | QUEUED: no definition in ROADMAP history (only "unknown item", 94af36cd). Nothing invented. | - |
| M6 SMC FX effect selection/throw | DONE for the approved effect mockup: held FX key + < > steps that key's effect (existing), approved footer panel (#815 item6-after-effect). Drum fill APPROVED by the owner 2026-11-12 02:29 UTC+10 ("Yes, build the drum fill per the #815 mockup in 0.0.25"): BRK strip REC (SMC note 2) on the FX page, per `item6-after-fill.png`. DONE: hold BRK REC (note 2) on the FX page; from the 909's next bar a 909 snare on every step, 50%..100% over the bar (pattern hits keep their own); release stops at once; stop, panic and leaving the FX page clear it; mute and part-off respected; panel per mockup. Test `drum-fill`. | `311897d7`, this commit |
| Restored lost features (engine swap, bass303 `c4ecd248` -> Chicago) | Owner 2026-11-12 02:36 UTC+10: restore audible accent and 303-style slide on Chicago. Owner 02:38: velocity-to-cutoff SKIPPED (not ported). Owner 02:40: fine Tune is ONE global setting for Chicago and Miami (not a Chicago pot), default 0 bit-exact. QUEUED. | owner 02:36, 02:38, 02:40 |
| Per-pattern preset recall | APPROVED by the owner 2026-11-12 02:42 UTC+10: preset number per part per pattern, switches on the bar with the pattern; design `54bb3c496` (docs/plans/2026-10-25-native-preset-workflow.md, ported). Firmware NOT STARTED: needs a save-format rev bump with rev-13 load, bar-boundary apply on the audio side and a UI. | owner 02:42, doc 5978ac9c |
| Per-note bass sound lock | APPROVED by the owner 2026-11-12 02:43 UTC+10 (visual change approved): enable `bass_note_sound.h` (donors `279df014c`, `76f9f88fd`). BLOCKED: the donors only CAPTURE the note sound into a motion lane (stream 7). No branch or commit ever PLAYED it back (searched `git log --all`). The capture functions were deleted with bass303 in `18481a63`. Turning the flag on alone would use motion lanes and refuse recording when the pool is full, with no audible effect. Playback (apply the 32-byte cell at the onset without an audio spike) is new design. Not enabled. | owner 02:43 |
| M6 drum-bus FX suit | Owner decisions 2026-11-12 02:33 UTC+10: "Let drums have similar fxs" and "It doesn't sound right that 808 and 909 have different fxs from break". 909, 808 and BREAK share one suit: REV, DLY, STUT, SHUF, GATE, HP (page 1: REV DLY STUT SHUF; page 2: GATE HP). Supersedes the #370 split (no GATE on 909/808). BREAK stutter is real: one mono ring channel (13 in total). | this commit |
| M7 part ON/OFF | DONE: core + saved state (#854 via `4e036296`); GLOBAL > AUDIO toggles per part (#815 item 7). | `4e036296`, `336eaa4f` |
| M8 PLAY answers question, HOME+PLAY redo | DONE | #860 via `4e036296` |
| M9 integration | DONE for built items. Not merged: L14 SMC snapshot (`9ffe5d6d`, `13b43154`, `fb6b5827`): it is built on the keyboard page peek that the owner withdrew in 3f7baf16 and needs +1,888 B .bss against 716 B RAM headroom. The default build has one SMC peer. | this merge |
| M10 pool >= 8,192 B | DONE: 8,328 B free on Elite and on Pro | build |
| M11 NIGHTLY | DONE: see the catalog entry for this source commit | publish |
| M12 completeness gate | PARTIAL: #822 not adopted. It needs a pinned proof for every required ROADMAP row (2 PRESENT / 144 MISSING at its last audit) and an independently accepted release selection; not populated. | #822 |
| M13 1/8, 1/4 rates | PARTIAL: timing core (`2a81b7ff`), RATE selectors for drum and bass parts and offline editor/import (`336eaa4f`). QUEUED: BREAK rate (fixed 1/16), downgrade policy (0.0.24.5 reads rate & 3, so 1/8 and 1/4 load as 1/16 and 1/16T there). | `2a81b7ff`, `336eaa4f` |
| SMC CLIP KEYS tag | DONE: ported from 42de2893 | `03c8f007` |
| Queued-mutes toggle | DONE on lane/025-small-b: GLO > AUDIO > QUEUE MUTES (default off, bit-exact). On, mute/unmute waits for the next 909 bar; a second press cancels; Stop applies a pending change; stopped presses are immediate. Saved in the global settings brightness byte (1 = on, any other value = off). | this branch |

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
