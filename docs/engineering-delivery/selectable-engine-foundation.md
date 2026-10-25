# Experimental selectable-engine foundation

Two existing bass parts now use a tested engine interface, with native303 still
the only firmware backend. There is no alternate DSP, menu, PRESETS remapping,
public binary or release-ready feature. Parent owns combined-source integration,
the separate original wobble prototype, previews, hardware and release approval.

## Inputs and decisions

Fetched nightly source: `08d3164a265123cb4c84e5104642fd93257d63d7`.
No active repair checkout was edited. Reviewed resource evidence was inspected,
not merged: PR202 `5cfe746f328767a8eab45dbb9e2c93231e46498b`, PR203
`c31b3dab00994951e8e9fad5ca84d937939237ae`. Their baseline differs from nightly.

Read task72 source-backed feasibility (SHA256
`4df9c3aff26474a214360561293f5575e39e5170def2de3e5be7e798e8ddb9e8`)
and task84 preset design (SHA256
`d2c5668af6d25cde6a81daac747bcee840de68b1f0238f766bbc643c23ea3e43`).
Task84's later namespace is adopted: engine1=native303/schema1=current16 controls,
engine0=invalid. It supersedes task72's earlier suggested303=0; old projects contain
no engine field. No unused sound cells, lane reserved bytes or targets are borrowed.

## Exact prototype interface

`dsp/synth_engine.h`: C99;44100Hz; caller-owned context; mono float fragments1..256
(caller validates bounds); init/render/note(note,accent,slide,velocity)/off/panic/
set(local parameter,value)/transport/active/idle/quality callbacks. Parameter/lock
keys carry engine uint32 + schema uint16 + parameter uint16. Legacy controls and
motion restore always construct303/schema1 keys and cannot affect alternate params.

The only alternate is a host test fixture under `X0X_SYNTH_HOST_TEST`. Firmware
rejects unknown IDs/schemas/incomplete backends. A prototype must use a different
ID and must not claim to be Miami. Mono is the smallest303 adapter; extend/test
stereo mixing and sends before registering the wobble engine. No fade, click-free,
acoustic equivalence or target-deadline claim is made.

Bindings/contexts must outlive the slot. Initialize a prepared context outside
audio. One request is allowed until audio acknowledges its ticket. Panic clears
voices while preserving settings. The dedicated mailbox is same-core main-loop/
ISR SPSC with compiler publication barriers, not cross-core/thread synchronization.
The audio command boundary drains old queued notes into their old backend, commits
before motion/sequencing and clears part/source MIDI held ownership. Project-held
rendering returns before switching; project init cancels pending state. Transport
observation preserves303 release behavior. There is no public firmware switch API.
A future selectable implementation still needs generation/stale-event, motion
runtime and external MIDI-output integration tests before exposing that API.

## Versioned state boundary

`app/synth_state.h` implements an explicit LE LXSEv1 envelope for an opaque legacy
project payload plus two engine selections and retained settings banks. Each part
has selected engine ID separate from retained bank ID/schema/bytes. A dormant
unknown alternate survives while303 is selected; raw303 pots survive selecting an
alternate. One dormant alternate bank/part is supported; more needs a new version.
Unknown IDs/schemas roundtrip without activation or silent303 reinterpretation.

Migration is explicit: validate the old file/header/CRC/semantic format through
its normal reader first, then migrate with the exact pinned raw ABI size. Reserved
bytes and old locks remain intact; all old locks belong to303. Borrowed views,
no heap;56-byte envelope overhead before up to128 settings bytes/part. Malformed
version/length/CRC fails without changing the caller's output view.

Decision: keep LXSE experimental and separate until the integrator chooses the
accepted combined source and explicit file/flash/editor/undo version/capacity plan.
Existing project_t, X0XPv1, stream formats, editor transfers and undo remain
unchanged. No production file route emits LXSE yet. Task84 preset snapshots/step
overrides remain separate, unimplemented work.

## Observed verification

Evidence and independent review: `evidence/synth-engine/`.

- 526336 bit-exact303 samples and state comparisons: all16 pots ×128 values, two
 parts, variable blocks, note/accent/slide/velocity, release/lite transitions.
 Switching/ACK/cleanup/identity/retained settings/ticket-wrap and firmware registry
 negatives pass. Adapter and independent ASan/UBSan pass.
- Independent whole-app original/candidate WAV matches1267756 bytes, SHA256
 `a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`.
- State codec70509 checks pass, including35,052-byte raw-project fixture, every
 truncation, malformed data, dormant unknown banks and retained303 bytes; sanitizers pass.
- Existing project runtime/SMC/update,303 format2 migration,24446 real-storage
 staging checks and protocol suite pass. macOS runtime needs
 `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE`; the three standalone project runners need a
 compiler wrapper adding `-DX0X_HOST`. Initial unadapted platform compile failures
 were not counted as passes.

Native JieLi clang4.0.1 pi32v2/r3 DEV affected-object compilation passes Elite and
Pro flags. Exact engine object sections (unlinked):

| Section | Baseline | Candidate | Delta bytes |
| --- | ---: | ---: | ---: |
|text|9356|9820|+464|
|rodata|221072|221120|+48|
|bss|19824|19872|+48|
|data|304|304|0|
|pool|213820|213820|0|

These include generated tables and are not full firmware sizes or fit proof.
Slots occupy40 native bytes; BSS alignment gives48 section bytes. Codec is dormant,
so later linked-code/settings/stack costs are additional. Reproduce after generated
headers with `JIELI_TOOLCHAIN=<pin> python3 tests/host/measure_synth_native.py`.
No budgets/reserves/quality were relaxed. PR202's separate reported4 app bytes
remaining and PR203's1440 bytes recovered cannot be added into a qualified fit
claim. Full combined Elite/Pro links, stack/cycles and real alternate remain gates.

## Ready handoff

DELIVERED, independently host-reviewed, experimental; no hardware/GUI/flash,
protected merge, public firmware or Actions dispatch. Parent must choose the single
combined integration head, relay this interface to the DSP owner and create one
bounded native-fit/persistence-route successor. No successor opened: combined
ownership is unresolved. Supported parent-thread messaging returned `thread not
found`; no message was delivered and no uncertain mutation occurred. This packet
is ready for parent retrieval; no unsupported account delegation is claimed.
