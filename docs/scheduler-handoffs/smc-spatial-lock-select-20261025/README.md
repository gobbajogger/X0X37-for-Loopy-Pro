# SMC spatial screen and transport/mixer interactions

Base: `58e2c5205a02836df678083eaa8db9994b6192e8` (the flashed NIGHTLY).
Scope: reviewed source, host tests, actual native rendering and Elite DEV
BUILD_ONLY. No hardware operation, protected merge or publication.

The prior touched-value screen obscured the controller's spatial arrangement.
This revision keeps all eight strips visible, with each encoder above its fader
and the M/S/R/SELECT buttons beside it. Compact parameter labels and values stay
with their strip; a lower readout gives the full touched destination, value and
pickup state. The 240 by 240 layout preserves physical grouping without claiming
physical scale. Current and earlier native frames were inspected before editing.

Physical geometry was checked against the manufacturer's editor resource
`Resources/Pic/SmcMixer.png` and an independently inspected product photograph.
The resource SHA256 is
`e75c0bcd445558f98234ecb6d3da52a10ff23fcf32c96ef6574fe1e612a322e9`.
The private editor resource confirms eight horizontal strips, encoder above
fader, four buttons to its right and global controls below. The product photo
confirms M/S/R/SELECT vertical order. Proprietary images and editor binaries are
excluded from Git. Existing knob destinations remain unchanged.

PLAY toggles the existing start/stop operations, including a queued start.
DAW PLAY and SELECT reject held Note repeats and recover after dropped releases.
Selecting another part retains first-press sound-page selection. Pressing SELECT
for the selected 909/808 opens that kit's individual-sound mixer. In drum MIX,
selecting another sound selects it; pressing the selected sound again returns to
main MIX. Drum SEQ retains its existing sound-selection semantics. Navigation
does not trigger a temporary SMC overlay or change recording or transport.

PR150's reviewed target-capture mechanism at
`025fba5a777b74012bf0cc1746fda207482423bc` was adapted to the current distinct
knob destinations. Lock captures complete parameter, kit, bank, pattern and
song-bar identity while the temporary screen still expires. Locked SELECT and
other target-changing SMC navigation stay parked. Keyboard navigation does not
redirect captured controls; unlock follows the current UI with fresh pickup.
Clip lock entry is refused, and structural song edits invalidate captured bar
controls until unlock.

The physical Pause event and LED address for the replacement SMC remain
unproven. Existing manufacturer presets confirm PLAY `0x5E`, STOP `0x5D` and
REC `0x5F`; they cannot label an unknown physical Pause gesture on the current
preset. Historical note53/CC53 is unconfirmed. STOP remains STOP. The neutral
lock API is deliberately unbound, and no blinking/steady Pause LED scheduler or
physically reachable Pause feature is claimed. A separate private input-only
capture handoff is prepared; no raw capture is published.

## Validation

See [validation.json](validation.json) and the exact-source
[independent review](independent-review.json). Native production UI tests pass
4,866 assertions; lock/interactions pass 527 assertions plain and with sanitizers.
UI and interaction tests reject five and four runtime mutants respectively.
Independent QA additionally rejects four meaningful mutants and checks dropped
release recovery through both input queues. Existing mapping, pickup, feedback,
Solo, drum, clip, recording, performance abort and five-route reachability
regressions pass. Two review findings were repaired: lost releases could strand
edge-sensitive buttons, and locked headings could show live keyboard context.

Actual 240 by 240 main pickup, pan, second-bank and locked-context frames were
reviewed independently. Native render hashes are in the validation receipt;
three owner previews were uploaded privately through Library. They do not prove
physical LCD timing or hardware behavior.

Elite DEV native build and controller composition pass. The app is 551,900 bytes,
with 772 bytes code space, 316 bytes static RAM and 8,324 bytes pool free. The
pool minimum is 8,192 bytes. The unchanged controller is reused from verified
exact-input evidence. The private Elite bundle is 651,396 bytes, SHA256
`dfe4e0cd40c3d943be871975651a0c5ca7a26408d6cddf9b4c4c911d160021af`.
No Pro build or device validation is claimed for this shared source stage.
Code space is tight; a future physical binding and LED implementation requires
its own exact native build and budget check.

The next owner resolves Pause input/light in a separately authorized hardware
observation stage, binds only proven messages, adds the requested feedback, and
revalidates the affected source and native budgets before integration. The
integrator owns any later combined-tree QA and guarded NIGHTLY merge. Owner
preview review precedes rollout.
