# Independent protocol analysis for #52 / #104

Status: source analysis and a proposed boundary only. No firmware implementation,
device traffic, live DeviceSync, persistence change, test-suite claim or owner
acceptance is included. The prototype's final PROTOCOL document controls its
chosen experimental mapping; the allocation below is a recommendation.

## Consumed inputs

Verified locally with `git hash-object inputs/ROADMAP.md inputs/SYNC.md inputs/editor.c`:

| Input | Commit | Exact Git blob |
|---|---|---|
| docs/ROADMAP.md | 12e72b586c31e7ed412b6f4315bdf408c6f712b0 | 923cf5a243ab4d6abe59ed493b506c3a928ca661 |
| web/offline-sequencer/SYNC.md | 94197fc839b00aff47b98ca02f3c6396e4a6eabf | 4b376cd6650ce3dec77c37bb77f9092db18b7754 |
| firmware/src/app/editor.c | 94197fc839b00aff47b98ca02f3c6396e4a6eabf | 83877daf4bbb111865ad3abf15d00698031870d5 |

The first materialization had one added terminal LF per file. Hashing `bytes[:-1]`
proved the cause; the parent removed that single LF and the subsequent hashes
above matched. No source reconstruction or newline normalization is implicit.
Also read exact GitHub `web/x0x_projects.js` at the combined pin, blob
`a33e5f31174565485d2ec03f011891a79f4f73e3`, and F6B-HANDOFF.md, blob
`e4c71f8d80c29d3ced5008d87354002f8440570d`.

## Ownership evidence and its limits

The pinned ROADMAP names the existing FMB pattern owner:
https://chatgpt.com/c/6af72638-bff4-83ea-8cc3-75336d14afba . Its scope is engine
identity per pattern, compatibility, save/reload and held/playing transitions.
The authenticated user room handoff `Sentinel_fc05aab4b50c81918c545fd6ae270afa`
names that same owner and source94197fc8. The later room status
`Sentinel_68a6f9b0cf7c819199743f8c95eeffde`, 2026-11-13T19:00:49Z, reports that
work active; this is historical activity, not current liveness.

Read-only GitHub recovery independently found
`backup/box-notes-20261114@56337359ed175801dd44f364180060b6d41bf235`, with
`notes/steers/dot-handoff-20261114.txt` blob
`dc0c2c5e02ba0cecb743f743c8b1b75834bb6894` naming the same pattern thread.
That historical backup is supporting evidence, not a current ownership registry.
Its tree response was non-truncated (9,143 entries); no pattern/stream16 receipt
was found in the relevant named paths. Authenticated matching-refs for
`heads/lane/fmb` returned twelve refs but no pattern/stream16 lane. Exact-term
stream16 PR and commit searches returned no results. No absence inference is
made about unpublished work. The current stream16 source SHA, protocol-owner
thread identity, current owner heartbeat and input consumption remain unverified.

SYNC and F6B-HANDOFF name the incumbent by role, "firmware/protocol owner";
they do not assign that role to the editor author. The retained F6b branch
`chat/f6b-web-editor-20261101` is absent from the currently readable matching-ref
result. The documented coordination/merge-queue ref is404 (parent receipt).
There is no current live registry from which to appoint a replacement.
Preserve the existing FMB/pattern/protocol owners and unpushed stream16 work.
No new firmware writer has been appointed or contacted by this subagent.

## Existing wire and failure mechanisms

External frames are `F0 7D 46 4C cmd args... F7`; PROJECT is cmd9. The dispatch
buffer omits F0/F7, so cmd is `f[3]`. Bytes on the wire are seven-bit; byte payloads
use groups of one high-bit mask and up to seven low bytes. The existing exact
decoder rejects empty/truncated/noncanonical masks, overlong payloads and high
bits. READ_MAX is224 raw bytes; the output buffer is732 bytes. PROJECT and
sample staging share call-scoped scratch with OTA. A new command must not fall
through the64-byte generic-copy path or truncate an owner-bearing envelope.

| Existing PROJECT subop | Meaning / current argument shape |
|---|---|
|64|QUERY, no arguments; returns rc, version, size(u21), mode|
|65|EXPORT BEGIN, no arguments|
|66|READ, offset(u21), count(u14), up to224 bytes|
|67|IMPORT BEGIN, size(u21)|
|68|WRITE, offset(u21), canonically packed bytes; sequential/exact retries|
|69|COMMIT, magic58 30; validates then applies, never implicitly saves|
|70|ABORT, no arguments; acts on the global hold|
|71|SAVE, no arguments|
|72|LOAD SAVED, magic58 30|
|73|RESET, magic58 30|
|74|VALIDATE, no arguments|
|75|ADOPT_BASS historical host reservation; deliberately refused by firmware|

Do not reuse75 for capabilities. Its absence from the current firmware switch
does not make it free: the pinned host constant and PR794 document its meaning.

The global ed_project fields have no connection or transaction owner. Any PROJECT
packet refreshes `at` before envelope validation (editor.c247-248), so a foreign
QUERY, malformed packet or failed BEGIN can extend another client's hold. ABORT
calls ed_project_end(0) regardless of sender (326-328). A separate export/import
cycle has no publication CAS, so another client's intervening mutation can be
silently replaced. A COMMIT marker is confirmation, not an identity or revision.

The15-second timeout (1100-1101) attempts abort; sample recovery can retain the
hold when samples differ. A failed import recovery may reuse the existing hold
(261-262). SAVE/LOAD/RESET and OTA already depend on global scratch ownership.
The protocol extension cannot discard these existing recovery semantics safely.

## Recommended additive namespace and semantics

Experimental PROJECT subops96-104 are a collision-avoiding proposal, not a
reserved production allocation:96 CAPS,97 SNAPSHOT/OPEN,98 READ,99 STAGE,
100 VALIDATE,101 COMMIT,102 STATUS/RESUME,103 ABORT,104 KEEPALIVE. Retain
64-75 encodings and outcomes when no new transaction exists. Against old firmware,
the candidate CAPS request receives format1 or timeout: new clients must remain
unsupported, and must never attempt legacy fallback writes or blind ABORT.
Allocate only after the actual protocol owner checks current and unpublished work.

The extension version is separate from the on-disk stream revision. A proposal
must not reinterpret revision13 bytes as stream16 or overwrite #7's format work.
CAPS must report the exact supported project representation and identity plus
complete capability bits; absence of any owner/snapshot/CAS/recovery requirement
keeps DeviceSync disabled. USB/BLE display names do not prove device identity.

Use an opaque owner bearer token, a fresh nonreused server transaction ticket,
a boot epoch and a nonwrapping live-project revision. Every mutation carries
the expected epoch/revision and transaction identity. Replies must correlate
the exact request; unrelated responses must not fulfill it. Do not expose an
active owner's token in capabilities, busy or foreign STATUS responses.

SNAPSHOT atomically captures complete opaque project bytes and revision under
one lease. READ never sees bytes from a later revision. STAGE writes private
candidate bytes; it does not alter live music. Out-of-order writes, conflicting
retries, size/representation mismatch and invalid complete payloads reject
without publishing. COMMIT rechecks owner, ticket, epoch, expected live revision,
candidate completeness and semantic validation in one publication transition.
Only then does it replace live bytes and increment the revision. No implicit
flash SAVE or sample-slot write belongs in this prototype contract.

Revision tracking must include every relevant live mutation: local controls,
MIDI edits, undo/redo, pattern/engine switches where serialized state changes,
load/reset and legacy commit. Comparing a digest or a dirty boolean is not a
substitute for a nonreused revision; A->B->A must still stale an old snapshot.
If writers are frozen during a lease, attempted writers must reject visibly;
the model must not assume those writes succeeded and then discard them.

ABORT affects only the matching owner/ticket. An unsuccessful BEGIN grants no
cleanup authority. Cancellation during uncertain OPEN resolves that exact request
or awaits expiry; cancellation during uncertain COMMIT must query terminal
status. A committed transaction cannot be turned into an abort. Abort/timeout
discard only uncommitted staging and retain complete live bytes.

On reconnect preserve local draft/history, recover the exact old transaction
using its persisted identity, and fence old connection-generation callbacks.
Renew only an authenticated valid owner operation before deadline; foreign,
malformed and stale operations cannot extend leases. Terminal commit receipts
must allow exact retries/status to prove an ACK-lost commit happened once.
Receipt eviction, reboot or epoch change returns explicit unknown/stale; never
blindly replay or auto-merge a local draft. Delayed BEGIN must not reopen an
aborted transaction. Retain tombstones for the entire accepted replay horizon,
or use a nonreusable admitted-session sequence; a silent bounded-cache eviction
is insufficient. Counters must fail closed before wrap, never reuse identities.

While a new lease owns the shared stage, all legacy mutation, ABORT, sample
rewrite and conflicting OTA operations must reject before any state or timer
effect. Legacy QUERY may return its unchanged shape but must not renew the new
lease. Conversely, a new OPEN must refuse legacy export/import/recovery/sample
or maintenance holds. This is coexistence compatibility; it cannot retrofit
owner safety to a transaction initiated by an old client.

## Mandatory owner boundary acceptance before integration

1. Recover the current protocol owner's identity, latest editor.c and stream16
   receipt; confirm the subop allocation and complete representation/identity
   negotiation against unpublished #7 work. Do not overwrite or migrate it.
2. Map every serialized live mutation to exactly one revision update and identify
   the actual atomic publication boundary. Prove interrupts/audio/second core
   cannot observe partially published project or stale engine ownership.
3. Qualify the shared project-stage/undo/OTA/sample scratch lifetimes, bounded
   token/receipt memory, boot-epoch generation, timeout wrap behavior and fail
   closed exhaustion policy. A Python model's extra buffers are not free RAM.
4. Keep sample restore and persistent durability as separately owned work. A
   project byte CAS does not transact sample flash or make a durable save (#7
   persistence ownership and #907 storage qualification remain unchanged).
5. Add actual production-parser and host integration tests, then native Elite/Pro
   resource/performance checks and independently authorized transport/device
   qualification. This source analysis and a model PASS establish none of those.
6. Establish a threat model. A bearer owner token prevents accidental foreign
   commands when tokens remain secret; shared MIDI can expose tokens to another
   listener. There is no authenticated hostile-peer identity or cryptographic
   replay resistance in the existing transport. Do not claim that guarantee.

No live acceptance can be inferred from this review. The parent can complete an
isolated model while these production boundaries remain explicit.
