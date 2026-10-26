# Actual bass engine / factory preset source preview

Review-only, feature `X0X_BASS_PRESET_PREVIEW` defaults OFF. No rollout, native qualification, device test, publication, or persistence migration is claimed.

The PNGs are rendered by the real host UI after the real runtime API applies the complete mapped factory cell. Chicago `Acid Lab 1` and Miami `16th Cutz` use the verified existing factory names and AU-schema bytes, converted to the native backend parameters by the independently reviewed pure mapper. Native approximations are not imported KORG engines or sound-equivalence claims. Chicago's 81 and Miami's 30 rows retain their existing mixed Factory2/heuristic provenance; no offline zero-volume workaround is applied.

On the SOUND page, knob 1 changes the actual backend of the current lane (not the selected lane), applying that backend's first named preset. Knob 2 and PRESETS browse one preset at a time in that backend's bank. Lane identity remains separate. One physical Miami context means the other lane cannot select it while occupied. Missing Miami builds refuse it. Parameter pages follow actual backend metadata. `EDITED` appears when the current cell differs from the named preset; parameter-lock state is labeled separately.

Selection refuses active transport, held/queued notes, note tails, held wheel owners and engine changes with existing target motion/snapshot data. The complete cell and live parameters are applied at a held, empty-command-queue boundary. Existing Chicago Comp/shared-bus and insert mapping is respected; explicit preset selection clears the selected lane's sends. Rejected requests do not alter the cell or backend.

Storage still lacks per-voice engine tags. After preview application, project_save refuses before writes and editor EXPORT BEGIN refuses before input hold/capture. Byte-only undo is also refused. Explicit validated project replacement initializes the normal source defaults again. No existing stored file is modified. Full tagged persistence, safe legacy identity admission, full sound snapshot compatibility, native budgets and owner visual approval remain OPEN.

Executed source checks:

- Pure mapper: 2,835 WBL0 / 3,964 WBL1; independent extra 245 / 257 endpoint/admission checks in the sibling `bass-map-review` evidence directory.
- `sh tests/host/bass_presets_live_test.sh "$PWD/docs/proposals/bass-real-presets-20261026"`: 19 WBL0 / 29 WBL1 checks: actual API/backend metadata and stored cell, UI engine control path, real audible note output for both backends, single-context occupancy, unavailable build, held note/wheel refusals, motion refusal, real rendered frames, edit detection, zero-write save refusal.
- Actual editor protocol probe: `sh tests/host/bass_presets_export_test.sh`: refused preview export leaves entire stage sentinel unchanged and no hold/flash writes/erase; normal export still starts.

ASan + UBSan also passed both live-test profiles with the same 19 / 29 checks.

These bounded checks do not replace combined-source regression, independent runtime review or native qualification. Full per-voice persistent engine assignment and complete preset delivery remain open until those gates and the coordinated schema change are complete.

## Independent review corrections

The original b6827c3d preview was BLOCKED: fast physical knob 2 turns advanced preset indices 1,2,3,6; EDITED text at band-local y64 was clipped. Prior API-only 19/29 checks did not cover these defects and do not establish input/layout acceptance.

The follow-up explicitly treats preset choice as discrete in the real turn_ref path and positions EDITED at x148,y12 beside the engine. The extended test sends actual encoder events through ui_input_only: six quick forward/reverse knob 2 detents and four forward/reverse PRESETS detents for both available banks. It also compares all 1,280 pixels of the EDITED region with an unclipped real-font reference. 39 WBL0 / 1,352 WBL1 checks pass, including ASan/UBSan; frames regenerated from source.

Independent review also found queued platform MIDI could start on a newly selected sound. This lifecycle finding remains OPEN at this UI-only correction checkpoint; this checkpoint is not ready for adoption or Library replacement.
