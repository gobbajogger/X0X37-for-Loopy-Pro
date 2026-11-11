# DnBOX one-build: Elite and Pro device acceptance checklist

**Scope:** Owner-operated Mac hardware acceptance for SMK-37 Elite and SMK-37 Pro, using the single review candidate from [PR #734](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/734) (`integration/all-in-one`). This is a **test plan**, not an install approval, test receipt, promise that features have merged, or authorization to flash protected areas. Use one exact immutable candidate and record actual results separately on each model.

**Source baseline when this checklist was written:** `integration/all-in-one@50866a1e43e3d2448daba4cc64a7a112c0a7cf42` (docs/evidence staging HEAD; the most recent listed firmware-test source was `3da0f0d27705088a9aff50471ea1c3d1b1e972a4`). The reported latest exact-source full host suite was **244 PASS, 69 FAIL, 3 NOT_RUN**, while Elite and Pro native SDK V1.2.1 development builds exited successfully. **Full-host 0-fail/0-skip, DEV CHECKED, installation and physical-device acceptance are not established.** Do not install this unqualified staging image simply because a `.fwsc` file exists.

**Source-of-truth crosswalk:** [#734 integration log](integration/all-in-one-log.md), [feature registry](FEATURE-REGISTRY.md), [#699 full 122-feature plan](lanes/missing-features-plan.md), [#734 feature audit](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/docs/feature-check-734/docs/feature-check-734.md), [SMC input mapping](SMC_MIXER.md), [#736](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/736), [#737](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/737), [#738](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/738), [#735 two-peer BLE](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/735), and [#741 partial SELECT-only FX correction](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/741). Audit labels below refer to **source presence**, not a previous hardware PASS. Check live branch and registry again before any hardware session.

## How to record results

1. Use a separate row/log for **Elite** and **Pro**. Start every test at **NOT_RUN**; after observing behavior record **PASS**, **FAIL**, **NOT_RUN**, or **NOT_AVAILABLE (not present in the exact image)**. Do not call a test PASS from a screenshot, PR text, build exit or simulator.
2. Record exact Git source SHA, app/controller image SHA-256, model confirmed from the physical case, SDK/toolchain pins, installer method, connection path, firmware identity, date, symptom, result and evidence file for each run.
3. Run read-only or reversible checks first. Use only a disposable revision-13 project and owner-authorised sample slots for destructive actions. Do not rely on USB device strings alone for model identity.
4. Mark a planned feature **NOT_AVAILABLE** until its exact port and accepted image are demonstrated. The unmerged #736/#737/#738/#735/#741 are not automatically in #734. Never invent an expected FX default map missing from the recovered owner specification.

## 0. Pre-install gate, loop-slot backup and checksum audit (MUST come before installation)

### 0.1 Freeze a qualified exact package, then protect user data

1. Confirm the **full** exact-source `tests/run_tests.sh` reports **0 FAIL / 0 SKIP / 0 NOT_RUN** for required coverage, that BOTH SDK V1.2.1 Elite and Pro development builds have proper model identities and `pool_free >= 8192`, and that the designated reviewer has matched app and controller SHA-256 bytes to those reports. If any condition fails, **STOP; do not install**. The baseline above fails this gate.
2. Obtain the matched, owner-approved **Elite** and **Pro** local development packages and verify SHA-256 on the Mac. Check the **physical model** label; some older Elite units may identify as Pro over USB. Verify the model-specific Bluetooth controller image independently if BLE is to be tested. No binary is sourced from an unverified public picker.
3. On a booted, still-working keyboard **before any install or reset**, connect USB to Mac Chrome/Edge, close other MIDI apps and open the existing authorised Samples workspace. Click **Read current slots**, then **Back up all slots**. Save a complete `.x0xslots` archive for each physical model, even if no slots appear populated. Do not confuse a project/song export with a sample-slot backup.
4. The read-only Mac CLI alternative from the matching source checkout is `python3 tools/x0x_slots.py backup dnbox-elite-before.x0xslots` and, separately on the Pro, `python3 tools/x0x_slots.py backup dnbox-pro-before.x0xslots`. Do not use `--force`; these filenames must not already exist. Archive creation validates the device's slot header/data CRC while reading.
5. Verify each saved archive **offline and before installation**: `python3 tools/x0x_slots.py show dnbox-elite-before.x0xslots` and `python3 tools/x0x_slots.py show dnbox-pro-before.x0xslots`. This validates the archive-wide CRC-32, each occupied slot image CRC-32, and each slot header's encoded sample-data CRC-32; corrupted or missing/empty-misreported occupied slots cause failure. Save the printed name, slot number, loop count, used length and CRCs, not just the archive filename.
6. For a directly printable per-slot checksum manifest, run the read-only script below from the repository root after saving each archive; it uses the actual parser and refuses corrupt archives.

```bash
PYTHONPATH=tools python3 - <<'PY'
from pathlib import Path
from x0x_slots import read_archive
for name in ("dnbox-elite-before.x0xslots", "dnbox-pro-before.x0xslots"):
    p = Path(name)
    if not p.exists():
        print(f"{name}: not saved yet; DO NOT INSTALL this model")
        continue
    meta, images = read_archive(p.read_bytes())
    print(f"{name}: {len(images)} slots; archive and slot CRCs VALID")
    for slot in meta["slots"]:
        if slot["empty"]:
            print(f"  slot {slot['slot']}: EMPTY")
        else:
            print(f"  slot {slot['slot']}: {slot['name']}; loops={slot['zones']}; "
                  f"image_crc32={slot['crc32']:08x}; data_crc32={slot['data_crc32']:08x}")
PY
```

**Expected before install:** For **both** devices, durable, independently readable complete backups with every originally occupied slot included; all per-slot image/data CRCs and archive CRC pass. If one read or checksum fails, leave the original firmware running. Never replace a missing or failed backup with an empty archive. Store backups on the Mac outside the project repository and never upload personal samples to GitHub.

### 0.2 Install only after 0.1 passes

1. Use the approved local installer in Mac Chrome/Edge (Web MIDI) with no other MIDI client attached, or the specifically approved model-safe install path. Select the matching Elite/Pro `.fwsc`; confirm the model and immutable package/controller digests against the exact build receipt. Do **not** rely on the public version picker for #734.
2. Read the slot-map/restore plan; reject any unexplained slot move, truncated loop, missing bank or impossible restore destination. Confirm the installer retained the pre-write recovery archive. No silent migration/re-encoding is acceptable.
3. Start the ordinary guarded application install on **one model at a time**, preserving USB power and logs. Do not write bootloader, BTIF `0xC0000–0xC0FFF`, USRTRIM `0xC1000`, efuse or `key_mac`; never invoke `fm1_rescue.py`, `--force` or any recovery-flash shortcut.
4. Restart normally; verify logo/firmware identity, original pads/keys, transport, output audio, MIDI and a usable HOME screen before touching the second model. If startup fails, stop further device work, keep backups and exact logs and hand off to the approved device recovery owner.

**Expected:** Explicit guarded model match, verified source-to-package/controller bytes, successful ordinary boot and no writes to protected regions. A build PASS alone is not an install PASS.

### 0.3 After install: re-read all sample slots and compare per-slot CRC

1. From the newly booted firmware, **Read current slots** and make another independent read-only complete archive per model. If using CLI, save as `dnbox-elite-after.x0xslots` and `dnbox-pro-after.x0xslots` (same `backup` command, new filename).
2. Run the following offline comparison, which validates *both* archives on every open and compares explicit slot occupancy, identity, image CRC-32, encoded sample-data CRC-32 and exact slot bytes.

```bash
PYTHONPATH=tools python3 - <<'PY'
from pathlib import Path
from x0x_slots import read_archive
failed = False
for model in ("elite", "pro"):
    try:
        before_meta, before = read_archive(Path(f"dnbox-{model}-before.x0xslots").read_bytes())
        after_meta, after = read_archive(Path(f"dnbox-{model}-after.x0xslots").read_bytes())
        if len(before) != len(after):
            print(f"{model}: FAIL slot count {len(before)} -> {len(after)}")
            failed = True
            continue
        for i, (old, new) in enumerate(zip(before, after), 1):
            a, b = before_meta["slots"][i - 1], after_meta["slots"][i - 1]
            keys = ("empty", "name", "zones", "samples", "rates", "data_len", "crc32", "data_crc32")
            differences = [k for k in keys if a.get(k) != b.get(k)]
            if (old is None) != (new is None) or old != new or differences:
                failed = True
                print(f"{model} slot {i}: FAIL bytes or metadata differ: {differences}")
            else:
                status = "EMPTY" if old is None else "CRC %08x" % a["crc32"]
                print(f"{model} slot {i}: PASS {status}")
    except Exception as exc:
        failed = True
        print(f"{model}: FAIL unreadable or missing archive: {exc}")
if failed:
    raise SystemExit(1)
print("Both models: ALL SAVED SLOTS EXACTLY PRESERVED")
PY
```

3. If installer intentionally moved a slot, do **not** mark the strict same-slot comparison PASS. Document the old/new slot map, verify byte identity and every loop-name/order reference independently, then check the A/B loop selectors in **all** saved BREAK patterns. Restore only to an explicitly approved disposable target after reviewing what it will erase. Any skipped slot or changed CRC remains a FAIL or partial result until resolved.

**Expected after install:** Same-slot byte-for-byte preservation of every user loop, or a separately evidenced, authorised full migration preserving loop identities and their A/B assignments. The code above deliberately fails closed on silent reordering. Sample checks must pass again after CLEAR SONG and RESET PROJECT.

## 1. Device and Mac functional tests for source present in #734

Run each applicable case on **both** physical models. Steps refer to the real exposed UI at the qualified SHA. If an ID is found in source but unreachable on the installed image, record FAIL, not a fabricated PASS. Each heading's number is the #699 feature ID.

## A. Engines, sequencing, clock, BREAKS and kits

### ID 001: Choose Chicago and Miami
1. On each keyboard, select the first synth part, choose Chicago and play a chromatic phrase.
2. Select Miami on the other synth part, play the same notes, then switch back.

**Expected:** Both original engines are selectable, sound, and remain in their intended slots; neither retired Bass303 nor Wobble engine is exposed as a selectable third engine.

### ID 002: Original engine sound
1. Record a short fixed-velocity dry phrase from Chicago and Miami through the same output path used for the approved original-engine reference.
2. Compare tone, envelope, filter sweep, transitions and effects against the corresponding approved reference at matched level.

**Expected:** No audible dropouts, distortion or unintended sonic change; bit-exactness is NOT established by listening and needs the separate golden-byte test.

### ID 004: Pitch wheel and damper
1. While holding a Chicago note, move the SMK wheel from minimum through centre to maximum, then release it.
2. Repeat on Miami; attach the approved pedal and press/release while playing held notes.

**Expected:** Bend responds in the correct direction, centres correctly and returns; damper and note releases follow their physical controls without stuck voices.

### ID 005: Chicago effect chain
1. On Chicago, step through every available effect type including phaser, chorus/ensemble, gate reverb, delay, EQ, crush and ring where exposed.
2. Turn wet and timing controls at quiet monitor volume, including tempo-synced delay at 20 BPM.

**Expected:** Each exposed effect has the expected distinct audible action, clean bypass and stable tail; no silence, excessive feedback or unexpected bypass.

### ID 006: Factory presets and names
1. Browse Chicago's 55 and Miami's 30 registered factory patches and confirm the first, middle and last labels.
2. Play a note at each, leave one preset selected, navigate away and return.

**Expected:** Preset identity and labels are meaningful, patches produce the selected sound, and choices persist while the part is active; do not assume old 81-program banks.

### ID 007: Engine and preset per pattern
1. Create two patterns on one synth part with different approved original-engine preset assignments.
2. Switch patterns in stop and play states, then save and reload a revision-13 project.

**Expected:** Each pattern recalls its own chosen sound/engine state rather than leaking the last edited state; any discrepancy is FAIL.

### ID 009: Only supported synth engines
1. Check both synth slots and their sound menus after a clean boot and project load.
2. Play a previously configured 303A and 303B pattern.

**Expected:** Chicago/Miami occupy the two established synth slots; no extra retired engine or incompatible legacy playback path appears.

### ID 010: Preset changes without clicks
1. Sustain a quiet phrase and change between two very different factory sounds.
2. Listen on headphones and inspect output meter while repeating at low and high velocities.

**Expected:** Displayed names match the selected patches, transitions do not produce a sudden full-scale click, and notes remain controllable.

### ID 011: Long gates and ties
1. Enter one note with a tie spanning at least three steps in the synth sequencer.
2. Play across the tie and stop before its end; restart.

**Expected:** The gate lasts the intended duration without retriggering each step, and Stop releases it without a stuck note.

### ID 012: Record held-note length
1. Record one short and one long physical key press into separate steps.
2. Replay and compare the resulting gate lengths to the captured durations.

**Expected:** The note lengths differ appropriately and replay follows the real key release rather than forcing one-step notes.

### ID 013: Probability and skipped notes
1. Program alternating probability values including 0%, 100% and an intermediate value on drums and synth notes.
2. Play for many bars while watching LEDs and recording emitted MIDI if connected.

**Expected:** 0% never triggers, 100% always triggers, intermediate steps vary as configured, and silent steps emit no phantom MIDI Note On.

### ID 014: RANDOM preview consistency
1. Open RANDOM preview on a known pattern and note the shown result without applying it.
2. Apply the preview, then replay several bars and reopen preview under the same seed/state where supported.

**Expected:** Committed notes match the preview; merely viewing a preview does not advance the underlying random sequence.

### ID 017: Motion and recording lifecycle
1. Record a filter-motion gesture, stop, restart and play the take.
2. Change project or reset only after a separate disposable save, then return to motion playback.

**Expected:** Motion starts/stops with transport, old held recording flags are cleared on stop/load/reset, and it never continues writing in an unrelated project.

### ID 018: Internal/external clock handover
1. Play at a chosen internal tempo, then start accepted external MIDI Clock from the Mac/Loopy Pro.
2. Stop external Clock, restore internal operation and repeat during live pattern playback.

**Expected:** Sequencer phase and BPM follow the active accepted source, do not jump to unrelated time, and recover after external clock loss.

### ID 019: MIDI transport order
1. From Mac MIDI, send Start, Clock, Stop, Continue, and rapidly alternating Stop/Start to the intended port.
2. Monitor physical transport LED and notes during a queued start and stop.

**Expected:** Commands are processed in order; Continue resumes appropriately, Stop drains scheduled notes, and no stuck notes or duplicated starts occur.

### ID 020: Beat indicator while stopped
1. With external Clock running but sequencer stopped, watch SMC beat feedback for several beats.
2. Start and stop sequencer, then stop external clock and edit internal tempo.

**Expected:** Feedback marks the actual measured beat and phase while stopped, responds to tempo edits, and never pretends transport is playing.

### ID 021: Miami RATE pads
1. Enter Miami Free Play and use all eight RATE-selection pads while holding an unrelated note.
2. Repeat a RATE change during play and return to the initial division.

**Expected:** Each pad selects a distinct intended rate, the chosen rate is shown and heard, and unrelated keys are not swallowed; four SHAPE pads are not included in this acceptance.

### ID 022: Legato records slide
1. While recording a synth pattern, overlap the end of one note with the start of another.
2. Play the pattern back and compare to separated-note recording.

**Expected:** The overlap produces slide/legato rather than two short unrelated triggers; Note Off is respected.

### ID 023: Separate 909 and 808 kits
1. Select the 909 kit and program bass drum, snare and hat steps; repeat separately on 808.
2. Switch kits, visit both drum sound pages and return.

**Expected:** Each kit retains its own 11-voice identity, levels and pattern; the kit-wide slot is not mislabelled as drum 12.

### ID 024: Two independent BREAK loops
1. Load authorised loop A and loop B, select different slice divisions and start them together.
2. Edit B's slice choice, stop B, and listen to A without changing its settings.

**Expected:** Each loop has independent slice, playback and storage state; changes to one loop do not overwrite the other.

### ID 025: BREAK slice key navigation
1. Open BREAK view and use its documented physical black-key slice/page controls.
2. Change slice subdivisions and visit both loops without entering the synth sound pages.

**Expected:** Displayed slice index, selected loop and resulting audio track the intended key; page switching never unexpectedly changes octave or part.

### ID 026: BREAK MIDI output
1. Trigger each active BREAK slice from the pads while recording Mac MIDI input.
2. Release the pad and stop the loop.

**Expected:** Only accepted BREAK hits create correctly paired MIDI note events on the configured output/channel, without hanging notes.

### ID 027: BREAK pitch bend
1. Hold a BREAK slice and move the SMK pitch wheel through centre and extremes.
2. Release the wheel and retrigger the slice.

**Expected:** BREAK pitch responds to the correct source, returns to centre, and does not bend a different synth part.

### ID 028: Jungle stretch
1. On a short BREAK loop, compare normal rate against Jungle stretch at a slower tempo.
2. Switch between loop A/B and back while playing.

**Expected:** Jungle stretching changes the rhythm without relabelling a legacy HALFTIME mode, freezes or cross-loop data loss.

## B. Physical SMC mixer, routing and control safety

### ID 030: Eight-strip mixer screen
1. Open main MIX and touch faders 1 through 8 in order.
2. Switch to 909 and 808 MIX and inspect both drum banks.

**Expected:** The full 8-strip page is readable on the 240×240 screen and follows the active part, kit and bank without clipping labels.

### ID 032: Pause context lock and LED
1. Move a fader to a known target and press SMC Pause once to lock that controller's destinations.
2. Navigate the keyboard to a different part, then press Pause again and hold at least 900 ms to unlock.

**Expected:** Fader and knob targets remain captured while locked, the lock LED is correct, and the next long hold unlocks without accidentally sending transport commands.

### ID 034: SMC disconnect/reconnect
1. Pair one authorised SMC over Bluetooth, hold an SMC Solo or momentary SELECT effect, then disconnect its radio link.
2. Reconnect and move a fader from a safe position.

**Expected:** All stale held notes, solo and effects clear on loss; reconnect does not replay the old key press or create a volume jump.

### ID 035: SMC versus other controllers
1. While an SMC is connected, send normal musical notes and transport from the Mac/Loopy USB/BLE port.
2. Send SMC USER-mode controls on the configured host channel 16, then send similar CCs from an unrelated channel.

**Expected:** Only correctly routed channel-16 User controls affect SMC targets; synth notes and APC40/clip messages keep their own routes.

### ID 036: BLE peer name and status
1. Open GLOBAL → BLUETOOTH MIDI and view the connected peer name/status while one SMC is linked.
2. Disconnect and re-pair it; with an approved two-link candidate also connect the second SMC.

**Expected:** Current names and connectivity update, stale disconnected names disappear, and both peer names appear only when two-link controller/app support is actually installed.

### ID 038: Drum mixer second SELECT
1. From 909 MIX, press SELECT on the current drum, then press the same SELECT again.
2. Repeat in 808 MIX and verify the current kit remains active.

**Expected:** The selected drum/kit is explicit and the repeated selection takes the documented drum-mixer navigation path without toggling recording or an FX.

### ID 039: Knobs have different targets from faders
1. In HOME MIX, use fader 1 for 909 level and knob 1 for its cutoff; repeat on strip 6 for reverb and strip 7 for delay.
2. Switch to a drum bank and use fader level versus knob pan.

**Expected:** Encoders adjust a different correct dimension from the matching faders; duplicate visible-field destinations are parked instead of doubled.

### ID 042: Full control overlay, 1.2 seconds
1. From HOME, move a fader then an encoder, and read the temporary complete SMC control screen.
2. Stop touching controls and wait more than 1.2 seconds; repeat while a help card or confirmation is open.

**Expected:** The overlay shows real destination, target and pickup state, extends only on real activity, then returns to the original view. Help remains usable; confirmation remains guarded.

### ID 043: Silent follow on keyboard page change
1. Open an SMC overlay, then change part and page using the SMK keyboard.
2. Observe the SMC display and send no new SMC input until the timeout finishes.

**Expected:** The mixer follows the keyboard context silently without reopening a stale overlay or changing the keyboard's chosen page.

### ID 044: Queued pattern and cancel
1. While a pattern plays, tap SMC Rewind/Fast-forward to queue a pattern switch.
2. Press the opposite direction or cancel before the boundary, then advance again.

**Expected:** Queued change occurs on the intended musical boundary; cancellation removes the pending change without accidental double navigation.

### ID 047: SAVE PROJECT
1. Edit a disposable revision-13 song and make a named save through the on-device SAVE PROJECT action.
2. Navigate to another view and return to the save status.

**Expected:** Save completion is explicit and stored data is revision 13; the operation does not erase user loop slots.

### ID 048: LOAD PROJECT confirmation and rejection
1. Make an unsaved visible change, open LOAD SAVED and cancel.
2. Accept load on a disposable test project; separately present a deliberately invalid revision or corrupted-CRC project in a guarded browser/import test.

**Expected:** Cancel preserves current state; valid revision-13 load restores exactly; invalid data is rejected before any device write or state replacement.

### ID 049: CLEAR SONG versus RESET PROJECT
1. On a disposable song with user samples in place, use CLEAR SONG and inspect patterns and slot list.
2. Reload, use RESET PROJECT only after confirming its 'SAMPLES ARE KEPT' warning, then inspect slots again.

**Expected:** CLEAR SONG affects song arrangement without removing stored samples; RESET clears project/sequencer state but preserves every loop slot and its CRC.

### ID 051: Sample backup and restore integrity
1. With a known disposable sample, use Samples → Back up all slots and validate its downloaded archive.
2. On an approved disposable slot only, restore the archive, read back, and compare slot image and data CRC-32.

**Expected:** Restore is byte-identical, reports any skipped slot explicitly, and retains recovery archives; never overwrite owner samples just to exercise this test.

### ID 053: Lost connection while writing
1. Prepare a disposable data operation in Samples or Projects, but first confirm the durable backup exists.
2. Interrupt the USB/MIDI connection only during a reversible read/preparation stage; reconnect and inspect page state.

**Expected:** The stale session fails closed without a hidden write, local backups remain available, and the user must re-read device slots before resuming.

## C. Keyboard, visual design and navigation

### ID 057: Pattern overview glyph
1. Create a pattern with a few widely separated steps and a second dense pattern.
2. View their HOME pattern previews and switch between them.

**Expected:** The overview shape changes with actual pattern content; no pattern is shown as empty when it has notes.

### ID 058: Velocity representation
1. Program steps with low, medium and strong velocity that share a preview bin.
2. Compare the HOME glyph while selecting different patterns.

**Expected:** The brightest/strongest bin preserves the highest step velocity and remains readable.

### ID 060: Waveform and morph glyphs
1. Edit each available Miami waveform/morph choice while watching its symbol.
2. Move the same control slowly and then rapidly.

**Expected:** Glyph shape follows the actual selected parameter and does not pretend to display measured audio waveform.

### ID 061: Envelope and FX glyphs
1. Change ADSR stages in the matrix, then select several Chicago FX types.
2. Inspect each glyph and compare parameter behavior by ear.

**Expected:** Envelope glyphs show attack/decay/sustain/release semantics and FX symbols represent the actual assignment without switched labels.

### ID 062: Knob and ROW captions
1. Open each Chicago/Miami sound matrix bank and cycle the physical ROW selector.
2. Move K1–K8 at safe levels and inspect shown caption versus parameter changed.

**Expected:** Every active knob/ROW label names its true destination; inactive controls do not edit a hidden unrelated parameter.

### ID 063: Safe page wrap
1. At the last page of a multi-page view, advance using SELECT/page navigation.
2. Go back past the first page; repeat in an editor and a non-wrapping bounded numeric field.

**Expected:** Page navigation wraps only where documented, never wraps a destructive action or changes out-of-range numeric values.

### ID 064: Grouped settings
1. Open deep GLOBAL settings and scroll through MIDI, AUDIO, KEYS, PROJECT, SYSTEM and BLUETOOTH groups.
2. Enter and exit one harmless setting in each group.

**Expected:** Group headers and options remain reachable, and Back returns without jumping to an unrelated setting.

### ID 065: Activity-sensitive animation
1. Observe tape/signal animation with transport stopped and no MIDI/loop signal.
2. Start actual playback, stop, then restart.

**Expected:** Animation responds to real musical activity and does not run a misleading full-motion loop while idle.

### ID 066: Complete ADSR preview
1. Choose a synth preset and change attack, decay, sustain and release separately.
2. Compare on-screen curve with quiet long and short key presses.

**Expected:** Curve sections move in their expected direction; graphics remain a control preview, not a claim of measured DSP output.

### ID 068: 4×4 control matrix
1. Open the current sound-control matrix, cycle every ROW and activate K1–K4 cells.
2. Move between the matrix and neighbouring part/page views.

**Expected:** The active row is clearly highlighted and all 16 applicable cells route to their named fields.

### ID 069: Sequence versus sound surfaces
1. Open SEQUENCE, then the sound-control page of the same part.
2. Edit a step in SEQUENCE and change a synthesis parameter on the sound page.

**Expected:** The views are distinct, remember current part and pattern, and do not edit the wrong domain.

### ID 070: All colour themes
1. Cycle GREEN, AMBER, CYAN, RED, MONO and LIGHT in device display settings.
2. Use HOME, SEQUENCE, MIX and help in each and restart on the selected theme.

**Expected:** Contrast and selection highlights stay legible, including LIGHT; the chosen theme persists where documented.

### ID 071: DnBOX name and visual consistency
1. Inspect boot, HOME, GLOBAL and mixer screens.
2. Compare names, fonts, title alignment and units across the two keyboard models.

**Expected:** DnBOX spelling is consistent and labels have no unintended clipping or contradictory units.

### ID 072: Static boot display
1. Power cycle the exact approved development package with a camera recording the screen.
2. Wait through boot to HOME and repeat after a safe restart.

**Expected:** Static boot image appears without an unsolicited video animation, and boot reaches the playable home state.

### ID 074: Read-only pad diagnostics
1. With no flashing operation active, open approved SMK pad diagnostic/INLOG reporting.
2. Press each of 16 pads singly, including the pads with prior intermittent problems, and compare reports.

**Expected:** Raw/base/peak/scan/mux events reflect individual presses; diagnosis is recorded without recalibrating or writing protected hardware.

## D. Mac installer, site, project workspace and offline editor

### ID 075: Combined Install/Samples/Projects workspace
1. Open the authorised locally served DnBOX site in a Mac Chrome or Edge browser.
2. Navigate Install, Samples and Projects tabs without changing connection.

**Expected:** All three sections work and report current connection state without asking for separate unverified sites.

### ID 076: Song/project file controls
1. In Projects, export a disposable revision-13 song to the Mac and inspect the file.
2. Exercise import preview against a valid generated test fixture without writing owner data.

**Expected:** Supported revision-13 data has an explicit format/CRC result; a full firmware-to-web-to-firmware portable roundtrip is NOT qualified by this partial interface check.

### ID 077: Site typography
1. Open the approved locally generated site on Mac at normal and high zoom.
2. Compare body headings and DnBOX marks across Install, Samples and Projects.

**Expected:** Golos Text or declared fallback renders legibly with consistent navigation and no font-dependent hidden controls.

### ID 078: Manual, What's New and History
1. Open each source-linked page from the installer.
2. Check that content matches the inspected development version.

**Expected:** All links resolve within the approved local site, explain the version accurately, and do not imply #734 was publicly released.

### ID 079: Connection and installation states
1. With a disconnected keyboard, follow the displayed Connect USB, Choose firmware, Back up and install steps.
2. Connect the supported model and inspect every changed state without starting installation.

**Expected:** State, errors and expected next action are understandable; no automatic write begins.

### ID 080: Local firmware selection
1. Select an approved local .fwsc file and then deliberately select a malformed test text file in a safe no-write preflight.
2. Observe file name, model warnings and nearby status.

**Expected:** The chosen local file is visible, invalid bytes are rejected with a nearby error, and no install begins.

### ID 081: Unsupported browser notice
1. Open the locally served page in a browser without Web MIDI support.
2. Open the same page in current desktop Chrome/Edge.

**Expected:** An unsupported browser is warned before connecting; supported browsers expose the guarded Web MIDI connect control.

### ID 082: Desktop and mobile layout
1. Resize Mac browser to a narrow viewport; separately inspect on a supported phone without installing firmware.
2. Switch tabs and check scrolling, controls, buttons and file selection.

**Expected:** All essential controls remain reachable and readable; phone visual access does not imply mobile MIDI installation support.

### ID 084: Version picker and history integrity
1. Inspect available versions in the local installer catalog and open its history.
2. Confirm each entry's model, digest and source/release state before selecting anything.

**Expected:** No fabricated version is shown; #734 does not appear as a published NIGHTLY or production version merely because a branch exists.

### ID 085: Data-preserving installer path
1. Inspect package slot-map preview against a saved .x0xslots archive without writing.
2. Perform an approved update only after all preflight gates; re-read every slot afterward.

**Expected:** Every at-risk slot is explicitly backed up and verified, with skipped or moved loops flagged. There is no silent overwrite or false 'all preserved' banner.

### ID 087: Sample preview before writing
1. Load an authorised local sample in the Samples preparation panel and audition the preview.
2. Change optional grit or speed-up and compare the local decoded preview.

**Expected:** The preview is audible without a device connection, settings are explicit, and no slot changes until the user separately authorises upload.

### ID 089: Speed-up import option
1. For a disposable WAV, compare default import against the speed-up encoded preview and displayed size.
2. If approved, write only an empty disposable slot, then replay at the target tempo.

**Expected:** Encoded memory use is smaller when selected, playback duration remains intended, and changed sound quality is audible/clear; no hidden reallocation of an occupied slot.

### ID 090: Offline pattern editing
1. Open web/offline-sequencer on the Mac without the keyboard connected.
2. Create a short pattern in two tracks, save a local draft and reload.

**Expected:** Editing works offline and preserves changed note data; device sync is not claimed.

### ID 091: Two synth piano rolls
1. Edit distinct pitches and gate lengths for Synth1 and Synth2 in the offline editor.
2. Swap part tabs and inspect saved notes.

**Expected:** Each synth piano roll retains its own melodic material without converting pitches to drum steps.

### ID 092: 909/808/BREAK editors
1. Edit a drum hit on 909, a different hit on 808 and one BREAK slice step.
2. Switch editor views and reopen each.

**Expected:** The three part-specific step models remain separate and retain their changes.

### ID 093: MIDI import preview
1. Import a known short standard MIDI file into a disposable draft.
2. Review its note/track/time preview before choosing Apply or Cancel.

**Expected:** Data is shown before replacement; Cancel makes no draft change and Apply uses only the reviewed mapping.

### ID 094: No silent MIDI-import loss
1. Import a deliberately dense or out-of-range multi-track MIDI file into a disposable draft.
2. Inspect warnings for dropped/clipped notes, channels and event limits, then cancel.

**Expected:** Any loss or unsupported content is disclosed; no silent truncation or hidden overwrite occurs.

### ID 095: Local drafts and Undo
1. Save a local draft, change multiple notes, Undo and reload the saved draft.
2. Repeat after closing/reopening the browser tab.

**Expected:** The original draft is recoverable, Undo restores one coherent edit and the saved version is distinct from a device backup.

### ID 096: New and Mutate generators
1. Create a disposable pattern via New, then apply Mutate with the seed/settings visible.
2. Undo and rerun using identical options.

**Expected:** The generator affects only the intended local draft; Undo restores earlier content and repeated seeded generation is consistent where specified.

### ID 099: Part operations
1. Copy one complete editable part, paste into an empty test part, then cut and clear a disposable source.
2. Undo each destructive operation.

**Expected:** Notes, relevant part metadata and durations move together; commands remain reachable and Undo restores the complete part.

### ID 100: Redo history
1. Make three separate local edits, Undo two and Redo them in sequence.
2. Make a new edit after an Undo and inspect redo availability.

**Expected:** Redo replays only applicable history in the right order; a new divergent edit discards invalid future redo.

### ID 102: Cancelled pointer gestures
1. On a touch-capable browser, begin a note drag, cancel or leave the capture surface, and pinch/rotate screen.
2. Reopen the same pattern without saving the aborted change.

**Expected:** No ghost note or stuck drag remains; accessible keyboard alternative works where offered.

### ID 103: Editor sound schema boundaries
1. In the offline editor, switch Chicago/Miami sound control categories and adjust a legal minimum and maximum.
2. Attempt a deliberately out-of-range input in a disposable draft.

**Expected:** Controls use the right engine labels and bounded values; this is draft control-schema validation, not successful bidirectional device control.

## E. Release evidence and retained source behaviour

### ID 108: Regressions from merged predecessors
1. Use the integration log to locate one physical regression scenario each for saved SMC role, MIDI note release, BREAK slice, Chicago FX and UI navigation.
2. Run those real-device scenarios on both models and record footage/observations.

**Expected:** Previously working behavior has no regression; the log's skipped PRs are not silently counted as integrated.

### ID 109: Truthful test statuses
1. Read the exact frozen-head automated result table and the model build report before any device step.
2. List each failure and NOT_RUN separately from PASS, and check the current package digests.

**Expected:** No FAIL/NOT_RUN is marked PASS or hidden; current #734 full host qualification is not green.

### ID 114: Evidence tier is clear
1. Record SOURCE, HOST, ELITE BUILD, PRO BUILD, DEV CHECKED, INSTALL and DEVICE VERIFICATION separately in the acceptance record.
2. Compare each entry against a verified SHA/digest and real execution receipt.

**Expected:** A successful native build or simulator screenshot never gets recorded as a device PASS.

### ID 115: Independent exact-input verification
1. Have an independent reviewer compare the chosen Elite/Pro package hashes, controller hashes and exact source SHA to the produced build evidence.
2. Cross-check complete logs/exit codes and the slot CRC audit.

**Expected:** The reviewer verifies the same bytes intended for the device; mismatched evidence blocks installation.

### ID 116: All requests retained
1. Compare the test results against the 122-item #699 crosswalk and the integration feature registry.
2. Mark every requested result as available, external/pending, not built, or proved on this package.

**Expected:** All 122 feature IDs remain accounted for; a planned item is never silently passed.

### ID 118: Production versus NIGHTLY separation
1. Inspect the installer/version history and the currently chosen local package metadata.
2. Check whether the binary is an unpublished DEV candidate, accepted NIGHTLY or hardware-proven production.

**Expected:** The displayed status matches actual evidence; testing #734 never modifies production or publicly deploys a NIGHTLY.

### ID 121: M-VAVE firmware and simulator research boundary
1. Read the source research conclusions under docs/remaining-features-20261030.md before using official tools.
2. Confirm selected test uses no proprietary firmware/source or different model image bundled without rights.

**Expected:** Research/reference findings are labelled documentation only; no unreviewed third-party binary is installed or claimed as an integrated feature.

## 2. Required separate acceptance for pending ports, unmerged source and BLE MIDI

**Do not mark pending ports PASS on the frozen #734 image.** First prove each PR's reviewed source was integrated into one fixed candidate and that image passed its full acceptance gate. #737 is the primary real-page peek owner. #738 is overlapping regression/reference evidence only. #736 is a separate FX display/caption port. #741 is a SELECT-only held-effect correction, not full #41. #735 contains the two-peer enabling path; #734's imported BLE stack alone is not proof that two direct links are enabled.

### P01: FX 32-button matrix, feature #41.1, NOT IN #734
1. On the approved future FX-context candidate, press and HOLD REC, SOLO, MUTE and SELECT beside each of all eight faders, one button at a time.
2. Release each button and compare the corresponding bus audio and LED/status to the approved 32-button assignment manifest.

**Expected / acceptance condition:** Each of the 32 physical buttons triggers only its own chosen temporary FX while held, returns on release, and does not toggle an ordinary REC/SOLO/MUTE state during FX use. This is NOT a current #734 pass condition; the full mapping remains unrecovered.

### P02: HOME/Back modifier, #41.2, NOT IN #734
1. While viewing ROOM, hold physical HOME/Back and tap one of the four buttons beside a chosen fader.
2. Release HOME, hold the same button to audition the new assignment, then test each of the 32 button positions.

**Expected / acceptance condition:** Only the chosen button's effect cycles; the other 31 assignments, page context, song state and transport remain unchanged. Defaults and effect order must come from the recovered approved map, not invented here.

### P03: ROOM / TIGHT / ECHO full-page sets, #41.3, NOT IN #734
1. Configure or load the approved mapping manifest for the full 32-button ROOM page; then switch TIGHT, ECHO and back to ROOM using the documented HOME/Back page gesture.
2. Press an affected button in all three pages and compare the whole 32-button assignment snapshot after each switch.

**Expected / acceptance condition:** Each page recalls exactly its own complete 32-button bus-suited mapping, with no shared state leakage. The old four-SELECT ROOM/TIGHT/ECHO table is a historical partial reference, not the final 32-button default map.

### P04: Held FX ownership, safe releases and tails, NOT IN #734
1. In the future 32-button FX mapping, hold two FX buttons on different buses; if dual SMC is qualified, hold competing FX on the same bus from two devices.
2. Release in reverse order, switch pages while a button is physically down and reconnect a controller.

**Expected / acceptance condition:** Bus priority, original press ownership and release are deterministic; a disconnected peer cannot leave a held FX stuck, and delay/reverb tails decay naturally.

### P05: #736 mixer FX page port, NOT IN #734
1. On a candidate that actually contains #736, navigate to HOME and the MIXER FX page with SMC connected.
2. Turn a HOME encoder, move a fader through pickup, change bank and compare captions and coloured fader destination travel.

**Expected / acceptance condition:** Real-page captions identify active knob bank and FX destination; colours track actual fader travel, pickup remains correct, and all eight strips and two-peer hold guards still work.

### P06: #737 true-page peek and restoration, NOT IN #734
1. On a candidate with #737 integrated, move each SMC fader and a HOME encoder, including a second turn on reverb/delay/master, then press PAGE while that SMC is locked.
2. Observe the actual corresponding MIX PARTS/FX REVERB/FX DELAY/MIX MASTER view, live meters and compact feedback, then wait 1.2 seconds.

**Expected / acceptance condition:** The real page becomes visible without changing the captured fader/knob targets and restores the exact original screen, page, part, selection and cursor. Menus, help, confirmation, clip and performance guards hold.

### P07: #738 alternate mapped-page evidence, DO NOT DOUBLE-MERGE
1. Review the frozen #738 native framebuffer captures for strips 1–5 to MIX PARTS and 6–8 to reverb/delay/master, plus BREAK overlays.
2. Compare its 1.2-second restoration regression cases to the chosen #737 implementation on one fixed qualified head.

**Expected / acceptance condition:** #738 remains an alternate evidence/checkpoint, not another firmware implementation: only a nonduplicated selected peek path is accepted.

### P08: #735 direct two-SMC central activation, NOT IN #734
1. On a future candidate with the correct two-link BLE controller image, pair two SMC mixers via direct Bluetooth and assign each an independent lock and fader target.
2. Hold overlapping Solo/FX from both, disconnect one, then reconnect and retest pickup.

**Expected / acceptance condition:** Both peers can send and receive correct independent feedback and released holds, without HCI-credit starvation or mapping to the wrong physical mixer. One-SMC success does not pass this.

### P09: BLE MIDI note/clock/transport acceptance, source in #734
1. Pair an approved BLE MIDI source on the keyboard and record accepted Note On/Off, MIDI Clock, Start/Stop/Continue on the Mac/Loopy Pro.
2. During playback switch source between accepted USB/BLE paths, stop clocks and reconnect the peer.

**Expected / acceptance condition:** No lost/duplicate note-on, hanging note-off, wrong clock phase or corrupted queue; BLE receiver must be verified with matching controller image and real hardware, not by a source-only test.

### P10: SMC momentary SELECT historical baseline, PARTIAL IN #734
1. On current #734 source, hold SELECT beside 909, 808, Chicago A and BREAK in main MIX (not drum MIX/SEQ).
2. Release, then use HOME plus SELECT and cycle ROOM/TIGHT/ECHO, recording LCD labels and sound.

**Expected / acceptance condition:** Only the existing four-bus SELECT implementation is evaluated; its historical mapped effects play while held and release correctly. It does not satisfy the new all-REC/SOLO/MUTE/SELECT requirement. M/S/R keep their old behavior until the new FX context is implemented.

### P11: Protected drum-context buttons remain intact
1. On 909 MIX/SEQ and 808 MIX/SEQ, use each drum strip's REC, SOLO, MUTE and SELECT buttons, including bank 9–11.
2. Repeat with a held Solo while switching the visible drum bank.

**Expected / acceptance condition:** Normal per-drum record, isolate, mute and select semantics continue in drum contexts; the future FX map must not steal these controls unintentionally.

### P12: BLE role/preset persistence and host route
1. Set GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST and select DAW, restart, then change to USER without restarting.
2. Inspect MIDI on the Mac, test SMC USER channel-16 control and unsupported channel interference.

**Expected / acceptance condition:** Saved role takes effect after restart; DAW/User preset changes apply as documented and do not hijack regular BLE MIDI musical notes.

### P13: Time-based overlays and lock vs peek
1. With legacy #734 overlay, move a fader once and observe complete 8-strip feedback for 1.2 s; with qualified #737, repeat in a locked PAGE preview.
2. Press the SMK navigation key before timeout; inspect return view and cursor after 1.2 s.

**Expected / acceptance condition:** Legacy overlay and new real-page peek have distinct expected geometry, both preserve the original screen, and stale timeouts cannot revert intentional navigation.

### P14: BLE failure conditions
1. With a durable sample backup and a disposable project, interrupt a BLE musical connection during long MIDI notes and SMC held-Solo events.
2. Reconnect and resend independent new notes/controls.

**Expected / acceptance condition:** No stale note, FX hold, Solo target or queued old-link message survives; device stays responsive and user sample CRC remains unchanged.

### P15: Source-only versus missing items from #699
1. Review the NOT BUILT and BUILT ELSEWHERE IDs listed at the end of this checklist before marking device tests complete.
2. For each absent capability record NOT_AVAILABLE rather than performing an invented workaround.

**Expected / acceptance condition:** No missing sound-lock, legacy migration, 32-button FX, dual-peer activation, complete offline sync or shipping claim is marked PASS on current #734.

## 3. Features absent from #734, non-device requirements, and acceptance boundary

The #699 crosswalk has 122 IDs; not all are delivered in the current integration. The following IDs are **NOT BUILT / NOT ACCEPTED** in the cited #734 source audit. Their absence must be recorded as **NOT_AVAILABLE**, not skipped silently or inferred from source comments. Some require an entirely different product/site/release milestone:

- **ID 3:** full 22/18 engine controls; no current #734 hardware PASS.
- **ID 8:** complete per-note sound locks; no current #734 hardware PASS.
- **ID 15:** cutoff lock save through navigation; no current #734 hardware PASS.
- **ID 29:** owner-approved stock break sample set; no current #734 hardware PASS.
- **ID 41:** complete 32-button FX spec; no current #734 hardware PASS.
- **ID 50:** portable rev13 firmware/browser roundtrip; no current #734 hardware PASS.
- **ID 52:** atomic writes across unrelated clients; no current #734 hardware PASS.
- **ID 54:** browser rejects all legacy project revisions; no current #734 hardware PASS.
- **ID 55:** byte-complete original sample bank; no current #734 hardware PASS.
- **ID 56:** complete reviewed Night UI; no current #734 hardware PASS.
- **ID 73:** recovery bootloader (do not exercise); no current #734 hardware PASS.
- **ID 83:** matched qualified Bluetooth controller in package; no current #734 hardware PASS.
- **ID 86:** website actual framebuffer screenshots; no current #734 hardware PASS.
- **ID 88:** multiple site/3D variants; no current #734 hardware PASS.
- **ID 97:** full FunkyBeats-style iPhone editor; no current #734 hardware PASS.
- **ID 98:** bulk note selection/operations; no current #734 hardware PASS.
- **ID 101:** snap/scale/ghost/touch keyboard; no current #734 hardware PASS.
- **ID 104:** bidirectional live device-editor sync; no current #734 hardware PASS.
- **ID 105:** TouchOSC project; no current #734 hardware PASS.
- **ID 106:** idle CPU at most 20% measured; no current #734 hardware PASS.
- **ID 107:** multi-workload robustness measured; no current #734 hardware PASS.
- **ID 110:** holistic owner usability acceptance; no current #734 hardware PASS.
- **ID 111:** accepted NIGHTLY release; no current #734 hardware PASS.
- **ID 112:** DEV CHECKED/installable Elite and Pro deliverables; no current #734 hardware PASS.
- **ID 113:** deployment through published installer; no current #734 hardware PASS.
- **ID 117:** measured lower time to owner availability; no current #734 hardware PASS.
- **ID 119:** community song library; no current #734 hardware PASS.
- **ID 120:** community suggestions/voting; no current #734 hardware PASS.
- **ID 122:** complete Cursor contribution audit; no current #734 hardware PASS.

**Extra external / partially ported IDs** needing explicit verification on a future combined image: **16** original held-control target, **31** explicit mixer mode/capture, **33** true two-SMC links (#735), **37** bank/page navigation, **40** SELECT/User contextual semantics, **45** unmute-level preservation, **46** beat-time pickup feedback, **59** extended composition geometry and **67** held full-overlay lifetime (source #688/#736; related #638 and #737). This checklist tests related baseline behavior but must not convert the external enhancements to PASS on #734. Item **41** is separately checked in P01–P04. Items **50/54/104** must never be described as complete firmware↔browser project synchronisation.

## 4. End-of-session report: keep evidence separate for each keyboard

1. Copy the model-specific package and controller SHA-256, exact Git revision, build/host evidence IDs, installed firmware version, before/after .x0xslots paths and their 32-bit CRC comparison results into the owner's private acceptance record. Keep raw sample bytes, credentials, device captures and firmware binaries out of GitHub.
2. Enter a status for **every test ID above on each model**, including P01–P15 as NOT_AVAILABLE until integrated. Attach error reproduction, device photo/video or MIDI logs where useful. State which tests were not possible, and whether the observed behavior matches source, reference audio and the approved recovered FX mappings.
3. Decide acceptance **only** after all required full host/build/proof gates and relevant device tests on the same app/controller bytes, with zero unresolved failures. If any sample, model, source identity, clock, FX, BLE link, key/LED or UI safety test fails, stop promotion and preserve the exact candidate and backups.
4. Do not merge #734 to `wip/one-build`, `nightly` or `main`, publish a public installer, overwrite a protected region or claim production qualification as part of this checklist. Route actual validated evidence to the sole integrator and independent QA.

**Current result at document creation:** PLAN ONLY; Mac installation **NOT_RUN**; Elite physical device **NOT_RUN**; Pro physical device **NOT_RUN**; slot backups/CRC **NOT_RUN** by this thread; unmerged #736/#737/#738/#735/#741 **NOT QUALIFIED ON #734**.
