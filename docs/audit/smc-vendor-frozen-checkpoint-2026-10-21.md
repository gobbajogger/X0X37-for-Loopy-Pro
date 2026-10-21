# Vendor SMC integration: paused, non-installable checkpoint

**The complete profile does not fit. No firmware package or hardware result is
claimed.** The owner paused this port at a measured placement blocker while a
separate, baseline-stack diagnostic awaits hardware evidence. Both Bluetooth
modes remain the final goal; central-only with USB to Loopy was authorized only
as a temporary test profile.

## What is implemented and tested

- The exact pinned vendor host/controller resolves without missing imports or
  protocol stubs. The real vendor startup, task dispatch, GATT receive and SM
  engines remain in use. One reference-derived SMC frontend owns the connection.
- Mailbox payloads are copied, not borrowed vendor DMA pointers. Priority radio
  OFF cannot be overridden by stale queued ON or blocked forever by MIDI retry.
- The real app adapter retains startup/watchdog, sink, interrupt and PC guards.
  Copied notifications enter only the dedicated SMC queue. The parser uses actual
  queue acceptance; INLOG and musical mappings remain. The fixed TEST role works
  with legacy projects whose stored Bluetooth role is DEVICE; the TEST menu
  explicitly labels USB use. Per-message generations reject old-link controls.
- Exactly 8192 B of immutable UI data was relocated in disposable target builds,
  with unchanged glyph bytes and verified pointers. Its independent app-owned CRC
  gate runs before normal UI. Recovery uses retained fonts. Owner/store/loader
  record exclusions preserve these sectors even with Bluetooth off.
- Owned CPU-only BSS and a scoped initialization pool fit the bounded high region.
  Actual target call routes confirm the four CPU allocations (1240, 128, 56, 480 B;
  1936 B charged including headers). This is a first-initialization-phase scope,
  not task-lifetime allocation routing. IRQ, DMA and ordinary runtime allocations
  stay low; no yield or fallback is permitted inside critical scopes. Critical
  allocation failure cannot return a null pointer to unchecked vendor code.
- Failure phase/ordinal/request size is retained in ABI4 `dbg[5]` before FAILED;
  no packet/key bytes are recorded. A high-region admission failure returns before
  reading any high diagnostic state.

Native scheduler/profile/heap/scope/bridge tests passed. Production UI dispatch
passed in default and TEST modes, including legacy role-bit zero, feedback,
coalesced disconnect/reconnect and queue saturation. Exact retained Mac capture:
1271 notifications to 2711 messages, zero parser rejects/drops. This is not the
Hortator device capture or a physical SMC checklist.

## Final measured budget

| Component | Used | Capacity | Result |
| --- | ---: | ---: | --- |
| App primary | 527460 B | 524000 B | **3460 B overlap** |
| App main RAM | 98100 B | 98304 B | 204 B free |
| App BLE RAM | 2936 B | 4096 B | 1160 B free |
| App pool | 335484 B | 344064 B | 8580 B headroom; 8192 B guard retained |
| Controller primary | 93348 B | 94208 B | 860 B free |
| Controller constants | 7272 B | 8192 B | 920 B free |
| Controller RAM-code load | 3524 B | 4096 B | 572 B free |
| Controller low RAM | 26352 B | 26368 B | 16 B free |
| Controller high CPU region | 6444 B | 7168 B | 724 B free |

The controller numbers include a 128 B **placeholder**, not a sealed manifest.
Its RAM-code load is external data; instructions execute only in the original low
RAM interval. No executable spare or widened PC window is used.

The 10112 B low heap and existing task/run/ISR stack sizes were not reduced. The
known simultaneous allocation lower bound was 10440 B; relocating the audited
1936 B leaves a low minimum of 8504 B. The remaining 1608 B is **not a certified
live margin**: scan/connection/security buffers, fragmentation and target stack
high-water still require verification. Bond persistence and physical pairing are
not established; the current diagnostic syscfg implementation does not persist
keys. Reference-parity authentication is not a universal pairing claim.

For app attribution: the historical BLE-disabled ceiling was 529492 B. Removing
8192 B UI data and adding its measured 164 B admission cost gives 521464 B. The
real current profile is 5996 B above that ceiling, including the conditional
platform startup/guard path, parser, mailbox, UI state and generation handling.
It is not valid to budget only the standalone parser or remove these guards.

## Why simple compression/shared sectors do not establish fit

Total used storage is 527460 + 8192 + 104144 = **639796 B**, against **638688 B**:
1108 B short before remaining transaction and admission work. Existing project-LZ
code compresses the vendor RAM loads from 5340 to 3953 B. The actual paired app
wrapper costs 176 B; one combined descriptor needs at least 16 B. Net saving is
1195 B, leaving only **87 B** before remaining format, startup and update glue.
That is a conditional arithmetic residual, not free admitted space.

Even a shared erase sector does not finish placement: app plus wrapper and vendor
executable primary requires 619168 B against 618208 B, **960 B too much**. Remaining
data-only holes are 920 and 127 B after compressed-load metadata. Further app data
relocation, metadata and code would be needed; none is assumed or implemented.

Independent review found that the existing persistent RAM-loader recovery flow
can support whole-main-area sector replacement in principle. Shared ownership is
not inherently impossible. However, the current loader does not bind and verify
all external app-data/controller components before clearing its update record,
and its finish/read-error path needs correction. An inseparable whole-image
transaction, standalone-write prohibition, descriptor sealing, writer quiescence,
interruption/mixed-version tests and paired rollback are not implemented here.
No further tuning or shared-sector implementation is authorized in this paused
checkpoint.

## Safety and reproducibility

Ordinary `X0X_SMC_VENDOR_TEST` activation is a compile error unless the explicit
`X0X_SMC_VENDOR_STUDY_ONLY` flag is present. The supplied study recipes suppress
packaging. The app descriptor remains unsealed; the controller retains its
non-admissible placeholder. This is not a supported installation profile.

Default regression builds remain byte-identical:

- Controller: `9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245`
- App: `4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9`

The companion JSON records current source/input hashes, private ELF/map/symbol
hashes and exact capacities. Proprietary archives, IR, generated source and binary
artifacts remain private, not in Git. Earlier reports are chronological studies;
this checkpoint supersedes their optimistic/incomplete-root fit numbers.
