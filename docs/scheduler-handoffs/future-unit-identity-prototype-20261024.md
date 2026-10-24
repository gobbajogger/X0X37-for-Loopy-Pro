# Future unit identity query prototype and native Mac receipt

**HOST PROTOTYPE / NOT INTEGRATED INTO FIRMWARE / TRANSPORT AND LEGACY BOOTSTRAP INCOMPLETE.**
This continues PR125 on the existing supplementary lane. PR114 Data Safety,
Independent QA and sole Safe Merge retain their roles; parent coordinates Mac
execution and any hardware action. No new task, protected ref or device write.

**Fresh ownership checkpoint during final validation:** PR114 advanced to R10
`77d571587fe91b5357ab7914e39e04715d737023`, root
`34ed69cfd7e4002acb077d4605a00dfb6fcd9c09`. That owner added separate output-token
guards and an `ota-output-endpoint` regression. This lane does not duplicate or
overwrite that implementation. PR125's tested base remains PR121 plus R9;
none of its reported whole-root counts or nine strict failures qualify R10.
Data Safety/Independent QA must reconcile the read-only CLI/runner additions
with R10 and test their actual combined tree. This future protocol prototype
does not modify production output selection or supply either physical provider.

## Sanitized actual Mac result supplied by parent

The parent reports validation of **exact commit
`632b3f21abdb4b8483e60b5a23fe87b90aea5584`, root
`ebaa344555eccbbc2fb882d56f07c12e60a4ce35`**, with eight needed files checked
against their Git blobs. This is the previously delivered native provider, not
the later portable prototype in this packet.

- Apple clang 21.0.0 (`clang-2100.3.34.2`), arm64, macOS SDK 27.0, runtime 27.2.
- Committed build wrapper: exit 0, no diagnostics. Provider host tests: 21/21
  normally and 21/21 with Python -O. Native helper, provider and installer
  `--native-endpoints`: each exit 0.
- Helper source SHA256:
  `92d3090e1d85dc74b6a664505790b94e5f931f85f697d315b1cd3b5e91ee172f`.
- Actual Mac helper binary SHA256:
  `f6e9cb9a3c0893f9f4cb6bbf8a43c70b94d7876fa4fe5aa5fd8d80ec5e6c9795`.
- Parent's local/private validation-receipt SHA256:
  `d7a7a724cb367b179efdcbab499fbb0cde951515028b349523334cd9a8782e27`.
  Receipt bytes were not retrieved or independently hashed by this cloud lane.
  No private path, raw native object refs, unit identifiers or binary is included.
- One active in/out pair shared entity/device; serial absent. Only
  `live_pair_observation` was true; all identity/write capabilities remained false.
  Native unplug/reboot/replacement negatives were **NOT RUN**.

The native helper, build wrapper, provider, installer and provider test source
are unchanged from that tested commit; the later evidence-only INFO update does
not invalidate their code hashes. Editor INFO `X0XDEV` reply evidence is separate
from the native provider test and establishes no model or persistent identity.

## Genuine UID candidate, with exact unknowns

The [primary-source audit](ac791n-unit-id-provenance-audit-20261024.md) identifies
two supported **candidate part profiles**, not an identified keyboard part:

- Puya P25Q80H V1.7 p54 and P25Q80SH V1.4 p89 specify manufacturer-set read-only
  128-bit UID via `0x4B`, **four dummy bytes**, then **16 bytes** of output.
  Both parts publish JEDEC `85 60 14`, so JEDEC alone does not identify the part.
- Official JieLi AC79 documentation exposes `IOCTL_GET_UNIQUE_ID` with a 16-byte
  buffer. Exact pinned SDK implementation and standalone-loader linkage are
  unread, and this documentation is release_v1.2.0 rather than verified v1.2.1
  implementation evidence.
- No verified UID contract for `EB6014`, no suitable public eFuse unit-ID
  contract, no actual fitted-part/die identity, and no provenance linking stock
  16-character serials to any factory UID were established.

These facts permit a bounded portable prototype and implementation plan. They
do **not** permit inferring a part from JEDEC, emitting raw SPI commands on a
keyboard, exposing protected trim/key_mac/decryption keys, or enabling OTA.

## Implemented portable source prototype

`tools/prototypes/ota_identity_v1.h` is a freestanding C99 prototype, **not
included by the firmware app/loader or production updater**. It contains no
hardware, bus, file, network, flash-write, session-arm or device-send API.

- `ota_uid_plan`: produces only an in-memory read recipe for an explicitly
  qualified H/SH part profile plus matching JEDEC. Unknown/default profiles and
  EB6014 refuse with a zero-length plan. It never selects a part from JEDEC and
  never executes its recipe.
- `ota_uid_reads_valid`: checks two full successful 16-byte observations agree
  and rejects zero/FF data and transfer errors. These are sanity checks, not
  proof of manufacturing provenance or uniqueness.
- `ota_id_query` / `ota_id_reply`: strict fixed-length, versioned **read-only
  payload** codec with a fresh 32-byte challenge, phase, exact Elite/Pro model,
  capability status and 16-byte public unit tag. No production SysEx opcode is
  assigned. Missing/unqualified provider yields UNAVAILABLE with a zero tag.
- `ota_id_matches`: compares a reply to an **already trusted** tag, fresh nonce
  and expected phase/model. Wrong unit, replayed nonce, phase/model mismatch,
  truncation, trailing bytes, malformed version and mutation refuse. It provides
  no initial enrollment or permission to send a destructive frame.

The public tag's real provider is deliberately absent. Future shared app/loader
code must derive the same domain-separated cryptographic public tag from a
qualified nonsecret factory UID, with adequate identity strength. Never use
model/location, JEDEC, CRC, random-per-boot data or a host confirmation as a
replacement. Challenge freshness requires real host CSPRNG in the eventual
integration. This prototype does not implement device/session persistence,
mandatory write gating, USB serial descriptors, SHA derivation or legacy support.

`tests/host/ota_identity_v1_prototype_test.c` uses only synthetic data. Native
host builds passed **201/201 checks** under GCC 14.2.0 with warnings-as-errors,
and separately under AddressSanitizer+UndefinedBehaviorSanitizer. Registered
selector: `ota-identity-prototype`. These are host prototype results, not app,
loader or installable firmware builds and not physical identity evidence.

## Bounded actual firmware implementation order

1. Establish actual part/die provenance and a covered read contract for each
   supported model/variant; keep EB6014 unsupported until documented. Read exact
   pinned SDK UID implementation before choosing its API. Do not execute the
   vendor sample program, which contains destructive test operations.
2. Implement one shared default-disabled RAM-only reader with status-propagating
   bounded SPI operations. Existing `fl_tx`/`fl_rx` discard timeout status and
   must not be reused unchanged. Every failure must deassert CS and restore
   SFC/pins/controllers; check readiness, XIP mode, IRQ and core1 safety. Prefer
   early boot before USB/audio/core1 where the source lifecycle permits it.
   Do not assume SDK driver symbols link in the standalone loader.
3. Cache validated UID in RAM, derive the same public tag in app and loader,
   and integrate a read-only identity query using a reviewed noncolliding wire
   command and canonical existing SysEx encoding. Verify both actual target
   builds, linker RAM placement and budgets before executing this code.
4. Independently test same-unit app/loader/cold restart stability and different
   unit discrimination while keeping raw IDs private. Explicitly test missing
   capability, failed/partial/inconsistent reads and unsupported parts. Model
   migration uses the initial app model and package-supplied destination loader
   model separately; a same-unit tag does not waive model/range/CRC guards.
5. Only after endpoint-bound transport is proved, design mandatory per-write
   session/package/stage/sequence gates and durable handoff proof through final
   application/restore. Unit identity query replies alone cannot prevent a wrong
   output from receiving data. Preserve the strict B-zero transport gate.

Legacy bootstrap is a **separate** unfinished work item, not a reason to delay
portable future-protocol source/testing. Existing installed serial-less firmware
does not implement this query and is not retroactively authenticated by it.
Personalized-loader bootstrap would require its own exact compatibility,
package-byte validation and safely bound initial target. No such flow is enabled.

## Target build input access result

Personal main Library index resolved the pinned compiler archive
`libfile_f0166b5d08748191b52b4733add83271`, expected ZIP SHA256
`ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`,
and the canonical private R10 package archive
`libfile_993d1e38ae808191b138e95ce535d5d8`, expected ZIP SHA256
`0b88e2dce13560a94db06d79170d9ba3c4453b4258a1196e91120ea8a4345cdc`.
The latter contains the three pinned SDK package components; it is not new
source-matched firmware for this prototype.

Actual attempts: the Library search download helper failed `hosted apps
tools/list request failed: network`; preparation of the index-resolved files
succeeded, but both complete-transfer materialization attempts failed
`download failed`. No archive bytes were placed, so no downloaded hash/integrity
claim is made. Vendor compiler endpoint returned HTTP 403; Gitee SDK source
lookup returned CONNECT 403; official GitHub SDK probe returned repository not
found. These are narrow input-transfer failures, not missing Library records
or a general development blocker. Portable host source/testing proceeded.

Firmware tree/build inputs are unchanged by the prototype because it is not
included in app/loader. No target binary was produced or relabeled. Target
integration/build and physical UID observations remain explicit next gates.

## Exact prototype validation receipt

Tested source: `c22932e6bb74dc978d0c466614d086b31eaa8c23`, root
`1c5501c9aa4942efe954add21364eaf9ccbf268a`; firmware tree unchanged
`5c209609b770fd13949230aeb80956594fd6b741`.
The [command/log-hash receipt](../future-identity-evidence-20261024/receipt.json)
records:

- **25 selected normal runner suites**, exit 0, including the registered C
  prototype, PR121 fixtures and existing installer/backup/restore/controller
  tests on this exact complete checkout.
- **24 direct Python -O suites**, exit 0 each.
- Prototype **201/201** in the normal runner; separate GCC14.2 build with
  `-DNDEBUG -fsanitize=address,undefined`: compile exit 0, **201/201**, exit 0.
- Original strict transport diagnostic: exit 1, nine failed assertions in 15
  cases in each Python mode, on this lane's R9-derived installer. Still NO-GO.
- Explicit Elite and Pro development `sh build.sh --board smk37` attempts:
  both exit 1 at `no toolchain in /workspace/ota-private-inputs/toolchain`,
  **before compiler execution**. These are failed prechecks, not builds.

No production code changed after that tested source; later receipt text/logs
are evidence-only. Native source/API review found no blocking issue for this
default-unavailable prototype, not acceptance of an actual hardware UID or OTA.
