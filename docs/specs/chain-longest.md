# Chain slot = longest pattern

Owner request t656u. This is a behavior change for chained patterns. The project file does not change.

Historical line numbers below refer to the original lane before its rebase.

Line numbers for the old behavior are from base `bdc4965e2c9b2025c2e2cf9ea5b4d4a6b03008ca`. Line numbers for the new rule are from this branch.

Old behavior:

- `pattern.h:53` 303 length, `pattern.h:63` drum length, `pattern.h:93` swing.
- `sequencer.h:15-17` the 909 wrap was the bar for a cue, a chain, and a song. `sequencer.h:74-82` song. `sequencer.h:148-157` chain pair (`a == b` is no chain), `song_on`, `song_pos`, `song_start`.
- `sequencer.c:66-69` `seq_part_len` clamps 0 to 1 and values above 64 to 64. The break is 16.
- `sequencer.c:82` swing comes from the playing 909 pattern.
- `sequencer.c:377-415` `bar_end`. `sequencer.c:409-414` chain advance, last wraps to first.
- `sequencer.c:467` the 909 wrap called `bar_end`.
- `sequencer.c:513-523` swing on the internal step, including even-to-even on an odd length.
- `sequencer.c:589-590` PLAY from the top calls `bar_end` immediately.
- `sequencer.c:661-677` an external clock arms the same steps.
- `sequencer.c:1103-1109` `seq_chain` stores the pair or rejects a bad endpoint.
- `ui.c:704-706` length edit. `ui.c:1929-1930` undo covers the project from `sound`, which includes pattern lengths. `ui.c:2286` and `ui.c:2304-2305` performance recall of the chain in RAM. `ui.c:3490-3499` chain gesture. `ui.c:5146-5147` "CHN". `ui.c:5662` and `ui.c:5706-5713` playhead. `engine.c:31` `seq` is not in the pool. `engine.c:966-971` `eng_step`. `project.c:32-33` the saved blob starts at `pat`. `x0x.h:107-112` `project_t` has no chain field.

New rule on this branch:

- `sequencer.c:73-103` `seq_chain_window`.
- `sequencer.c:410-448` latch and `drum_boundary`.
- `sequencer.c:485` chain selection, unchanged.
- `sequencer.c:541-548` the 909 bar uses the window.
- `sequencer.c:605` step count.
- `sequencer.c:670-671` PLAY from the top latches the slot.
- `sequencer.h:214-215` `chain_win`, `chain_at`. `sequencer.h:239` declaration.

## Chosen reading

A chain is the inclusive pattern range `chain_a`..`chain_b` (`seq_chain`). While that chain is running, each pattern in the range stays on for one slot. The slot length is the longest 909 length in the range, in steps. A shorter pattern keeps looping. The last loop is cut off at the end of the slot (a partial repeat). The chain moves to the next pattern only when the slot ends, and that pattern starts at step 0.

"Pattern length" means the 909 drum length (`pattern_t.drum[0].len`), clamped the same way as `seq_part_len`: 0 becomes 1, and anything above 64 becomes 64. It is a step count, not a time. The 808 length, the 303 lengths, and the break (always 16 steps) do not set the slot. Those parts keep their own clocks until the chain actually changes pattern.

The slot length is taken when the slot starts and held until the slot ends. A length edit during the slot does not move the end. The next slot reads the lengths again.

## Behavior before this change

Read at base `bdc4965e2c9b2025c2e2cf9ea5b4d4a6b03008ca`. The same chain selection is still in this branch; the clock that calls it is what changed.

- A pattern is `pattern_t` (`pattern.h`). The 909 and 808 each store `len` (1..64 in use). Each 303 stores its own `len`. The break has no length field; playback uses 16. `pattern.h` `dpart_t.len`, `bpart_t.len`.
- `seq_part_len` (`sequencer.c`) returns that length for the part that is playing. A stored 0 plays as 1. A value above 64 plays as 64. The break returns 16.
- The original lane base published `chain_a` and `chain_b` as one 16-bit store. The `wip/one-build` target uses separate endpoint fields; this continuation preserves that target layout. `a == b` means there is no chain. `seq_chain` rejects an endpoint outside 0..31 and leaves the old pair.
- The song is `song_t`: a bar count plus `song_bar_t` entries (`sequencer.h`). Each bar stores one pattern number per part and a mute byte. It is not a chain list.
- The 909 wrap ended the bar. `fire_step` called `bar_end` when the 909 step index wrapped. `bar_end` then applied a cue, or advanced the chain, or advanced the song. On a chain advance, every part moved to the next pattern in the inclusive range, wrapping from the high end back to the low end: `c = (c < a || c >= b) ? a : c + 1`. A cue won over the chain. PLAY (`do_start` with from-top) called `bar_end(s, 1)` at once, so a pending cue or `song_start` applied immediately and the chain did not step.
- Swing is the playing 909 pattern's `swing` byte. It delays odd steps. It does not change how many steps fit in a bar. External MIDI clock (24 ppqn) and the internal tempo both feed the same step counter.
- Chain editing is a HOME gesture in `ui.c` (second white key calls `seq_chain`). The header draws "CHN". HOME underlines the patterns inside the range. The step playhead is `eng_step`, written from the sequencer step callback (`engine.c` `s_step`) and drawn by `lane` in `ui.c` as the step inside that part's own length.
- Save and load store `pattern_t` (including lengths) and `arrange_t` (the song) from `project_t.pat` onward (`project.c` `RAW_OFF`). `seq.chain` is not in `project_t`. Undo is a byte swap of the project from `sound` onward (`ui.c`), so a length edit is undoable and the chain endpoints are not. The performance screen copies the chain in RAM only; it does not write the project.

## New rule

`seq_chain_window(pat, a, b)` (`sequencer.c`) is the pure function. It returns the longest clamped 909 length in the inclusive range. Endpoint order does not matter. A bad endpoint or a null bank returns 1. Nothing is written back to the pattern or the project.

The sequencer latches that value in `seq_t.chain_win` when a slot opens (`chain_slot_open`): on PLAY from the top, and after each chain boundary. `chain_at` counts 909 steps played in the slot. Both bytes sit in padding that `seq_t` already had; they are not saved.

`drum_boundary` (`sequencer.c`) decides the 909 bar:

- No latch (`chain_win == 0`): the 909 wrap is the bar, as before.
- Latch open and `chain_at` is still below the window: a wrap of the short pattern is not a bar. The step index loops. Other parts are not restarted.
- `chain_at` has reached the window: that step is a bar even if the pattern index is in the middle of a loop. `bar_end` runs, then the next slot is latched.

Opening a slot clears `chain_win` when there is no chain (`a == b`), when the playing 909 pattern is outside the range, or while the song is playing back (`song_on`, a song pointer, and not recording). Song playback always ignores a retained recording latch; its next natural 909 wrap clears that latch. Song record still uses the chain slot, so a short loop is not written as an extra song bar.

## Edge cases

| Case | Choice |
| --- | --- |
| Different lengths | The slot is the max clamped 909 length. An 8-step pattern and a 16-step pattern in a range with a 32-step pattern each play 32 steps. The 32-step pattern plays once. |
| 12 vs 16, and 3 vs 16 | The short pattern loops. The last pass is truncated. 12 then steps 0..3. 3 plays five full loops and one extra step 0. Phase does not skip ahead and does not restart at 0 in place of the partial. The next pattern starts at step 0. |
| Length 0 | Counts as 1, same as `seq_part_len`. The chain element is not skipped. It occupies a full slot (repeating step 0 if the slot is longer than 1). |
| No events | The stored length still counts. Empty hits do not remove the pattern from the window or skip its slot. |
| Chain of one (`a == b`) | No latch. The pattern wraps on its own 909 length. A cue still lands on that bar. Unchanged. |
| Live length edit | The open slot keeps the latched window. The next slot uses the edited lengths. Stop then Continue keeps the same latch and the same count. |
| Same pattern twice | The chain is a range, so each index appears once per pass. Two slots can hold the same notes under two indices; each has its own length. When the range wraps, the next visit starts at step 0. It does not resume the partial step from the previous visit. |
| Wrap from last to first | Unchanged selection: after the high end, the next slot is the low end, at step 0, with a newly latched window. |
| Outside the range | The pattern that is playing, if it is not in the range, still ends on its own 909 length. The next bar enters the chain at the low end. The window applies from that slot. |
| Song playback | Unchanged. Each song bar lasts for the 909 length of the pattern that bar selected. The chain window is not used. `song_pos` advances on that bar. |
| REC off while a song records | Playback immediately uses the current pattern's natural 909 wrap. An open recording chain window does not delay the next song bar. |
| Part-only cue | The cue waits for the slot end and changes only its part. An unchanged 909 keeps its current loop phase, including a partial repeat. |
| PLAY from `song_start` | Unchanged. PLAY applies `song_start` immediately inside `bar_end(s, 1)`. It does not wait for a window. |
| PLAY / Continue | PLAY from the top opens a new slot at step 0. Continue keeps the step, the latch, and `chain_at`. |
| Tempo | The window is a step count. A tempo edit changes how long a step is, not how many steps remain. |
| MIDI clock | Internal clock out does not change the count. External clock (24 ppqn) advances the same step counter; the slot still waits for the longest step count. |
| Swing | Swing stays on the pattern step's parity, including a partial last pass. An even-to-even wrap (odd length) still has no extra shuffle. There is no separate step-repeat operator. The repeat is this loop. |
| Cue during a slot | The cue waits for the slot end, then wins over the chain advance, as a cue already won over the chain at the bar. A cue that is already pending at PLAY still applies at the start. |
| Rate | A 1/32 pattern and a 1/16 pattern are compared by step count. The playing pattern's own rate sets the time of each of its steps. |
| 303 / 808 while the 909 loops | They are not restarted on the short 909 wrap. They restart when the chain changes the pattern. |
| UI playhead | The step callback still reports the step inside the pattern. HOME draws that cell. During a partial pass the playhead is on the wrapped step (0, 1, 2…), not on a count of the slot. |

## UI

`ui.c` is not modified. Another lane owns it. No UI change is required for this reading: the chain mark, the "CHN" badge, and the per-step playhead already show the pattern and the step that are playing.

A UI change would be required only if the screen should show progress through the slot (for example "step 13 of 16") instead of the pattern cell. That would mean reading `chain_at` and `chain_win`, or a new playhead, from `ui.c`. This branch does not add that.

## Project format

None. Lengths were already stored on the pattern. The window is derived. `chain_win` and `chain_at` are runtime fields on `seq_t` and are cleared by `seq_init`. Undo of a length edit still restores the pattern bytes; the open slot keeps its latch until the next boundary, then the restored length is included.
