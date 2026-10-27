# Adaptive BREAK slices

This source candidate divides loop A and loop B independently into 4, 8, 16 or 32 equal regions. It is not a MIDI-routing mode. It retains the 16 sequence steps and the original eight-position generator cadence: larger counts give finer regions, not a faster clock. Sequential selection maps beat position `q` to `floor(q*N/8)`; random selection and walks can reach every slice. Anchor weights follow normalized loop position.

## Controls

- In BREAK, SCALE/SEL exposes **A SLICES** and **B SLICES**. Selecting an A/B loop or count row sets the count-edit target; editing the row also sets it. The screen explicitly shows **EDIT A** or **EDIT B** and the requested count.
- The two outer left white keys **C3 / D3** decrease/increase that target's count: 4, 8, 16, 32. E3 remains unassigned.
- The two outer left black keys **C#3 / D#3** move the local slice page backward/forward. The separate **A KEYS start-end/count** label always describes loop A, even when editing B's count or when the generator is playing B.
- The first eight mapped black keys audition A's displayed slice slots. For A4, slots5–8 are inactive and marked `-`. White-step plus black-slice key pins that A slice to the step. Stored pins are retained when reducing count and resolve modulo the applied A count; increasing count restores their original intent.
- The last three mapped black keys remain reverse, half-time and stutter. All sixteen white step keys, OCT and SAVE+OCT retain their old roles.
- Paging waits while a local slice/pin gesture is held. Count edits wait until affected held A notes/gestures end; they do not remap sounding note owners. A pending A configuration is marked **PENDING**.
- These outer shortcuts operate only in normal BREAK. They do not take over clip-mode musical input, help/modals, diagnostics, project transactions or other parts. A key already held at entry does not trigger the shortcut.

These note names use this project's octave convention; they are not a claim about printed keybed labels. Physical indices0/1/2/3 are C3/C#3/D3/D#3 (MIDI48/49/50/51).

## MIDI

External notes on BREAK's input channel always wrap chromatically around **C0 = MIDI12**: `positive_mod(note - 12, applied_A_count)`. For N32, note0 addresses slice20. USB, Bluetooth, TRS and local notes retain independent ownership, including aliases of the same slice. Disconnect releases only that source. A channel edit retires external ownership without releasing local keys or macros.

The proposed output default is one immediate matched Note On/Off pulse for each accepted top-level sequenced/pinned hit, on BREAK's output channel, note `12 + slice`. Both A and B use their actual accepted slice index. Muted, probability-rejected, disabled and invalid-loop hits do not emit. Incoming/live audition and internal stutter sub-hits do not echo. Queue backpressure drops the entire two-message pulse, never only its Note Off. Accepted render events cross to the CPU0 transport owner only after render completion; a failed remote job discards its unverified descriptors before local fallback. This default still requires musical owner acceptance before rollout; host traces are not hardware validation.

## Saved projects and paired deployment

Recognized old projects migrate to A8/B8 whether their old MIDI-mode byte was0 or1. Their reserved byte and pin intent remain unchanged. New project streams are8 (ordinary endpoint sounds) and9 (per-note sounds); the flat container and project byte sizes are unchanged. Old firmware rejects these stream IDs instead of silently misinterpreting the count byte. Do not load new projects into old firmware.

The matching hosted project editor must understand streams8/9 before owner rollout. The firmware repository's editor parser and the hosted-site companion are reviewed as one paired source package; a branch push or source merge alone is not an owner-available feature. Existing catalogs, packages, calibration and samples are not changed. Other UI/material/metronome work remains separate. This package is source-only until its independent review, native qualification, owner preview and applicable hardware/publication gates are satisfied.
