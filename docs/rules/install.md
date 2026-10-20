# Install and release

### Rule 5: exact artifacts and owner data

The designated installer owns device writes. This is Firmware Main Builder unless ownership has transferred.
Use the standing development install permission in [rule 14](owner-messages.md). Do not ask for it again.

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

### Rule 6: finished set and release evidence

Install the complete requested set only after all queued changes and required checks are complete.
Use the exact validated artifacts and configuration. No required check can remain failed.

Never install a fallback only to make the keyboard playable.

Use development configuration by default: `X0X_DEV_PADCAL=1`.
For a development install:

1. Use `tools/dev_validate.py` with the intended model and a justified plan of affected checks.
2. Use its DEV CHECKED record for the build, budgets, selected checks, package, and controller.
3. Pass `--commit HASH --dev-record FILE --config dev` to `tools/fm1_install.py`.
4. Select the controller explicitly with `--ctl FILE` or `--no-ctl` as required by the validated package.

Never omit a required controller to avoid checks. Reuse matching evidence for repeated installs.
Skip the full gate for development work under [rule 11](integration.md).

Add checks only for an affected dependency or a concrete risk. State that reason in the plan.

A release install needs an explicit release-build request and the required full-gate result.
`tools/require_gate_pass.sh` accepts development evidence only for development configuration. It cannot create a release PASS.

Preserve configuration, queue, model, artifact, and data guards. Do not use a team web install or bypass a refusal.
A missing device or required validation blocks installation. State that dependency, not a new approval requirement.

A public release or release package needs the formal gate and the owner's device PASS for the same build.
Set `X0X_DEV_PADCAL=0`. Include model-specific packages and licensing files.

Publish only on owner request. Exclude `refs-oscuro` and proprietary SDK and toolchain files.

### Rule 7: safe writes

Never write the bootloader. Check the device model and write ranges. Hold the device lock.
Run the install in the foreground or a named session that you monitor.

Do not use `nohup` or an unattended install.
