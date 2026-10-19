# Install and release

### Rule 5: exact artifacts and owner data

The designated installer owns device writes (Firmware Main Builder unless
currently reassigned). Before writing, use approved artifacts, back up loop
slots, and send the owner a short notice. Log commit, gate result, configuration,
model, and CRCs in the install ledger. Read back the image and compare each
loop slot CRC after install. Report mismatches at once; claim preservation
only with those checks.

For machine transfer, use Git or bundles to preserve commits and include
required gate records/logs. No box transfer is needed for local build/device
access. Exclude proprietary JieLi files.

### Rule 6: finished set and release evidence

Install only within authorized scope with every queued change included. Use
exact gated artifacts/configuration with zero failed or skipped checks.
Gate records and CRC manifests, not agent messages, prove readiness.
Use `tools/fm1_install.py --commit` and `tools/require_gate_pass.sh`; preserve
configuration, queue, model, artifact, and data guards. No team web install
or bypass of a refusal. Missing device or valid PASS leaves install pending.

A public release or release package requires the formal gate and owner's device
PASS for the same
build. Set `X0X_DEV_PADCAL=0`; keep model-specific packages and licensing files.
Publish only on owner request. Exclude `refs-oscuro` and proprietary SDK/toolchain.

### Rule 7: safe writes

Never write the bootloader. Check model and write ranges. Hold the device lock.
Run in the foreground or a named watched session; no `nohup` or unattended install.
