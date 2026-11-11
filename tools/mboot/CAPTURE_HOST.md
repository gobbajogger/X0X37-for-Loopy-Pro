# Private backup adapter — no live device transport

`capture_host.py` consumes the approved C capture core through a locally built ABI
shim. It adds an actual host file-retention path and offline verifier, not a second
mock state machine. Platform support remains absent; no device/USB/MIDI/network or
installer code is imported. Normal NIGHTLY MBOOT installation remains OPEN.

## Usable entry points

```
python3 -B tools/mboot/capture_host.py
# reports live_capture=UNSUPPORTED, write_capability=false; no acquisition
python3 -B tools/mboot/capture_host.py --verify-backup /private/owner/backup
# local consistency check only; never grants identity/freshness/write authority
python3 -B tests/host/mboot_capture_host/test_host.py
```

A future independently qualified caller may use `with Core() as core:` and
`retain_capture(transport, core, private_parent, new_name)`. The default
`UnsupportedTransport` refuses before creating files. There is no CLI option that
loads an arbitrary backend/library or enables a live session. `Core` builds only
the adjacent reviewed C source using the already selected host compiler and checks
its ABI layout before use. Review/install provenance of that source remains a
caller prerequisite, as with the existing offline adoption tools.

The injected transport interface is a **host boundary**, not allocated device
opcodes or a completed production transport. It must supply:

| Method | Return / obligation |
|---|---|
| `acquire()` | `(Binding, qualified_facts, now, deadline)`; actual held exclusion and independent qualified identity/session required |
| `chunks()` | Ordered `(Binding, offset, immutable_bytes, now)` tuples; maximum 224 B, exact 1 MiB coverage; no out-of-order/partial resume |
| `finish()` | `(Binding, full_digest_bytes, head_digest_bytes, now)` independently obtained on target |
| `revalidate()` | `(Binding, fresh_challenge_bytes, full_digest_bytes, head_digest_bytes, now)` under the same held lease |
| `current()` | `(Binding, now)` fresh immediate use-boundary observation after file fsync |
| `release(binding)` | Idempotent release of this instance's owner only; None means its own provisional acquisition, never another owner |

Binding uses fixed 32-byte binary unit/session/challenge/selection values, uint64
epoch and Pro 1/Elite 2 model. Times must use one trusted monotonic domain. Python
booleans, mutable payloads, invalid widths and out-of-range integers are refused
before ctypes conversion. Core checks every subsequent binding and state. These
facts are not established merely by setting `supported=True`: only tests provide
a synthetic transport today. A producer that raises during acquisition must clean
its own provisional resources; the adapter additionally attempts idempotent release
on every attempted acquisition, including malformed responses. Failed release is
reported as unresolved, not a clean cancellation or retry permission.

## Private retention and crash boundary

The caller supplies an existing owner-only parent, no symlink components, and a new
restricted child name. POSIX dirfd/no-follow/exclusive creation prevents overwrite
of existing destinations; there is no recursive cleanup. The new directory is 0700,
partial files 0600; successful files become 0400. These are privacy/read-only modes,
not immutable storage against the same OS account/root or a malicious filesystem.
This is a POSIX host implementation; unsupported platform primitives fail rather
than silently weakening permissions. No macOS/browser storage compatibility is
claimed from these Linux tests.

The adapter writes bounded validated chunks, handles short writes, fsyncs and then
rereads/hashes the retained file. Only matching actual stored bytes proceed. It
revalidates after that work and checks the lease again after receipt fsync. Local
publication uses no-overwrite links; `receipt.json` is the last completion marker,
then the directory is fsynced. Neither the receipt nor a successful capture keeps
write authority: the read-only lease is released before return. Every receipt is
`PRIVATE_READONLY_BACKUP_NOT_INSTALLABLE`, with false lease authority and false
adapter-verified physical identity. No raw identity/salt/challenge is serialized.
Per-unit raw bytes and hashes are private and must not be uploaded or logged.

A normal failure removes only files this invocation created and attempts owner
release. An unexpected collision preserves the unowned file/directory. A process or
machine crash may leave private `.partial` files or an incomplete pair; the verifier
requires a complete strict receipt and matching exact-size bytes. It never resumes
capture from a partial file. Release failure after durable retention leaves the
useful backup but raises an unresolved-lease error; it does not report success.
Offline verification cannot establish live identity/currentness, recover a lease or
resume an adoption. Files are not passed into PR640 automatically and no synthetic
backup is labelled installable.

The verifier rejects symlink files, nonregular files (nonblocking open avoids FIFO
hangs), extra hardlinks, permissive modes, duplicate JSON fields, unknown receipt
fields/schema, changed authority claims, truncation and byte/hash mismatches. CLI
output is sanitized: no raw payload, identity or per-unit digest appears. Library
receipts contain private hashes solely for local owner storage.

## Exact proof and remaining source dependencies

The 24 host groups build/use the unchanged approved core and create/delete only
synthetic temporary backups. They cover successful retention/offline CLI, strict
chunk/identity/time/capability refusals, provisional and final release failures,
permissions/symlinks/collisions, no-overwrite ownership, short/damaged writes, fsync
errors, partial residue, FIFO/hardlinks and forged receipts. R2 regressions additionally cover private CaptureError text
at every transport boundary and preservation of a pre-existing active core on
failed start or pre-start destination failure. Prior core sanitizer,
SHA differential and native-object evidence remains applicable; no firmware build
is triggered by this Python-only adapter. There is no real capture or device test.

Remaining source integration is the actual target-wide writer barrier and qualified
raw/session/identity/digest adapter, strict encoded transport, bounded target memory/
time and browser-private backup integration by its existing owner. This adapter
removes the host retention prerequisite; it does not claim those missing target
features. Single-slot boot-commit safety remains a separately unresolved design
and no writable transaction is authorized by these tests.
