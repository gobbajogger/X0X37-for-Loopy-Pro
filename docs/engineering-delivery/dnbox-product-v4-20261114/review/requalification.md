# Changed-source requalification

2026-11-14. Independent changed-only pass against frozen `/workspace/dnbox-v4/site`, actual Chromium/Playwright at `http://127.0.0.1:8774/site/`. This supplements `independent-product-review.md`; the unchanged 28-route/24-render sweep was not repeated.

**Disposition: PASS for bounded NIGHTLY product-website preview testing.** All four initial correctness findings are resolved. No new changed-scope blocker found. The website presents the firmware's concrete musical proposition, installation route, file workflows and honest status of unavailable tryout/companion. This does not accept a playable browser emulator, a public hardware companion, physical hardware behaviour or live-origin Chromium operation.

Verified in real Chromium:

- R1 fixed: zero page exceptions in this pass. Initial home heading focus is H1 at desktop and 390px. Returning from Files focuses home H1 at scrollY 0. Native recording playback advances; opening mobile menu pauses it. Navigation closes menu.
- Existing gallery buttons retained: Pause enables while recording plays; Mute sets audio.muted, aria-pressed=true and “Unmute recording”; second press restores unmuted/false; Pause stops playback and disables itself.
- R2 fixed: actual `#help/recover` app/partial option changes now explain portable included samples versus project-only files needing matching samples. Both phone renders inspected.
- R3 fixed: home caption explicitly assigns audio import to sample tools and slice sequencing/playback to firmware.
- R4 fixed: session copy describes Chicago's octave-above pulse against Miami's repeated low notes; the score supports that statement.
- Home now gives Chicago a 303-style character and Miami a wobble role. Shorter illustration labels preserve truthful recording/illustration distinctions. Changed desktop/390px home renders inspected; no horizontal overflow.
- Files preset status now targets `https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/915`; file-only/no-device/no-public-deployment limits retained.
- Companion detail is discoverable and opens correctly on phone. It names CC1 modulation, CC64 damper, bend and routed clock/transport; excludes arbitrary mixer CCs, preset selection, identity acknowledgement and complete state feedback; links exact immutable e2d36dba… capability contract. No overflow in opened detail.

Evidence: `recheck-results.json`, `recheck-1440-home.png`, `recheck-390-home.png`, `recheck-390-recovery-app.png`, `recheck-390-recovery-partial.png`, `recheck-390-companion-details.png`, runnable `recheck.cjs`. Complete new source identity in `rechecked-source.json`.

Only three site files changed since initial review:

| File | Reviewed SHA-256 |
| --- | --- |
| app.js | 3a151bd45904269c3cbc3716d03be8e1f78d4ce3545b3d09b5a933a333fc162d |
| experience.js | 35cf4c30112afa6c48f582b337f15e621b6b5066c6608cd883a715b1288cdc96 |
| product.js | 9d4e92c201bb794d58a8c0f4459da0d425d13c7edc290f57712978ae11592b63 |

No product source, firmware, emulator, codec, MIDI adapter, installer or deployment was edited by this reviewer. No MIDI permission, device connection, flash, TLS trust change or certificate bypass was attempted. Initial live-origin Chromium certificate limit remains; verified-TLS HTTP evidence remains distinct from browser journey acceptance. Genuine public playable tryout and hardware companion remain unavailable. Richer authentic musical comparisons remain an opportunity rather than a claimed delivered capability.
