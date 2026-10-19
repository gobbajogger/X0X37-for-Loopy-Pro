# Device bug

Read after a device bug report or failed device test.

### Rule 8: evidence and regression

Trust the owner's report and setup. Search project code for the cause first. Use available device logs, source evidence, and a meaningful host regression test. Request an INLOG trace when it is needed to resolve uncertainty. If the existing evidence proves the cause, do not require the owner to reproduce it again just to satisfy an old procedure. Record an explicit owner skip when the applicable check requires it.

Prove a code fix with the relevant test where possible. A new regression test should fail on the broken base and pass after the fix. If the failure cannot run on a host, explain that limit and use the best available evidence. Do independent repair work while hardware is unavailable. Keep the device result NOT TESTED until the owner reports it.

If a fix fails on the device, name the failed assumption and seek the missing data before another guess. After two consecutive device failures, use last-good device evidence and bisect when a safe bounded test can distinguish the cause. Never overwrite current work or install an unapproved fallback to perform that diagnosis.

Preserve notes, pad colour updates, and the Bluetooth connection. Owner fact: "Optimize without cutting features. Font size is not a feature." (2026-10-16 18:41).
