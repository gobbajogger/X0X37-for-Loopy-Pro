# Exact sources and verification

**Build/source:** `reference-smc-integration` at `4cf8c2f1d641bf91041a39f0d34d77cd51fddbf8`, reviewed on 2026-10-21. Line numbers below refer only to that revision; paths are repository-relative. Symbols remain useful if a later package changes line numbers. These are local source references, not a claim that this native commit was published to GitHub.

**Replaces:** the earlier `ab1694e` usage guide's current-build advice. No firmware, installer, or device result is replaced.

**Checked against decisions:** coherent main/909/808 strips; selected drum and SMK pad sequencing in MIX and SEQ; one direct SMC; Loopy over USB in the temporary central-only TEST profile. See `docs/USER_PREFERENCES.md:13–17,73–75` and `docs/SMC_MIXER.md`. Requested future capabilities are not represented as delivered.

The listed SMC mapping, UI, protocol, clip-editor and controller-profile files are byte-identical between `81459573792a68edd02fa1ee5658796b55597ba0` and `4cf8c2f1d641bf91041a39f0d34d77cd51fddbf8`. [Input hashes](validation/independent-review/source-sha256.txt) identify the inspected files. The guide may accompany a later package only after its owner verifies the relevant behavior remains unchanged; it does not assign that package a hardware PASS.

## S1 Setup and profile

- `firmware/src/app/ui.c:287–305`: `SMC_ROLE_N`, `SMC_ROLE_P`, `SMC_MAP_P` and `BTSTAT_N`. TEST has the sole role label `SMC TEST / USB` and a zero range; ordinary builds expose DEVICE/SMC HOST.
- `firmware/src/app/ui.c:533–550,1300–1311`: Bluetooth/preset setters and the GLOBAL Bluetooth MIDI rows. Bluetooth can require restart; the ordinary role setter asks for restart; the preset flag is changed immediately.
- `firmware/src/app/smc_mixer.c:126–133`: `smc_ble_role_get()` forces the central role in TEST. `firmware/hal/x0x_smc_profile.h:18–40` guards TEST, Bluetooth, SMK-37, and the named Elite development package.
- `ble/ctl/src/ctl_vendor_profile.c:51–57,89–114`: one owned connection and one discovered MIDI characteristic. This is not three-central or simultaneous peripheral service support.

## S2 Loopy and routing

- `firmware/src/ble_vendor.c:231–265`: dedicated SMC ingress calls `smc_ble_midi_accept`, not APC/Loopy handling.
- `firmware/src/app/smc_mixer.c:491–505`: SMC rewind/forward/stop/play/record act on X0X pattern, sequencer and recording state.
- `firmware/src/app/main_fm1.c:301–319,388–406`: APC/clip receive handling and its reserved-User-message exception. Do not confuse SMC feedback with Loopy pad feedback.
- `firmware/src/app/smc_mixer.c:126–151` and `firmware/src/app/ui.c:539–549`: TEST is the central/USB combination. Use the existing Loopy USB route, not a claim that peripheral Loopy BLE operates simultaneously in TEST.

## S3 Preset selection is not mode detection

- `firmware/src/app/smc_mixer.c:545–577`: DAW pitch-bend faders use MIDI channels 1–8; relative encoders use CC16–23 on channel 1; note buttons use channel 1. User additionally accepts CC40–47 faders and the stock six-byte MMC form. User permits the compatible direct CC/note messages on other channels too.
- `firmware/src/app/smc_protocol.h:5–14,24–42`: stock MMC-to-button mapping, signed relative encoding, and the opt-in channel-16 host reservation. The reservation does not identify the original physical sender.
- There is no physical DAW/User identity discriminator in these handlers. Overlapping CCs or notes cannot be rejected merely because they originated from a physical User preset. No automatic mode detection or reliable physical User-mode ignore is promised.

## S4 Destinations and page changes

- `firmware/src/app/smc_mixer.c:175–237`: `smc_home_target`, `smc_page_target`, `smc_drum_mix`, `smc_drum_track`, `smc_target`, and `smc_knob_target`. Main destinations are five part levels, reverb level, delay level and master DJ filter. Drum mapping requires no overlay and no clip mode. Help/confirmation block continuous controls. Ordinary knobs use four visible cells; menus use six visible rows without duplicate targets.
- `firmware/src/app/ui.c:1009–1018,1129–1138,1174–1177`: both page-building paths order MIX as PARTS, 909 MIX, 808 MIX, MASTER; the visible drum MIX page determines the kit.
- `firmware/src/dsp/master.c:124–135`: DJ-filter values below 60 select low-pass, 60–68 bypass, above 68 select high-pass. This parameter is not master volume.
- `firmware/src/app/smc_mixer.c:335–344`: M6/M7/M8 store a nonzero level then zero it; a later press restores it, or 100 if no remembered nonzero value exists. M8 targets `FX_VOLUME`, not `MST_DJF`.

## S5 Eleven drums and pad targets

- `firmware/src/seq/pattern.h:15`: `NDRUM = 11`.
- `firmware/src/dsp/drum909.h:21`: BD, SD, LT, MT, HT, RS, CP, CH, OH, CR, RD.
- `firmware/src/dsp/drum808.h:25`: BD, SD, LT, MT, HT, RS, CP, CB, CY, OH, CH. `DR_KIT`/`D8_KIT` follow the track count and are not twelfth playable tracks.
- `firmware/src/app/smc_mixer.c:202–205,357–386,408–430`: selection determines the eight-strip bank; Bank Left/Right selects track 0/8; SELECT and R set the visible kit and track; empty bank-2 positions return without becoming navigation buttons.
- `firmware/src/app/ui.c:2307–2345,2618–2653`: `drum_step` edits the selected track; MIX pads explicitly call it for the visible kit; ordinary drum pads, including SEQ, call it for the selected part. There is no REC requirement for these step edits.
- `firmware/src/app/ui.c:4658–4669,4733–4747` and `firmware/src/app/main_fm1.c:786–798,1765–1772`: drum step display follows `ui.sel[kit]`; drum MIX retains SMK step-grid mode.

## S6 Buttons and feedback

- `firmware/src/app/smc_mixer.c:45–93,347–375,392–481,528–533`: temporary Solo targets, project cleanup, R/SELECT selection, ordinary part/FX shortcuts and original-target release.
- `firmware/src/app/smc_mixer.c:70–76`: DAW hold/release Solo versus MMC press-again latch. Repeated held presses do not move a target after navigation.
- `firmware/src/app/smc_mixer.c:580–608,655–656`: link transitions, project-busy, performance-test, help and confirmation cleanup. These paths clear temporary Solo without rewriting saved mute state.
- `firmware/src/app/smc_mixer.c:659–696`: `smc_led` is the authoritative light-state map. R follows recording plus actual selection; S follows temporary Solo; M means muted/zero; SELECT follows selection. Drum bank 2 empty strips are dark. R6 reflects a non-NONE overlay; R7 is off. Navigation lights are view/page indicators, not acknowledgments. Transport follows sequencer/REC state.
- `firmware/src/app/smc_mixer.c:699–729`: `smc_frame` emits changed button Note On values 127/0 and pending pitch-bend target positions, retrying later if application enqueue fails. This is source-level output, not a measured physical colour, meter, ring or motor response.

## S7 Pickup and feedback output

- `firmware/src/app/smc_protocol.h:17–21`: pickup at a crossing or within two MIDI steps of the target.
- `firmware/src/app/smc_mixer.c:239–310`: scaled target, `PICK F`/`SMC F`/`SMC K` display, destination/value-change pickup reset, and writes only after pickup. The value shown during PICK is the current X0X parameter value, not evidence that the physical fader changed it.
- `firmware/src/app/smc_mixer.c:313–324,590–599,710–728`: knob dispatch, reconnect reset and feedback. A change is noticed at the target's 0–127 scaled resolution; a tiny change that quantizes to the same value is not a new pickup target. This precision qualifies the user guide's “detected” value-change wording.

## S8 Navigation and clip editors

- `firmware/src/app/smc_mixer.c:395–405,433–447,470–524`: help/confirmation precedence, R6/R7, S6/S7/S8, SELECT6/7/8, bank/channel stepping, pattern selection and four-arrow navigation. Drum strip handling at 408–430 precedes ordinary shortcuts.
- `firmware/src/app/main_fm1.c:1354–1405`: clip grid consumes knob edits without touching the hidden X0X page. SCALE/CHORD expose four fields, ARP eight; bank/channel navigation also reaches the ninth ARP field, LATCH. Up/Down cycles stages, Left/Right edits, R7 returns to grid.
- `firmware/src/app/smk_clip_keys.h:152–166`: actual per-page field order and field navigation.
- `firmware/src/app/smc_mixer.c:313–324`: menu/help/confirmation precedence prevents covered clip editors from taking knob turns. The independently repaired visibility fixture tests this ownership and event draining, not the full persistence implementation.

## S9 READY, CONNECTED and first feedback

The exact TEST path is **runtime → eligible callback → queued LINK-up → copied notification → BLE-MIDI decode → application queue → dispatch → control-specific effect**. Stages can fail independently.

- `firmware/src/ble_vendor.c:267–281`: READY means runtime active with no application ingress link; CONNECTED means the ingress link flag is set. READY does not distinguish scanning from a physical connection, discovery or subscription before first notification. Application feedback enqueue also requires that link.
- `ble/ctl/src/ctl_vendor_profile.c:204–215`: an eligible callback requires wanted/non-retiring state, kind `0xa7`, the owned connection and MIDI value handle, zero offset, non-null data, and length 3 through MTU−3. It attempts LINK-up **before** notification copy. Only successful notification copy sets `receiving`.
- `ble/ctl/src/ctl_vendor_profile.c:295–303`: controller MIDI output is refused until `wanted && receiving`. It therefore cannot be the initial required wake/feedback exchange in this implementation.
- `firmware/src/ble_vendor.c:253–261`; `firmware/src/ble/smc_host_ingress.h:104–126`: the application accepts LINK separately, then validates/decodes copied notifications. A LINK-up can already have been copied even if notification copying fails; a copied notification can still fail decoder validation.
- `firmware/src/app/smc_mixer.c:590–629`: the connected banner precedes dispatch. Dispatch counts are incremented before `smc_message` can reject a preset format or a fader can wait for pickup. Therefore neither CONNECTED nor a dispatch count establishes an applied control.

No SMC-specific startup handshake is inferred from lights. As historical counterevidence, the retained Hortator source at `45df60aa41e02196dd3375a2bee7e258f331a417` has subscription and receive handling, but no application MIDI-output API: [central implementation, lines 305–332](https://github.com/DeadActive/hortator/blob/45df60aa41e02196dd3375a2bee7e258f331a417/firmware/src/ble/ble_central.c#L305-L332). The retained upstream comparison adds output later. That history is not a new device test and does not prove which stage failed on the owner's SMK.

## Verification

Independent review reran these checks on 2026-10-21 using GCC 14.2.0 and Python 3.12.14. Commands were run from the unchanged reference checkout or with its absolute path. Logs: [environment](validation/independent-review/environment.txt), [clean reference status](validation/independent-review/reference-status.txt).

| Command | Result | Scope |
|---|---|---|
| `python3 tests/host/smc_mapping_test.py` | PASS | Main/drum destinations, 11 tracks, bank bounds, visible-field ownership |
| `python3 tests/host/smc_buttons_test.py` | PASS | Ordinary shortcuts and view/page/menu arrows |
| `python3 tests/host/smc_fader_test.py` | PASS | Pickup, motion writes, inactive input and final display |
| `python3 tests/host/smc_feedback_test.py` | PASS: 1,838 expectations | LED state and emitted MIDI with substituted UI/sound/transport |
| `sh tests/host/smc_part_select_test.sh` | PASS | Selection and held-step cancellation |
| `sh tests/host/smc_clip_visibility_test.sh` on unmodified fixture | Expected compile failure | Unused `mark_dirty`, `-Werror` intact |
| `python3 test-fixture-repair/review.py /path/to/reference /path/to/logs` from this guide directory | PASS review controls | Repaired fixture 138 checks in each of default and TEST profiles; remove-only control 9 failures; hidden-edit mutant 21 failures; replay mutant 45 failures |

The five focused checks compile extracted production logic with host substitutes; they are not full target integration tests. The five focused logs are in `validation/independent-review/` under their test names. Fixture explanation, exact patch and limits: [review](test-fixture-repair/REVIEW.md). Earlier logs directly under `validation/` are retained prior work, not relabeled as this independent run.

**Verdict:** the bilingual guide is consistent with the inspected source. The isolated fixture repair is acceptable as a test-only correction. No active firmware source, compiler flags or production persistence code was changed. No target build, package gate, installation, proprietary-producer replay or physical-device test was run for this documentation/fixture review. The owner's earlier report that light feedback looks correct is historical user evidence, not a PASS for a different build.
