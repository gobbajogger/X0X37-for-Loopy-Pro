# Bounded selected-note sound snapshot proposal

A note can retain its actual sixteen sound controls while later global edits
shape other notes. The record stores values and engine/schema identity, never a
preset number or a pointer into a preset bank. This is a source/runtime proposal;
the frozen normal piano-roll UI is deliberately not imported here.

## Storage and edits

The unchanged 160-record motion pool also stores sparse snapshots. One snapshot
uses one 72-byte lane, shared with existing motion/probability capacity. At most
160 snapshots fit in an otherwise empty pool. There is no dense all-note array,
new persistent allocation, reduction of undo capacity or pool reserve.

A snapshot record uses target254, part2/3, pattern0..31, step0..63, record schema1.
Two sixteen-byte banks and an engine-identity byte beside each bank allow a
same-core main-loop writer to publish a complete bank with one selector-byte
store. Engine identity1 means native303/schema1; identity2 means WBL1/schema1.
Identity is not a preset index. The audio ISR copies selected values before main
can run again. A returned storage pointer is only valid until the next mutation
of that record; an active note keeps its own effective copy.

New capture rejects a full pool or invalid input before mutation. Replacement
and knob edit reuse the record's reserved alternate bank, including when the
pool is full. An edit changes the stored sound for its next note boundary; it
does not retune an already sounding note. Removal clears only the snapshot record
and preserves independent knob lanes. Raw project/undo bytes include both banks.

The proposal uses stream4 and reads old streams3/2 with absent snapshots. Imported
records validate bounds, selected bank, identity and duplicate keys before staged
commit. This stream4 conflicts with the independently developed recording
endpoint stream4: a combined integrator must allocate a new combined revision
and explicitly migrate both exact frozen formats. These changes are not mutually
compatible simply because both use the number4.

## Runtime precedence and note lifecycle

At an accepted NOTE, or an already sounding TIE boundary, a matching snapshot
supplies the whole note base. Native303 explicit knob lane values at that step
overlay that base. Continuous motion ramps and later global commands cannot
change a locked note. Native303 knob lanes never become WBL1 controls. Storage
bytes remain independent of all playback changes. Backend identity mismatch
ignores the snapshot.

Global commands update a separate baseline. A later unlocked accepted note
restores that baseline and its applicable motion. Note-off, REST, mute and a
probability-skipped attack keep the old parameters through the existing release
tail. Only the backend's actual idle report permits restoration without a new
note. There is no independent duration timer, voice reset, backend switch,
retrigger or unconditional output bridge. Existing synth_note/off callbacks
remain the lifecycle authority; live input uses the same backend dispatch.

The capture endpoint refuses undrained parameter commands and copies all sixteen
observed effective bytes with a revision check. WBL1 parameters use the separate
identity-qualified command endpoint; legacy303 commands cannot reinterpret them.

## Outstanding admission gates

Low-level fixed-record admission is implemented. Compressed-storage capacity and
whole-gesture undo admission are not preflighted by the capture endpoint. Existing
save preflight rejects an overfull save before flash writes, but that does not
make the earlier edit transactional. Existing UI copy clears/copies before a
capacity failure and publishes lane values piecemeal. It must be replaced by a
preflighted publication path by the coordinated integrator. This proposal is not
whole-feature acceptance until those gaps are resolved.

The owner accepted the frozen normal piano-roll visual direction at UI source
7ca652b537e2a54733178424e7326f625d9fce79. That source is an interface/layout input,
not runtime or native acceptance. Recorder source remains separate; no moving
recorder/UI edits were imported. Exact combined-source capacity optimization
belongs to the designated performance/integration owner, with unchanged sound,
data, UX, feature bounds and reserve. Standalone savings are not combined fit.

Target cycles/deadline, total stack/high-water, hardware, controller pairing,
packages and installation are outside this bounded run. Native compiler/map
results and independent host review are recorded in the delivery receipt.
