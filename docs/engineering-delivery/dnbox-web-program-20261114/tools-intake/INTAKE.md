# DnBOX browser preparation, editor and preset intake — 2026-11-14

This is an actual source and delivery intake for the website product owner. It does not take over the active browser preparation/librarian writer `6af85da6-73fc-8191-af07-9d25609320f1`, editor-controls writer `6af84a83-48d8-8191-aeef-62c85b3be0db`, firmware owners or sole publisher. No component source, Git ref, external comment, device or publication operation was performed.

The relevant Working/Pending/Complete sections were consumed from the supplied queue snapshot at `e7b22e30a519c1b127155066a27fc5dc38539aa1`. Each local queue copy has one extra terminal LF compared with its Git blob; removing that final LF reproduces the exact pinned blob. No queue copy was changed. Scored ROADMAP history is excluded from this packet and reviewer inputs.

**Exact delivered source**

| Component | Live verified source | Observed delivery state |
| --- | --- | --- |
| W3 explicit-target correction, [#915](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/915) | `0472ca4ca75cf477be171554071f972f60dba88c`, parent `7bece9b819790600bb755e2900817b249d875567` | Open draft, unmerged. Production change corrects actual-engine target eligibility and warning text only. |
| W3 local-file librarian, [#901](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901) | `7bece9b819790600bb755e2900817b249d875567` | Open draft, unmerged. Includes existing file-side code and an unimplemented broader UX proposal. |
| Native F1 host codec/librarian, [#900](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900) | `d963c3925a5b05f329e24c1a793a6326118fa466` | Open draft, unmerged. Host-file path; device transport/storage/UI are separate. |
| Offline sound audit, [#913](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/913) | `f65de4148cdbbadefa896f2b59f6c156f7e5ed74`; audited application `94197fc839b00aff47b98ca02f3c6396e4a6eabf` | Open draft, two evidence files only. This PR does not change or repair the editor. |
| Active editor-controls continuation | Intake pointers: archive `5664a4cce007bc4ca8f777dcacd7a775ca5de7805997f9fb22d1ec5701e5b9f8`; patch `13196607186759a026b673ad3860d219a6f6600906937f812b19479302477a71`; virtual tree `da2b1f159dadf24005caaf75755765d116595331` | Pointer-level only in this intake. No corrected runnable delivery or exact-byte receipt was supplied to this reader. |

All 15 W3/F1 source, contract and evidence snapshots saved in `source-915/` and `source-900/` match their immutable Git blob IDs, checked by locally recomputed Git SHA-1 and SHA-256. The receipt names each consumed file and its source URL. The separate [editor intake](editor/REPORT.md) records 21 exact byte-verified files, 80 matrix references across 28 tree paths, additional firmware contract consumption and source limitations. Its existing 33-file manifest is `f4eeb8cfd9e00a7bb75e82523c84dfcf255fd688528300f6aaf7ac9f98fee51f`; no test suite was rerun.

**Available entry points**

Normal, unmodified TLS HTTP reads at 13:29–13:31 UTC returned the following results. The web-reading tool could not access these Pages URLs; the ordinary trusted Python HTTP path succeeded where recorded. No certificate setting changed.

| Musician entry | Canonical Pages | NIGHTLY Pages | Meaning |
| --- | --- | --- | --- |
| Projects | HTTP 200, `/projects/` | HTTP 200, `/projects/` | Existing project tool is reachable. This read establishes serving, not hardware transfer acceptance. |
| Samples | HTTP 200, `/breaks/` | HTTP 200, `/breaks/` | Existing sample tool is reachable. Separate importer ownership and qualification remain intact. |
| Installer | Not tested in this intake | HTTP 200, `/install/` | Existing NIGHTLY installer is reachable. No connection, backup or install was attempted. |
| Preset librarian | HTTP 404, `/presets/` | HTTP 404, `/presets/` | Source is delivered; a public tool at these tested endpoints is unavailable. |
| Offline editor | HTTP 404, `/offline-sequencer/` | HTTP 404, `/offline-sequencer/` | Editor implementation exists; these tested public endpoints do not serve it. |

The working site links already use [NIGHTLY projects](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/projects/), [NIGHTLY samples](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/breaks/) and source/status #915 for presets. Retain those truthful distinctions until an authorized tested component deployment is consumed.

The safe offline-editor destination is its [exact source directory](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer). Use a source-labelled CTA. The adapter accepts only a narrow 36,452-byte raw revision/stream-13 format-1 subset. It does not open portable `.x0xproject` bundles, sample archives or streams 14/15. It rejects sound-draft export and Juno/Drone identities rather than implying compatible conversion. Keep this distinction beside a separate Projects backup/restore path. [Ready-to-use wording](INTEGRATION-COPY.md) preserves the musical proposition without a false launch CTA.

The byte-verified W3 snapshot is locally runnable without building native firmware:

```sh
python3 -m http.server 8765 --bind 127.0.0.1 --directory docs/dnbox-web-program-20261114/tools-intake/source-915/web
```

Open `http://127.0.0.1:8765/presets/`. This is a local source entry, not a publication receipt. The server was not started and unchanged producer tests were not rerun in this intake.

**Coverage by musical job**

| Musician job | Existing usable source or evidence | Remaining integration action |
| --- | --- | --- |
| Write a drum pattern or bass phrase away from the keyboard — #90–92 | Frozen editor source has synth piano roll and distinct 909/808/BREAK editing. Historical Complete scope is off-device preparation, not current firmware/device parity. | Consume the existing editor owner's corrected candidate, then provide a clearly labelled offline entry. Preserve its version and lack of audio/device execution. |
| Bring a MIDI phrase into that pattern — #93–94, #101 | Import preview and explicit replacement exist. Prior scoped qualification is reusable for unchanged code. The lower-rate default-window finding R1 remains recorded; manual valid windows were qualified previously. | Existing preparation owner corrects the affected default at 1/8 and 1/4 and requalifies only that changed path. The site should lead from preview to resulting notes, then saving the local draft. |
| Keep and reshape a good phrase — #95–96, #98–100, #102 | Local draft Save/Load, Undo/Redo, New/Mutate and part/group operations exist in the frozen editor. #98 has prior atomic rejection and exact Undo/Redo evidence. | Preserve prior proof. Resolve the active-modal rejection visibility G1 and verify any changed controls with an actual save/reopen/Undo result. Complete mobile acceptance is not established by desktop touch emulation. |
| Edit a sound honestly — #103 | The source audit separates 303 exposed catalog descriptors and 26 missing Juno/Drone controls from actual firmware patch support. Metadata matches are not DSP fidelity. | Consume the active owner's versioned descriptor/Unset correction. Clearly distinguish stored patch values, runtime pitch bend/damper and legacy draft keys; preserve fail-closed project export. |
| Inspect and back up a user-made engine sound — W3/26.8b | #915 opens raw DNPR v1 files or flat DNPR ZIPs, CRC/schema/engine/provenance checks, explicit preview, atomic Keep and byte-preserving user-file download. All seven engine IDs are recognized. | Existing librarian owner closes the verified warning-locality, action-focus and mixed eligible-only export UX gaps, with internal visual review and changed-path browser evidence. Then root consumes the exact candidate and integrates its entry. |
| Reuse the sound on the instrument — 26.8a/W3 device half | Current browser preset path has no device operation. F1 source has an actual-engine-checked host codec/librarian; this is not the keyboard transfer/storage/UI. | Wait for the existing native/transport owner's versioned model/firmware handshake, stopped/quiet policy, inventory/capacity, sample CRCs, confirmation, ACK/NAK and atomic commit/undo contract. Keep device transfer unavailable until the corresponding A1–A8 system acceptance is real. |
| Move a whole song and its loops | Reachable project and sample applications are distinct from engine presets. DNPR contains parameter bytes and optional sample references, never embedded sample audio or a project/song chain. | Keep a direct path from project-only versus portable project to separate sample backup. Integrate the existing importer owner's verified capability contract without changing its implementation or implying a preset file contains the loops. |
| Exchange user sounds with real plugins — 26.8b | Exact Chicago/Miami gap document was consumed; source has no working plugin converter. Plugin metadata IDs in DNPR do not identify or parse an installed plugin. | Existing owner needs version-matched AU capability log, owner-created saved preset fixtures and exact plugin version, then map/writer/round-trip and acoustic evidence. AudioKit J6 identity/schema/fidelity remain separate unverified work. User-preset work is not blocked by factory redistribution; factory-derived file export remains off. |

**W3 contracts the integrated journey must preserve**

The temporary browser session holds at most 32 records. ZIP input is bounded to 64 flat DNPR members, 2 MiB total; a DNPR record is 100–548 bytes. The stricter application session cap applies before Keep. Refresh closes this session; it is not local draft or device persistence.

The record's actual `engine_id` defines target eligibility; recognized `original_engine` remains provenance. #915 corrects the former extra provenance-target restriction in both Chicago/Miami directions. It does not convert parameters or erase provenance. A filename model suffix is a conflict hint, never authenticated Elite/Pro identity.

The implementation preflights the batch, preserves existing session entries on rejection, requires explicit Keep, and downloads original eligible user bytes. Factory/derived records remain inspectable and blocked on download. Sample references remain unassigned/unchecked warnings until a genuine inventory verifies them.

Source inspection confirms the three older UX findings still exist in #915: `renderPending()` places warnings in the separate `#preview-warnings` sibling after metadata; Keep calls `renderSession()` without restoring focus and Discard only hides the preview; the ZIP builder maps every session entry through `ensureExportable`, so one factory entry blocks the whole ZIP. The existing independent reproduction/acceptance receipts are retained. No duplicate reproduction suite was launched here.

The full Library text for `w3-901-workflow-mockup-rev2.html`, ID `libfile_fde3_f-7b9c1fbeb6788191a64cb15c120f5cc1_2021479872048191acbfdca1f870a539`, version `0`, was read. It explicitly labels itself unimplemented and proposes adjacent visible warnings, eligible-only export and deliberate focus. No visual render or acceptance is claimed from reading HTML. [Independent proposal semantics receipt 6870119278](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901#issuecomment-6870119278) also explicitly leaves visual/browser acceptance open. Maintained repository policy replaces the proposal's stale owner-aesthetic wait with mockup → independent internal review → implementation → actual validation; this is not permission to bypass a real denial.

**Accepted proof and actual limits**

- #915's committed receipt reports the original red run, final Node 24/24 (including 17 DOM workflows), focused actual Chromium 6/6 and frozen-source SCOPED ACCEPT. That scoped acceptance covers target/provenance eligibility only. It reused older 548-assertion/native replay evidence.
- #901's prior warning-omission fix reports Node 18/18, 15 DOM scenarios and Chromium 19/19, with four negative-control failures. Independent source review accepted that omission correction. It did not accept warning locality, post-action focus or mixed export UX.
- #900's exact-head producer receipt reports 180 focused assertions across factory-OFF, factory-ON and ASan/UBSan configurations. [Independent acceptance 6869385190](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900#issuecomment-6869385190) accepted actual-engine/provenance semantics without rerunning those tests. Factory-ON testing is not authority to enable factory export.
- This intake ran source-identity and HTTP-serving checks only. It did not run Node/Chromium/native suites, audio, real iPhone/Safari/VoiceOver, firmware builds, physical MIDI, device storage or transport acceptance.

**Concrete integration sequence**

The next authorized action is receipt intake from the two existing active component owners, without creating another writer. Root can immediately use the verified local W3 source and editor contract to compose the file/preparation journey while preserving the accepted website candidate. A corrected component must arrive with its exact immutable source or manifest, actual changed-path test/review receipts and an explicit persistence/transport contract. Root then serves that exact component locally, validates the joined entry → useful edit/import → explicit save/download → reopen/result path and passes the changed runnable experience to its existing independent reviewer. Public links change only after the sole publisher can lawfully deliver and verify the exact bytes.

The separate public handoff comment on #901 was denied and not posted. It was not retried. The editor continuation's inherited public-export denial remains binding; no replacement export route was attempted. PR #130's two publication denials and sole-publisher authority are preserved. No new transfer, export, merge or publication is claimed by this packet.

Requested routing for disjoint helpers was `gpt-6.1-sol` with `ultra` reasoning. The spawn request records those requested settings; effective inference routing is not independently attested.
