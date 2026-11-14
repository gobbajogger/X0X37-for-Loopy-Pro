# Independent transport-contract design review

First consumed: `PLAN.md`, then firmware `engine.c:1635–1795` at source pin `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. PLAN SHA-256 at review: `4012063786c0388590ba9f20de16384ce8844157a5163d8f231219f34f6ae1e5`; engine source SHA-256: `9d8e011e5b87e2e82bf791e5a16f632aaa3f78a13f35c3b1c3f7ed91cb62bde1`.

The plain fake-only diagnostic fixture is suitable for this stage. It exposes local transport state and wire bytes without suggesting a redesigned or deployed remote interface. There is no blocker to implementation under these conditions:

- A send exception must produce a sticky unconfirmed-delivery/release warning. A later successful cleanup, re-arm or replacement must not silently turn that into hardware success.
- Endpoint replacement must clean the original route before assigning the replacement; all cleanup attempts retain original note/control channels.
- A failure on one cleanup send must not prevent independent remaining releases. Stale timer, close and incoming callbacks must not affect a replacement connection; capture a route generation or remove callbacks reliably.
- Incoming observations are bounded and channel scoped, cause no outgoing send, and establish neither ACK nor hardware identity. Clear observations on lifecycle changes.
- The fixture must label connection state as a fake endpoint state. Physical MIDI, Web MIDI permissions and device state remain untested.
- F2 Song Position must remain rejected as a sequencer positioning control: `sequencer.c:1171–1183` handles only F8/FA/FB/FC; `seq_clk_take:1218–1219` only filters its source. `engine.c:1665–1666` updates clip position observation, not the sequencer playhead.

Notes/CC1/CC64/bend remain conditional on firmware routing, clip/SMC modes, channel assignment, mute and record state. The fake fixture must not present an emitted message as confirmed musical effect. Safety Stop after a locally issued Start/Continue is reasonable only as an explicitly documented cleanup attempt; it cannot establish exclusive device clock ownership.

This is an independent source/design review, not Sol, formal GitHub approval, native browser acceptance or device proof. No implementation tests had run at this checkpoint.
