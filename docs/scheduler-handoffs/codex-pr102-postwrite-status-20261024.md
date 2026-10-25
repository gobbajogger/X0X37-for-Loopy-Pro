# Preserve installer restoration result during reconnect

Source commit ec73abf78f751648fc8f5515811657552faafc26, ROOT df13c066d01c81c00047cc68ccf2490cdf5b0743; parent PR102 source 22d522a4cde4c18f92a846bc40d504b701cf6d17, ROOT 041d51d6563cf8b2f458a07787a1a88c8a43a96d.

The complete parent checkout reproduces one failure in the real Node slot backup/OTA/restore suite: postwrite scan(true) calls suggestModel(), which launches loadSelectedPackage() and replaces the final restoration result with package-selection status. Preserve the selected package during postwrite reconnect by calling suggestModel only during ordinary scans. Manual model suggestion and connection identity updates remain exercised. Register the existing 30-case product-identity suite in the full runner; it was otherwise omitted.

Fail-before parent exit 1, repaired full registered suite exit 0 using X0X_TEST_TIME_SCALE=20. This explicitly scales long waits; it is not the real-time pre-install gate. Raw logs, exact commands, SHA256 inventory and immutable trees: evidence/pr102-postwrite-status-20261024/receipt.json. These are mocked-device/AST tests from a complete checkout; they establish neither real USB continuity nor device PASS. Firmware, BLE and assets trees equal parent; no target compilation required for this browser-only change. No binary, public artifact, protected branch, site or device was written.

## Existing receiving owners and pending packet

Deduplication key: PR102-ec73abf78f751648fc8f5515811657552faafc26-postwrite-status-Installer-QA.

This is a narrow review proposal targeting the existing installer producer lane, not a new integrator or publisher. Existing installer owner consumes/reconciles the patch; sole Independent QA rechecks exact candidate, full Node suite, failed and successful restoration disposition and manual model selection. Safe Merge alone integrates firmware nightly; separate website source owner reconciles publisher-approved download handling into website PR6 while preserving both catalog and installer test suites. Hardware identity, full combined source, lawful artifacts and immutable HTTP remain downstream blockers, not waived here.

Bounded receiving prompt: read current installed Classic Build policy and live AGENTS/BUILDING/licensing; independently refresh PR102 and this proposal heads and exact changed files. Verify the source patch and run the receipt commands on the full tree. Confirm final success/partial/failure/backup filenames survive reconnect, that ordinary scans still suggest the correct model, and that package changes/revocations still disable installs. Review both language strings and relevant browser checks. Consume only through the existing producer ownership and subsequent independent QA/sole integration gates. Do not merge protected refs or publish/install firmware on this evidence. Deliver exact SHA/root acceptance and the next verified noncompeting successor under current policy.

The receiving producer and independent QA already exist; retain this packet rather than launch a duplicate writer. No fresh user-accessible Codex task was opened. The supported Cloud account route previously returned 401; no new login flow or local session is claimed. NEXT CODEX THREAD NOT OPENED — HANDOFF READY; existing Git consumers can use this review packet without a new thread.
