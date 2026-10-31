# Read-only session transport (platform disabled)

This source connects the capture collector to a framed host consumer. It is not
first MBOOT adoption, an installer, or permission to read a device. Ordinary
application/controller installation remains independent and keeps the existing
16 KiB head guard. No flash-write operation is defined here.

## Implemented consumer and production boundary

- `session_wire.c` is an executable bounded C session dispatcher. An explicitly
  injected provider supplies identity, held exclusion, raw reads and monotonic
  time. The tests alone supply such a provider. It streams through the existing
  C SHA-256/lease core, checks ownership and sequence, compares final host/target
  digests, revalidates, and releases or quarantines unresolved ownership.
- `session_host.WireTransport` accepts an injected synchronous packet exchange and
  a separately established expected unit digest, model and package-selection
  digest. It implements the transport consumed by `capture_host.retain_capture`.
  Synthetic tests run the actual C dispatcher through packed frames into the
  actual private backup writer and independently rehash the complete 1 MiB file.
- Production editor command **49 (`ED_CAPTURE`)** uses only the shared
  `mb_wire_disabled` responder. CAPS reports zero qualified facts. OPEN and all
  session operations report unsupported. There is no runtime provider, session
  allocation, raw read, UID probing, boot writer or host device discovery.
- The target object for the injected dispatcher is compiled for pi32v2, but is
  **not linked into production**. Only its disabled responder is linked. Normal
  maintenance admission still applies: an already-held test lease discards old
  frames. No active-session dispatch through held firmware loops is claimed.

## Packet contract

The outer editor frame is `F0 7D 46 4C 31 packed(payload) F7`. The command byte is
hexadecimal `31` (decimal 49). Packing uses the existing canonical groups: one
high-bit mask followed by up to seven low-seven-bit bytes. Noncanonical masks,
truncated groups and high bits in encoded bytes refuse. Frames are bounded; the
largest binary request is 134 bytes, response 383 bytes. Transport fragmentation,
response deadlines and authenticated peer association belong to a future backend.

All integers are unsigned little endian. Requests start with version 1, operation
(one byte), and sequence (four bytes). Responses echo these six bytes and add a
one-byte status. Error responses contain no private payload. Status values are
OK=0, unsupported=1, bad packet=2, busy=3, foreign=4, stale=5, failed=6,
release pending=7. Unknown operations never map to another command.

The binding envelope is unit digest (32), session token (32), original challenge
(32), selection digest (32), mutation epoch (8), model (4; 1=Pro, 2=Elite), and
current monotonic time (8). Unit digest is **not** a raw hardware identifier. No
mapping from JEDEC, product, encryption key or MAC is implemented or accepted as
physical identity. The injecting provider must qualify its identity and unique,
unpredictable per-acquisition token before it can be used outside synthetic tests.

| Op | Request after header | Successful response after status |
|---|---|---|
| CAPS 0 | Empty | Facts u32, size u32, head size u32, max chunk u16 |
| OPEN 1 | Challenge 32, selection 32; sequence 0 | Envelope, facts u32, deadline u64 |
| READ 2 | Session 32, offset u32, length u16 | Envelope, offset u32, exact raw bytes |
| FINISH 3 | Session 32, full digest 32, head digest 32 | Envelope, both verified digests |
| REVALIDATE 4 | Session 32, fresh challenge 32, both digests | Envelope, echoed challenge and digests |
| CURRENT 5 | Session 32 | Envelope |
| CLOSE 6 | Session 32 | Empty |

After OPEN, sequence starts at 1, increases for each successful operation and
must not wrap. Requests are not retried. CLOSE deliberately accepts any sequence
for the correct owner so an ambiguous response cannot prevent release. A foreign
session never invalidates or releases the existing owner. Once released, the
last closed owner is retained as a close-only tombstone: a matching CLOSE gets an
acknowledgment without any callback. A new acquisition replaces this tombstone;
it cannot acknowledge or release a different active owner. An active OPEN never
replaces it. CAPS is informational, not an admission token.

## Lifecycle and interruption

Every read checks fresh provider binding/time before touching raw bytes, checks
ordered coverage and bounds before the callback, and checks again after the
callback before emitting any data. Expiry, clock reversal, mutation epoch change,
unit/session/model/challenge/selection changes and unexpected sequence invalidate
the collector and attempt owner-bound release. An invalid digest or state also
releases. Failure to release keeps the context active and quarantined; it refuses
new acquisition and exposes no data. An owner CLOSE can retry release.

The serialized caller must invoke `mb_wire_tick` while silent, including after a
lost OPEN response. This is an explicit provider integration requirement, **not
an existing firmware scheduler guarantee**. A failed acquisition callback must
clean its own provisional resources. If a successful OPEN reply is lost, the host
cannot know its token and reports unresolved acquisition/release; it does not
invent a clean state, retry OPEN, resume, or authorize a write. Device-side expiry
and fresh connection reconciliation are still required before a real backend.

The backup adapter retains an offline consistency receipt, not a usable lease or
installation authority. File hashes are not attestation. No raw identity or
session token is serialized in that receipt. Exceptions from injected transports
are normalized; response/error text is never included in diagnostics.

## Qualification still required

A real provider needs verified chip/raw-read semantics, stable physical identity,
unique session generation, complete interrupt/core1/storage exclusion and ownership,
monotonic bounded lease timing, silent-client ticking, exact target RAM/stack/time
budget and a bounded authenticated transport/host backend. Enabling the existing
HAL acquire function alone does not supply these capabilities. Production support
remains OFF. This source grants no device action.

First boot replacement additionally needs an independently reviewed interruption-
safe write/reconcile design for the actual boot layout. There is no boot writer,
journal guarantee, ROM fallback assumption or installable synthetic snapshot here.

## Reproduce without devices

```
python3 -B tests/host/mboot_session/check.py --output /tmp/mboot-session-proof
python3 -B tests/host/mboot_capture_core/check.py --output /tmp/mboot-core-proof
python3 -B tests/host/mboot_capture_host/test_host.py
```

The session check runs strict C, ASan/UBSan, Python packet/retention tests and an
extracted exact production editor CAPS/OPEN case. `--target-compiler` accepts only
the previously pinned compiler hash and compiles objects, not firmware support.
Synthetic provider source is test-only. No actual unit input is needed or acquired.
