# Reference-based SMC integration: measured architecture audit

**No new runtime candidate is admitted.** None of the measured vendor-host layouts
fits the current controller region. The bounded redesign study below also has
not demonstrated a feature-preserving, fully resolved fit.
The baseline, approved mappings, UI, feedback, samples, loader and model guards
are unchanged. MTU/interval-only tuning was considered and rejected before any
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
feedback and reconnect—not just a CONNECTED flag or notification count. All
hardware work, installation, merge and external push remain pending. The parent
is the receiving owner of this audit and architecture decision.
