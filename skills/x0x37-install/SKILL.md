---
name: x0x37-install
description: "Install validated x0x37 firmware, transfer install artifacts, or prepare an explicitly requested release package with data safeguards."
---

# Install and release

### Rule 5: exact artifacts and owner data

Before artifact selection, transfer, or installation, read [artifact and data requirements](references/artifacts.md).
They define installer ownership, permission, model match, backups, notice, ledger, and the conditions for readback and CRC checks.
Use local access without a machine transfer when available. Exclude proprietary JieLi files.

### Rule 6: finished set and release evidence

Default: development, `X0X_DEV_PADCAL=1`.
Before install-package validation, installation, release packaging, or publication, read [evidence requirements](references/evidence.md).
They define the complete queued set, exact artifacts, DEV CHECKED record, explicit controller selection, model and data guards, and release conditions.
When specifically authorized, a development install uses `tools/dev_validate.py` and `tools/fm1_install.py`; it skips the full gate under [rule 11](../x0x37-integrate/SKILL.md). Any installation needs current model- and action-specific permission; the authorized Pro investigation is read-only and Elite flash remains withdrawn.
Release packaging, publication, and external pushes keep their separate authorization requirements.

### Rule 7: safe writes

Never write the bootloader. Check the device model and write ranges. Hold the device lock.
Use a foreground install or a monitored named session. Do not use `nohup` or an unattended install.
