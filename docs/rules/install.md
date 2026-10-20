# Install and release

### Rule 5: exact artifacts and owner data

The designated installer owns device writes (Firmware Main Builder unless
currently reassigned). Before writing, use approved artifacts, back up loop
slots, and send the owner a short notice. Log commit, validation record, configuration,
model, and CRCs in the install ledger. Read back the image and compare each
loop slot CRC after install. Report mismatches at once; claim preservation
only with those checks.

For machine transfer, use Git or bundles to preserve commits and include
required gate records/logs. No box transfer is needed for local build/device
access. Exclude proprietary JieLi files.

### Rule 6: finished set and release evidence

Never install a fallback just to make the keyboard playable.
Install only within authorized scope with every queued change included. Use
exact validated artifacts/configuration with no failed required checks.
For development installs, use `tools/dev_validate.py` with a justified plan of
affected checks and the intended model. Its DEV CHECKED record covers the build,
code/RAM/pool limits, selected checks, package, and selected controller. Pass it
with `tools/fm1_install.py --commit HASH --dev-record FILE --config dev` and
explicit `--ctl FILE` or `--no-ctl`; never omit a needed controller to save checks.
Reuse matching evidence for repeated installs. No full gate is required merely
because a development build will be installed. Broaden checks only for affected
dependencies or a concrete risk; explain the reason in the plan.

Release installs require the full gate. `tools/require_gate_pass.sh` accepts
development evidence only for dev configuration; it never creates a release PASS.
Preserve configuration, queue, model, artifact, and data guards. No team web
install or bypass of a refusal. Missing device or applicable validation leaves
install pending.

A public release or release package requires the formal gate and owner's device
PASS for the same
build. Set `X0X_DEV_PADCAL=0`; keep model-specific packages and licensing files.
Publish only on owner request. Exclude `refs-oscuro` and proprietary SDK/toolchain.

### Rule 7: safe writes

Never write the bootloader. Check model and write ranges. Hold the device lock.
Run in the foreground or a named watched session; no `nohup` or unattended install.
