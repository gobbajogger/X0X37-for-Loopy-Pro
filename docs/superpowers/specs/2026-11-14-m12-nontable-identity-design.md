# M12 non-table canonical identity

## Outcome

Allow an independently accepted stage policy to select the existing non-table COMP1 canonical record without inventing an original historical table row or omitting a required obligation. This is a generic source-only schema extension; it chooses no product policy, changes no release hooks, and does not resolve or retry companion publication.

## Approach

Preserve the current source-section/original-row selector unchanged. Add an explicit `identity` selector containing `kind: request-source`, the canonical queue `document`, and the SHA-256 of the exact nonempty one-line `**Request/source:** ` value (UTF-8, no newline). The existing externally pinned roadmap commit/root/all-queue blobs authenticate that line. Only canonical records without a preserved source section or original table rows may use this selector. Mixed selectors, duplicate source lines, wrong documents and changed source hashes refuse validation. Do not make missing source metadata a permissive fallback.

Alternatives considered: fabricate a historical table row (misrepresents provenance), require a ROADMAP migration (unnecessarily changes the sole writer's scope), or hash the complete volatile record (couples identity to status/owner updates). The additive selector preserves explicit provenance without any of those changes. This is the implementation choice within the delegated schema-extension scope.

## Bounds

Existing evidence-level and immutable-input rules apply unchanged. Selecting COMP1 proves only the independently chosen obligation consistency; it never proves its implementation, real MIDI, publication or hardware acceptance. Deferred #119, declined 26.7 and undefined M5 stay held. No existing checker/runner, firmware, package, preview or publisher changes.
