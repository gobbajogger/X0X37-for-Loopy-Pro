# Independent proposal review, #52 / #104

Final scoped result: **ACCEPT FOR INCUMBENT BOUNDARY REVIEW**. All three review
findings below are resolved in the exact final files. No lost-update or
foreign-write defect was identified in this review of the default semantic
contract. This is not live protocol-owner,
firmware, transport, host application, native build or device acceptance.

## Exact final reviewed files

| File | SHA-256 |
|---|---|
| PROTOCOL.md | 27205e21bdccb48ed0125eb3264c2a722693939ddbef741705bf8ac15c331dbd |
| model.py | 18851a2b3a6d3579b3eb7c90a231cee47be97d15af26429bfe91ed1bb517f973 |
| test_model.py | 3a036b06a056c72b6a83569b057d7ec3e5f9f20e0952ffcb2d26f48b99051d6e |
| explore.py | 90c5a4f144f780aa2aa46436bed697b656a25e60f14b67787c100b9f0ab8f72d |
| negative_controls.py | 6b144f2e40fffa971476d549814d060fccada468a9327545656abd17a0c07ffe |
| run_checks.py | 52f1f3f9a0ca6a7c0806663931a156e543af70ca9864d7105339523fbe89abc4 |

Also inspected the independent adversarial review and scheduler to confirm that
the claimed separate live-byte/authorization ledger exists. Existing input-blob
verification and protocol source analysis remain valid unchanged.

## Resolved findings

1. **Lease retry wording differs from model behavior.** PROTOCOL says retries
   do not renew a lease. `keepalive()` has no rpc_id/nonce argument and renews on
   every accepted semantic call, including two identical argument tuples. A
   read-only probe at model times10 and11 produced deadlines25 then26. The hard
   deadline remains bounded, so this is not an indefinite lease or data-loss
   finding. **Resolved:** final text explicitly treats each accepted KEEPALIVE
   as a renewal event, including repeated delivery at this abstraction. Its
   nonrenewal claim is limited to the actually idempotent
   OPEN/STAGE/WRITE/VALIDATE/RESUME cases, and rpc deduplication remains unmodeled.

2. **Configurable revision bound can exceed the stated wire bound.** Defaults
   are safe. However `Server(..., max_revision=COUNTER_MAX+1)` is admitted, and
   `_publish` used only that configured bound in the initially reviewed code.
   This is an internal simulation configuration issue, not malformed wire
   acceptance. **Resolved:** final constructor requires a strict integer
   max_revision in0..COUNTER_MAX. The attempted boundary probe raced the fix
   and correctly raised ValueError; no executed old-overflow result is claimed.
   The added focused regression covers negative,2^56,bool and float bounds.

3. **Minor legacy malformed-envelope clarification.** The phrase "every legacy
   PROJECT packet, including malformed packets, with legacy BUSY rc2" should
   define behavior when no OP byte exists. An empty PROJECT body has no opcode
   to echo. **Resolved:** final text specifies `[OP,2]` for legacy envelopes
   containing an OP, and drops PROJECT bodies with no OP without state/timer
   effects. This prevents inventing an incompatible ambiguous busy frame.

Initial review hashes retained for provenance: PROTOCOL
`8a3512aea2e1cf03db824e812d27d9ef3ac214b45411ba6bb58e59989d220e5b`, model
`cf095490b0e3f61c4ca08e712af97fd3151d6342138c6157d0b7a73372023fbc`, focused tests
`0385fec4002ed661ccb6943494b59f544f4b7c210e2ecf8ea42bad9ab1f1e6f6`.

## Checked boundaries

- The u56/u32/u16 raw little-endian definitions,16-byte identities and P78
  lengths are internally consistent. WRITE raw BODY is
  `21+1+43+4+2+224=295` bytes; packed length is338, framed length345. The732-byte
  existing output buffer is large enough for the proposed reply sizes, but is
  not evidence that every transport/input queue accepts the requests.
- The ASCII magic bytes44 58 53 32 and version02 total five raw bytes. All
  variable byte chunks have explicit count fields; exact lengths and canonical
  seven-bit packing are specified. The real wire codec/parser is explicitly
  unimplemented and untested by the model.
- Subop75 ADOPT_BASS is preserved/refused.96-104 are tentative owner allocations,
  not claims of production reservation. Legacy64-74 idle semantics remain;
  v2/legacy conflicting holds are isolated before state/timer effects.
- The legacy CAPS probe caveat correctly acknowledges that even unknown op96
  can refresh an already-active v1 global hold. A bounded probe grants no cleanup
  authority; old-format replies/timeout do not authorize fallback writes.
- Snapshot bytes remain separate from candidate/live data. Final publication
  CAS, owner binding, generation fences, receipt retention and cancellation
  tombstones are concretely specified. CANCEL intentionally spans generations
  for the same owner request; it does not pretend to be a fenced old ABORT.
- Source pins, unresolved incumbent liveness/latest editor/stream16, protected
  pattern/#7 ownership, sample/durability separation,32-request availability
  limit, bearer-token threat model and physical/native acceptance gates are
  explicit. No DeviceSync enablement or hardware safety inference is present.
- Exploration limits are stated truthfully: bounded ordered events and synthetic
  bytes; no unbounded proof, parser, memory ordering, actual project semantics,
  native budget or hardware qualification.

## Runner and negative-control review

The negative controls operate only on disposable copies, verify mutation anchor
counts, and require an actual unittest FAIL rather than a generic process error.
They remove the publication CAS, owner check and cancellation tombstone lookup
one at a time, exercising relevant focused cases. The real model is not edited.

The run wrapper runs all five suites with the selected Python interpreter,
records process exit codes and logs, derives all_passed only from successful
process exits, and verifies executable/proposal hashes are unchanged across the
run. JSON/count parse failures raise errors rather than fabricate a PASS.
Subprocess output concatenates stdout then stderr; it is not a chronological
combined stream, and no such chronology is claimed.

Read the producer's final results.json and its exact-input hashes:18 focused
methods,29 adversarial methods,38,610 primary schedules and3,150 independent
schedules, plus three detected negative controls; every suite process exit0.
Those execution receipts are producer evidence inspected here, not a new
independent full rerun. The final model/test/runner hashes match the receipt.
The receipt's proposal hash is the execution-time
`a6c10ba98f223abfd467bad098be13ab0ddbda83642473f126100f5b8275358c`; the final
proposal's sole later clarification is the no-OP legacy drop described above.
No executable changed for that documentary clarification, so no redundant
runtime suite was launched by this reviewer.

Final scoped reread found no remaining review finding. This isolated contract
is suitable for presentation to the incumbent protocol owner for allocation
and boundary acceptance. That presentation does not authorize integration,
DeviceSync enablement, firmware source edits or device operations. Current
incumbent identity/liveness/latest source and stream16 still require recovery.
