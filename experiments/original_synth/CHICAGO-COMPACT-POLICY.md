# Chicago compact cold-selection policy

LX-REQ-032 explicitly selects seven original FX2 algorithms: Bypass 0, Chorus 4,
Phaser 5, Ring 10, Master Compressor 18, Decimator 22 and Peaking EQ 24.

The compact owner also admits the five archived Chicago inserts on the
remaining FxType values: Ensemble 3, Short Delay 12, BPM Delay 13, Hall 15 and
Gate Reverb 16 (`chicago_fx2_inserts.h`). These are the earlier firmware insert
algorithms (chicago_ens/sdly/bpmdly/hall/gaterev.h), not original-binary ports.
They run mono on the L+R sum inside the same 4096 B Chorus arena, so they add
no per-part history RAM. BPM Delay follows the wrapper tempo. Factory admission
is unchanged (55 factories); the inserts are reachable through the FxType edit. This policy
discards the previous selected algorithm and its tail when the pending type is
committed. It preserves the original selected processor's arithmetic, precision,
control range, constructor defaults, ordinary reset behavior and local invocation
boundaries after that fresh selection.

`original_chicago_fx2_compact` owns one scalar union and one stereo Chorus
history. Each firmware part requires its own stable object. There is no heap
allocation, frame-sized temporary, shared history between parts or shortened
Chorus history. The original bank, `original_chicago_fx2_owner` and
`original_chicago_backend_init` retain their explicit full original reference
contracts. A compact owner cannot be passed as a complete original owner.

## Integration

Initialize each compact object once, obtain its
`original_chicago_fx2_compact_owner` descriptor, and pass that descriptor to
`original_chicago_backend_init_compact`. Factories retain their raw original
IDs 0..80. The compact backend admits the 55 factories whose exact converted FX2
type is supported. The other 26 fail before either backend or FX state changes.
The caller owns the visible factory mapping and preserves the original float
values. There is no omitted-type substitution.

The cold wrapper checks type support without constructing an object. At an
actual selection boundary it clears the wrapper's old/active pointers and calls
the cold owner. The owner retires the previous selected object exactly once,
clears the scalar union, runs the requested constructor and binds its own
history. The wrapper then calls begin, reset, Edit1, Edit2, tempo and activity in
the original order and uses the original null-to-new transition. Only the new
kernel runs during that transition. A pending A→B→A sequence still constructs a
fresh A. An identical A write with no intervening pending change keeps its state.

Construction is necessary. Master Compressor's ordinary reset retains its two
highpass output histories; it cannot initialize arbitrary bytes left by another
union member. Ordinary resets and canonical edits within a selected algorithm
retain that original behavior. Factory reset discards the selected state and
the borrowed effect-event stream; backend retirement does the same. A healthy
in-flight block must finish before reset or retirement.

`backend_set` rejects unsupported macro 17 values without mutation or a sticky
fault. `backend_begin` also checks every queued macro 17 value before preparation
or any earlier event executes. A downstream owner-contract failure becomes a
sticky backend fault and does not copy a successful dry result into caller
audio. Explicit reset recovers the backend.

## Effect events and the current scope

The compact owner accepts a separate immutable, sorted effect-event stream.
Offsets refer to the actual selected kernel invocation after the caller's
retiming. Binding starts cursor 0. The selected processor consumes events with
its unchanged local indices; queued type 4 resets before the due sample, other
records are consumed, and direct type 5 resets the selected state. Direct type 2 also resets
Chorus. Empty kernel calls do not drain the queue. Ordinary reset, tempo and
edits retain its cursor. Cold selection carries the stream's unconsumed cursor;
it never replays consumed records. Bypass consumes no effect records. Final
discard releases the borrow. This is an explicit single-selected-stream policy,
not preservation of inactive per-algorithm queues.

The saved R3 output functional report proves the internal decoding
F4,03→canonical4 and F4,02→canonical5. It explicitly leaves the original
host/MIDI meaning unproved. The retained modulation and dynamics functional
reports prove the canonical reset handlers, but do not establish a note-on,
note-off, transport-stop or host-reset mapping. The modulation report also keeps
the signed/unsigned original due-domain distinction at the caller's bounded
nonnegative retiming boundary.

The current firmware profile supplies ordinary notes, controls and explicit
cold factory/type adoption; it does not synthesize an effect event from a note
or Stop command. An empty effect stream is valid for that profile. Original
external host reset/transport timing and arbitrary raw host-offset retiming
remain unqualified. Kernel event fixture passes do not prove those upstream
routes. This policy adds no invented note-to-reset mapping.

## Target resource measurement

The pinned JieLi pi32v2/r3 compiler measures:

| Object | Bytes | Alignment |
| --- | ---: | ---: |
| Selected scalar union | 96 | 4 |
| Stereo Chorus history | 4096 | 2 |
| Complete compact owner | 4244 | 4 |
| Cold owner descriptor | 16 | 4 |
| FX2 wrapper with explicit cold descriptor | 68 | 4 |
| Chicago backend with that wrapper | 3364 | 4 |

Two compact owners need 8488 bytes. The pinned base wrapper and backend measure
48 and 3344 bytes, so each grows by 20 bytes. Two external cold descriptors need
32 bytes. These are separate component measurements: an existing runtime union
whose larger Miami member already covers the new Chicago size absorbs backend
growth, and the caller chooses whether descriptors occupy static RAM or pool.
The integrator must derive the actual application delta, alignment and reserve
from the final linked allocation rather than summing those components twice.

The target assembly shows a 20-byte own stack frame for cold selection, with no
local history or audio buffer. The wrapper process owns 68 bytes; backend
initialization owns 520 bytes and begin owns 328 bytes including saved registers.
These are individual function frames, not a maximum call-chain or timing proof.

## Focused proof

Run `sh tests/host/original_chicago_compact_test.sh <output-directory>`. It runs
normal and ASan/UBSan builds with native and exact software FMA, disables leak
checking for the supported container sanitizer runtime, and compares all four
result logs.

The direct-kernel checks cover all seven algorithms,36 control pairs per
algorithm, zero/irregular/4096-frame calls, exact state/history, same-offset
events, reset/direct events, tempo, rejected inputs and disjoint part storage.
Wrapper checks cover interrupted transitions, A→B→A, null-to-new output,
single retirement, preserved queue cursors and ordinary reset. Backend checks
compare finished Drive/FX2/master stereo output for all 55 supported factories,
all seven live type edges, exact macro floats, queued edit order, unsupported
factories/edits, reset/retire and injected downstream faults. The preserved full
reference backend and wrapper suites also cover their existing contracts.

These checks are implementation validation. They do not restart R1–R3 review,
constitute Sol R4, establish AU equality, establish target audio timing or
authorize hardware or publication.
