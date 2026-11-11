# Guarded tagged bass storage unit

This source unit composes the reviewed live selector with REC endpoint stream 6 and its unsupported-continuation preservation fix. It is not rollout-ready. Both `X0X_BASS_PRESET_PREVIEW` and the new `X0X_BASS_TAGGED_SOUND` default OFF; tagged sound requires the runtime flag. Native capacity, owner UI approval, full snapshot integration and independent review remain gates.

## Exact representation

Sound revision 11 appends two 12-byte wire records AFTER the existing 2,688 sound bytes. No unused cell is repurposed. Each record contains canonical engine ID LE u32 (native303=1, WBL1=0x57424c31), PRODUCT schema LE u16 (1), full product count u8 (Chicago32/Miami23), known/unknown state u8, preset index LE u16 (0xffff unnamed), EDITED bit u8, reserved-zero u8. This product schema is distinct from native303's smaller DSP-only parameter surface. Codecs write bytes explicitly, without ABI padding/endian memcpy.

Unknown is all twelve zero bytes. Unknown schema/engine, unavailable backend, count, reserved/flag corruption, invalid named preset and contradictory named/edited metadata refuse admission. Staged tags choose their own descriptor surfaces; validation never derives staged identity from the current live backend. Miami's single-context availability is retained.

`sizeof(sound_t)=2712`; stored sound object=2716 including its existing header (<3840). `sizeof(project_t)=36420`; portable file=36452. Delta is +24 per project, +48 for live/stage copies. Stream remains 6 for this endpoint-only checkpoint; producer pair is (6,11). Snapshots will need an explicitly separate stream contract, not a silent reinterpretation of stream6 or historical4/5.

## New and legacy transitions

New project defaults explicitly select their source-defined backends. Named application writes the actual engine/schema/full count/preset and EDITED state. Real project save/local load/import/export restores those identities before applying their parameters; full-project restore occurs before audio starts or under the existing explicit project transaction. Normal live preset requests retain their voice-scoped audio path.

Untagged local sound objects retain the real existing16/24/28/32-byte cell decoders. Provided bass bytes remain unchanged. No historical part number/value guesses, Chicago default injection or bass remapping runs in tagged mode. Untagged identities are UNKNOWN: bass MIDI, keyboard REC and sequencer note admission are quarantined, other parts remain available, and sticky preservation prevents all storage writes. Boot defaults after unsupported input also cannot authorize a rewrite. Revisions before10 remain preserved but require a separately qualified semantic migration; this first adoption API deliberately admits only the exact rev10 full32 surface. Rev10 paired with a shorter sound object is unsupported rather than filled-and-guessed.

Portable preparation accepts ONLY the actual known fixed layouts (3,10) and (6,10), exact old size, valid CRC and the appropriate endpoint-bit contract. It rejects4/5, frozen snapshots, unsupported headers/layouts and CRC damage before altering staging. Successful preparation expands only the in-memory layout by24 bytes, sets UNKNOWN tags and returns -10. All original sound bytes and pattern/arrangement bytes remain equal. The source file and persistent objects are untouched.

`project_adopt_legacy_stage(bankA,bankB)` takes explicit choices for BOTH voices. It validates available engine/count/value surfaces and the entire staged project. No parameter value is converted. Invalid choice or any failed project validation restores the complete pre-adoption state. Successful staged adoption changes only tags and sound revision; ordinary explicit commit remains separate. It never guesses a historical identity.

## Editor and browser contract

Tagged firmware advertises project protocol2 and exact current size36452; default firmware retains protocol1. IMPORT accepts the current size or the exact supported legacy size. Sequential upload/retry behavior is unchanged until legacy preparation. VALIDATE on legacy returns9: explicit identity is required, no sample writes are authorized. Once staging has expanded, further WRITE/retry is refused; ABORT retains the live project/old storage. Operation75 requires `[bankA,bankB,0x58,0x30]`, performs staged adoption only, rebuilds the current header and still requires ordinary confirmed COMMIT69. Invalid/unconfirmed adoption never ends the session or changes live data. No adoption or persistence operation retries blindly.

The LOCAL firmware browser module recognizes current producer(6,11), protocol2's exact size and explicit `adoptLegacyBass(...,{confirmed:true})`; legacy staging reaches the explicit identity-needed response. Existing high-level restore UI does not yet offer the two-engine chooser and must continue refusing that workflow. The separately maintained website still needs its owner's matching parser/client patch and fixture; it was not modified here. Existing standard project browser tests remain applicable; earlier archive fixes from the website must not be overwritten by this older firmware checkout's module.

Identity-changing undo remains conservatively unavailable after preview selection; ordinary flat sound-byte callbacks exclude the new metadata. A full identity-aware undo/snapshot lifecycle is the next unit, not falsely claimed here. Preset-bank provenance is unchanged; native approximations are not KORG engine imports.

## Executed evidence

`sh tests/host/bass_tagged_storage_test.sh` runs the real host engine, project compressor/object path, stage validator, UI project transaction and actual editor parser; only wire/flash boundaries are substituted. It also parses the generated real(6,11) exports in the actual browser module.

Coverage includes named and edited save/reload, staged-owned surfaces differing from live routing, invalid/future metadata, exact known producer fixtures, Carrier48 ambiguous values, atomic failed adoption, full file/stage/editor upload/adopt/commit/export, zero stage storage/sample mutation, unknown quarantine through accepted keyboard/MIDI REC, original16/24/28 objects, unsupported future preservation and explicit reset recovery. The actual historic fixtures are synthetic and contain no user data. Their hashes and producer pins are checked before use. Current counts are recorded in the checkpoint handoff; ASan/UBSan is required for the frozen source.

The source protocol1 tests and endpoint-storage preservation regression are also run with the new feature OFF. Actual REC held-duration30/300ms tests are run with tagged mode ON before qualification. No native build, device proof, publication or full per-note snapshot completion is implied.
