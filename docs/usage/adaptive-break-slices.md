# Adaptive BREAK slices

This source candidate divides loop A and loop B independently into 4, 8, 16 or 32 equal regions. It is not a MIDI-routing mode. It retains the 16 sequence steps and the original eight-position generator cadence: larger counts give finer regions, not a faster clock. Sequential selection maps beat position `q` to `floor(q*N/8)`; random selection and walks can reach every slice. Anchor weights follow normalized loop position.

## Controls

- In BREAK, SCALE/SEL exposes **A SLICES** and **B SLICES**. Selecting an A/B loop or count row sets the count-edit target; editing the row also sets it. The screen explicitly shows **EDIT A** or **EDIT B** and the requested count.
- The two outer left white keys **C3 / D3** decrease/increase that target's count: 4, 8, 16, 32. E3 remains unassigned.
- The two outer left black keys **C#3 / D#3** move the local slice page backward/forward. The separate **A KEYS start-end/count** label always describes loop A, even when editing B's count or when the generator is playing B.
- The first eight mapped black keys audition A's displayed slice slots. For A4, slots5–8 are inactive and marked `-`. White-step plus black-slice key pins that A slice to the step. Stored pins are retained when reducing count and resolve modulo the applied A count; increasing count restores their original intent.
- The last three mapped black keys remain reverse, jungle stretch and stutter. All sixteen white step keys, OCT and SAVE+OCT retain their old roles.
- Paging waits while a local slice/pin gesture is held. Count edits wait until affected held A notes/gestures end; they do not remap sounding note owners. A pending A configuration is marked **PENDING**.
- These outer shortcuts operate only in normal BREAK. They do not take over clip-mode musical input, help/modals, diagnostics, project transactions or other parts. A key already held at entry does not trigger the shortcut.

These note names use this project's octave convention; they are not a claim about printed keybed labels. Physical indices0/1/2/3 are C3/C#3/D3/D#3 (MIDI48/49/50/51).

## MIDI

External notes on BREAK's input channel always wrap chromatically around **C0 = MIDI12**: `positive_mod(note - 12, applied_A_count)`. For N32, note0 addresses slice20. USB, Bluetooth, TRS and local notes retain independent ownership, including aliases of the same slice. Disconnect releases only that source. A channel edit retires external ownership without releasing local keys or macros.

The proposed output default is one immediate matched Note On/Off pulse for each accepted top-level sequenced/pinned hit, on BREAK's output channel, note `12 + slice`. Both A and B use their actual accepted slice index. Muted, probability-rejected, disabled and invalid-loop hits do not emit. Incoming/live audition and internal stutter sub-hits do not echo. Queue backpressure drops the entire two-message pulse, never only its Note Off. Accepted render events cross to the CPU0 transport owner only after render completion; a failed remote job discards its unverified descriptors before local fallback. This default still requires musical owner acceptance before rollout; host traces are not hardware validation.

## Pitch bend and jungle stretch

The physical pitch wheel bends BREAK by up to one octave each way. It adds a
runtime offset to the saved Pitch control. A gesture keeps its BREAK owner while
you navigate and returns exactly to the saved pitch at wheel centre. It does not
acquire a BREAK gesture in clip-keyboard mode. Returning to centre uses a latest-value
mailbox, so a full parameter queue cannot leave bend stuck.

Incoming pitch bend on BREAK's input channel uses the same range. A held physical
gesture has priority; otherwise the last external source owns bend. Disconnecting
that source or changing the input channel clears external bend. Input bend does not
echo. Bend is transient and is never written into the project.

Black macro key 10 now holds **JUNGLE**, replacing HALF. It repeats 90 ms grains,
advancing halfway through the source between grains. Each grain keeps the normal
playback pitch, including Pitch and wheel offsets. The existing two voices provide
a 2 ms crossfade without a new sample buffer. Reverse and stutter remain on keys 9
and 11. Grain origins wrap within the sounding slice and retain its immutable loop snapshot.
Forward grain tails continue through the source like dry BREAK, so short slices
do not introduce uncrossfaded boundary jumps.
Sequencer, pin and stutter hits restart the grain origin. Grains do not skip clock
triggers or emit additional MIDI notes. These are host-validated source behaviours;
audible owner acceptance and hardware timing remain separate.

## Saved projects and paired deployment

The Chicago/Miami profile remains project revision and stream **13**. This lane
adds no migration: earlier project streams remain rejected by that profile. The
existing count byte preserves independent A/B counts and pin intent through
save, staged load and export validation. Bend and held jungle state are not saved.
The legacy reference profile's existing streams 8/9 and old-count handling are
unchanged. The matching hosted editor remains a separate delivery boundary.

## Stock bank

This lane retains the current 909 GR / 909 FL factory pack. No exact owner-selected
three-loop bank with verified available assets was found in the repository. Item 29
is blocked on that bank and its provenance; no personal sample files were accessed.
