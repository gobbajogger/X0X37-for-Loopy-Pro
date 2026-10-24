# Native macOS pairing provider — supplementary PR125 continuation

**SOURCE IMPLEMENTED / HOST CHECKS ONLY / NATIVE SDK BUILD AND EXECUTION NOT RUN.**
**PRODUCTION TRANSPORT INCOMPLETE / DEVICE NOT QUALIFIED / NIGHTLY NO-GO.**

## Ownership and new evidence

Re-read live Data Safety PR114 head and discussions before this change. Producer
head remains `5b16ff81ea1dcb731b719f0505ae2349571481cf`; PR121 remains
`2e8c2795ffab81d663890e2ed3b88f470da924de`. Continue only the existing isolated
`lane/transport-contract-20261024-1236` / PR125. Data Safety, sole Independent QA,
Safe Merge, browser admission and PR124 roles are unchanged. No new cloud task
or schedule is created by this continuation; parent coordinates Mac work.

Parent-supplied read-only Mac evidence from task
`01a16df8-2534-75b8-b856-6eb1c1c09009` reports:

- Public CoreMIDI endpoint -> entity -> device APIs pair the two active directions.
  AppleUSBMIDIDriver properties include USBLocationID, USBVendorProduct and optional
  SerialNumber; saved stock SMK records had serial properties.
- A subsequently attached live USB `1209:0001` unit has one active source and
  destination sharing an entity/device, with USB serial index zero and no
  CoreMIDI SerialNumber. Its USB display name does not prove running firmware
  model/version or OTA compatibility.
- Nonwriting identity and Editor INFO sends returned OSStatus 0 but no valid
  reply within two seconds. This does not prove successful request/reply or
  receive failure; passive physical input observation is separate work.

These are attributed parent observations, not execution by this cloud lane and
not qualification of this newly written helper. No Mac code or device commands
were executed from this task. Linux observations from the earlier PR125 stage
must not be generalized into a claim that macOS is unavailable to the project.

## Actual production-source changes

`tools/native/ota_coremidi_inventory.m` uses the actual native SDK's public
CoreMIDI types/API, avoiding a Python-guessed native ABI. It starts exclusively
from `MIDIGetNumberOfSources` / `MIDIGetNumberOfDestinations`, then reads
`MIDIEndpointGetEntity`, `MIDIEntityGetDevice`, offline state and properties.
It never enumerates saved devices, creates a client/port, connects a source,
sends a MIDI packet or claims USB interfaces. It emits only bounded numeric
object/USB metadata and a serial-presence boolean, never serial strings, names,
uniqueIDs or full property dictionaries. Missing hierarchy/properties are
excluded. Device online state is mandatory downstream; any reported offline
endpoint/entity also disqualifies a row.

`tools/ota_coremidi.py` invokes that fixed helper without a shell, verifies its
source/binary hash manifest, validates its schema, rejects duplicated refs and
ambiguous pairs, and pairs exactly one source/destination having the **same
entity AND device**. Equal USB locations, serial presence, model or display name
cannot join different native units. Missing directions never borrow a sole
output. Conflicting parent metadata, cached/offline or malformed records never
become pairs. Every discovery calls the reader anew; old observations are never
used as fallback after disappearance or reader failure.

This is a **live CoreMIDI pairing observation**, not a leased native USB handle,
an atomic inventory, or an installed transport. CoreMIDI inventory calls are
sequential and refs may disappear/reuse while collecting. Consequently all these
capabilities are explicitly false, even when pairing succeeds:

- Native USB handle binding and disconnect invalidation.
- Atomic send and persistent physical_id.
- Cross-reboot continuity, OTA install and sample restore.

No `physical_id`, `open` or `send` method is added to the provider. MidoBackend
and the existing destructive path retain their identity requirements. Optional
stock serial evidence is not promoted to a serial for the serial-less app/loader.

`tools/fm1_install.py --native-endpoints` exposes the read-only report before
Mido or hardware/install guards. Combining it with package, port or install
options is rejected; it cannot authorize an install. Unsupported platform,
missing/stale helper, invalid inventory or no eligible pair returns exit 3;
there is no Mido/name/cached-record fallback. A valid report returns 0 only to
indicate a pairing observation was obtained, with all write capabilities false.

`tests/host/ota_coremidi_test.py` is registered as `ota-coremidi`. Tests exercise
the full provider/installer integration, simulated native subprocess boundary,
inputA/outputB, same metadata across different units, ambiguous/absent pair,
offline/cached/malformed/duplicate data, replacement between observations,
serial redaction, helper provenance mismatch, no-capability refusals and no MIDI
opening/writing. An allowlist assertion checks the native source has only the
eight read-only CoreMIDI calls. These are host/static tests, not actual CoreMIDI
runtime or native compilation evidence.

## Build and read-only native verification packet

For parent-coordinated native macOS validation at the **exact delivered SHA**:

```sh
python3 -B tools/build_coremidi_inventory.py
python3 -B tools/fm1_install.py --native-endpoints
```

The build wrapper runs `xcrun --sdk macosx clang -fobjc-arc -Wall -Wextra -Werror`
with Foundation/CoreMIDI and writes only `build/native/ota_coremidi_inventory`
plus its JSON source/binary SHA256 manifest. It never runs the helper. Missing
macOS refuses before build or execution. No native binary was produced here.
The hash manifest detects stale source/build combinations; it is not a signed
hardware attestation or an atomic execution/endpoint guarantee.

Require the native SDK/compiler/OS versions, actual compiler exit and output
hashes, no-USB and cached-only negatives, and the real active-pair report with
serial values withheld. Verify the output against independent parent/native
endpoint inspection. Do not interpret helper success as an identity-handshake
response or assume USB `1209:0001` identifies a supported firmware package.

For future transport work, independently establish a retained same-unit native
input/output mechanism and its disconnect/retarget semantics before sending
destructive bytes. Then independently solve legacy initial handoff and loader
continuity. A new session protocol must gate each destructive request, reject
wrong/replayed sessions, and bind final app verification plus any restoration.
The currently attached old app's behavior is not proved by future firmware.
Owner interaction must physically attribute the already-open live unit; a
confirmation of a guessed name or USB location cannot supply that evidence.

## Receiving roles

Data Safety consumes this source-only provider without treating pair observations
as physical_id. Sole Independent QA reviews the exact new installer/provider,
normal and -O tests and native helper once compiled. Sole Safe Merge preserves
NO-GO for destructive native transport until those separate gates pass. Existing
strict PR125 boundary tests remain red; no negative was weakened or converted to
an expected-failure PASS. Target firmware input tree is unchanged; no firmware
build, controller, SDK material, private sample or native executable is committed.
