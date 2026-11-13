# Disabled capture: shared mutation gate and reset invalidation

LX-REQ-039 bounded runtime-source dependency after PR673. This stage adds actual
hooks to existing common flash mutation and reset paths, not a new write command.
`fm1_capture_supported()` returns 0 and `fm1_capture_begin()` always returns
UNSUPPORTED. There is no production acquisition call or device operation. MBOOT
installation through ordinary NIGHTLY remains OPEN.

## Exact ownership and scope

Root confirmed no overlapping writer for this isolated HAL lane on 2026-10-28.
The live coordination snapshot 12f3b3cd and older private active-packages ledger
retain historical UI/Settings/Miami/SMC claims; their branches and files remain
untouched. Accepted private LX-REQ-039 ownership continues. Base is merged PR673
`738af0108dd8d3034ea61f91c2a11dc19aec355c`. Only the new HAL gate, existing
`fm1_flash.h`/`fm1_sys.h`, focused tests and this document change. No website,
UI/Settings, controller, loader policy or official 16 KiB head guard is edited.

## Implemented behavior

The pure gate tracks an opaque local owner handle, held/revoked state and a
saturating mutation epoch. Acquisition requires explicit qualified/quiescent
assertions, a nonzero owner, no active lease and no epoch exhaustion. These are
caller assertions, not proof of physical identity or global exclusion. The runtime
entry point refuses acquisition unconditionally; only host tests inject a lease.

- Actual `fl_erase4k` and `fl_write` wrappers check the common gate **before** core
  parking, IRQ changes or their RAM erase/program call. An attempted mutation while
  held revokes the observation and returns BUSY without invoking the mutator.
  Revocation does not unlock the lease; only the same owner may release it after
  its synchronous raw read has returned. There is no auto-retry or queued write.
- Each admitted nonempty mutator call advances the epoch conservatively, even if
  its later low-level operation fails. Zero-length programming does not revoke a
  lease or count as mutation. At epoch exhaustion, ordinary writes remain allowed,
  but capture admission is disabled; the epoch never wraps into stale validity.
- `fm1_reboot`, `fm1_enter_uboot`, `fm1_core_reset` and `fm1_enter_update` revoke
  before their mailbox/reset side effects. They do **not** start returning early:
  existing callers rely on reset not returning. A handoff terminates the session,
  not a successful capture or transferable lease. Independent boot-session identity
  still must be qualified before any future observation can survive reconnect.
- Hooks execute on the normal XIP side. Nothing is inserted into `.ram_text`,
  inside XIP-off loops, in an ISR or on the other core. The gate is single-owner,
  non-reentrant bookkeeping, not a multicore atomic lock or synchronization proof.

## Audited closure and explicit remaining bypasses

Current application raw mutation calls are the storage/OTA wrappers in `x0x.c` and
sample/boot-screen operations in `editor.c`, all reaching the common wrappers.
The separate ordinary app loader in `loader.c` calls RAM erase/program functions
directly. It has no capture lease or acquisition route; application handoff revokes
before the reset that starts it. This is a deliberate execution-phase boundary,
not a reason to broaden boot writes or bypass loader range checks.

This does **not** establish the complete maintenance barrier. Before enabling any
capture, audit/admit settings/autosave, sample/project transfers, staged operations,
startup picture/text, OTA and pending loader requests *before* they alter logical
application state; the low-level BUSY backstop alone cannot preserve those callers'
UI/transaction semantics. Confirm controller/SDK/core1 writes cannot bypass it,
and serialize the actual read/release/admission boundary. No current `stage_lock`,
source grep or parked-core timeout is accepted as that proof. Physical UID/session,
raw capture transport, private browser backup, watchdog/latency and interruption-safe
single-slot boot commit remain separate qualifications. No writable transaction is
introduced or approved here.

## Proof and reproduction

```
python3 -B tests/host/mboot_mutation_gate/check.py --output /tmp/mboot-gate-proof
```

Nine strict C groups run normally and under ASan/UBSan. A second harness compiles
the **exact changed wrapper bodies**, substituting only the low-level HAL calls
with counters; it verifies ordinary operation, no raw calls while held, owner
release, zero-length behavior and reset invalidation. No MMIO or assembly runs in
host tests. Four actual reset bodies are checked for hook-before-side-effect order;
source scan rejects a production acquisition call outside the gate header.

The genuine current Elite DEV native app+ordinary-loader build passes using the
previously qualified 20260730.1 compiler and pinned SDK. `.ram_text` has no calls;
app 337188 B, `.data+.bss` 98056/98304 B, pool 323000/344064 B. A fresh
same-input Elite DEV build of the exact base has app 337060 B, normal RAM
98056 B and pool 322972 B: candidate delta is +128 B app, **zero** normal RAM,
and +28 B pool. The gate is explicitly in the application's cleared `.pool`;
the separate loader has no pool and uses its local zeroed storage. The first
candidate used 98072 B normal RAM (232 B spare); it was moved before freeze rather
than accepting that erosion merely because it fit. Final normal RAM remains only
248 B spare, so a future 456 B capture context cannot be placed there casually.

The unchanged locked safety reserve is at least 8192 B free pool; candidate has
21064 B free, 12872 B above that minimum (base 21092 B free). `app.ld` preserves
96 KiB normal RAM, separate stacks and 256-byte guard bands; no reserve, linker
budget or locked setting is relaxed. Actual placement/stack/timing for a future
full capture implementation remains unqualified. Shared changes use the maintained
Elite-first scope; no Pro-specific behavior was added. No Pro artifact/hardware PASS
is inferred. Controller source is unchanged and no complete owner package is
published from this unit; generated build files remain private BUILD_ONLY evidence.

Native command (private build output, no installation):

```
JIELI_TOOLCHAIN=/path/to/qualified/toolchain AC79_SDK=/path/to/pinned/sdk \
  X0X_GEN_CACHE=0 X0X_DEV_PADCAL=1 python3 -B tools/build.py --board smk37 --model elite
```

The first attempt encountered an unwritable default home cache; the supported
`X0X_GEN_CACHE=0` setting disables that cache without changing HOME or permissions.
Final source/build checks pass. No claim of complete target capture, power-loss
safety, browser integration or hardware behavior follows from native compilation.
