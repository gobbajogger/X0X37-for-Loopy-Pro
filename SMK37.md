# LoopyX0X37 / DnBOX firmware manual

**Development source guide — not a release or installation approval.** This manual
explains the inspected `wip/one-build` source at
[`48e92b76c2f476cf40771e00a04e1838136bb55d`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/48e92b76c2f476cf40771e00a04e1838136bb55d).
It does not establish what is installed on your keyboard. Check your version and
model first. Hardware behavior on these exact bytes is **NOT TESTED by this manual**.
Later candidate changes need their own source check.

This expands the existing SMK guide. The [FM-1 manual](docs/MANUAL.md) remains an
upstream musical reference; its installer, recovery procedure, legacy synth
parameters and compatibility claims do not apply to this DnBOX candidate.
The [source and architecture reference](docs/FIRMWARE_REFERENCE.md) gives the
implementation evidence, differences and test limits behind the instructions below.

## Find your task

- [Start safely](#start-safely)
- [Learn the controls](#learn-the-controls)
- [Make and save a first beat](#make-and-save-a-first-beat)
- [Play Chicago or Miami](#play-chicago-or-miami)
- [Patterns, recording and songs](#patterns-recording-and-songs)
- [BREAK loops](#break-loops)
- [Effects and mixing](#effects-and-mixing)
- [Loopy Pro clip mode](#loopy-pro-clip-mode)
- [Use an SMC mixer](#use-an-smc-mixer)
- [Saving and project files](#saving-and-project-files)
- [Panic and troubleshooting](#panic-and-troubleshooting)
- [Installation safety](#installation-safety)

## Start safely

The instrument combines 909 and 808 drums, two synth parts still labelled **303A**
and **303B**, and BREAK. The two synth slots select **Chicago** or **Miami** in this
source profile. Those part labels do not mean that the old bass303 engine is active.
The parts can play different patterns together. Clip mode instead makes the pads a
Loopy Pro controller; it has its own keybed scale/chord/arpeggiator controls.

1. Use the firmware already authorized for your exact model. Turn the output down
   before listening. This tour does not require installing a candidate.
2. Identify the version in the Global/About screen and record Pro or Elite. If it
   differs from the source above, treat these instructions as a comparison guide.
3. Back up an existing project before changing it. Startup can load saved work;
   do not assume every part is empty. RESET PROJECT is not a harmless way to start
   a tour: autosave can replace your saved project afterward.
4. For the examples, leave clip mode off, turn recording off, and choose an unused
   pattern. Check the screen before pressing pads.

Source-verified here means the relevant code was read. A host test means only the
named test ran. Neither proves physical pads, lamps, audio quality, latency or
Bluetooth reliability. Planned behavior is labelled separately below.

## Learn the controls

These are the default panel assignments. A previously calibrated panel can differ.
The names in parentheses are the internal names used in the older manual.

| Printed control | Ordinary groovebox use |
| --- | --- |
| K1 | Master volume. Start low. |
| K2 (SELECT) | Page selection; selects a row in a list. With a step held, edits its probability. |
| K3 (ALGORITHM) | Selects the part; edits the highlighted value in a list. |
| K4 (PRESETS) | Chooses patterns except on the synth SOUND page, where it browses factory presets. HOME affects all parts; other part pages affect that part. |
| K5–K8 | Edit the four labelled fields at the bottom of the current page. |
| PARA (EDIT) | Opens sound pages; repeated presses advance their pages. |
| SEQ | Opens sequence pages; repeated presses advance pages. |
| ARP | Opens TB-3PO for a synth part. In clip mode it has the separate clip-key role. |
| SCALE (SEL) | Opens the current page's full list; activates an action row; confirms a question. |
| PATCH (HOME) | Opens HOME, then SONG; backs out of a list or cancels a question. |
| GLOBE (GLO) | Opens/closes Global settings. |
| FX | Shared reverb/delay pages. |
| NOTE REPEAT (LFO) | Mixer pages; held with note entry, slide. |
| CHORD (ENV) | Held with note entry, accent. |
| SEQ PLAY / SEQ REC | Start/stop playback / toggle recording. |
| BT (SAVE) | Saves or requests a deferred save. It is not the Bluetooth settings button. |
| OCTAVE− / OCTAVE+ | Step/pattern banks, or keyboard octave, depending on the current view. |
| < / > | Pages in ordinary views; values in menu lists. Clip editors own their arrows. |
| KNOB BANK | Toggles Loopy clip mode once per press. |
| FADER BANK | Switches keyboard fader banks; lit on bank B. |

On the normal step grid, pads 1–8 are the top row and 9–16 the bottom row.
When pads become notes their layout changes; do not read a note layout as a step
layout. The middle keyboard range F3–G5 supplies the older white/black-key commands.
In the ordinary grid, its white keys mirror step/pattern selection and its black
keys choose drum tracks or BREAK slices. The outer keys are not universal shortcuts.
**All 37 keys** have a dedicated synth Free Play route under the conditions below.

Read the four on-screen labels before turning K5–K8. A list exposes settings that
are not on a four-knob page. Use K2 to select, K3 to edit, SCALE to run an action,
and PATCH to back out. At a confirmation, SCALE means yes and PATCH means no.
The inspected baseline blocks other button taps at a question, including PLAY;
newer PLAY-through-question work is separate.

Hold a button alone to see contextual help where available. Release it without a
combination to leave help. Help and menus can change input ownership; first close
them if the normal playing or editing gesture does not work.

**Undo:** hold PATCH and press SEQ REC. **Redo:** hold PATCH and press SEQ PLAY.
These are groovebox history commands; clip mode can send separate Loopy history
messages. Project load/import/reset clears local undo history.

## Make and save a first beat

Starting state: ordinary groovebox mode, stopped, REC off, unused pattern.

1. Turn K3 to 909, press SEQ, and choose the bass drum with the first black key in
   the mapped F3–G5 range. Check the displayed drum name.
2. Press pads 1, 5, 9 and 13. Their steps should light. Press SEQ PLAY to hear the
   four-beat pattern. Press a lit step again to remove it.
3. Select a different drum, then add steps 5 and 13. In stopped mode selecting a
   drum can audition it; while playing, KEY SOUND and recording affect audition.
4. Press NOTE REPEAT for the mixer. Select the part and adjust its LEVEL. If a
   physical fader does not take over, move it toward the displayed value first.
5. Stop with SEQ PLAY, turn REC off, release notes and let controls settle. Press
   BT to save. Wait for the save result; a pending or failed save is not success.

This uses existing sequencing rather than generating a factory demo. If your
starting pattern contained notes, the example adds to them; it does not clear them.

## Play Chicago or Miami

Select 303A or 303B with K3. Press PARA and find the **SOUND** page. Its engine and
factory fields choose Chicago or Miami and an admitted factory program. On this
SOUND page, K4 also browses factory presets; on other part pages it selects patterns. A pending sound
change is distinct from an applied sound: follow its status field and avoid
assuming a held note has already changed engine.

For playing without writing notes, open the synth's SEQ pages and set **FREE PLAY**
to ON. Turn REC off. Return to SEQ or PARA, close menus/help and release PATCH/BT.
In this baseline the dedicated 37-key route is restricted to those ordinary
synth views, with no clip mode, SMC overlay, project operation or performance test.
It is not verified as an all-keys route through settings and HELP. FREE PLAY is
runtime state, not a saved project preference.

For Miami with Free Play active, the top eight physical pads select RATE in this
order: **1/2, 1/4, 1/8, 1/16, 1/3, 1/6, 1/12, 1/24**. Read the `PADS: RATE` caption.
The bottom pads retain note behavior; they are **not shape selectors in this pin**.
This route is conditional on the original-engine/rate-pad build options. Turning
REC on removes this rate-pad ownership so recording can use its normal controls.

The pitch and modulation wheels are live performance inputs. Their destinations
depend on the selected engine; do not use the old blanket “both wheels are saved
303 parameters” interpretation. Original synth pitch bend/damper are performance
state and restore neutral rather than becoming patch edits. BREAK has its own
transient pitch-bend route.

## Patterns, recording and songs

The project has 32 pattern slots. Each part has its own current pattern and can
have its own sequence length/rate. A pattern change is not a new project or a
factory sound change. Sounds and shared effects do not automatically switch with
patterns.

- On HOME, pads choose a pattern for all five parts. OCTAVE switches between
  patterns 1–16 and 17–32. From a part view, K4 changes only that part's pattern, except on the synth SOUND preset page.
- While playing, pattern selection can be queued to a boundary; stopped it is
  immediate. Watch the queued indication instead of repeatedly selecting it.
- Hold BT and press a destination pad to paste the currently selected parts into
  that pattern. An occupied target asks before replacement. BT alone saves.
- Hold BT and press SEQ REC to request clearing the selected part in its pattern.
  Global CLEAR PATTERN has the broader all-part scope. Read the question.
- Pattern chaining exists, but the newer “longest pattern sets chain duration”
  repair is a separate candidate (#855). Do not use that promise for this baseline.

**Recording:** SEQ REC toggles recording. While running, drum selection keys can
record hits and synth keys record notes. With a synth stopped and REC on, notes
advance step entry; CHORD supplies accent and NOTE REPEAT supplies slide. Their
OCTAVE combinations enter rests/ties. Verify the displayed step before overwriting
an existing line. ARP opens the separate TB-3PO generator on a synth; generating or
mutating a line changes its sequence, not the clip-key arpeggiator.

**Motion:** with playback and REC on, turn a sound knob to record movement. REC off
ends recording. Hold BT and turn that knob to clear its motion. Motion uses finite
project storage; `MOTION FULL` is a limit, not confirmation that the new lane saved.

**Per-step edits:** on PARA, hold a step and turn a sound field to make a parameter
lock. Hold that step and BT while turning the field to clear the lock. On SEQ,
synth step fields instead edit the note. Holding a step and turning K2 sets its
probability/cycle condition. For example, 50% means a fresh chance on each pass,
not a guaranteed alternation; a cycle condition such as 1:4 is deterministic.

**Songs:** press PATCH to HOME and again to SONG. A song is an arrangement of
patterns and part mutes, with up to 192 bars. Select a bar with the labelled field;
pads write pattern choices. The second page has REPEAT. Start playback from SONG
to play the arrangement; starting elsewhere plays patterns. With REC enabled,
SONG playback can overwrite bars with your live pattern/mute choices. Use SCALE
for INSERT BAR, DELETE BAR and CLEAR SONG. **CLEAR SONG clears the arrangement;
RESET PROJECT also resets sounds, patterns and settings.**

## BREAK loops

BREAK is a sequenced loop/slice player. Select BREAK with K3, use SEQ for its steps
and PARA for its sound fields. Ordinary step pads enable playback steps; mapped
black keys select/audition slices. Check the page's labels for loop, slice and
playback settings before recording. A transient wheel bend is not a stored loop
rewrite.

Imported sample slots are separate from the project arrangement. Changing a
pattern does not import a sample. The existing sample importer and project-file
workflow need a matching, qualified firmware/website pair. Keep the original WAV
and a backup of the encoded slots; an older firmware's slot layout can differ.
Do not assume every historical “three slots” or “11 seconds” statement describes
an arbitrary installed build. This manual has not tested importing on a device.

## Effects and mixing

There are three different control layers:

| Layer | Use |
| --- | --- |
| Synth patch effect | Chicago SOUND parameters `FxType`, `FxEd1`, `FxEd2`. The inspected compact policy offers OFF, CHO/FLG, PHASER, RING MOD, COMPRESSOR, DECIMATOR and EQ. |
| Shared sends | FX opens reverb/delay. Mixer REV and DLY send a part or drum to those effects; return levels control what comes back. |
| Live SMC bus effect | Hold the relevant SELECT button for a momentary effect; release returns the bus. It is not a replacement Chicago factory insert. |

Do not expect Hall, Ensemble or the restored delay inserts from other PRs in this
compact baseline. The source admits 55 Chicago factory programs; expanding the
factory/insert policy is a separate implementation decision.

NOTE REPEAT opens MIX: PARTS, 909 MIX, 808 MIX, MASTER and WAVE. The drum pages mix
individual sounds; PARTS mixes whole parts. MASTER applies to the combined output.
FILTER is neutral in the middle, low-pass to the left and high-pass to the right.
COMP compresses and PUMP ducks to kick triggers; a silent/muted kick can still drive
pump. Start with small adjustments and watch output level.

Keyboard fader bank A controls 909, 808, BREAK and 303B levels. Bank B controls 303A,
reverb return, delay return and master FILTER. **Pickup is required:** after a
bank/target change, move through the existing value before expecting a change.
This prevents a fader at an unrelated physical position from making a sudden jump.

## Loopy Pro clip mode

Press KNOB BANK and look for **LOOPY CLIPS**. Pads now address clips rather than
local sequence steps. Press KNOB BANK again to return (`LP OFF`). The clip grid
uses APC40 mkII-style MIDI feedback; pad colour comes from the host. A waiting or
empty grid is not proof that firmware audio has failed.

Connect through a supported USB/Bluetooth MIDI path and configure Loopy's controller
routing for the keyboard. Enable the intended input and feedback output; avoid
routing the same clock back into itself. Bluetooth carries MIDI, not audio.
The clip keybed uses its own SCALE, CHORD and ARP pages. With no clip editor active,
K2–K8 have the clip CC mapping; with an editor open they edit that editor's fields.
Do not confuse this ARP with the local TB-3PO sequence generator.

OCTAVE navigation moves the clip viewport. The exact viewport/feedback conventions
are documented in the [APC40 implementation history](APC40_IMPLEMENTATION.md);
its old release receipts are not acceptance of this candidate. For Loopy undo/redo,
the existing guide describes learning PATCH+SEQ REC and PATCH+SEQ PLAY as “On”
triggers in Loopy. Check those learned actions in your own Loopy profile before a
performance. This manual did not operate Loopy or verify a current Loopy UI.

## Use an SMC mixer

The SMC is a separate control surface, not a replacement firmware mode. In Global,
open BLUETOOTH MIDI, choose **BT ROLE → SMC HOST**, enable Bluetooth and restart
only when safe to do so. The application and Bluetooth controller image must match.
Choose the SMC preset that matches the physical mixer. **This baseline still has
DAW and USER**, despite a separate DAW-only proposal (#768). DAW is the useful
starting point for release-sensitive held controls; do not mix preset instructions.
The hold/release and Pause-lock instructions below assume **DAW Note messages**.
Stock USER MMC has no release: Solo and momentary-effect controls use a latch/toggle
fallback. USER MMC STOP stops transport; it does not invoke the DAW Pause lock.

Start with one mixer and a known page. Move a fader toward the displayed target.
`PICKUP: MOVE UP/DOWN TO TARGET` means no edit has been acquired yet; `ACTIVE` means
it has. A change from another control can reset pickup. Temporary SMC feedback can
cover or briefly show a mapped page, then expire after about 1.2 seconds. Close it
before relying on the dedicated Free Play key route described above.

| Context/control | Practical action |
| --- | --- |
| Main faders 1–5 | Levels in displayed order: 909, 808, BREAK, 303B, 303A; reverb/delay return and master filter on 6–8. |
| Up/Down | Change the SMC encoder bank without changing the keyboard page; in a menu, select rows. |
| Left/Right | Change ordinary page; in a menu, edit its selected value. |
| R6/R7 outside drum MIX/SEQ | Open/activate the list, or back/cancel. At a question, confirm/cancel. |
| R8 / S6 / S7 / S8 outside drum MIX/SEQ | Toggle HOME/SONG / MIX / Global / cycle sound → SEQ → generator (synth only) → sound. |
| PLAY / STOP / REC | Sequencer play toggle / stop / recording. They are not APC clip commands. |
| Pause | Capture target lock immediately; a separate press held 900 ms unlocks. A short press while locked peeks the captured page. |
| Drum MIX or drum SEQ | Bank Left selects drums 1–8; Bank Right 9–11. The last five strips in bank 2 are inactive. |
| Drum SELECT / Record | Select that drum / select it and toggle recording. SMK pads sequence the selected drum. |
| Drum Solo / Mute | Hold to isolate / toggle mute. Release belongs to its original target after navigation. |

Example: open 909 MIX, use SELECT to choose the snare, then edit its steps on the
keyboard pads. Changing to bank 2 selects access to the remaining three drums;
it does not create drums 12–16. Return to a non-drum page before using R6–R8/S6–S8
as navigation shortcuts.

**On the FX view**, with no menu or clip mode, all eight SELECT strips are bus
effects in physical strip order: 909, 808, BREAK, 303B, 303A, reverb, delay and master filter. Hold SELECT
to play that bus's assigned effect. On strips 6–8 a short tap navigates instead;
a hold of at least 180 ms activates the effect. Outside FX, SELECT chooses the
part; selecting an already selected drum part opens its mixer. SELECT 8 enters
MIX and cycles its pages rather than always opening MASTER.

HOME+SELECT on FX changes a bus's assigned effect; HOME+Left/Right selects the
ROOM/TIGHT/ECHO assignments. HOME+Up stores the five cutoff reset values;
HOME+Down recalls them (fully open if no preset). Release a held effect to return
the bus; send tails can continue. **The source still has low-pass hold caption
state/comments, but the audio publication path sends the assigned effect.** Do not
identify the applied effect from that stale caption alone.

The [candidate SMC reference](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/docs/SMC_MIXER.md)
provides useful protocol history, but its four-bus/non-drum SELECT and Up/Down view
navigation descriptions are older than these source routes. Some older SMC-09
wording also says Pause “toggles”; the actual lock handler and later SMC-11A specify
the separate 900 ms unlock hold. This manual uses the source for those differences.

Two peer contexts exist in application source. That does not prove two physical
mixers, their controller image or lamps work together on this candidate. A shared
host MIDI port cannot identify two independent senders. Keep multi-peer radio and
physical feedback acceptance marked unknown until matching hardware evidence exists.

## Saving and project files

**BT means SAVE on the keyboard.** Save includes sounds, patterns, arrangement,
motion and project settings. It does not turn a pattern into a separate file.

Stop playback, turn REC off, release held notes and leave controls quiet. A save
requested while playing is deferred. Clip-key activity can also delay it; watch
`SAVE AT STOP` / `SAVE WHEN QUIET`. With AUTOSAVE on, stopped/quiet edits can save
after four seconds. Wait for the completion indication before power-off. A flashing
failure or `MEMORY FULL: NOT SAVED` means the work is not safely stored.

The original-engine profile accepts **project revision 13 only**. There is no
migration promise for older projects. Retain old backups; do not repeatedly reset
or overwrite storage to try to make an incompatible project load.

Use the [Projects guide](docs/PROJECT_FILES.md) for browser export/import. It
separates a portable file with original sample-slot images from a project-only file
whose sample identities must match. Prepare and retain a recovery backup before
replacing occupied slots. Stop playback, recording and performance mode, and close
other editors before transfer. Browser availability and actual device transfer
remain unverified here.

LOAD SAVED replaces the live project. RESET PROJECT restores defaults and keeps
sample slots, but defaults include AUTOSAVE ON: the reset can subsequently replace
your saved project. Export before either operation. Import/reset is not itself an
explicit Save command; autosave still applies. Multiple flash objects and a CRC
check do not make a save atomic against power loss.

## Panic and troubleshooting

**No global panic chord is implemented in the inspected baseline.** FX+SEQ on the
keyboard and one SMC peer's DAW BANK L+R belong to later M1 candidate work (#839,
#853), not to this source guide's verified controls. Do not depend on those chords
until the exact installed candidate has been checked. Their intended behavior is
to release stuck sound without stopping future sequencer playback; that is not a
hardware acceptance claim. PLAY stop and muting are different operations.

| Symptom | Check first |
| --- | --- |
| Pads launch clips instead of editing steps | KNOB BANK mode; exit LOOPY CLIPS. |
| Keys write notes when you meant to audition | REC state. Use FREE PLAY with REC off in the supported synth view. |
| Outer keys stop working in a menu/help card | Close the overlay; this baseline's full-key route has explicit view/overlay guards. |
| Fader appears dead | Pickup target, correct bank, page and SMC lock. Move toward the shown value. |
| Wrong drum changes | Visible kit and drum bank; SELECT a drum before editing pads. |
| Bluetooth says READY but SMC is absent | BT ROLE, physical preset and matching controller/app. READY can mean waiting, not connected. |
| Clip colours or lamps disagree | Confirm host feedback routing and exact build; simulated/rendered colours do not certify physical LEDs. |
| Save stays pending | Stop playback, disable REC, release notes, stop moving controls; read the status. |
| Save fails or project will not load | Keep backups; record the exact error/version. Revision mismatch is not a reason to force an import. |
| Stuck sound | Lower K1 or mute the output safely, release keys/Solo/effect holds, stop transport if appropriate; report source and trigger. Do not assume the pending panic patch is present. |

For a useful report, include model, displayed version/source if known, mode/page,
controller preset/connection, exact press/release sequence, expected/actual result
and whether it repeats. Remove private project/sample data from public reports.
Physical latency, USB audio interoperability and radio reliability remain measured
hardware questions, not guarantees inferred from this manual.

## Installation safety

This documentation PR publishes no firmware and qualifies no downloadable package.
An open PR, source build or BUILD_ONLY receipt is not proof of installability.
The old FM-1 installer/recovery links are not a route for installing this candidate.

For a separately authorized qualified release, verify the exact model, source and
package identity, controller compatibility, checksums, current installer admission
and a usable backup/rollback path before installation. Preserve projects, imported
samples, calibration and pairing. Do not assume Pro, Elite or another branded model
is interchangeable because an older guide says “same hardware.”

Current project boundaries withdraw Elite recovery/flash; assigned Pro investigation
is read-only. This manual grants neither action. Do not use a force option or an
FM-1 rescue procedure. Bootloader, BTIF `0xC0000–0xC0FFF`, USRTRIM `0xC1000`, efuse
and `key_mac` are outside this task's authority. If an update is interrupted, retain
the exact package and backup and contact the assigned integrator; do not follow the
old unconditional “replug and retry any package” instruction.

Maintainers should start with [BUILDING.md](BUILDING.md), [LICENSING.md](LICENSING.md)
and the [source reference](docs/FIRMWARE_REFERENCE.md). The build instructions do
not grant device or publication permission.
