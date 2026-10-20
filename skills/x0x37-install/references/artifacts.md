# Artifacts and owner data

The designated installer owns device writes. This is Firmware Main Builder unless ownership has transferred.
Use the standing development install permission in [rule 14](../../x0x37-owner-decisions/SKILL.md). Do not ask for it again.

Before each install:

1. Select the approved, validated artifact for the device model.
2. Back up every loop slot.
3. Send the owner a short install notice.

Record the commit, validation record, configuration, model, and CRCs in the install ledger.
After installation, read back the image and compare every loop slot CRC.

Report each mismatch immediately. Claim data preservation only after those checks pass.

For transfer between machines, use Git or bundles to preserve commits and required validation records and logs.
Local build and device access do not require transfer to another machine.

Exclude proprietary JieLi files.
