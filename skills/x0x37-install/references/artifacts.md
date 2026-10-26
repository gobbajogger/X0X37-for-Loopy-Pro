# Artifacts and owner data

The designated installer owns device writes. This is Firmware Main Builder unless ownership has transferred.
Check current owner permission under [rule 14](../../x0x37-owner-decisions/SKILL.md). Current permission must cover the exact device action; read-only Pro investigation does not authorize installation, and Elite flash remains withdrawn.

Before each install:

1. Select the approved, validated artifact for the device model.
2. Back up every loop slot.
3. Send the owner a short install notice.

Record the commit, validation record, configuration, model, and checked CRCs in the install ledger.
Keep the native loader and selected-controller verification after an actual firmware write.
Repeat full loop-slot readback and CRC comparisons only when committed edits affect loop storage,
its layout or serialization, installer writes, or flash boundaries, or when a mismatch or failure needs investigation.
For unchanged inputs, reuse matching slot-test evidence. An install or stage change alone does not trigger another full-slot readback.
Record which checks ran and which prior evidence was reused; do not claim a fresh slot CRC that was not read.
Report every observed mismatch immediately.

If the package and controller are byte-identical to an already verified install, reuse that install evidence
unless the owner requests a reinstall or the current device state needs verification.

For transfer between machines, use Git or bundles to preserve commits and required validation records and logs.
Local build and device access do not require transfer to another machine.

Exclude proprietary JieLi files.
