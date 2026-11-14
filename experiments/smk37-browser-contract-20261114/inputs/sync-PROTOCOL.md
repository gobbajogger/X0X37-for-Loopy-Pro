# Proposed atomic PROJECT contract for ROADMAP #52 / #104

**Prototype proposal, pending incumbent protocol-owner boundary acceptance.**
No opcode is reserved by this document. No firmware, web runtime, DeviceSync,
project format, stream16, #7 persistence or device behavior changes here. The
Python model executes the semantics below; it does not implement this wire codec.

## Exact inputs and ownership

Repository: `gobbajogger/X0X37-for-Loopy-Pro`.

| Input | Commit | Blob |
|---|---|---|
| docs/ROADMAP.md | 12e72b586c31e7ed412b6f4315bdf408c6f712b0 | 923cf5a243ab4d6abe59ed493b506c3a928ca661 |
| web/offline-sequencer/SYNC.md | 94197fc839b00aff47b98ca02f3c6396e4a6eabf | 4b376cd6650ce3dec77c37bb77f9092db18b7754 |
| firmware/src/app/editor.c | 94197fc839b00aff47b98ca02f3c6396e4a6eabf | 83877daf4bbb111865ad3abf15d00698031870d5 |
| docs/FEATURE-REGISTRY.md | 94197fc839b00aff47b98ca02f3c6396e4a6eabf | 9fff09e3251a0d2d65c2a0ce93857125ca543a9a |

Maintained policy read at `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`.
The documented coordination/merge-queue ref returns404. This is not evidence
that its historical roles are inactive.

The existing pattern owner remains [FMB's per-pattern thread](https://chatgpt.com/c/6af72638-bff4-83ea-8cc3-75336d14afba).
Pinned SYNC/F6b evidence names a firmware/protocol owner by role. Current protocol
owner identity, latest editor.c, live thread heartbeat and unpublished stream16
pin were not recoverable through the available safe readers. This prototype
owns only its new experiment directory. It appoints no competing source writer.
Its results must go through the parent coordinator to that incumbent for
allocation and boundary acceptance before any shared implementation.

## Current failure and compatibility

Current PROJECT command9 owns one global export/import hold. Op70 ABORT ends
that hold without caller identity. Any PROJECT packet refreshes its15-second
timer before validation. A separate read/import cycle has no revision CAS.
Host-side Web Locks cannot protect external clients.

Keep legacy op64-74 encodings and responses when no v2 transaction exists.
Keep op75 reserved for historical ADOPT_BASS and refused as today; pinned host
blob `a33e5f31174565485d2ec03f011891a79f4f73e3` and PR794 establish that boundary.
While v2 owns staging, reject **every** legacy PROJECT packet, including QUERY,
ABORT and malformed packets containing an OP, with legacy `[OP,2]`, before
lease/state effects. Drop PROJECT bodies with no OP without a state/timer effect.
Reject sample/OTA/install/maintenance operations that compete for the resource.
Conversely OPEN must refuse an existing legacy/recovery/sample/OTA/undo hold.
The model abstracts the legacy interlock; it does not emulate v1 parser behavior.

Coexistence does not make transactions initiated by legacy clients owner-safe.
DeviceSync stays fail-closed until real firmware negotiation and qualification
exist. No browser read/compare/import fallback is permitted.

## Tentative exact wire proposal

Use the existing framing `F0 7D 46 4C 09 OP P78(BODY) F7` with proposed OP96-104.
These values require the actual owner's check against current/unpublished work.
All BODY integers are unsigned little-endian raw bytes. `u56` is exactly7 raw
bytes and never wraps; u32/u16 use4/2. Digest is SHA-256,32 raw bytes. IDs and
secrets are16 raw bytes. P78 groups at most7 raw bytes: one7-bit mask of their
high bits followed by their low7-bit bytes. Unused mask bits must be zero.
Reject truncated groups, extra bytes, high wire bits, wrong exact lengths,
noncanonical integers and oversized frames before state or timer effects.

Every request BODY begins `44 58 53 32 02` (ASCII DXS2, version2), followed by
`rpc_id[16]`. Every reply repeats that prefix and rpc_id, then `status:u8` and
the operation-specific result. The reply's outer OP also matches. rpc_id is
per-call correlation, including retries, and is distinct from the transaction
request ID. A receiver accepts only its outstanding opcode/rpc_id and expected
transaction identity. rpc correlation and real parser behavior are specified
here but are not implemented or proved by the semantic model.

Define `A = owner_secret[16], epoch[16], ticket:u56, generation:u32` in that
order. `I = owner_secret[16], epoch[16], ticket:u56` omits generation.
The server never discloses another owner's secret in CAPS/BUSY/error replies.

| OP | Request tail after common prefix | Success reply tail after status |
|---|---|---|
|96 CAPS|empty|features:u32, epoch[16], current_revision:u56, representation:u16, project_length:u32, max_chunk:u16, idle_ms:u32, hard_ms:u32, max_transactions:u16, max_resumes:u16, model:u8, instance_id[16]|
|97 OPEN|owner_secret[16], epoch[16], transaction_request[16]|epoch[16], ticket:u56, generation:u32, base_revision:u56, snapshot_length:u32, snapshot_digest[32]|
|98 READ|A, offset:u32, count:u16|offset:u32, count:u16, exact bytes[count]|
|99 STAGE manifest|selector0:u8, A, expected_revision:u56, length:u32, digest[32]|next_offset:u32|
|99 WRITE|selector1:u8, A, offset:u32, count:u16, bytes[count]|next_offset:u32|
|100 VALIDATE|A|digest[32]|
|101 COMMIT|A, expected_revision:u56, digest[32]|COMMITTED uses result_revision:u56, result_digest[32]|
|102 STATUS|selector0:u8, I|ACTIVE uses phase:u8, next_offset:u32, base_revision:u56|
|102 RESUME|selector1:u8, A, resume_request[16]|new_generation:u32|
|103 ABORT|selector0:u8, A|empty unless already COMMITTED|
|103 CANCEL OPEN|selector1:u8, owner_secret[16], epoch[16], transaction_request[16]|empty unless already COMMITTED|
|104 KEEPALIVE|A|idle_remaining_ms:u32, hard_remaining_ms:u32|

Status numbers: OK0, FORMAT1, BUSY2, CONFLICT3, FOREIGN4, STALE_EPOCH5,
STALE_BINDING6, UNKNOWN7, EXPIRED8, ABORTED9, COMMITTED10, EXHAUSTED11,
STATE12, INTENT_MISMATCH13, UNSUPPORTED14, ACTIVE15. CONFLICT includes only
current_revision:u56. Any owner-authorized terminal COMMITTED reply includes
result_revision and result_digest; other terminal replies have no tail.
Other errors have no tail. Phase: SNAPSHOT0, WRITING1, VALIDATED2.
Malformed bodies too short for correlation are dropped without a reply.

The maximum data chunk is224 raw bytes. The largest proposed WRITE request
is295 raw BODY bytes (21 common +1 selector +43 A +4 offset +2 count +224),
338 packed bytes,345 framed bytes, within732 output capacity. Long PROJECT
requests already use the direct dispatch route at the input pin; any real port
must preserve that route and prove exact length/queue limits for every transport.
The independent analysis mentions undo/OTA scratch and the generic64-byte path.

CAPS bits0-5 respectively require immutable snapshot, owner binding, publication
CAS, reconnect generation, terminal receipt and cancel tombstones (all six must
be set). Remaining bits must be zero for this version. Representation13 refers
only to the supported serialized project schema, never protocol version2 or
stream16. No conversion or adoption is negotiated. Model0 means UNKNOWN,1 Elite,
2 Pro. instance_id is a separately qualified device identity; unknown/allzero
is insufficient for live DeviceSync. The model has only synthetic identity and
does not establish hardware identity, project-schema validation or format reuse.
Persistent commit and sample writes have no capability bit and are unsupported.

An old firmware CAPS96 request returns legacy `[96,1]` or times out. That is
unsupported, not a v2 response. The old handler can refresh an already-active
legacy hold even for an unknown subop: a future host should make at most one
bounded capability probe, never use its response as acquisition authority, never
send cleanup ABORT and never retry probing indefinitely. This backward
compatibility limitation is explicit; no current device is probed here.

## State and publication rules executed by model.py

One active transaction owns model staging. OPEN atomically records complete,
immutable opaque project bytes, epoch and current revision. It returns a new
ticket under the client's unpredictable owner secret. transaction_request is
globally unique within the epoch. A duplicate OPEN returns the same initial
identity without renewal, cannot clear staging and cannot reopen a terminal
request. After RESUME it returns STALE_BINDING, so an old OPEN reply cannot
recover the new generation. Foreign request-ID collisions reject.

READ returns that snapshot at any staged phase; it never reads live newer bytes
or extends a lease. STAGE fixes one length/digest intent and checks snapshot and
current revisions. WRITE accepts only the next sequential bytes or identical
fully-contained retries. Gaps, overlaps extending the prefix and different
retries reject. The snapshot remains separate from candidate staging. VALIDATE
requires complete bytes, matching digest and a semantic validator. The model's
default validator checks only fixture length; tests inject a refusal oracle.
Real revision13 parsing/CRC/engine/sample checks are **not** implemented here.

COMMIT requires the owner, epoch, ticket, current generation, exact original
revision, fixed digest, complete staged bytes and validation. The final revision
comparison, publication, revision increment and terminal receipt are one model
event. A real firmware port must establish this atomic boundary against both
cores, interrupts and every live writer. A local edit between OPEN/VALIDATE and
COMMIT makes COMMIT conflict; even A→B→A changes stale the snapshot. Conflicts
retain snapshot and staged candidate until explicit cancellation/expiry.

Every modeled local edit and legacy commit increments the same nonwrapping
revision. Real integration must cover all serialized edits, controls, MIDI,
undo/redo, pattern/engine switches, reset/load, imports and other mutation paths,
or serialize/fail those writers visibly. A dirty flag or current-content digest
cannot replace this revision source. Snapshot/candidate buffers cannot alias live
project/undo data unless actual firmware lifetime proof supports it.

Only a validated accepted COMMIT changes live bytes through v2. ABORT/timeout
discard staging; they never restore an old snapshot over someone else's edit.
COMMIT does not SAVE, mutate samples or prove power-loss durability. Sample
inventory/content changes must be separately locked or included in a future
revision source; multi-resource sample transactions are outside this proposal.

## Reconnect, cancellation and bounded resources

Only a valid advancing operation or explicit KEEPALIVE/RESUME renews the idle
deadline. Reads, status, malformed/foreign/stale traffic and idempotent
OPEN/STAGE/WRITE/VALIDATE/RESUME retries do not. Each accepted KEEPALIVE event
renews even if its packet is repeated; rpc-level deduplication is not modeled.
Expiry occurs at `now >= min(idle_deadline, hard_deadline)` before each event.
Defaults are15 seconds idle,60 seconds absolute. KEEPALIVE cannot extend the
absolute deadline. Firmware clock wrap, maintenance pauses and scheduler service
must be separately qualified; the Python clock is a mathematical monotone int.

RESUME uses the secret and exact token, plus expected old generation and a
unique resume_request. It atomically increments the generation, invalidating
all old-generation delayed writes/commits/aborts. Retrying the exact resume
request returns the same generation without renewal while still current. A
later rebind makes that old retry stale. Eight resumes are retained per active
transaction; further attempts fail closed. Expired/terminal transactions cannot
resume. These are message fences, not authentication of physical endpoints.

Terminal receipts distinguish COMMITTED, ABORTED and EXPIRED. Lost COMMIT ACKs
are resolved through STATUS or exact COMMIT retry; a different digest/revision
cannot borrow an old success. ABORT/CANCEL after commit returns COMMITTED with
its receipt, never claims rollback. A cancellation that linearizes first makes
a later commit return ABORTED. Local draft/history must stay with the client
until it has resolved uncertainty, including after reconnection.

CANCEL OPEN checks the exact owner_secret/transaction_request/epoch. If
OPEN arrived but its reply was lost, it cancels only that transaction. If OPEN
has not arrived, it records an ABORTED tombstone, so a delayed OPEN cannot
resurrect the cancelled request. Cancelling a different request never aborts
the active hold. This cancellation intentionally spans transport generations
for the same owner transaction. A failed BEGIN grants no global cleanup right.

The prototype retains all terminal metadata/tombstones for an epoch, with a
negotiated cap32 admitted/cancelled transaction IDs. It never silently evicts
them. New IDs fail EXHAUSTED at capacity; known terminal IDs remain queryable.
Active snapshots/staging are freed at termination. Production must accept this
conservative availability limit or independently prove another bounded replay
scheme; deleting tombstones to keep accepting work would invalidate the proof.
This is a model tradeoff, not a resource-qualified production recommendation.
Revision/tickets fail before56-bit wrap. No old identity is reused.

Reboot changes the unpredictable epoch and loses volatile receipts. Old requests
return STALE_EPOCH. Neither that result nor UNKNOWN says whether a previous
commit happened before reboot. Preserve local work and reconcile a fresh
snapshot with explicit conflict handling; never auto-replay an uncertain write.
Epoch uniqueness/entropy, token generation and client-secret storage are outside
the executable model and require qualification.

## What the model can and cannot establish

The model checks accepted writes and live-byte history against an independent
journal oracle, foreign capability refusal, stale revisions, delayed aborts,
expiry, reconnect fences, cancellation and idempotent uncertain outcomes. Its
events are sequential atomic steps. Interleaving enumeration is bounded, not an
unbounded theorem or firmware memory-order proof. Synthetic full-length byte
transfer preserves unknown tails; it does not validate a real project format.

Owner strings represent distinct secret bearer capabilities. The model can
show refusal when a caller lacks the capability. An observer on broadcast MIDI
may see/steal/replay that secret; this protocol supplies no encrypted channel,
authenticated peer identity or hostile-observer protection. It also provides no
fairness or denial-of-service guarantee. Do not claim cryptographic authorization.

NOT_RUN: real wire parser/correlation/dispatcher integration, host application
suite, actual DeviceSync, schema/stream16 compatibility, SDK Elite/Pro builds,
RAM/flash/performance/audio checks, emulator/browser and physical devices.
Required before live integration: recover incumbent ownership/latest pins;
reserve numbers; approve the threat/lease/receipt/memory/revision contract;
implement independently reviewed firmware regressions; qualify exact native
resources and transport/device behavior under separately authorized gates.
