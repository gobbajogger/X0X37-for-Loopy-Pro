# Temporary SMC control screen

Build: development source based on accepted firmware `nightly` at
`54f2ffbc3124f7449a69067dc8215a2d91a2e675`.
Replaces: the SMC fader/encoder notice in the yellow header.
Checked against decisions: the owner's request for a full control UI followed by
the exact previous UI, and the maintained SMC mapping and pickup behavior.

Prior work inspected: PR #13 at `d3f4269c19d5d3597a6185e414ed6c8dc9b30c05`
and its menu/help/clip cancellation follow-up, PR #19 at
`2b310479c73c1c7dad8feb64c94109fd757c6110`. Their inactivity and navigation
cases informed this implementation. Those branches still emitted header notices
and were not merged into this lane.

## UX and state plan

The screen shows the destinations and current values of the controls the SMC
actually operates. Fader feedback distinguishes waiting for pickup from acquired
pickup. Main mix, drum banks, visible sound/menu fields and clip editor controls
keep their existing mappings; unused controls remain inactive.

The temporary screen is presentation state. It does not navigate to MIX or FX,
rewrite a page index, or save and later replay a copy of the application state.
The underlying screen, selection, menu and clip state remain authoritative.
Dismissal redraws that state, including the header and lower control strip.

Valid SMC activity starts or extends a 1200 ms inactivity window. This is an
initial implementation choice inherited from the earlier peek work, not an owner
specified duration. Panel input and intentional SMC navigation dismiss the screen
immediately. There is no delayed navigation restoration to overwrite that input.
Project reset or blocked controls end temporary presentation.

Confirmation retains its input lock. Help retains its card and existing control
behavior. Menu controls retain their visible-row destinations. Clip faders retain
the main mixer; clip encoders retain the visible editor, and the clip grid consumes
inactive encoder input. Screen presentation must not leak edits to a hidden page.

## Verification scope

Use real production UI and SMC C in the host simulator. Check live values, pickup,
repeated activity, timer boundaries and rollover, navigation cancellation, menus,
help, confirmations, clip transitions, reset and full framebuffer restoration.
Include deliberately broken variants so the tests demonstrate the relevant
failure. Replay affected SMC mapping and routing regressions. Review separately
from implementation. Produce private Elite/Pro development BUILD_ONLY evidence
where local pinned inputs are available.

Host/render results do not prove physical LCD timing, radio behavior, hardware,
installability or production readiness. No MIDI ports, hardware flashing, release
publication or protected branch integration are part of this change.
