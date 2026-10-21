# Single-central vendor-host memory and security audit

Status: source-bounded analysis of a resolved, **non-installable** diagnostic link.
This is not a heap high-water measurement, stack proof, hardware test, or fit claim.
No runtime layout or pool guards were changed for this audit.

## Reproduction inputs

Use SDK `64afb30b805af5f5d3c2e6471e970c3fd0ba40ad`, controller archive SHA-256
`885cd53747753488b380f761318bf30d691f75541dd28adc72457690aa51506a`, and host archive
`d4c02f42e1be97a5288ecbcb5d42e6d0727e2ca4ffa2e0e2e6bb2ae61fede4bc`.
Inspect the named archive members below with the pinned pi32v2 tools. For local,
private inspection only, extract each member with `ar x`, then use
`clang -target pi32v2 -S -emit-llvm -x ir MEMBER -o MEMBER.ll`.
Do not commit vendor objects, IR, or generated vendor source.

Configuration: private HCI; LE modules=2, features=1; one hardware link,
five RX buffers, five ACL buffers, ACL length27; one host LE connection, one GATT
client, zero GATT servers; SM enabled, secure-connections extension disabled.
The reference frontend uses ATT MTU128 and a512-byte discovery buffer.

The integration owner's latest `runtime-link-central-roles` diagnostic selects
roles25 (central, observer and initiator; no peripheral or advertiser). It reports
image104536 bytes, whole RAM extent28664 bytes, and no unresolved symbols. Its RAM includes
10112 bytes of heap and separate2048-byte run and ISR stacks. These numbers are
not the admitted layout. The existing low-RAM window is26368 bytes: excess2296.

## Allocations that cannot be omitted from the budget

The current shim charges `align8(payload + 8)` per allocation. Splitting can waste
an additional small tail; fragmentation and simultaneous transient allocations
are not included in the following lower bound.

| Owner / exact member and function | Payload | Heap charge |
| --- | ---: | ---: |
| Host LE connection: `btstack_memory.c.o`, `btstack_le_memory_init` |1240|1248|
| Host GATT client: same function |128|136|
| Device DB: `le_device_db_memory.c.o`, memory initialization |56|64|
| SM: `sm.c.o`, `sm_init` |480|488|
| Two shim task stacks,2048 each |4096|4112|
| RF/baseband: `RF_ble.c.o`, `le_hw_init` |3710|3720|
| Linklayer: `link_layer.c.o`, `ll_init` |140+512|672|
| **Subtotal** | |**10440**|

The RF payload follows the exact allocator formula with these configuration
values: `36 + 28 + 636 + (5*294 +35) + (5*294 +35) =3710`.
`ram_malloc` and its fallback `malloc` use the same shim heap; they are alternatives,
not two allocations. Keep this DMA-related allocation in low RAM.
The DB and SM assertion branches likewise contain alternative calls, not duplicate
allocations. Linklayer allocations persist with the initialized layer. Host fixed
pool handles are static and must not also be counted as heap allocations.

This subtotal already exceeds the10112-byte heap by328 bytes. It is not enough
for operation. Scan initialization adds32 payload/40 charged; scan-open state
adds20/32. Advertising initialization can add40/48 when that path is invoked,
but is not assumed mandatory in a central-only lifecycle. Initiator allocations
include32/40 and60/72. Connection creation includes336/344, and hardware-open
state40/48. Their phase overlap and releases require instrumentation before
summing a peak. Events and ACL bookkeeping add transient allocations. The80-byte
startup semaphore costs88 and is released after controller readiness; do not
blindly add it to the two-task steady-state subtotal.

Excluded inactive alternatives: standard-HCI buffers48+2560 (private HCI selected),
classic-mode3072+512 allocation (LE-only modules), and ATT TWS restore188.
ATT storage828 and search storage512 are already static in the linked RAM total.
Do not add the donor's generic13824-byte heap on top of these allocations or treat
that historical both-role high-water setting as a proven single-central minimum.

## Safe next memory discriminator

The independently identified high region `[0x01c7d400,0x01c7f000)` is7168 bytes,
subject to new ABI, loader lifetime, overlap and zero-initialization validation.
Do not move task stacks or RF DMA pools there. Own CPU-only static candidates are
ATT storage828, search storage512, task records576 (not stacks), software timers320:
2236 bytes total. Moving these alone still leaves60 bytes of the current low-RAM
excess, before increasing the insufficient heap. Host globals such as the184-byte
`btstack_global_info` are further candidates only after individual pointer-use and
section-placement validation. Generic "host task" allocation routing is unsafe:
host calls can enter controller allocation paths. Do not use allocation sizes or
return addresses to guess memory ownership.

A testable next step is explicit data-only sections for audited CPU-owned objects,
then a full resolved link and allocation traces by lifecycle: startup, scan,
connect, discovery, subscription, MIDI burst, peer security request, disconnect,
reconnect, and radio-off while congested. Record both minimum free space and
largest free block. Preserve the8192-byte app pool guard and all sample capacity.
No current evidence establishes enough heap or a safe reduced heap limit.

### Allocation phases and scoped CPU-pool option

| Phase | Ownership / additional demand |
| --- | --- |
| Controller startup | RF and linklayer pools; controller task stack; temporary readiness semaphore88 charged |
| Host startup | Host task stack; `btstack_mem_init` calls `btstack_le_memory_init` for LE connection and GATT pools1384 charged |
| Profile initialization | Public `le_device_db_init` allocates64 charged; `sm_init` allocates488; `gatt_client_init` registers static objects, no direct allocation |
| Scan | Scan state40 and open state32 charged; check lifetime through scan stop |
| Create connection | Initiator40+72 charged; do not assume overlap with all scan allocations |
| Connected / discovery / security | Connection344 and hardware state48 charged, plus variable event/ACL allocations; ATT/search already static |

A narrowly scoped CPU-pool context around DB/SM initialization could relocate552
charged bytes without altering vendor structures. DB initialization directly
allocates and clears its CPU table. SM also calls persistence helpers, event and
fixed-channel registration, and key setters. Those nested helpers and the actual
shim persistence implementation must be checked before admitting this scope.
The current diagnostic `ctl_syscfg.c` read/write return -1 synchronously, with no
allocation or yield. Thus there is no persistent bond/key store in this candidate;
secure reconnect must not be claimed. Recheck the scope when real persistence is
introduced. The scope must not survive a yield, nested scheduler entry, callback, or error;
free must recognize the correct owning pool. GATT initialization alone does not
move the128-byte GATT pool: that pool is allocated earlier in vendor memory init.
Routing all host allocations by task name is not an acceptable substitute.

Relocating these552 bytes would reduce the10440 lower bound in the low heap to
9888, leaving224 of the current10112 before all later phase allocations. That
still does not establish runtime fit. A public-function wrapper around the earlier
`btstack_mem_init` could be investigated separately, but requires exact-link
interposition proof and complete nested-call ownership, not a private-layout
replacement or vendor binary modification.

## Stack evidence and limitations

The exact vendor task functions allocate message arrays: host32 words (128 bytes),
controller14 words (56 bytes). Startup readiness can synchronously wait through
the bounded bootstrap scheduler. HCI command forwarding has a16-word local array;
encryption processing has three16-byte local buffers. These are source-level
locals, not complete target frames or a call-chain bound. Callback nesting,
compiler spills, context frames and interrupt behavior remain unbounded by this
audit. Keep both2048-byte task stacks and the run/ISR stack guards. Require target
stack painting/high-water evidence before claiming stack adequacy; native ucontext
tests validate scheduling behavior, not target stack size.

## Retained security capability, not pairing success

The exact config sets `config_btctler_le_features=1`. The linked symbol table retains
`ll_hci_encryption`, `ll_hci_start_encryption`, encryption-change/refresh events,
`sm_pdu_handler`, `sm_run`, and device-DB encryption get/set.
In `hci_ll.c.o`, encryption queues command0x2017; start-encryption queues0x2019.
`ll_hci_cmd_handler` gates encryption on feature bit1, calls AES and sends command
completion; start-encryption checks role bits and feature bit1 before passing
control data to the linklayer. This is stronger evidence than the presence of an
SM API declaration alone.

In `sm.c.o`, the central idle engine state52 accepts peer Security Request PDU0x0b.
It enters state56 when the key lookup result requires pairing, or uses the stored
key path; deferred lookup records the pending request. Disabling automatic master
pairing does not remove this peer-request path. The frontend follows the reference
SM setup, but does not supply automatic JustWorks confirmation. A pending user
confirmation is therefore not proof of completed pairing. No claim is made that
every SMC requires security or that this solves the prior transport failure.
