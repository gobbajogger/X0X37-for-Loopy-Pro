# Independent adversarial validation

Scope: isolated executable model and proposed contract for ROADMAP #52/#104. This review does not implement, enable or qualify firmware, DeviceSync, transport, samples, persistence or hardware.

## Initial source intake

Read `inputs/SYNC.md` and `inputs/editor.c`. The former explicitly keeps DeviceSync read/write fail-closed; the latter implements PROJECT v1 with a single global `ed_project` and blind ABORT. Before opcode validation, every PROJECT packet updates `ed_project.at` while a hold exists. SAMPLE commands can affect import state after validation. The model must state its narrower project-only boundary and exclude those external writers until a future firmware implementation accounts for them.

The initial materialization had one extra trailing LF per input. This was reported and corrected by the implementation owner before experiments. Independent recheck now matches Git blobs: SYNC (5,283 bytes) `4b376cd6650ce3dec77c37bb77f9092db18b7754`; editor.c (41,553 bytes) `83877daf4bbb111865ad3abf15d00698031870d5`; ROADMAP (109,171 bytes) `923cf5a243ab4d6abe59ed493b506c3a928ca661`.

## Independent attack plan

1. Two clients use the same revision. Only the first serially accepted commit succeeds; the second preserves its draft and receives a conflict without mutating live bytes.
2. Foreign ABORT, foreign WRITE, malformed traffic and QUERY must neither change the owner's stage nor extend its deadline.
3. Expire A, admit B, then deliver A's late chunk/COMMIT/ABORT/RECONNECT. B's state remains unchanged.
4. Lose BEGIN's reply, retransmit its request ID, and resolve exactly the same transaction rather than minting a second owner or issuing blind ABORT.
5. Lose COMMIT's reply, retry COMMIT and reconnect. Resolve the existing terminal receipt without a second revision increment or write.
6. Race cancellation and commit in both serial orders. Cancellation first prevents commit; commit first returns committed status. A client must never label committed music as rolled back.
7. Replay stale snapshot/revision across intervening local edits, legacy writes, reset/load, project change and reboot. Epoch and full revision identity must prevent ABA; digest alone cannot establish freshness.
8. Disconnect/reconnect with mismatched owner, transaction, epoch and intent digest. No ownership transfer is inferred from a port name or generic query.
9. Legacy mutators and keepalive cannot operate on, abort, overwrite or indefinitely renew a v2 hold. Conversely v2 never releases a v1 hold.
10. Incomplete, changed duplicate, oversized and out-of-order chunks do not commit or corrupt either accepted bytes or another transaction. Rejected mutation never changes live music.
11. Exercise validation failure, stale revision at final commit and unknown terminal receipt. Preserve unsaved drafts; do not auto-replay a write after an uncertain outcome.
12. Exercise revision overflow, epoch uniqueness assumptions, clock discontinuity, token secrecy/guessing assumptions and receipt eviction. Identify assumptions instead of presenting them as proved behavior.

Firmware integration must separately account for ED_MAINTENANCE_HELD pausing editor housekeeping/timeouts, sample write and recovery paths sharing scratch, local editing and REC, reset/load, OTA and boot/install paths. A monotonic clock and revision barrier in an isolated model do not demonstrate that those real paths obey it.

## Results

Independent tests are in `adversarial_test.py`; the separate scheduler is in `adversarial_interleavings.py`. Initial ordinary scenarios and 3,150 complete A/B/environment interleavings ran against model SHA-256 `e6238bbbee0758098c2515d09cb77c5eb86e9976c89f496598fcdda727ba08af`. The scheduler passed 191,932 independent live-ledger/authorization assertions. Each trace preserves client-script order and checks accepted bytes/revision after every event; it covers two four-event clients plus concurrent local edit/timeout, with foreign mutation attempts at each active checkpoint. This bounded exploration is not exhaustive over arbitrary state or trace length.

The first 22-method run had one test-only failure: the test treated READ as keepalive. The implementation intentionally has explicit KEEPALIVE; the test was corrected to call it. This was not an implementation defect.

The expanded hostile input tests exposed two implementation defects in that initial model: Python `False`/`0.0` revision values compared equal to revision 0 and could publish a commit; noncanonical Token serials (`True`/`1.0`) aliased serial 1, and an unhashable serial caused an exception before rejection. These findings were reported with reproductions. The implementation owner added strict revision/token shape and counter-bound checks. The 27-method red run is retained as `adversarial-first-run.log`: 19 failing subcases and one exception. Several red subcases only distinguish FORMAT from CONFLICT/UNKNOWN; the mutation and crash cases above are the material defects. The initial test-only READ/KEEPALIVE assumption failure was a separate earlier run.

Independent retest of final model SHA-256 `18851a2b3a6d3579b3eb7c90a231cee47be97d15af26429bfe91ed1bb517f973`: **29 unittest methods PASS**, process exit 0. **3,150 complete interleavings / 191,932 assertions PASS**, separate process exit 0, was executed on the previous `cf095490b0e3f61c4ca08e712af97fd3151d6342138c6157d0b7a73372023fbc` model and is reused for this narrow constructor-guard change: all of the scheduler's constructor arguments remain valid and its transition behavior is unchanged. The owner will also execute the final whole-prototype runner. The malformed revision/token regression cases now reject without mutation or exception. Resume quota, terminal quota and revision exhaustion also fail closed. An additional regression verifies that repeated VALIDATE does not renew the lease, after the implementation owner found and corrected that renewal mismatch. The final constructor rejects noncanonical or out-of-range max_revision/TTL/quota configuration, and the final document explicitly states that repeated accepted KEEPALIVE events renew within the hard deadline while rpc-level deduplication is unmodeled. This accepts the model's checked safety behavior within its stated assumptions; it does not constitute incumbent protocol-owner acceptance or firmware integration approval.

The suite also transfers a synthetic 36,452-byte opaque payload, including high bits and tail changes, in bounded chunks with retries. It checks exact copied bytes, not revision-13 semantic validity.

No firmware, host application, native build, transport/parser, browser, emulator or device test was run by this reviewer. No incumbent protocol-owner boundary acceptance has been established.

Reproduction from this directory's parent: `python3 review/adversarial_test.py` and `python3 review/adversarial_interleavings.py`. Both use only the Python standard library and synthetic bytes.

## Exact proposal review

Read the entire PROTOCOL.md against model.py, then reread the narrow final bounds/KEEPALIVE clarifications. Required corrections sent to the owner: the WRITE maximum is 295 raw BODY bytes (21+1+43+4+2+224), 338 P78 bytes and 345 framed bytes; its initial stated 302/346/353 sizes were an arithmetic error. Replace “authenticated terminal” with owner-authorized bearer terminology to match the explicit absence of peer authentication. Independently read back both corrections and final retry wording in final proposal SHA-256 `a6c10ba98f223abfd467bad098be13ab0ddbda83642473f126100f5b8275358c`. No additional semantic inconsistency was found. Scoped independent model/proposal review: **ACCEPTED within the documented prototype assumptions**, with no remaining model defect identified by these checks.

The proposal explicitly separates tentative opcode allocation from ownership acceptance, preserves legacy op75, keeps DeviceSync fail-closed, states broadcast-secret theft outside its guarantees, and lists unqualified revision/clock/memory/identity/transport boundaries. Retaining 32 receipts/tombstones per epoch is conservative safety behavior with an availability cost; it is not a production resource recommendation. A unique epoch, pure trusted validator, serialized events and complete revision coverage remain assumptions. No test proves real multi-core atomicity, firmware writer coverage, shared MIDI secrecy, reboot durability or local browser draft retention.
