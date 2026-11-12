# Actual bass engine / named-preset source preview

Review-only. `X0X_BASS_PRESET_PREVIEW` defaults OFF. No native qualification, tagged persistence, owner rollout approval, device test or publication is claimed. The previously reviewed visuals remain a source proposal; runtime acceptance requires the new independent review.

## Real controls and data

The PNGs come from the actual host UI after the actual audio engine applies complete mapped factory cells. Chicago `Acid Lab 1` and Miami `16th Cutz` retain verified existing names/data. Native approximations are not imported KORG engines or sound-equivalence claims. Chicago's 81 and Miami's 30 rows retain their mixed Factory2/heuristic provenance. No offline zero-volume workaround is applied.

On SOUND, knob 1 requests a backend on the SAME selected lane, applying its first named preset. Knob 2 and PRESETS browse one real name at a time in that requested backend's bank. The primary label is the actual committed engine, with a stable A/B suffix only when both slots use the same engine; internal lane/MIDI routing remains unchanged. The redundant engine heading is removed from the current-preset card. While waiting, the current sound stays visible and the lower card names the pending latest choice and reason. Knob 3 cancels it. EDITED appears beside the actual engine when its cell differs from the named preset. Parameter pages rebuild when the actual backend changes. Single-context Miami occupancy/reservations and missing builds are explicitly unavailable.

## Voice-scoped live application

Return 0 means an asynchronous request was accepted, not already applied. There is one bounded latest-wins request per voice; further browsing replaces only that not-yet-applied request. Current actual bank/index getters remain applied state; requested getters report the browsing choice. Pending reasons distinguish next audio boundary, held note/release tail, wheel owner, sound locks, earlier input and publication.

Main publishes packed bank/preset/generation plus finite accepted MIDI/CQ producer watermarks with a bounded sequence protocol. An audio ISR interrupting any multiword publication sees odd/changed sequence and defers without spinning. Audio snapshots token and both bounds together; acknowledgment records only the consumed token, without clearing a potentially newer publication.

The audio boundary runs AFTER production CQ/MIDI drain and BEFORE new sequence triggers. It first waits until finite pre-request input watermarks have passed, including a NoteOn beyond the MIDI drain's 64-packet budget. It then waits only for the TARGET voice's note, legato, damper/release tail and wheel ownership to end. Clock/CC traffic, running transport, other voices and BREAK are not global refusals. A continuously held/legato voice can remain visibly pending until released or canceled; no live-note sound is replaced early. Existing independent CQ/MIDI queues do not provide a total cross-queue chronological FIFO, and no such claim is made here.

Application is audio-owned at the first eligible block boundary. It replaces the complete target cell/backend and native parameters without project_hold, project_release, MIDI consumption/drop, transport stop or any BREAK hold. Explicit target sound-lock/motion/snapshot ownership refuses admission instead of reinterpreting or overwriting that data. If such ownership appears while a request is pending, the visible wait preserves it. Chicago Comp still uses its existing shared master-FX mapping; this is not a new per-voice compressor claim.

The MIDI watermark covers the shared accepted application ring used by native USB/BLE/TRS. Raw upstream USB traffic, BLE messages or UART DMA bytes not yet published there lack an application generation timestamp. Their physical wire arrival order is not claimed.

## Storage and remaining gates

Storage still has no per-voice engine tags. Accepting a preview request sets a sticky guard: project_save refuses before writes, editor EXPORT BEGIN refuses before hold/capture, and byte-only undo is refused. Canceling a request does not remove this conservative guard. Explicit project replacement initializes normal source defaults and cancels pending state. No saved bytes or storage revision/producer pairs change.

Full engine-tagged persistence, legacy unknown-identity admission, full sound-snapshot compatibility and actual native capacity/qualification remain OPEN. Existing target motion/sound locks are an explicit refusal, not full preset/snapshot integration. When applying this source island onto newer REC source, preserve its incompatibility latch and legacy guards before all writes.

## Evidence

- Pure mapper commit4623193: 2,835 WBL0 /3,964 WBL1 checks; independent extra245/257 endpoint/admission checks passed. All111 ROM rows/names match canonical JSON.
- `sh tests/host/bass_presets_live_test.sh "$PWD/docs/proposals/bass-real-presets-20261026"`: 194 WBL0 /1,527 WBL1 checks, also ASan+UBSan. Actual physical fast K2/PRESETS input; bounded latest-wins and cancel; complete applied cells; actual backend metadata; Miami occupancy/reservation; save zero-write; 1,280 EDITED pixels against unclipped real font; target-lock refusal; ISR-at-each-publication-stage deferral; actual audible Miami note/legato sound ownership through pending release.
- The same live test queues 64 clocks before an old NoteOn for USB/BLE/TRS, preserves its whole old sound through overlapping legato and correct note release, and admits the pending preset after its tail despite continuing clocks/CC. It also applies an idle voice during running transport, preserves subsequent notes, and checks byte-identical 16-block sounding BREAK output with/without a bass request: `f47cc48b92f16d68` in both build profiles.
- `python3 tests/host/bass_midi_pending_native_test.py`: compiles the actual native query/producer mark/consumer-pass bodies with volatile counters, empty/pending/wrap/nonmutation checks. This is not a firmware build.
- `sh tests/host/bass_presets_export_test.sh`: actual editor parser refuses export before hold/capture/write while preserving normal baseline export.

Historical review failures are preserved in Git: b682 had accelerated preset skipping and clipped EDITED; 9e32 fixed those with independent UI review. 58fd's conservative global quiet policy then failed independent live traffic (refusal during playback and dropped accepted clock). The voice-scoped asynchronous unit supersedes that policy; earlier API-only 19/29 or quiet 86/1399 checks do not establish this new runtime's acceptance. Fresh independent review remains required.

Primary engine-label follow-up also checks same Chicago slots, mixed committed backends, pending engine choice/cancel, absent/occupied Miami, and header text width. The hypothetical same-Miami label helper is tested without pretending the single-context engine permits both bindings.
