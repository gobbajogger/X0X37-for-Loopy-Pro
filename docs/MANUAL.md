# X0X

X0X turns the M-VAVE FM-1 into a groovebox: a 909, an 808, two 303s and a breakbeat player,
all playing at once, in stereo. Each part can play its own pattern, you can arrange patterns
into a song, record knob moves, and give single steps a sound and a chance of their own.

New in this version: see [What's new](https://charlesvestal.github.io/fm1-x0x/new/).

![The FM-1 (a drawing), running X0X, each control labelled](img/fm1-panel.svg)

---

## Contents

1. Install
2. Getting started
3. Controls
4. The screen
5. 909 and 808
6. 303A and 303B
7. TB-3PO
8. Break
9. Patterns
10. Recording
11. Knob motion, p-locks and probability
12. Song
13. Effects
14. Mix and master
15. Settings and saving
16. MIDI and USB audio
17. Troubleshooting
18. Parameters
19. Credits

---

## 1. Install

**Install:** open the [web installer](https://charlesvestal.github.io/fm1-x0x/install/) in Chrome or
Edge, connect the FM-1 with a USB data cable and press Install. Don't unplug it while it's
writing; when it finishes, the FM-1 restarts into X0X.

**Update:** the same way. An update keeps your project: the patterns, sounds, song and settings.
Installing another firmware does not: it keeps its own things in the same memory, so coming back
to X0X starts with an empty project.

**Your loops and updates.** An update never writes your sample slots. If the new firmware keeps
its slots in other places (for example an older X0X, which has fewer slots, or the stock
firmware), the installer tells you which slots would lose their loops. It then saves every slot to a
`.x0xslots` file in your Downloads, installs, and writes the loops back into the new slots by
itself. If a loop does not fit anywhere, it tells you. If writing back fails, keep the file and
restore it with **Restore a backup** on the break loops page. After the stock firmware, restore
it when X0X is back.

**Go back:** "Back to the stock firmware", at the bottom of the same page, puts M-VAVE's firmware
back (download the FM-1 V15 file from the link there, choose it, press the button). M-VAVE's own
updater, M-UPGRADE, works too.

> Installing other firmware is at your own risk. If the FM-1 ever won't start, see
> [Recovering an FM-1 that won't start](#recovering-an-fm-1-that-won-t-start).

<details><summary>Install from the command line</summary>

Download the firmware file (for example `x0x-1.0.fwsc`) from the
[releases page](https://github.com/charlesvestal/fm1-x0x/releases), get the X0X source from GitHub,
install Python 3 with `pip3 install mido python-rtmidi`, and run:

```
python3 tools/fm1_install.py x0x-1.0.fwsc
```

It saves and restores your loops the same way as the web installer. `--backup FILE` chooses
the file; `--no-slots` skips this.

(The web installer on hugelton.github.io installs Felucca, not X0X.)

</details>

---

## 2. Getting started

A five-minute tour, from switching on to saving your first beat. When X0X starts, the screen
shows the 909 and every part is empty.

1. **Add a kick.** Press black key 1 to pick the bass drum (you'll hear it), then press white
   keys 1, 5, 9 and 13.
2. **Press PLAY.** The kick plays on every beat.
3. **Add a clap.** Press black key 7, then white keys 5 and 13. While the pattern plays, picking
   a drum is silent, so you don't add a stray hit.
4. **Add open hats.** Press black key 9, then white keys 3, 7, 11 and 15.
5. **Write a 303 line.** Turn ALGORITHM two clicks clockwise to 303A and press ARP. Press OCT+:
   each press writes a new line. Keep going until you like one.
6. **Shape the 303.** Press EDIT. The knobs are now the 303's filter: KNOB 1 opens and closes it,
   KNOB 2 sets the resonance.
7. **Add the break.** Turn ALGORITHM two more clicks to BREAK and press all 16 white keys, so the
   break plays the whole bar.
8. **Mute a part.** Press HOME, then black key 5 to mute the break. Press it again to bring it
   back.
9. **Record a filter sweep.** Turn ALGORITHM back to 303A and press EDIT. Press REC, slowly turn
   KNOB 1 for a bar or two, then press REC again. The sweep now plays every time the pattern
   comes round.
10. **Save.** Press PLAY to stop, then SAVE. Everything is kept when you switch off. (X0X also
    saves by itself a few seconds after you stop; see section 15.)

Next: section 9 shows how to make more patterns and switch between them, and section 12 how to
arrange them into a song.

---

## 3. Controls

The drawing at the top of the manual names every control. In more detail:

A button that opens a screen always opens the same one; press it again for that screen's next
page. SEQ, EDIT and ARP show the part you choose with ALGORITHM.

| Control | What it does |
|---|---|
| MASTER | Volume. |
| SELECT | The previous or next page. In a list, up and down. Hold a step and turn it for the step's chance (section 11). |
| PRESETS | The next pattern: on a part's screen for that part only, on HOME for all five. |
| ALGORITHM | The part: 909, 808, 303A, 303B or BREAK. In a list, it changes the value. |
| OCT-, OCT+ | The steps, 16 at a time (1–16 … 49–64). On HOME and SONG, patterns 1–16 or 17–32. On the 303 keyboard, the octave. On TB-3PO, mutate the line or write a new one. |
| KNOB 1–4 | The four values at the bottom of the screen. Each click is one step, however quickly you turn a few; spin and they speed up. |
| FX | The reverb and the delay. |
| SEL | Everything on this screen as a list, including the settings on no page (see Lists). In a question, yes. |
| ENV | Hold while pressing keys: an accent. |
| LFO | The mixer and the master. Hold while pressing keys: a slide. |
| EDIT | The selected part's sound. The keys still write its steps. |
| GLO | The settings. Press again to close. |
| HOME | The pattern: all five parts, which pattern they play, mutes. Again: the song, and again back. In a list, back; in a question, no. |
| SAVE | Save everything (while playing: when you stop). Hold it to copy the pattern; then a white key pastes it. |
| ARP | TB-3PO, the 303 line generator (on a 303). |
| SEQ | The selected part's steps, with its LENGTH and RATE (on a 303 also DIRECTION and TRANSPOSE). |
| PLAY | Start and stop. |
| REC | Record on and off: drum hits, 303 notes, knob moves, and on the SONG screen the song. On a 303 the keys become a keyboard while it's on. |
| White keys | Steps. On HOME, patterns. On MASTER and FX they do nothing. |
| Black keys | Drum tracks, or the break's slices. On HOME they mute parts; on the mixer they pick a channel. |

**Hold HOME** for these: + REC undoes, + PLAY redoes, + SELECT sets the tempo, + a white key picks
a pattern from any screen, and + a black key mutes a drum track (on the 909 or 808 screen) or a
channel (on the mixer).

![What the keys do](img/fm1-keys.svg)

(The SEL button is labelled SCL in Felucca.)

### Lists

SEL turns the screen you're on into a list of everything on it: every knob from its pages,
grouped (on the 909, for example, BD: THIS TRACK and 909: ALL TRACKS), and then **MORE**, the
settings that are on no page. MORE is where the less used settings live, such as a drum
track's DRIVE and DIST or the compressor's details.

In a list, SELECT moves, ALGORITHM changes the highlighted value, and SEL runs a row with a
return arrow (an action). HOME goes back. X0X always asks before doing anything that would lose
notes: SEL for yes, HOME for no.

![A list](img/screen-list.png)
![A question](img/screen-ask.png)

> **Not sure what a button does?** Hold it for a second on its own: a card says what it does on
> the screen you're on, with its combinations. Let go and nothing happens.

### Undo

Hold HOME and press REC to take back your last change; the screen says what it undid, for
example UNDO 909 P1. HOME + PLAY redoes it. You can go back up to 32 steps (fewer after very big
changes). A step is whatever you did before pausing for a moment: a few steps tapped in a row,
one turn of a knob, or a whole recording pass. Undo covers the sounds, patterns, song and knob
motion, including CLEAR PATTERN, but not the tempo or the settings. Loading a saved project,
importing a project or choosing RESET PROJECT clears the history; you cannot undo those
replacements. The history is also cleared when you switch off.

<details><summary>What the key lights show</summary>

The key lights have two levels: bright for what is on or chosen, dim for what is there to
choose.

| Screen | Bright | Dim |
|---|---|---|
| HOME | the pattern playing (a cued one blinks); the parts playing | patterns with notes; muted parts |
| SONG | the bar's pattern; the bar's parts | patterns with notes; the bar's muted parts |
| 909 / 808 | the track's hits; the track | the other tracks with hits |
| 303 | notes | ties |
| 303 with REC on | the keys you hold | the Cs, to find your place |
| BREAK | the steps it plays | the slice pads 1–8 |
| Mixer (PARTS, 909 MIX, 808 MIX) | the channel the knobs set (blinking while muted) | the other channels |

</details>

---

## 4. The screen

![The 909](img/screen-909.png)

The **top line** shows, from left to right:

- the part, in its colour (909 orange, 808 red, 303A green, 303B blue, BREAK violet), and the
  screen if it isn't the sound: SEQ, TB-3PO, or KEYS while the 303 keyboard is on;
- the pattern playing, such as P3. P3 >5 means pattern 5 is coming up next. On a part longer
  than 16 steps, the steps the white keys show follow (33-48);
- the tempo, amber when X0X follows an external MIDI clock;
- a play symbol while playing and a red dot while recording. A small grey dot means there are
  unsaved changes.

When a button does something, the top line says what for a second, for example SAVED or PARTS
PASTED TO P5.

The **knobs** sit under a tag that says whose they are: the selected track (BD), the whole
machine (909 KIT), a 303's sound (303A TONE), the pattern (P3 SEQUENCE), the mixer (PARTS: 303A,
909 MIX: CP), a held 303 step on SEQ (303A STEP 5), or a p-lock (BD P-LOCK STEP 5). When a
section has more than one page the tag counts them (BD (1/2)), and the dots at the end of the
line are all the screen's pages, the current one lit. Settings with only a few choices show a
row of small squares instead of a ring.

When you turn a knob, its value shows in large type for a second, with whose it is: a track's
(BD TUNE) or a pattern's (P3 LENGTH). Settings that belong to the pattern change when the
pattern does, and are copied with it; the sound stays the same whatever pattern plays.

The **footer** at the very bottom says what the keys do on this screen (BLACK: TRACK, WHITE:
STEP). Until you first press a button it says HOLD ANY BUTTON: WHAT IT DOES.

![Turning a knob](img/screen-readout.png)

---

## 5. 909 and 808

Each drum machine has eleven tracks, one on each black key:

| Key | 909 | 808 |
|---|---|---|
| 1 | Bass drum (BD) | Bass drum (BD) |
| 2 | Snare (SD) | Snare (SD) |
| 3–5 | Low, mid and high tom | Low, mid and high tom, or congas |
| 6 | Rim shot (RS) | Rim shot, or claves |
| 7 | Clap (CP) | Clap, or maracas |
| 8 | Closed hat (CH) | Cowbell (CB) |
| 9 | Open hat (OH) | Cymbal (CY) |
| 10 | Crash (CR) | Open hat (OH) |
| 11 | Ride (RD) | Closed hat (CH) |

The 909's hats and cymbals are samples, as on the original machine; every other sound on both
machines is synthesized. On the 808, a track's SOUND setting switches it between tom and conga,
rim and claves, or clap and maracas. On both, the closed hat cuts off the open hat.

**Writing a beat.** Press a black key to pick a track; while the pattern is stopped you hear it
too. (While it plays, picking is silent so you don't add a stray hit; GLO > KEY SOUND = ALWAYS
changes that.) Then press white keys: an empty step turns on, and a tap on a step that's on
turns it off. Hold ENV while pressing white keys to place accents instead; an accent makes every
drum on that step louder.

A pattern can be up to 64 steps long. The white keys show 16 at a time: OCT+ moves to the next
16 and OCT- back, and the top line shows which 16 you're on.

**Muting a track:** hold HOME and press its black key; again to bring it back. A muted track's
row turns grey. Track mutes aren't saved, and muting the whole part on HOME leaves them as they
are.

**The pages.** EDIT shows the selected track's sound (one page, two for the 909's kick), then KIT
with ACCENT, how much louder accented steps are. A track's DRIVE and DIST are in SEL's list,
under MORE. SEQ has the machine's LENGTH and RATE. SWING is on HOME, because it swings every part.

Each track's level, pan and reverb and delay sends are on the mixer: press LFO and go to 909 MIX
or 808 MIX, which shows all eleven levels at once. The black keys pick a track and the knobs are
its LEVEL, PAN, REV and DLY. On the 808 the alternate sounds share their track's pan, so the
conga sits where the tom does.

---

## 6. 303A and 303B

There are two identical 303s. Each has its own sound and its own line.

![303A](img/screen-303.png)

**Steps.** Press a white key to turn that step on; tap it again to turn it off. To edit a step,
press SEQ, hold the step's key and turn the knobs: KNOB 1 sets the note, KNOB 2 the gate (REST,
NOTE or TIE), KNOB 3 the accent and KNOB 4 the slide. OCT- and OCT+ move the held note an octave.
An empty step turns on as soon as you press it, so you can press, hold and set its note in one
go. A slide glides from this step's note into the next one. (Holding a step on EDIT gives it a
sound of its own instead: see P-locks in section 11.)

A **tie** holds the note before it through the step, so it doesn't play again. On the screen a
tied note joins the note it holds in one long bar. Turning KNOB 1 on a tie makes it a note of
its own.

For a longer bass note, enter a NOTE, then set each following step to TIE with
KNOB 2. For example, `NOTE C2 | TIE | TIE | REST` sounds one C2 across three
step positions, with no new attack on the ties. The final tie releases halfway
through its step, as a normal note does. To glide into a new pitch instead, use
SLIDE on the step before that NOTE. The part's RATE sets the time between steps;
adding ties extends a note without changing that rate.

**Keyboard.** Press REC and all 27 keys become a keyboard (KEYS shows on the top line). The
notes you play are written: step by step while the pattern is stopped, live while it plays.
OCT- and OCT+ change the octave. Play a key while still holding another and the 303 slides
between them. Press REC again to go back to steps.

While stopped with REC on, enter the note, then hold LFO and press OCT- or OCT+
once for each continuation TIE. Hold ENV and press OCT- or OCT+ to insert a REST.
Live recording currently writes a NOTE when you press a key; holding that key
does not automatically write ties. Add the ties in SEQ after recording.

**Free play.** To play the 303 without writing anything, press SEQ, turn SELECT to the KEYS page
and set FREE PLAY to ON. The keys then play the 303 on every 303 screen, stopped or playing, so
you can try lines and turn EDIT's knobs as you go; the top line says FREE PLAY. Nothing is
written until you press REC, which records as above. Set FREE PLAY back to OFF for the steps.
Each 303 has its own, and both are off at power-on. In FREE PLAY, the SMK keybed's strike
velocity controls volume and cutoff: soft notes are quieter and darker; hard notes are louder
and brighter. Incoming MIDI notes use the same response. Velocity 100 is neutral. Overlapping
notes still slide.

**The pages.** EDIT has TONE (CUTOFF, RESO, ENVMOD, DECAY) and VOICE (ACCENT, WAVE, saw or
square, DRIVE, SLIDE time); TUNE, VOLUME, DRIVE TYPE (off, soft or RAT) and ACCENT DECAY are in
SEL's list. FILTER ADSR adds FATTACK (0–2000 ms), DECAY (200–2000 ms), FSUSTAIN (0–100%) and
FRELEASE (0–4000 ms). In SEL's list, set FENV to ADSR to use these filter controls; 303 keeps
the original envelope and is the default for existing sounds. DECAY is the same control on
TONE and FILTER ADSR. ENVMOD sets the filter envelope depth. A slide continues the envelope;
a fresh note starts it again. These controls affect the filter only: the original volume
envelope still determines how long the release is audible.

SEQ has the line's LENGTH (1–64 steps), RATE, DIRECTION (forward, reverse, ping-pong
or random) and TRANSPOSE. Each 303's level, pan and sends are on the mixer (LFO).

---

## 7. TB-3PO

TB-3PO writes 303 lines for you. Pick a 303 with ALGORITHM and press ARP.

![TB-3PO](img/screen-tb3po.png)

Press **OCT+** to write a completely new line, or **OCT-** to mutate the current one (about a
quarter of its steps change).

TB-3PO has two pages (press ARP again, or turn SELECT):

- GENERATE: DENS, ACCENT and SLIDE set how many steps play, are accented and slide. OCTS sets
  the range, from 1 to 3 octaves.
- SCALE: ROOT, SCALE (minor, Phrygian, harmonic minor, minor pentatonic, Dorian or major),
  OCTAVE, and MUTATE, which mutates the line by itself every 1–16 bars (or never).

These knobs don't change the line you're hearing. They set up the next OCT+ or OCT-, so your
line stays exactly as it is, hand edits included, until you press one. When the knobs no longer
match the line, the screen shows NEW LINE with a star. The same settings and seed always give
the same line, the same one Schwung's TB-3PO module writes.

To edit the line by hand, press SEQ; to shape its sound, EDIT.

---

## 8. Break

The break is a drum loop cut into eight slices, which X0X rearranges as it plays.

![The break, slice 5 playing](img/screen-break.png)

**Steps.** The white keys are the bar's 16 steps: press one to let the break play there, tap it
again to stop it.

**Playing slices.** Hold black keys 1–8 to play those slices yourself; let go and the generator
takes over again. Hold key 9 to play backwards, key 10 for half speed, key 11 to stutter.

**A step's own slice.** Hold a white key and press black key 1–8: that step now always plays
that slice of loop A, from its start, and its box shows the slice's number. Press the same black
key again (holding the step) to give it back to the generator. This way you can fix the kick and
snare where you want them and let the generator play with the rest.

**The generator.** SEQ has its two pages, GROOVE and RETRIG:

- COMPLEXITY: how often it jumps to a different slice instead of playing the next one. This is
  the amount: at 0 the loop plays straight, and ROLL and FILL come in with it, at full effect
  from 25%, so low settings stay close to the loop.
- ANCHOR: how strongly it keeps the kick and snare slices on beats 1 and 3.
- ROLL: how often it repeats a slice or moves to the one next to it.
- FILL: how much the last bar of a phrase breaks these rules.
- RETRIG 2X, 3X, 4X and 8X: the chance in each bar of stuttering a beat at that rate.

In SEL's list: PHRASE, the length of a phrase (2, 4, 8 or 16 bars, or off), and A LENGTH and B
LENGTH, how often a new slice is chosen (every 1/4 bar to every 8 bars).

**The loops.** EDIT has LOOP A, LOOP B, B CHANCE (the chance of switching to loop B in the last
bar of a phrase; with PHRASE off, in any bar) and PITCH; LEVEL and 8 SLICES are in SEL's list.
8 SLICES is off unless you turn it on. Off, MIDI notes 36–43 on the break's channel play the
eight slices. On, every MIDI note on that channel plays a slice (the note number divided by 8,
the remainder picks the slice), so higher and lower notes replay the same eight slices. Loops follow the
tempo by playing faster or slower, so their pitch follows it too, as on a sampler; PITCH shifts it
on top. The default app build leaves the loop bank empty; a packaged firmware may include stock
loops in the free code tail. The installer keeps the selected package's loops by default. You can
replace them with the three stock loops, CRAMEN, TASTY and HOTPANT, or your own WAV files.
The stock pack retains complete loops with one-bar metadata; see `assets/factory-breaks/provenance.json`
for sample lengths and conversion details. Loops uploaded into sample slots follow the installed loops.

All of these, the steps, slices and loops, are saved with the pattern. The break's level, pan
and sends are on the mixer's PARTS page (LFO, black key 5).

### Your own breaks

Load your own loops with the [break loops page](https://charlesvestal.github.io/fm1-x0x/breaks/), in
Chrome or Edge with the FM-1 connected. They appear after X0X's own loops as BR1, BR2 and so on.
Loops are sent at full quality, 44.1 kHz. The SMK-37 has two slots: slot 1
holds about 3.7 seconds and slot 2 about 5.5 seconds, about 9.2 seconds
in all. The FM-1 has two slots of about 3.7 seconds, about 7.4 seconds in all. A one-bar loop
at 120 BPM takes 2 seconds. A loop must fit in one slot, so the longest loop on the SMK-37 is
about 5.5 seconds. The page fills the slots in order and puts a loop that is too long for one
slot into the next slot that can take it.
Tick **22 kHz (more room)** to send new loops at half the rate: about twice as much fits, but the
sound is duller. Tick **Trim silence** to cut the silence at the start and end of each file first.

**Back up your loops.** On the same page, **Back up all slots** saves every slot to a `.x0xslots`
file. It only reads, it doesn't change anything. **Restore a backup** writes such a file back; it
replaces the slots it holds. Do a backup before you update the firmware or try something new.

<details><summary>From the command line</summary>

With the FM-1 connected, run:

```
python3 tools/upload_breaks.py --bars amen.wav think.wav funky.wav ...
```

`--bars` keeps one bar of each file; without it, each file is sent whole. This replaces all your
loops. Loops are sent at 44.1 kHz; `--rate 22050` sends them at 22.05 kHz to fit twice as many.
`--trim` cuts the silence at both ends first. `--dry-run FOLDER` shows what would fit without the FM-1. If you have schwung-breakbeat next
to X0X, `--bbgen` sends BB Gen's classic breaks. To build loops into the firmware itself, see
BUILDING.md.

`python3 tools/x0x_slots.py backup my.x0xslots` saves every slot; `python3 tools/x0x_slots.py restore
my.x0xslots` writes them back (it asks first).

</details>

---

## 9. Patterns

![HOME](img/screen-home.png)

There are 32 patterns. A pattern holds each part's steps, length and rate, the break's settings,
the swing, and any knob motion, p-locks and probabilities. Sounds, effects and master settings
are not part of a pattern and don't change when the pattern does.

Each part plays its own pattern, so the 303s can play pattern 3 while the drums play pattern 7.

**HOME** shows patterns 1–16 as boxes, one per white key; OCT+ shows 17–32 (P17-32 appears under
the boxes) and OCT- goes back. Every fourth box is numbered. In each box, a line for each part
with notes in that pattern: bright where that part is playing it now, dim otherwise.

**Choosing patterns**

- **All five parts:** on HOME, press a white key or turn PRESETS. From any other screen, hold
  HOME and press a white key.
- **One part:** on that part's screen (SEQ, EDIT, TB-3PO), turn PRESETS.
- **A chain:** on HOME, hold two white keys at once. All five parts play each pattern from the
  first to the second, a bar each, then start again. Choosing a single pattern ends the chain.

While playing, the new pattern starts when the current bar ends; stopped, it takes over at once.
A part that changes pattern starts again from step 1; the others carry on.

**Copy and paste.** Hold SAVE: the top line says CHOOSE PASTE TARGET. Press a white key to copy
the five parts currently playing onto that pattern (PARTS PASTED TO P2). Each part can come from a
different pattern. If the destination has notes, X0X asks first; the source pattern numbers are
held from when you pressed the destination key, and their contents are copied when you confirm.
SAVE does not make a clipboard snapshot. While holding SAVE, OCT- and OCT+ switch between patterns
1–16 and 17–32.

**Clearing.** Hold SAVE and press REC to clear the selected part in its pattern. GLO > CLEAR
PATTERN clears all five.

**Muting.** On HOME, black keys 1–5 mute and unmute the parts. Mutes aren't saved, except inside
a song.

**Lengths and swing.** Parts can have different lengths, so they drift against each other. The
909's length sets the length of a bar, and the 909's pattern holds the swing, which applies to
every part. Swing delays every second 16th note: 50% is straight, 75% the most.

HOME's knobs are for playing live: TEMPO, SWING, and the master's FILTER and PUMP.

---

## 10. Recording

Press REC to turn recording on, and again to turn it off.

- **Drums:** while the pattern plays, press black keys to record hits (with REC on you always
  hear them). Hold ENV for accented hits.
- **303, live:** with REC on, the 303's keys are a keyboard; play while the pattern runs.
- **303, step by step:** with REC on and the pattern stopped, each key you press fills the next
  step, from step 1. Hold ENV while pressing a key for an accent, or LFO for a slide. For a rest,
  hold ENV and press OCT- or OCT+; for a tie, hold LFO and press OCT- or OCT+.
- **Knob moves:** with REC on and the pattern playing, turn any sound knob (section 11).
- **The song:** REC + PLAY on the SONG screen (section 12).

---

## 11. Knob motion, p-locks and probability

All three give steps values of their own, and all three are saved with the pattern.

### Knob motion

![A recorded cutoff sweep playing](img/screen-motion.png)

Turn on REC, press PLAY and turn any sound knob. X0X records the knob on the steps where you moved
it, fills in short pauses so a slow sweep has no gaps, and on playback glides from step to step.

- A knob with motion has a dot in its corner, and its ring follows the recording as it plays.
- On steps you didn't touch, the knob's own setting plays.
- Turning the knob without REC takes over for one pass of the pattern; then the recording plays
  again.
- **To clear a knob's motion,** hold SAVE and turn the knob.
- Motion belongs to a part's pattern: the 303's knobs record into the 303's pattern; the effects
  and master into the 909's. Copying or clearing a pattern copies or clears its motion too.
- When you stop, every knob goes back to its own setting.

### P-locks

A step can have a sound of its own: a higher tune on one kick, an open filter on one 303 note,
the break pitched up for one step.

- On EDIT, hold a step that's on and turn a sound knob. That step plays the new value; the
  others keep the knob's. While you hold the step, the knob row says P-LOCK STEP 5, the knobs
  show the step's values, and an amber dot marks the locked ones. Stopped, you hear the lock
  while you hold the step.
- A tap still turns a step on or off: only a hold with a knob turn makes a lock. An empty step
  turns on as soon as you press it, so you can press, hold and lock in one go.
- **To clear one lock,** hold the step, hold SAVE and turn the knob.
- On SEQ, holding a step locks nothing (on a 303 the knobs set the step's note instead).

### Probability

Hold a step (on SEQ or EDIT) and turn SELECT to set the chance it plays, from 100% down to 5%
in steps of 5; the top line shows it (STEP 5: 50%). The dice are rolled every time the step comes
round, so 50% plays about every other pass.

Keep turning past 5% for **cycles**: 1:2, 2:2, 1:3 and so on up to 8:8. A step at 1:4 plays on the
first of every four times round its part's pattern, 2:4 on the second; nothing random about it.
Each part counts its own passes, from 1 again whenever you press PLAY or it changes pattern.

- On the 909 and 808 the chance belongs to the selected track's step, so the kick and the hats
  on one step can each have their own.
- On a 303 a skipped step is a rest; on the break, a skipped step doesn't fire.
- A skipped drum hit sends no MIDI note.

### Good to know

- Steps with p-locks or a chance below 100% have a small notch on the step grid.
- P-locks and probabilities are stored like recorded motion, one "knob" per locked setting per
  part pattern. There's room for 160 across all patterns; when it's full, the top line says
  MOTION FULL.
- Two locked steps next to each other glide from one value to the other, like a recording.

---

## 12. Song

![SONG](img/screen-song.png)

A song is a list of up to 192 bars. Each bar says which pattern each part plays and which
parts are muted. To open the song, press HOME twice (HOME again comes back to the pattern).

- **Write bars:** press a white key. The selected bar gets that pattern for all five parts,
  and the next bar is selected. Pressing keys at the end of the song adds new bars.
- **Edit a bar:** turn KNOB 1 to select the bar, then use the other knobs to set each part's
  pattern for that bar.
- **Mute parts in a bar:** press black keys 1–5.
- **Repeat a bar:** on the SONG screen's second page (turn SELECT), REPEAT plays the selected bar
  up to 8 times (x2 … x8 shows at the end of its row), so four bars of the same pattern are one
  row.
- **Play the song:** press PLAY on the SONG screen. The song starts at the selected bar and
  loops, and keeps playing wherever you go (HOME for mutes, EDIT for sounds) until you stop it.
  While it plays, the top line shows S and the current bar number. PLAY anywhere else plays the
  patterns: whatever screen you start from decides, and the top line says which (S or P).
- **Record the song:** on the SONG screen, turn on REC and press PLAY. As it plays, change patterns
  and mute parts however you like. Each bar is written into the song as it goes by, starting
  from the bar you started on, and the song grows if you play past its end. Recording
  replaces what was in those bars before.

Press SEL on the SONG screen for LENGTH, INSERT BAR, DELETE BAR and CLEAR SONG.

---

## 13. Effects

![FX](img/screen-fx.png)

There is one reverb and one delay, and every part can send to both: each part's sends are on the
mixer's PARTS page (LFO), each drum track's on 909 MIX and 808 MIX. FX shows how much every part
sends, and has two pages:

- REVERB: DECAY and TONE. The reverb is stereo: whatever you send it comes back wide.
- DELAY: TIME (in note values from 1/32 to a dotted half note), FEEDBACK, TONE and TAPE. TAPE at
  the far left (DIGI) is a clean 12-bit digital delay; turned up, it is a tape delay that wobbles,
  saturates and wears more the further you turn it, and can feed back on itself at high feedback.

In SEL's list, under MORE: each effect's low cut and return level, and PING-PONG, which bounces
the echoes between left and right.

---

## 14. Mix and master

![MIX, with the compressor pumping](img/screen-mix.png)

Press LFO to open the mixer; press it again (or turn SELECT) for its five pages. The red bar on
the right is the compressor's gain reduction.

- **PARTS**: a level and a meter for each part. Press black keys 1-5 to pick the 909, 808,
  303A, 303B or the break; the knobs are its LEVEL, PAN, REV and DLY.
- **909 MIX** and **808 MIX**: each drum machine's own mixer, every track's level at a glance;
  the black keys pick a track, and the knobs set its LEVEL, PAN, REV and DLY.
- **MASTER**: four knobs over the whole mix. The screen shows the output level over the last
  couple of seconds, so you can see the pump and the compressor working.
- **WAVE**: the same four master knobs, with a live mid-line plot of the master output (after
  the compressor, pump, filter and limiter). The compressor's gain-reduction bar stays on the
  right. Press LFO again from MASTER to open it.

On PARTS, 909 MIX and 808 MIX, hold HOME and press a black key to mute or unmute that channel (a
part on PARTS, a track on the drum pages). The key lights show it: the channel the knobs set is
bright (blinking while muted), the others dim, muted ones dark.

Turning ALGORITHM on the mixer goes to that part's page: 909 MIX or 808 MIX for the drum
machines, PARTS (with the part picked) for the 303s and the break.

| MASTER | What it does |
|---|---|
| DRIVE | Saturates the whole mix (the 909's drive stage). |
| COMP | One knob for the compressor: 0 is off; turning up lowers its threshold, raises its ratio and adds makeup gain together. Three quarters is the factory's hard squeeze (4:1 from -24 dB, +16 dB). |
| PUMP | Lowers the whole mix each time the kick plays, by up to 24 dB, and lets it swell back up: the big-beat breathing. Accented kicks pump harder. It works even with COMP at 0, and even when the kick is muted, so a silent kick can drive it. Which kick: PUMP BY in SEL's list (the 909's, the 808's or both). |
| FILTER | One knob: in the middle it's off; to the left a low-pass closes down, to the right a high-pass opens up. |

FILTER and PUMP are also on HOME's PERFORM page, for playing live.

> **The factory mix is set up for big beat:** the break up front with the 909 kick under it, the
> 303s through RAT distortion into a tape delay, reverb on the clap. For a clean mix, set COMP and
> PUMP to 0.

<details><summary>The master in detail</summary>

In SEL's list, under MORE: the compressor's THRESH (-48 to 0 dB), RATIO (1:1 to
20:1 and INF), ATTACK (0.1 to 100 ms), RELEASE (10 to 1500 ms), MAKEUP (0 to 24 dB) and MIX (dry
to fully compressed, for parallel compression); PUMP BY (the 909's kick, the 808's, or both); the
filter's RESO; LIMIT, which prevents clipping (leave it on); and the drive stage's DIST type, its
GLUE compressor and VOLUME. Turning COMP or FILTER sets their details again.

</details>

---

## 15. Settings and saving

![GLOBAL](img/screen-global.png)

Press GLO for these settings, in five sections (MIDI, AUDIO, KEYS AND SCREEN, PROJECT, SYSTEM).
The SMK-37 has a sixth, BLUETOOTH MIDI (see [Bluetooth MIDI](#bluetooth-midi-smk-37)):

| Setting | What it does |
|---|---|
| CLOCK OUT | Sends MIDI clock and start/stop messages. |
| NOTES OUT | Sends the patterns out as MIDI notes. |
| 909 … BREAK MIDI CH | Each part's MIDI channel, for notes in and out: 1–16, or OFF to ignore incoming notes for that part (and send none). |
| OUTPUT | NORMAL, or LOUD: 6 dB more at the headphones and line out (and USB audio), for headphones that need it. The master's LIMIT (on by default) keeps it from clipping. |
| USB AUDIO | FOLLOWS KNOB (the default): USB audio is as loud as the FM-1, MASTER knob and all. FULL: USB audio stays at full level whatever the knob says, so you can record over the cable with the speaker turned down. |
| LIGHTS | ON: the key lights show the steps, and the unlit buttons glow dimly so they can be read. KEYS: the key lights only. OFF: neither. |
| KEY SOUND | STOPPED (the default): picking a drum or editing a 303 step is silent while the pattern plays, unless REC is on. ALWAYS: you always hear it. The 303 keyboard always plays either way. |
| THEME | The screen colour: green, amber, cyan, red or mono. |
| SAVE PROJECT | Saves everything. |
| LOAD SAVED | Loads the last saved project. Stop playback and recording first. Asks before discarding unsaved edits. Keeps the sample slots and clears undo history. |
| AUTOSAVE | ON (the default): while the pattern is stopped, changes are saved by themselves once nothing has been touched for four seconds. OFF: only SAVE saves. |
| CLEAR PATTERN | Clears all five parts of the current pattern. |
| RESET PROJECT | Asks before resetting the live project to factory sounds, empty patterns and default settings, with no song or knob motion. Keeps the sample slots and clears undo history. AUTOSAVE returns to ON, so the reset state can replace your saved project automatically. |
| PERFORMANCE | Opens the performance page (below). |
| ABOUT X0X | Shows the version, how full the memory for patterns is (MEM), and the audio load. |
| BATTERY | The battery's charge: FULL, 2/3, 1/3 or EMPTY; USB POWER while a computer powers the FM-1. Read only. |
| BLUETOOTH | SMK-37 only. OFF (the default) or ON: Bluetooth MIDI. Turning it ON takes effect at the next power-on ("RESTART TO TURN ON"); turning it OFF drops the connection at once. |
| STATUS | SMK-37 only, read only. NO IMAGE (no Bluetooth controller installed), OFF, RESTART, STARTING, READY (waiting for a connection), CONNECTED, FAILED or BAD IMAGE (the installed controller is damaged or does not fit this X0X). |
| NAME | SMK-37 only, read only. The name the keyboard shows to Bluetooth apps: X0X- and four characters of its own. |

### Saving

**SAVE** saves the sounds, all 32 patterns, the song, the knob motion, the tempo and the
settings. UNSAVED in the header and a SAVE button that flashes mean the project needs a save.
If the header has little space, it shows a small amber mark. You lose unsaved changes when you
switch off. Hold SAVE to see the current save state.

With **AUTOSAVE** on, X0X saves after you stop the pattern and leave the controls untouched
for four seconds. AUTOSAVE is on by default. X0X then says AUTOSAVED.
If REC is on, turn REC off for autosave. Hold SAVE to see this reminder.

> **X0X never saves while the pattern plays.** A save pauses the sound for a moment. The FM-1
> plays from the same memory that it writes. A press on SAVE while the pattern plays says
> SAVES WHEN STOPPED and shows SAVE AT STOP in the header. X0X saves when you stop. The SAVE
> button stays lit until then.

In clip mode while X0X plays, SAVE first shows SAVES WHEN STOPPED and then SAVE AT STOP in the header. Once X0X stops, the header changes to SAVE WHEN QUIET. A SAVE pressed while clip keys or encoders are active shows SAVES WHEN QUIET. X0X waits
until the transport stops and keybed or control activity stays quiet for four seconds. The
SAVE button stays lit until the save completes.

If a save fails, the header shows SAVE FAILED (or a red mark when space is tight). The SAVE
button flashes fast. The warning stays until a save succeeds. A flash write failure also
shows SAVE FAILED after an autosave.

<details><summary>How much fits</summary>

X0X packs the patterns, the song and the knob motion to fit them in the FM-1's memory; empty
steps take almost no room, so a typical project uses a small part of it (GLO > ABOUT X0X shows
how much, as MEM). If a project ever grows too big to fit, X0X says MEMORY FULL: NOT SAVED and
keeps what was saved before; clearing patterns you don't need makes room. Projects saved by
earlier versions load as they were, with their steps on 1–32 and their patterns on 1–16.

</details>

### Portable project files

The **Projects** companion page exports and imports `.x0xproject` files over USB MIDI.
Export includes the project and the original samples by default, preserving their encoded
bytes and slot positions. A smaller project-only file is also available; it can be imported
only when every sample slot matches, including empty slots.

Choose **Projects** in the generated site's home page, or serve the repository over localhost
and open `web/x0x_projects.html`. See the [project files guide](PROJECT_FILES.md) for browser
requirements, backups, import confirmation and recovery. Import replaces the live project
and uses the file's autosave setting. Use SAVE to store the new project while stopped.

<details><summary>Performance: how hard the FM-1 is working</summary>

GLO > PERFORMANCE shows how hard the FM-1 is working, updated every second: the audio load and
its peak, any dropouts, and how much of the processor each part is using.

For a proper measurement, press SEL and choose RUN PERF TEST. X0X plays three patterns of its
own for about 15 seconds, from a simple loop up to everything as busy as it gets, then puts your
project back exactly as it was and shows the results. HOME or PLAY stops the test early.

X0X uses both of the FM-1's processor cores: the 808 and the break run on the second, so the 808's
share on the performance page is the time the first core waits for them, and the whole load is lower: on
one FM-1 the test's three patterns went from 43%, 83% and 115% on one core to 39%, 54% and 68%,
and the busiest moment of the worst case from over the limit to 84%.

The top right of the performance page says 2 CORES. On some FM-1s the second core reads its work
wrongly; X0X notices the first time and carries on with one core, and the page says 1 CORE: BAD
READS in red. Everything still works, but the busiest patterns may drop out.

When the FM-1 gets close to its limit, X0X lightens the load by itself instead of dropping out:
the 303s stop oversampling (bright, resonant notes get a touch grittier) and the 808's sounds end
a little sooner as they fade. While this is happening, GUARD lights up in amber next to the audio
load; it switches off again after three quiet seconds. The performance page counts how often it
has kicked in (GUARD 3x).

If X0X drops out on one of your patterns, a photo of the test results and of the pattern is the
most useful thing you can send.

KNOB 1 turns on some extra counters (STALLS). They're experimental: if the FM-1 acts up with them
on, switch it off and on again.

</details>

---

## 16. MIDI and USB audio

When connected over USB, the FM-1 shows up as a MIDI device called "X0X FM-1".

X0X takes MIDI from USB and from the FM-1's TRS MIDI IN jack, both the same way. Each part
listens on its own channel; these are the defaults:

| Input | What it plays |
|---|---|
| Channel 10 | The 909, with General MIDI drum notes: 36 kick, 38 snare, 41, 45 and 50 toms, 37 rim, 39 clap, 42 closed hat, 46 open hat, 49 crash, 51 ride. |
| Channel 11 | The 808, with the same notes, plus 56 for the cowbell and 49 for the cymbal. |
| Channel 2 | 303A. Overlapping notes slide, and a velocity of 100 or more plays an accent. |
| Channel 3 | 303B, in the same way. |
| Channel 4 | Notes 36–43 play break slices 1–8. With 8 SLICES on, every note on this channel plays a slice (note mod 8). Notes on this channel do not also play another part. |
| Clock | X0X follows an incoming MIDI clock automatically, and returns to its own tempo half a second after the clock stops. Start, Stop and Continue start and stop the patterns. |

GLO sets each part's channel (909 MIDI CH and so on), or OFF to ignore its notes, for example when
another box sends you its clock with notes on the same channels. X0X can also send MIDI: clock and
start/stop when CLOCK OUT is on, and the patterns as notes on each part's channel when NOTES OUT
is on.

While X0X follows an external clock, the tempo on the top line is averaged over about a second,
so it doesn't flicker with every clock.

### Bluetooth MIDI (SMK-37)

The SMK-37 can also send and receive MIDI over Bluetooth (BLE MIDI, as iPads, Macs and most
music apps use it). Bluetooth audio is not supported.

The Bluetooth controller is a separate program that uses JieLi components. Qualified NIGHTLY
packages include the matching controller, so select one complete package in the NIGHTLY
installer. No separate `ctl.img` selection is needed for that path. Availability depends on
the package checks, applicable component terms and authorized publication; see LICENSING.md.

For an existing unbundled package, supply its exact matching controller explicitly:

1. Run `ble/ctl/build_ctl.sh` from the X0X source (needs the JieLi toolchain and Docker; see
   BUILDING.md). It downloads JieLi's files from JieLi's own site, checks them and makes `ctl.img`.
2. Install X0X with `tools/fm1_install.py PACKAGE.fwsc --ctl ctl.img`, or select the
   matching `ctl.img` in the legacy web installer before you press Install. The installer
   refuses a current package with an empty controller block when no matching image is supplied.
3. On the keyboard, set GLO > BLUETOOTH MIDI > BLUETOOTH to ON and switch it off and on again.

STATUS then shows READY. Connect from your app's Bluetooth MIDI settings (on an iPad or a Mac:
"Bluetooth MIDI" or Audio MIDI Setup), where the keyboard appears under its NAME. There is no
pairing code. Bluetooth MIDI works like USB MIDI: the same channels, notes and clock come in,
and what X0X sends over USB (clock and notes, as set in GLO) also goes over Bluetooth. Send clock
from one place only (USB or Bluetooth), or the tempo doubles. SysEx is not sent or taken over
Bluetooth.

Bluetooth MIDI is new and not yet tested on the keyboard itself.

### USB audio

Over the same USB cable, the FM-1 also shows up on a computer as an audio input called
"X0X FM-1": two channels at 44.1 kHz, with no driver to install. It carries what the headphones
hear, after VOLUME, in stereo.

Record it in any audio app, or monitor it live through one. The FM-1 keeps its USB audio in step
with the computer's clock, so live monitoring doesn't drift or drop out. The first moment after an
app opens the input can click while the FM-1 settles.

It is also an audio output (two channels, 44.1 kHz): an iPhone, iPad or computer can play through
it. What the host plays comes out of the headphones and line out, added on top of the DnBOX
engines; the host's own volume sets its level (the MASTER knob does not). The host's audio is not
sent back to the USB input, so recording the input while playing through the output does not loop.

---

## 17. Troubleshooting

**Update mode:** hold OCT- and OCT+ together for five seconds. A countdown appears; let go
before it ends to cancel.

**Calibration:** if a button or knob does the wrong thing, hold OCT- and OCT+ while you switch
the FM-1 on. The screen then asks you to press each button and turn each knob in turn.

**After a crash:** X0X shows a red CRASH screen and restarts by itself a few seconds later. The
next start shows RESTARTED AFTER A CRASH and an address (PC ...) for a moment. If you report the
problem, include that address and what you were doing.

**Safe mode:** if X0X crashes twice in a row while starting, it starts in SAFE MODE instead. There's
no sound, but USB works, so the web installer can reach it: reinstall X0X, or put the stock
firmware back from the same page. Press PLAY to try starting X0X normally again. If even safe
mode fails twice, the FM-1 drops into the chip's own update mode, and only the rescue below can
reach it.

<details><summary>Stuck on the start screen after changing BRIGHTNESS (0.9-beta and earlier)</summary>

Versions up to 0.9-beta had a brightness setting that could freeze the FM-1, and because the
setting was saved, it stayed frozen at every start, even after reinstalling. Install 0.10-beta or
later from the web installer: the setting is gone, the saved value is ignored, and the FM-1 starts
normally.

</details>

### Recovering an FM-1 that won't start

Since 0.5-beta, an FM-1 that keeps crashing starts in safe mode (above), where the web installer
can fix it. Older versions instead dropped into the chip's own update mode after two crashes (0.2-
beta did this: the X0X screen, a red X0X CRASH screen, then a blank screen). Neither the web
installer nor M-VAVE's updater can see an FM-1 in that state, but a script can put M-VAVE's
firmware back, on a Mac, with the USB cable you already have:

Open Terminal, paste this line and press Return:

```
bash -c "$(curl -fsSL https://raw.githubusercontent.com/charlesvestal/fm1-x0x/main/tools/fm1_rescue.sh)"
```

1. If a window asks to install the command line developer tools, click Install. When it has
   finished, paste the line again.
2. If it asks for a password, type your Mac password (nothing shows as you type) and press Return.
3. When it says it's waiting, plug the FM-1 in and switch it on. It crashes and goes blank, and
   the script catches it. It checks the chip, saves a backup of the FM-1's memory in the
   `fm1-rescue` folder in your home folder, and asks before it writes anything.
4. Type **yes** and press Return. Don't unplug anything until it says it's restarting the FM-1.
   The FM-1 then starts into its original firmware, and you can install X0X again from there.

The script downloads M-VAVE's FM-1 V15 firmware and JieLi's flash loader, and checks both against
known fingerprints. It writes only the firmware area of the memory, never the bootloader or the
FM-1's own settings, and checks everything it writes. Running it again is always safe. If it fails,
copy everything in the Terminal window and
[open an issue](https://github.com/charlesvestal/fm1-x0x/issues). If the FM-1 never shows up at
all, Felucca's [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) (a small
RP2040 board wired to the USB lines) can still reach it.

---

## 18. Parameters

Drum settings range from 0 to 127. DIST, the distortion type, can be DIODE, CLIP, SAT, BFZ,
PDIST, FOLD or CRUSH. PAN goes from L64 through C (the middle) to R63.

Each track's Level, Pan, Rev and Dly are on the mixer (909 MIX, 808 MIX); its Drive and Dist are
in SEL's list; the rest are its sound pages. Sound-page knobs follow a fixed order across tracks —
Tune, Tone, Attack, Sound, Decay, then Snappy / Tail / pitch envelope — so the same kind of
control stays on the same physical knob when you change drum or kit. Tables below list each
track's settings (including mixer and SEL fields), not the knobs' left-to-right order.

<details><summary>909 tracks</summary>

| 909 track | Settings |
|---|---|
| BD | Tune, Attack, Decay, Level, Pitch depth, Pitch, Drive, Dist, Rev, Dly, Pan |
| SD | Tune, Tone, Snappy, Decay, Level, Drive, Dist, Rev, Dly, Pan |
| LT, MT, HT | Tune, Decay, Level, Attack, Drive, Dist, Rev, Dly, Pan |
| RS | Level, Tune, Drive, Dist, Rev, Dly, Pan |
| CP | Level, Tune, Tail, Drive, Dist, Rev, Dly, Pan |
| CH, OH | Decay, Level, Tune, Drive, Dist, Rev, Dly, Pan |
| CR, RD | Tune, Level, Decay, Drive, Dist, Rev, Dly, Pan |
| KIT | Accent (how much louder accented steps are) |

</details>

<details><summary>808 tracks</summary>

| 808 track | Settings |
|---|---|
| BD | Level, Tone, Decay, Tune, Attack, Drive, Dist, Rev, Dly, Pan |
| SD | Level, Tone, Snappy, Tune, Decay, Drive, Dist, Rev, Dly, Pan |
| LT, MT, HT | Level, Tune, Decay, Sound, Drive, Dist, Rev, Dly, Pan |
| RS | Level, Tune, Decay, Sound, Drive, Dist, Rev, Dly, Pan |
| CP | Level, Tune, Decay, Attack, Sound, Drive, Dist, Rev, Dly, Pan |
| CB, CH | Level, Tune, Decay, Drive, Dist, Rev, Dly, Pan |
| CY, OH | Level, Decay, Tune, Drive, Dist, Rev, Dly, Pan |
| KIT | Accent |

</details>

<details><summary>303</summary>

| 303 setting | Range |
|---|---|
| Cutoff, Reso, EnvMod | 0–127 |
| Decay | 200–2000 ms |
| Accent | 0–127 |
| Wave | Saw or square |
| Tune | A from 400 to 480 Hz; the middle is 440 Hz |
| Volume | 0–127 |
| Drive, Drive type | Off, soft or RAT |
| Slide | 2–360 ms |
| Accent decay | 30–3000 ms |

</details>

<details><summary>Break</summary>

| Break setting | Range |
|---|---|
| Complexity, Anchor, Roll, Fill | 0–100 |
| Retrig 2x, 3x, 4x, 8x | 0–100 |
| Phrase | Off, 2, 4, 8 or 16 bars |
| B chance | 0–100 |
| A length, B length | 1/4, 1/2, 1, 2, 4 or 8 bars |
| Level | 0–127 |
| Pitch | -12 to +12 semitones |

</details>

<details><summary>Effects and master</summary>

| Effect page | Settings |
|---|---|
| REVERB | Decay, Tone; in the list: low cut, return level |
| DELAY | Time (1/32, 1/16T, 1/16, 1/8T, 1/16., 1/8, 1/4T, 1/8., 1/4, 1/2T, 1/4., 1/2, 1/2.), Feedback, Tone, Tape (DIGI, then tape and its wear); in the list: low cut, return level, ping-pong |
| MASTER | Drive, Comp, Pump, Filter; in the list: the compressor's details, pump by, resonance, limiter, the drive's type, glue and volume |

The delay follows the tempo, including an external MIDI clock. A delay longer than two seconds
(1/2. below 90 BPM, 1/2 below 60) plays at half that length, which still lands on the beat. With
PING on, the echoes bounce from left to right, one delay time apart; delays longer than one
second play in the middle instead.

</details>

---

## 19. Credits

X0X is free software under the GNU General Public License, version 3. If you share the
firmware, you must also share its source code.

- The platform (hardware layer, USB, update loader, installer and storage) is from
  **Felucca** by Leo Kuroshita, Hügelton Instruments.
- The 909 is from **9W9** by athousanddetails, which is based on **ER-99** by Matthew
  Cieplak. Its hat and cymbal samples are ER-99's.
- The 808 is from **8W8** by athousanddetails.
- The 303 is from **schwung-303**, built on Open303 by Robin Schmidt (MIT licence), with
  extensions based on jc303 and dm-Rat.
- TB-3PO is from **schwung-tb3po**, which comes from the Phazerville Hemisphere Suite.
- The break generator is from **BB Gen** by mestela, used with permission.
- The fonts are Barlow Semi Condensed and Terminus (SIL Open Font License).

M-VAVE and FM-1 are trademarks of their owners, and TR-808, TR-909 and TB-303 are Roland
trademarks. X0X is not connected with M-VAVE, Roland or Hügelton Instruments.
