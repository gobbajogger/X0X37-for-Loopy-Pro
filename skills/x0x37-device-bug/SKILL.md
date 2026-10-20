---
name: x0x37-device-bug
description: "Diagnose an x0x37 device or controller-input bug using recorded captures, simulated sends, source evidence, and regression tests."
---

Initialize this skill when its description matches the current step. Read it once per task.
Load linked skills only when their described step applies. Current owner instructions take priority.

# Device bug

### Rule 8: evidence and regression

Trust the owner's report and setup. Search the code first.
Use logs, source evidence, and meaningful host regression tests.

For controller-input diagnosis and regression, reuse recorded captures and simulated sends.
Do not request another hands-on capture or ask the owner to move controls.

An idle capture with zero packets does not prove a receive failure.
Identify these evidence types separately: exact recorded payload replay, simulated HCI framing, MTU resegmentation, and physical-device tests.

Replay and simulation do not establish a physical-device PASS.

For other device bugs, request INLOG only when existing evidence cannot resolve the uncertainty.
Do not request another reproduction when the evidence proves the cause.

Record an explicit owner skip when the applicable check requires it.

Where possible, make the regression fail on the broken base and pass after the repair.
If the host cannot reproduce the defect, state that limit and use available evidence.

Complete independent work without hardware. Keep the device result NOT TESTED until the owner reports it.

After a device failure, identify the failed assumption. Get missing data before another fix.
After two consecutive failures, use last-good evidence and bisect with safe, bounded tests.

Never overwrite current work or install an unapproved fallback.
Preserve notes, pad colour updates, and the Bluetooth connection.

Owner instruction, 2026-10-16 18:41: "Optimize without cutting features. Font size is not a feature."
