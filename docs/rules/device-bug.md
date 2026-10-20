# Device bug

### Rule 8: evidence and regression

Trust the owner's report and setup. Search code first. Use logs, source evidence,
and meaningful host regression tests. For controller-input diagnosis and regression,
reuse recorded captures and simulated sends; do not request another hands-on
capture or ask the owner to move controls. An idle zero-packet capture does not
prove a receive failure. Label exact recorded payload replay, simulated HCI
framing, inputs resegmented for a supported MTU, and physical-device evidence
separately. Replay or simulation does not establish a physical-device PASS.
For other device bugs, request INLOG only when existing evidence cannot resolve
uncertainty; do not demand another reproduction when evidence proves the cause.
Record an explicit owner skip when the applicable check requires it.

Where possible, a regression fails on the broken base and passes after repair.
If a host cannot reproduce it, explain the limit and use available evidence.
Finish independent work without hardware; device result stays NOT TESTED until
reported by the owner.

After device failure, name the failed assumption and get missing data before
another fix. After two consecutive failures, use last-good evidence and bisect
only with safe bounded tests. Never overwrite current work or install an
unapproved fallback.

Preserve notes, pad colour updates, and the Bluetooth connection. Owner fact:
"Optimize without cutting features. Font size is not a feature." (2026-10-16 18:41).
