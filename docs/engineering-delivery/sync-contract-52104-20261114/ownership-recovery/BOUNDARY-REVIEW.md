# DnBOX #52/#104 ownership and input recovery packet

Request: DNBOX-52-104-CONTRACT-20261114
Parent: 01a1af1b-2207-7017-b4fb-4e7f6c2c90ea
Observed: 2026-11-14 06:36:02 UTC
State: PENDING_OWNER_INPUT. Boundary acceptance has not been requested or attributed.
This stage owns read-only recovery and this local report only.

The delivered prototype and its exact original inputs are verified. The actual current protocol owner, incumbent editor.c input, unpublished stream16 input, absence of an overlapping active writer and incumbent consumption remain unverified. The missing receipt below is the next dependency. No successor or shared implementation owner was appointed.

## Exact delivered candidate and input verification

Repository: gobbajogger/X0X37-for-Loopy-Pro.
Remote commit read: fde38845e7474fffae24ae835907d7ca9197b589.
Proposal directory: experiments/sync-contract-52104-20261114.

| Artifact | Verified identity |
|---|---|
| model.py | SHA-256 18851a2b3a6d3579b3eb7c90a231cee47be97d15af26429bfe91ed1bb517f973; Git blob 276ae6ecf226b78922285156d1880c6b3c748c8b |
| Final PROTOCOL.md | SHA-256 27205e21bdccb48ed0125eb3264c2a722693939ddbef741705bf8ac15c331dbd; Git blob 4827515858255185aeb74bff55624ceee76b0d73 |
| inputs.json | Git blob d5592fd5db17bfe71067b962ab6da6f9adba34c1 |
| docs/ROADMAP.md | 12e72b586c31e7ed412b6f4315bdf408c6f712b0; blob 923cf5a243ab4d6abe59ed493b506c3a928ca661 |
| web/offline-sequencer/SYNC.md | 94197fc839b00aff47b98ca02f3c6396e4a6eabf; blob 4b376cd6650ce3dec77c37bb77f9092db18b7754 |
| firmware/src/app/editor.c | 94197fc839b00aff47b98ca02f3c6396e4a6eabf; blob 83877daf4bbb111865ad3abf15d00698031870d5 |
| docs/FEATURE-REGISTRY.md | 94197fc839b00aff47b98ca02f3c6396e4a6eabf; blob 9fff09e3251a0d2d65c2a0ce93857125ca543a9a |

Recovered exact UTF-8 bytes were independently hashed in this stage. All four input blobs and the model/final-proposal hashes match. See hash-verification.json. This is identity verification, not a test rerun.

Reuse the delivered evidence/results.json: 18 focused methods, 29 hostile methods, 38,610 producer schedules and 3,150 independent schedules, three detected negative controls; all suite exit codes 0. These are inspected predecessor receipts, not executions in this stage. The execution-time proposal hash a6c10ba98f223abfd467bad098be13ab0ddbda83642473f126100f5b8275358c predates the documented no-OP legacy-packet clarification. documentary-final.json and the independent final review reconcile it to final proposal hash 27205e21...331dbd without executable changes.

## What current ownership and source evidence establishes

- Trusted user-authored room handoff Sentinel_fc05aab4b50c81918c545fd6ae270afa, sent 2026-11-13T15:52:34.175879Z, names FMB per-pattern engines owner thread https://chatgpt.com/c/6af72638-bff4-83ea-8cc3-75336d14afba and source 94197fc8. Its opening says bot turns were stopped at that historical checkpoint. Later room status Sentinel_68a6f9b0cf7c819199743f8c95eeffde, 2026-11-13T19:00:49.821046Z, reports the per-pattern work active. Neither proves a current heartbeat, current protocol ownership or current admission.
- Pinned SYNC.md and F6B-HANDOFF.md assign the missing capability to the “firmware/protocol owner” by role. They provide no verified current identity. The F6b author, Git commit account and FMB pattern owner must not be equated with that role without an explicit receipt.
- Parent-reported CloudThreadNotFound for the FMB thread remains an access observation. This stage has no supported cloud-thread read/create/send tool. It did not repeat that lookup, inspect browser metadata or infer abandonment.
- Current published main ref was read as 035026fe70059af35e25c7c121b40f318b732fe3. Its editor.c blob is fc9fc24ccbd8d5dd77f40a5d29d01b38b01ffa8a, SHA-256 4e6a28ed194bd458c7aaba63622e8d6cb2f1b7edcdb74e20155468ff29919ae9, 40,777 bytes. The prototype editor input is 41,553 bytes and differs in sample-import/recovery handling. The diff is retained. Published main is not asserted to be the current incumbent lane or unpublished input.
- Maintained nightly resolves to 66073c68dd80d51a7053432c2c161a8b720a1341; its AGENTS.md and workflow were read. coordination/merge-queue returns GitHub API 404. This does not release historical owners.
- The referenced backup/box-notes-20261114 still resolves to 56337359ed175801dd44f364180060b6d41bf235, the same immutable backup already investigated by the predecessor. Its prior non-truncated tree analysis and ownership findings are reused. No unchanged tree scan was repeated. The fetched commit response has 300 changed-file entries and is not treated as a complete tree inventory.
- Complete paginated branch searches returned no names containing stream or pattern. The box backup search exhausted its cursor after 75 refs and a final empty page. stream16 commit and PR searches returned no results. These bounded results do not search unpushed files or prove work absent.
- Authorized room searches for stream16 and editor.c returned no matches; protocol and per-pattern searches supplied no current owner/input receipt. Personal Context returned “Personal context is unavailable for this conversation.” No missing-source conclusion is generalized beyond these readers.

## Six boundary-review questions

This is an evidence packet for the existing coordinator. Each disposition remains PENDING_INCUMBENT; it is not an acceptance request sent to an unverified owner.

| Question | Established proposal/evidence | Required incumbent disposition and evidence |
|---|---|---|
| 1. Current editor.c/stream16 scope and opcode allocation | Original editor blob is fixed above. Proposed PROJECT 96–104 is tentative. 75 remains historical ADOPT_BASS, reserved/refused. Current published main differs and cannot identify unpublished work. | Name actual protocol owner and pattern/#7 boundaries; supply current branch/full SHA/editor blob and exact stream16 bytes/hash or a verified current owner statement describing its absence; check all 96–104 collisions and retain 75. Do not negotiate migrations or reinterpret representation 13 as stream16. |
| 2. Bearer capability, epoch and identity | Secret bearer capabilities isolate accidental clients only while secrets remain secret. Broadcast MIDI observers can steal/replay them. CAPS/BUSY/errors must not disclose other secrets. Model identity and epoch are synthetic. | Explicitly accept/decline this threat scope; qualify epoch uniqueness/entropy, ticket reuse prevention, hardware instance identity and client secret retention. Identify required protections if hostile observers are in scope. A USB/BLE name is insufficient identity. |
| 3. Revision and atomic publication | Snapshot is immutable; staging is separate. Model COMMIT owner/epoch/generation/revision/digest/validation check, publication, increment and receipt form one sequential event. A→B→A must stale the snapshot. | Inventory every serialized live writer: controls, MIDI, undo/redo, engine/pattern switches, resets/loads/imports and legacy commit. Map each to the same nonwrapping revision, then identify/prove the actual multicore/interrupt-safe snapshot and publication boundary. Sequential model schedules do not prove memory ordering or audio safety. |
| 4. Interlocks and resources | Pinned editor.c shares project-stage allocation with undo; project/sample/OTA paths use shared scratch and sample-recovery holds. OPEN must refuse conflicting legacy/recovery/sample/OTA/undo holds; all competing legacy packets must reject before timer/state effects while v2 owns staging. Existing sample-recovery changes differ between published source pins. | Name the existing scratch/undo/sample/OTA/maintenance owners and their exact pins; establish buffer lifetimes, recovery behavior, lock order and mutation coverage. Supply measured staging/receipt RAM, flash/pool impact and timing on exact Elite/Pro builds. No budgets or resource PASS is inferred from the Python model. Preserve separate #7 durability and sample ownership. |
| 5. Receipt/tombstone policy | Model retains 32 admitted/cancelled IDs per epoch, eight resumes per active transaction, and fails closed at exhaustion. Known terminal IDs remain queryable. CANCEL-before-OPEN tombstones prevent resurrection; committed ABORT/CANCEL returns COMMITTED. Epoch changes leave pre-reboot commit uncertainty unresolved. | Accept the 32-record availability limit and bounded memory cost, or provide separately proved replay/lifetime/identity rules. Never silently evict receipts, reopen cancelled IDs or auto-replay an uncertain write after reboot. |
| 6. Real boundary qualification and DeviceSync | Proposal specifies a wire codec/correlation contract but the semantic model implements neither. The maximum WRITE is 295 raw / 338 packed / 345 framed bytes; existing 732-byte output capacity does not prove all input/transport queues. DeviceSync stays fail-closed with zero commands. | Assign future parser/dispatcher/correlation, exact-length/P78, malformed/high-bit/truncation/oversize, legacy interlock, lost ACK/retry/reconnect/timeout/counter-wrap and every-writer regression checks. Validate actual current project parser/CRC/engine/sample semantics and full opaque-tail preservation against the owner's stream input. Qualify exact native Elite/Pro resources/audio/timing and each authorized transport separately. Keep hardware/device acceptance and DeviceSync enablement under their distinct gates. |

## Precise missing receipt and resume condition

The existing coordinator must obtain one authentic current receipt through a supported owner-thread/history reader or legitimate owner delivery, containing:

1. Actual protocol-owner identity/thread and scope, with timestamp and source of the assignment; continued FMB pattern/#7 ownership; confirmation of any active overlapping writer.
2. Current repository/branch/full commit and editor.c blob, plus the exact unpublished stream16 source bytes and SHA-256/Git blob, or a current incumbent-authored statement that no such unpublished input applies. A commit author's account or an empty search is insufficient.
3. Once those are verified, a scoped incumbent action that consumes fde38845e7474fffae24ae835907d7ca9197b589, names model hash 18851a2b...17f973 and final proposal hash 27205e21...331dbd, and substantively disposes of all six questions. A pointer post or ceremonial ACK alone is not observed consumption.

The next fresh boundary-review handoff requires verified owner, no duplicate writer and observed input consumption. Those gates are currently unmet. Do not create it, transfer source ownership or request incumbent acceptance on the strength of this report. Resume only when this receipt or a materially new actionable source is available; do not repeat the same searches/tests on a timer. #52/#104 remain PARTIAL; no ROADMAP write was made.

## Scope, delivery and retained restrictions

No repository source or coordination record was edited. Local report/evidence files are outside both checkouts. No tests/builds/device operations, firmware/DeviceSync/ROADMAP changes, Git writes, PR creation, merge, publication, browser inspection or successor creation occurred. PR creation and the remote browser-tab metadata process remain explicitly denied from the predecessor; neither was retried or routed around. No new approval is requested to bypass them.

Local checkout is branch work at 035026fe70059af35e25c7c121b40f318b732fe3. This stage has no pushed changes; PR: none. The verified prototype commit is remotely readable, but its current branch name was not established by this stage.

Parent report delivery is this packet and final task result. Receipt/acceptance by the actual protocol incumbent is UNKNOWN. No background continuation is promised. Required human decision: none for this bounded read-only recovery; the next dependency is the coordinator's authentic owner/input receipt.

## Primary immutable references

- [Delivered HANDOFF](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/fde38845e7474fffae24ae835907d7ca9197b589/experiments/sync-contract-52104-20261114/HANDOFF.md)
- [Final PROTOCOL](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/fde38845e7474fffae24ae835907d7ca9197b589/experiments/sync-contract-52104-20261114/PROTOCOL.md)
- [Original inputs](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/fde38845e7474fffae24ae835907d7ca9197b589/experiments/sync-contract-52104-20261114/inputs.json)
- [Recorded results](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/fde38845e7474fffae24ae835907d7ca9197b589/experiments/sync-contract-52104-20261114/evidence/results.json)
- [Independent final review](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/fde38845e7474fffae24ae835907d7ca9197b589/experiments/sync-contract-52104-20261114/review/protocol-final-review.md)
- [Pinned published main editor](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/035026fe70059af35e25c7c121b40f318b732fe3/firmware/src/app/editor.c)

Supporting local files: hash-verification.json, recovery-evidence.json and editor-main-vs-prototype-input.diff.
