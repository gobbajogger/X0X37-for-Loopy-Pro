# TouchOSC feedback repair continuation

This bundle preserves the repaired layout, all six source files, GPL licence,
original qualification/input archives, affected tests, actual app-state and
loopback packet evidence, independent review, and the exact successor brief.
Verify `manifest.json` before extraction, then verify `evidence-manifest.json`
inside the ZIP. No screenshots, credentials or third-party runtime binaries
are included. Download genuine TouchOSC separately from Hexler when needed.

The repair clears old incoming highlights and feedback on init/disarm,
including RELEASE, both channel controls and a local held-note timeout.
Eight new Lua feedback scenarios, four packing checks,206 repaired `.tosc`
app comparisons and eight XML comparisons pass. The baseline's stale-feedback
case was reproduced; its separate dirty-flag assertion failed and is retained.
Independent review approves this bounded repair. The lost Note Off under a
loopback receiver outage remains an adverse transport limitation.

Overall companion qualification remains partial. Actual endpoint lifecycle,
independent-touch held-note RELEASE/channel changes, multitouch, and
outside-release/cancel/focus-loss behavior remain unqualified. Use virtual MIDI
only, preserve release-before-route-change guidance and existing release
approvals, and do not infer disconnected delivery or hardware acceptance.

The producer's PR/final receipt pins the delivered commit. The ZIP's
`successor-handoff.md` gives the next actual runtime stage and consumption
check. No fresh Work/Codex successor was created by this worker because no
top-level creation/result-reader tool was exposed; the parent must create it
and verify its first real scoped action. A review subagent is not a successor.
