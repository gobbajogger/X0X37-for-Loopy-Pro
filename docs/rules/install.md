# Install and release

Read before an install, after an install, or before a release. The designated installer owns device writes. Firmware Main Builder is the default role; a current explicit owner assignment can name another installer.

### Rule 5: data and exact artifacts

Before a device write, use the approved built artifacts, back up loop slots, and send the owner a short notice. Keep the commit, gate result, configuration, model, and CRC evidence in the install ledger. After the install, read the image back and compare each loop slot CRC. Report a mismatch at once. Do not claim preserved slots without the checks.

Move commits by Git or a bundle when a second machine needs them. Preserve hashes; do not require a box transfer when the approved build and device access are already local. Transfer the gate records and logs that the guard needs. Do not bundle proprietary JieLi files.

### Rule 6: finished set and evidence

Install only when the authorized task includes it and the queued set is complete. The install commit contains every queued change. Use the exact artifacts and configuration covered by the gate PASS, with zero failed or skipped checks. The formal gate result and CRC manifest, not an agent's message, are the evidence.

Use `tools/fm1_install.py --commit` and `tools/require_gate_pass.sh`. Keep their configuration, queue, target model, artifact, and data guards. A team install does not use the web installer. Do not bypass a refusal. A missing device or valid PASS leaves install pending and does not prevent completion of a local task.

A public release or package for release needs the required formal gate and the owner's device PASS for the same build. Release builds use `X0X_DEV_PADCAL=0`. Keep model-specific packages and licensing files. Publish only when the owner asks. Never package `refs-oscuro` or proprietary SDK/toolchain files.

### Rule 7: safe device writes

Never write the bootloader. Check the device model and write ranges. Serialize hardware access with the device lock. Run the installer in the foreground or a named watched session. Never use `nohup` or an unattended background install.
