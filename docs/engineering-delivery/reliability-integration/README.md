# Combined reliability candidate

This branch combines only the reviewed REC endpoint/preservation, CQ ISR,
SMC SELECT/LED, deterministic bass parameter/damper/enum guard, project export
and logical sample restore, and OTA identity validation units. Exact inputs,
conflict resolutions and executable-file digests are in `source-receipt.json`.

The bass selector UI proposal and new codec unit are excluded. Pattern stream6,
sound revision10 and three-byte steps remain. Untagged historical bass engine
identity, full sound snapshots/combined5 migration, actual PINGPONG/RANDOM
per-occurrence REC and usable validated engine/preset selectors remain OPEN.
No feature completion is inferred from the presence of UI code or old tests.

Manual conflicts were additive comments, CQ extraction stubs and unique test
registration. The REC project/storage/sequencer subtree remains byte-identical
to its accepted input. Current MIDI RT/MQ and BLE paths were retained. Combined
host regression, independent source review and exact Elite/Pro/controller
native budgets must be established on this new tree. Standalone proof files
are historical component evidence and do not qualify this combined input.
`proposed-closure.json` proposes applicable checks; it is not a test result.

No GitHub merge, publication, push or device operation is part of this source
integration. Hardware recovery/testing remains with the physical coordinator.
