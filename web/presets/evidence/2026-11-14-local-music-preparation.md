# Local music preparation and preset librarian delivery

## Source and intake

The isolated successor consumes PR #915 at
`0472ca4ca75cf477be171554071f972f60dba88c`, tree
`6f1e73988fc25e98635c7da9ab9a9dc577ee0677`, and its committed
`2026-11-14-explicit-target-parity.md` evidence. The upstream W3 parent is
`7bece9b819790600bb755e2900817b249d875567`. The existing actual-engine target
rule and retained original-engine provenance are preserved. Historical 548
assertions and earlier independent native results are reused only as dated
evidence; they were not rerun for this successor.

Initial roadmap intake read Working/Pending before Complete at #887
`f7eaf9e2d76f7feaf133cb4ad687c54df0eac6ee`. A later read at
`d58c4769ed298f13f278a948177593773a4c4f07` confirmed #915 remained unchanged
and exposed an unresolved #103 ownership/export-denial note. This receipt does
not change canonical queues.

The starting offline editor tree is
`b29301712e0a30eefe1125f9459cdc726ded9b05`, identical to the editor tree at
`94197fc839b00aff47b98ca02f3c6396e4a6eabf`. PR #913 at
`f65de4148cdbbadefa896f2b59f6c156f7e5ed74` is a documentation-only source
comparison, not an application fix. Its report blob is
`4cb1bf89f8a0d07305c180bf3f3f81912974ab51` and matrix blob is
`fd290041bf0c665576a98a8254b013ba1fab5fef`. The named producer thread's
current release could not be independently read with the exposed tools.

## Ownership and user outcome

Root owns this isolated branch and integration. Disjoint agents handled the
contract read, novel browser tests, warning/focus work, export work, editor
repairs, source-backed sound descriptors, and independent reviews. Product
changes stay in `web/presets/` and `web/offline-sequencer/`; no shared website,
firmware, companion, canonical ROADMAP, or device path is written.

The librarian previews DNPR/ZIP input, validates all records before temporary
import, keeps the prior preview when a replacement fails, places warnings
beside affected records, and downloads eligible user records unchanged.
Mixed ZIP export excludes protected records while retaining them in the
session. Download messages describe a browser request, and failures retain
the session and release temporary URLs. Closing or refreshing clears the
temporary list. The editor and librarian link to each other and keep separate
local state.

## Preset acceptance

File-side A1–A8 are exercised by the focused Node tests: all-engine unchanged
round trips, invalid CRC/header/length refusal, newer schema/version refusal,
actual-engine target refusal, capacity atomicity, sample warnings, user export,
and factory/derived export OFF. Device counterparts remain NOT_RUN.

At the resumed source, `node --test web/presets/test_codec.mjs
web/presets/test_page.mjs web/presets/test_empty_sample_warning.mjs` passed
25/25, zero fail/skip. The four new Playwright files passed 15 Node test units,
including four workflows in `test_ux_browser.mjs`, zero fail/skip. These cover
mixed ZIP bytes, all-blocked outcomes, download/ZIP failures and retry,
record-local warnings, keyboard recovery, bad ZIP atomicity, preserved preview,
and stale asynchronous read cancellation. The older actual Chromium runner
passed 19/19 scenarios after its obsolete mixed-export expectations were
updated. The unchanged target/provenance runner passed 6/6 against the earlier
frozen successor snapshot. These are browser-file results, not device results.

Independent preset review found two additional defects: empty 808 reference
lists had no local warning, and a failed replacement lacked a filename and
retained-preview explanation beside Review. Both were reproduced, repaired,
and independently rechecked in Chromium at 1280x800 and 390x844. The review
verified unchanged downloaded bytes, retained protected entries, useful
keyboard focus and no horizontal overflow at those viewports.

Accepted preset SHA-256: app.mjs
`063a2d820a9090dfe8104e501891f82f39ec6718de2634649143dec2f003a9a8`;
index.html `f63cc118a89b4125edf93df9acd059a37dfd5fc688e773124790d1af2748dc0d`;
codec.mjs `6c627bc544333d9037e116a0a43f48d8699291fc96995f338d7dde4a0b33449c`;
zip.mjs `2a1d9da77317281065f53b35b494d83f2e8ddddc25795951ee478dbaaa9138c9`.

## Editor continuation and row coverage

#90–96, #99–100 and #102 are preserved existing journeys, with regression
coverage rather than rewrites: synth/drum/BREAK editing, explicit MIDI
replacement/loss preview, guarded draft storage, Undo/Redo, generators,
part operations, grid/scale/snap/ghost/touch entry and gesture cancellation.
#97/#98/#101 repairs address named dialogs, local rejection messages, keyboard
draft opening, focus recovery and all six MIDI rate defaults. BREAK slice
announcements describe the slice separately from its gate. Browser download
errors are recoverable and do not claim a completed disk save.

Editor frozen-source verification and independent acceptance are pending at
the time this initial receipt is written. Exploratory runs that overlapped
source changes included a deliberate new regression failure and a MIDI
preview timeout at 390px; neither is counted as final acceptance. The earlier
90/90 shared-tree Node pass precedes final UI changes and must not stand in
for the final frozen replay.

## Interfaces and limitations

F1/26.8a owns authenticated model/firmware identity, preset storage, engine/slot
and sample inventories, quiet-state policy, atomic writes and final device
acknowledgement. A DNPR filename hint is not a handshake. #52/#104 own genuine
revisioned editor synchronization; the current DeviceSync remains disabled
without that contract. COMP1 and TouchOSC have separate owners. No fictitious
message IDs, port traffic, transport acknowledgement, firmware save, sample
audio transport or device backup is introduced here.

The existing project adapter still refuses unsupported identities/ranges,
sound edits, changed engine assignments, RNG export and unsupported BREAK
intent. Raw revision-13 musical import is not universal lossless conversion.
Plugin formats/fidelity require genuine captures and independently qualified
adapters. Physical phone/iOS/VoiceOver, device, SDK/native firmware build,
audio/DSP parity, merge, deployment and public artifact publication are
NOT_RUN or outside this delivery's authority.

The previously denied public handoff COMMENT on #901 is not retried or routed
elsewhere. Any #103 public-export denial remains action-specific and pending
exact reconciliation; source review does not grant publication authority.

## Run and downstream consumption

From the repository, run `cd web && python3 -m http.server 8765`, then open
`http://localhost:8765/presets/` or `/offline-sequencer/`. The existing website
owner consumes the verified component source for whole-site integration. No
supported direct thread-message/consumer-execution tool was exposed here;
Git readback alone is not a consumer execution receipt.

Remote branch/commit/PR and final evidence will be recorded after permitted
source delivery. At this receipt's initial state: NOT PUSHED, PR none.
