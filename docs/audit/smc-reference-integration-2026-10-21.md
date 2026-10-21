# Reference-based SMC integration: measured architecture audit

**No new runtime candidate is admitted.** None of the measured vendor-host layouts
fits the current controller region. The bounded redesign study below also has
not demonstrated a feature-preserving, fully resolved fit.
The default baseline, approved mappings, UI, feedback, samples, loader and model
guards are preserved; later inactive test-only adapter sources are recorded below. MTU/interval-only tuning was considered and rejected before any
runtime edit: it would retain the unproven host without addressing the observed
pre-decoder receive boundary.

Baseline: `816e0d3987bcb69be972cff2cd4df4469969f196`, tree
`74db8ca92ab208c3a8fd963ad02e5e60a509d3a8`. Fresh measurements and input/artifact
hashes are in [the machine-readable record](smc-reference-fit-2026-10-21.json).
This is an architecture audit, not an SMC fix or hardware PASS.

## Proven reference and protected boundary

[Hortator `45df60a`](https://github.com/DeadActive/hortator/tree/45df60aa41e02196dd3375a2bee7e258f331a417)
reports 422 MIDI messages in 236 packets without errors. That establishes reported
transport reception, not its full physical-control checklist. Its pinned SDK is
`64afb30b805af5f5d3c2e6471e970c3fd0ba40ad`; exact host, controller and RF archive
hashes match the donor manifest. The host was already available locally; only the
missing exact controller/RF archives were fetched and hash-verified.

The faithful receive path is vendor controller/private DMA ownership → vendor
host task → ATT/GATT notification callback → bounded copied BLE-MIDI payload →
application parser → dedicated SMC queue → existing SMC dispatch. No private
DMA pointer may cross the current byte-ring mailbox. The notification must be
copied before the vendor frees it. Host event registration must survive stack
initialization. Only one SMC owns the direct input/feedback route; no APC/Loopy
common-ingress redirection is permitted.

Hortator's GPL glue cannot be pasted into the MPL/proprietary controller program.
The existing MPL ABI-3 glue and separately identified MPL fm1-base port are useful
implementation references; each needs its exact revision and behavior checked.
The proprietary host cannot be linked into the GPL app under the project's
existing licensing boundary. A separate vendor program and message ABI remain
necessary.

## Fresh baseline build

Commands ran in the isolated worktree, using the verified cloud input environment:

```sh
X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite
CTL_NATIVE=1 CTL_NO_CACHE=1 X0X_JOBS=4 sh ble/ctl/build_ctl.sh
```

| Resource | Measured use | Available margin |
|---|---:|---:|
| Application code area | 548480 / 552672 B | 4192 B |
| Application main RAM | 97972 / 98304 B | 332 B |
| Application pool | 335740 / 344064 B | 8324 B, **only 132 B above mandatory 8192 B** |
| BLERAM | 3856 / 4096 B | 240 B |
| Controller flash | 65524 / 65536 B | 12 B |
| Controller RAM | 25824 / 26368 B | 544 B |

The controller is freshly rebuilt, not retained: CRC `e3f31407`; the record has
its SHA-256. These are unchanged-baseline builds, not validation of new firmware.
No install or device check was run.

## Exact-library link studies

Private scratch `reference-smc-measure/` retains scripts, logs, symbols and maps.
No proprietary archive, generated vendor code, IR or measurement binary is
committed or packaged. `measure-host.sh` first probes the host API roots alone;
its standalone size is not added to the controller because cross-library LTO
changes it. The combined study is the relevant comparison.

The combined study copies the baseline shim into private scratch, substitutes
exact donor archives and donor LE/security/resource configuration, and roots
`btstack_init`, device DB, SM policy setters, GATT-client setup/callbacks,
`user_client_init`, and `ble_user_cmd_prepare`. `config_stack_modules=4` selects
LE (verified in the exact host's IR). SDK log stubs and LTO are enabled. Duplicate
raw-HCI callback naming is isolated in the scratch shim only. No operational
callback is replaced with a successful fake result.

For size reporting **only**, scratch linker regions are enlarged and unresolved
imports are ignored. Separate normal error-reporting links enumerate the 12
missing generic-study imports and 11 both-role private-core imports in the JSON
record. Thus a scratch `ld exit 0` is **not a complete
link or executable firmware**. It lacks circular-buffer/allocator support,
notification callbacks, timer adapters and other imports. The single-task shim
and its 10112-byte heap are scaffolding, not a valid host runtime. The 2 KiB host
task, its queue, central ATT/search buffers and live allocation headroom are not
yet provided. Root retention and remaining raw-HCI glue may also retain excess
code: these numbers are measured layouts, not a mathematical minimum.

| Study | Flash extent | Excess over 64 KiB | RAM extent | Excess over 26368 B |
|---|---:|---:|---:|---:|
| Exact donor configuration: two hardware contexts, ten RX/ACL buffers, client + server configured | 106660 B | 41124 B | 27336 B | 968 B |
| One hardware context, five RX/ACL buffers, vendor server disabled | 105956 B | 40420 B | 27336 B | 968 B |

The second study saves only 704 B of flash. Heap allocation changes are not
visible in these static totals. It is **not** the tested donor configuration, and
server removal cannot count as preserving the peripheral feature without a
separate functioning peripheral path.

The donor's later fit document reports a 10824-byte live heap peak and chooses
13824 B with margin. This makes shrinking the original 26624-byte reservation a
credible investigation, not an immediate fix. That measurement excludes its
statically reserved task stacks; our stacks are heap allocations. Borrowing its
heap number without reconciling those owners would underbudget RAM. Neither its
heap peak nor its stack high-water has been verified on this keyboard/port.

## Options and costs

1. **Keep ABI-2 and fit the full host into its current region.** Both measured
   configurations fail. A 40 KiB code reduction plus additional RAM is needed
   before an implementation can be admitted. No evidence supports achieving that
   merely by one-link sizing or deleting unused default task slots.
2. **Restore the historical ABI-3 architecture.** It has useful host lifecycle and
   callback-order fixes, but its old layout used sample slot 3, reduced pool
   headroom and borrowed application RAM. Those changes violate this task's
   preservation constraints. Do not restore the old image, allocator hacks,
   guard reductions or slot layout. Its successful peripheral transport record
   is not SMC-central validation.
3. **New separate host/controller extents, replacing both app hosts.** The
   actual removal-ceiling link below improves on symbol attribution, but even
   direct-core rooting and raw-transport removal exceed the available flash
   before mandatory implementation costs. High RAM has a bounded candidate
   region, so RAM alone is not an absolute blocker. This is a substantial
   loader/storage/package/ABI migration, not a region-limit change.
4. **A narrower standards-based receive/security capability port.** This remains
   an alternative if a specific missing capability is established and reviewed.
   Merely changing MTU/interval or sending another SMP reply does not qualify.
   Complete security needs controller encryption support (currently disabled),
   key/event handling and a fitted host state machine; it is not an app-only
   Pairing Request patch. No such causal repair is claimed here.

## Bounded redesign discriminator

Additional private studies retained genuine vendor HCI/ACL, SM, L2CAP and GATT
entry points while removing the generic vendor app command/profile-search
frontend. These are **root-reachability probes**, not a functioning replacement
for `btstack_init`. Startup order, controller-ready notification and the vendor
task's event/ACL/credit loop must be supplied and tested. Omitting those roots
without an equivalent implementation is not a valid optimization.

| Additional study | Flash extent | Static RAM extent |
|---|---:|---:|
| One-link direct core, central only | 98020 B | 27232 B |
| Direct core with peripheral ATT/advertising roots retained | 103932 B | 27288 B |
| Both roles, private vendor transport replacing obsolete raw transport | 103636 B | 26984 B |
| Central only, private vendor transport | 97724 B | 26928 B |

The direct-core roots cover HCI initialization/event/ACL dispatch, profile loop,
DB/SM/L2CAP initialization and policy, GATT initialization/callbacks, primary
service/characteristic/descriptor discovery, CCCD writes, notification listeners,
write-without-response, scan/connect/cancel/disconnect. Both-role studies add
ATT server initialization/callback/notification/CCC state and advertising setup,
data, scan response and enable. Using function/data-section LTO options did not
help: the one-link generic study grew from 105956 to 106520 B. Classic protocol
functions remain reachable from vendor common dispatch; deleting them blindly
would not be a defensible LE-only port.

A separate app-only **removal ceiling**, never packaged, replaced both host public
APIs with inert wrappers and made their old implementations unreachable while
retaining guarded settings/configuration. Its actual app link leaves **16896 B**
code slack, a **12704 B** reduction relative to baseline. This includes LTO effects
and is more useful than the earlier 11028 B symbol attribution. It is deliberately
nonfunctional Bluetooth and therefore an optimistic upper bound on reclaim, not
an implementation candidate. Pool allocation is unchanged; main RAM saves 16 B.

Whole-sector app reclaim is only 16384 B, leaving 512 B that cannot be counted
without an independently proven shared-erase protocol. The sector-separated
optimistic capacity is:

```
65536 + 16384 + 4096 + 16384 = 102400 B optimistic capacity
103636 - 102400             =   1236 B excess before new implementation
```

This arithmetic does not establish actual segmented placement: alignment,
manifest, loader, mailbox adapters and app-side functional wrappers all consume
space. A measured compression probe on the both-role predecessor round-trips its
RAM load sections using the repository LZ4 codec: RAM text 3524→3141 B, initialized
data 1804→429 B, **1758 B total gross savings**. Even carrying that saving forward
unchanged would leave only **522 B** before an independently licensed decoder,
metadata, missing imports, host task startup and all adapters. It is not a fit
claim or permission to link GPL app helpers into the separate vendor program.

### Safe redesign obligations

The candidate 16 KiB store-spare extent is `[0xAB000,0xAF000)`; the other is
`[0xFE000,0xFF000)`. Current `st_owner_check` erases the former on unknown/foreign
ownership. A new precise erase allowlist and migration tests are mandatory.
Older firmware can erase that piece on rollback; restore matching old app and
controller together. Never take sample slot 3, OTA staging, stock rollback data,
calibration, keys or the 8192-byte pool guard.

A possible **data-only** high-RAM extent is `[0x01C7D400,0x01C7F000)` (7168 B),
between current BLERAM and the protected top-page policy. It still needs linker,
loader lifetime and zero/guard proof. Place known CPU-only buffers there first;
keep task stacks and RF DMA pools low until exact SDK addressability is proven.
Do not widen the core-1 stack guard across app globals or generalize heap
fallback into this region. Park the radio before loader transitions.

New ABI admission must validate every segment's version, address, length and CRC
before starting BLE. Write and read back secondary pieces first, commit manifest
last; reject mixed, missing, corrupt and interrupted sets while leaving app/USB/
updater working with BLE off. New bounded segment commands and narrow flash
allowlists are required; do not broaden the existing 64 KiB write command. Test
first boot, foreign store owner, interrupted update, restore and paired rollback
while proving sample capacity unchanged. This is a contract, not implemented
support.

### Explicit feature-choice option: dedicated central-only profile

This option is **not approved or implemented**. It would retain musical features,
sample capacity, mappings and USB MIDI, but deliberately omit SMK Bluetooth-
peripheral / Loopy-wireless mode in this test profile. Loopy would use USB.
Removing both custom hosts and retaining vendor central only yields a 97724-byte
private-transport root probe: **4676 B** below the sector-rounded 102400-byte
ceiling before adapters, imports and manifest. This is a credible bounded flash
investigation, not guaranteed fit. Preserving the other Bluetooth role instead
requires the both-role budget above; exclusive runtime modes do not themselves
save stored code.

RAM remains conditional. Exact target type-size probes give `bleh_t=768 B` and
`blec_t=444 B`; replacing their union can reclaim at most 768 B before replacement
state, **not the entire 3856-byte BLERAM allocation**, which also owns mailbox,
boot/frame buffers and hardware diagnostics. Central-private static RAM exceeds
the current low window by 560 B. High RAM 7168 minus that overage plus union 768
provides at most 7376 B; existing BLERAM slack adds at most another 240 B. The
later donor's 13824-byte heap plus two 2048-byte task stacks totals 17920 B, or
7808 above the scaffold's 10112-byte heap. Even this optimistic comparison is
192 B short before new queue/bridge costs and region alignment. It also assumes
movable CPU-only allocations; RF DMA pools and task stacks cannot simply move
to high RAM. Donor sizing is not a certified minimum: a genuinely smaller central-
only live heap/stack requirement might fit, but requires measured high-water and
failure-path evidence on the actual port. Do not silently reduce margins.

Owner choice is therefore between retaining both modes and authorizing a bounded
central-only feasibility profile, **not between two ready firmware builds**.

**Bounded conclusion:** no fully resolved, feature-preserving reference-host fit
has been demonstrated. This is not a mathematical impossibility proof: root
retention, explicit adapters and a real segmented linker could change totals.
However, the remaining optimistic 522 B after compression is not established
budget for those missing components. Preserve the baseline; any further vendor
port should be an explicit architecture choice with a fully resolved link and
preservation tests as admission gates, not an installable firmware claim.
Alternatively, identify a concrete receive failure in the existing host against
the now owner-confirmed historical physical-control session. That session is
new evidence that a full vendor port is not proven causally necessary; its exact
installed app/controller pair remains unresolved.

## Verification required after an admitted implementation

Use the owner's preserved 1271-notification / 2711-MIDI Mac capture for payload
replay; it is **not** the donor's raw capture, which has not been recovered.
Label simulated HCI/ATT framing separately. Exercise the actual vendor callback,
copy/queue ownership, parser and SMC dispatch, including reconnect, congestion,
malformed input and outgoing feedback. Keep all approved mapping/feedback tests
and both input sources isolated. Then build Elite development firmware, validate
exact app/controller/package identities and submit independent review.

Only the designated Mac installer performs installation and keyboard checks.
Hardware completion means observed physical faders, encoders, buttons, transport,
feedback and reconnect—not just a CONNECTED flag or notification count. At study completion, all
hardware work, installation, merge and external push were pending. Source-only
audit publication does not change hardware or runtime admission status. The parent
is the receiving owner of this audit and architecture decision.

## Reproduction and subsequent owner decision

The committed [study runner and instructions](../../tools/studies/smc_reference/README.md)
contain the numeric configuration, roots, pinned input hashes, inert app patch and
compression probe. They reproduce measurements without relying on private scratch
script hashes. They never create installable controller images or packages.

After this audit, the owner approved a temporary central-only **test profile**,
using USB to Loopy. Both Bluetooth modes remain the desired final product. This
approval permits investigation, not dropping more features or claiming fit; the
default dual-mode baseline remains protected. No runtime implementation is part
of this source-only study checkpoint.

### Follow-on faithful-startup discriminator

Retaining the actual vendor `btstack_task_init` in the central-private root study
raises the linked flash extent from 97724 to **104276 B**, and static RAM from 26928
to **27016 B**. The report-all link fails with 12 imports (adds `cbuf_read`). This
proves that the 97 KiB direct-root number omitted real initialization reachability;
it must not be used as a complete port size. The exact task wrapper preserves the
private L2CAP pool/dispatcher and ATT initialization; no private structure guesses
are accepted as a replacement merely to recover the smaller figure.

A final nonfunctional app ceiling, using `-DX0X_BLE=0` only in a disposable
non-packaging baseline build, measures 529492 B image, **23180 B code slack**,
97844 B main RAM and 335484 B pool. Compared with the inert-host ceiling, another
6284 B belongs to BLE orchestration and related state. At most 20 KiB whole-sector
primary expansion would then leave 2700 B app room to restore safe startup,
parser, controls and bridge, and provide 106496 B total controller capacity. This
is an upper bound with **all Bluetooth removed**, not an implemented layout or
permission to omit guards. The fixed 80+16+4 KiB manifest helper has not been
silently changed to this potential 84+16+4 KiB layout.

The subsequent source adapters are opt-in scaffolding. Native scheduler, event,
heap/CRC/timer and copy-bridge checks are not execution of the proprietary protocol
engine and do not establish a full resolved link, memory fit or hardware success.

### Fully resolved adapter checkpoint — does not fit

The actual conditional runtime adapter sources now link with **zero undefined
imports**, retaining vendor startup/task dispatch, HCI/private DMA transport,
SM/L2CAP/GATT, public central discovery and the exact vendor circular-buffer
implementation. The MPL hardware adapters provide real queues, bounded bootstrap
waits, timeout/CRC/allocation support and copied messages; no protocol engine is
reimplemented. This is the first resolved link, not a fit or hardware success.

The diagnostic image measures **108708 B flash and 28760 B RAM extent**. RAM extent
includes 10112 B heap plus 4096 B run/ISR stacks; it is not all static objects. It
exceeds the current low RAM window by 2392 B. The conservative donor heap plus
allocated task-stack allowance requires another 7808 B, so even the candidate
7168 B high region plus host union 768 and existing BLERAM slack 240 is 2024 B short,
before new app state. Reclaiming more old orchestration storage would require a
real lifetime/ownership layout, not counting all BLERAM as free.

Flash exceeds even the all-Bluetooth-removed app ceiling's 106496 B sector budget
by **2212 B before the 632 B standalone manifest validator, placement and app-side
restoration**. Exact-image RAM-load compression roundtrips 3524→3141 and 1824→444 B,
saving 1763 B gross: still 449 B over before decoder/metadata. The bounded fit gate
therefore fails. Do not begin hardware installation or claim an admitted package.

[Resolved metrics and source/input hashes](smc-vendor-resolved-link-2026-10-21.json)
and [reproduction commands](../../tools/studies/smc_reference/README.md) retain
this result. No sample capacity, pool minimum, stock/boot/calibration/key range or
default musical mapping was reduced. The separate default controller remains
ABI2; test-only sources reject ABI2 startup and are not enabled by default.

Native tests cover scheduler wait/queue ownership, ready-event copies, vendor
frontend API ordering with mocks, real shim allocation-tail release, CRC vectors,
one-shot timeouts, mailbox copies/congestion/busy retry/priority radio-off, and
the segmented validator. Exact Mac replay and actual SMC queue/UI host tests pass,
but do not execute the vendor binary or prove physical controls/feedback. The
parser retains the documented existing SysEx timestamp/realtime ambiguity.

### Central-role and actual placement gates

A stale queued radio-ON behind a newer priority radio-OFF was corrected: a valid
ABI4 extension is now the sole authoritative radio-control source. The final
all-role diagnostic measures 108724 B flash (16 B more), 28760 B RAM extent.
Regression tests cover that ordering; the default ABI2 controller rebuild remains
byte-identical to the recorded baseline SHA.

The temporary approved central-only profile can legitimately use the exact SDK
controller role mask 25 (master, scan, initiate), rather than 31 (also peripheral
advertising/slave). This is a declared reference-configuration deviation, not a
new protocol engine. Encryption remains explicitly enabled, mask 1. With this
single change the same actual adapters fully resolve at **104536 B flash / 28664 B
RAM extent**, saving 4188 B flash. Thus total-byte arithmetic has 1960 B headroom
against 106496 B, before manifest, app restoration and placement. It does not meet
the owner's eventual both-Bluetooth-modes goal.

**Placement is still blocked under existing PC guards.** The current guard has
two PC windows, occupied by contiguous app/controller XIP and RAM text. Admitting
executable secondary/tertiary segments would broaden the XIP window across store
and sample flash. No such guard weakening is implemented. Keeping those segments
data-only requires all executable code in the primary; this candidate's `.text`
is 91672 B, exceeding an 84 KiB primary 86016 B by 5656 B, or **5784 B including a 128 B
manifest**, even before other primary content. A total-size fit must never be
confused with safe executable placement. Further hardware-window confirmation and
an explicit architecture decision are required before startup/packaging work.

The only eligible cold-code compiler change (`pattern_init` from -O2 to -Oz) saves
16 B in a real full-feature app. All UI/config/routing already compile with -Oz;
no extra 4 KiB sector is available from this bounded flag test. Audio/DSP hot paths
were not changed. Actual single-central allocation analysis is separate from the
later donor's generic heap reservation; neither live heap nor task-stack safety
has been certified by a device run.

### Final bounded Classic-command discriminator

The exact SDK supports `more_avctp_cmd_support=0`; its documented configuration
uses this for dual A2DP source, not as an official LE-only library switch. Exact
IR review bounds its six branches to extra AVRCP operations, not BLE/GATT/SM.
With central controller roles25, this scalar change fully resolves at **103188 B
flash / 28648 B whole RAM extent**, saving1348/16 B. Reproduce with
`link_runtime.py --controller-roles central --avctp-extra-commands 0` and the same
pinned inputs. No opaque functions were patched or protocol handlers stubbed.

Its `.text` is **90504 B**, so text plus128 B manifest still exceeds the optimistic
84 KiB primary by **4616 B**, before other primary content. Both PC windows remain
occupied. This closes the bounded Classic-config check without demonstrating a
guard-preserving fit. Independent allocation analysis separately establishes a
10440 B simultaneous heap lower bound versus10112 B available, before transients;
see the single-central memory and PC-window reports. No app startup, segmented
installer or firmware package is claimed. Baseline dual-mode remains default.

### Guard-preserving flash placement discriminator passes

The subsequent app-data study moves exactly8192 B of immutable UI font/table
data, preserving glyph bytes and RAM/pool use. Its only recovery rendering change
is a retained-font substitution for the safe-mode heading; all normal rendering
requires independent app-data admission. See the app-readonly relocation report.

With those two sectors reclaimed, `link_runtime.py --controller-roles central
--avctp-extra-commands 0 --placement data-only-92k` actually resolves with:

| Storage | Used | Capacity | Remaining |
| --- | ---: | ---: | ---: |
| Contiguous primary: placeholder header, XIP code, RAM-code load | 94156 | 94208 | 52 |
| Data-only secondary: vendor constants | 7272 | 8192 | 920 |
| Data-only tertiary: initialized RAM load | 1816 | 4096 | 2280 |

The separate8192 B app-data sector pair is counted once, not free extra storage.
No executable code resides in the secondary/tertiary; RAM code executes in the
existing low interval. This **passes only flash placement**, independently
reviewed. The128 B header is still a placeholder, not a complete manifest.
RAM remains deliberately oversized at28648 B; app bridge/admission/update code
and target stack/live-heap evidence are still missing. No package was generated.
Earlier placement-blocked findings above describe the preceding84 KiB layout,
not this later bounded92 KiB discriminator. Default dual-mode firmware is unchanged.
