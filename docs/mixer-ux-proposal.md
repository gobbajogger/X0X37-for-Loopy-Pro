# Mixer UX proposal

This is a proposal, not an implemented render. Base: e888d046.

Reuse the eight-strip temporary SMC presentation from PR #168. Keep the selected lane title, physical M/S/R positions, true parameter values and underlying screen ownership. Add a dynamic `Knobs` bank caption and colour the fader travel from minimum to its target. Pickup direction remains visible and its physical marker follows musical phase; stopped/waiting feedback stays steady.

Up/Down change a separate knob bank. Main MIX/HOME banks offer cutoff, pan, reverb send and delay send; drum banks offer pan, cutoff and sends where the voice exposes them. Ordinary pages expose successive groups of visible parameters, filtering all active fader destinations. Keyboard page selection stays independent.

Reuse eighth-strip actions: repeated R8 cycles HOME/SONG; repeated S8 cycles the selected part's PART/SEQ/GEN views (GEN only for synths); repeated SELECT8 cycles main MIX pages. Left/Right retains ordinary page selection. Existing reverb/delay shortcuts and FX holds retain their recovered mappings. Do not invent a complete new FX button map: item 41 remains partial until its agreed map is recovered.

Context lock captures faders/knobs while context buttons and recording remain usable. Navigation does not redirect captured controls. Explicit drum bank navigation recaptures that bank's faders/knobs and rearms pickup while preserving held Solo/FX owners. Releases finish the press-owned Solo/FX action even after navigation; repeats cannot acquire a different action. User-preset messages are ignored at mixer dispatch, without connection-layer edits.

Temporary feedback stays open for its peer's accepted held action, including Rec and Mute. All note actions suppress repeated presses until release. Existing lost-release handling must clear this ownership too. Release starts the 1200 ms idle interval; panel navigation, confirmation and project changes still cancel immediately. No synthetic fader-touch detection is claimed.

Pattern arrows accumulate from the pending cue and clear it when they return to the playing pattern. Unmute adopts a known physical fader position once only when that fader's live or captured destination matches the unmuted level, and keeps pickup acquired. Unknown physical positions retain normal crossing pickup. In particular master output M8 must not adopt the filter fader's position.

The explicit known FX assignment is Select = low-pass, only in FX. Preserve the existing recovered effect banks for now, but do not claim their configurable Select gesture as the complete requested FX page; item 41 remains partial. Do not add undocumented Rec/Solo effect assignments to compensate.

Reuse the audio-side metronome observer and tests inherited from PR #638 (already present in this base). The display uses its atomic phase byte: neutral in STOP/WAIT, reset on Start and preserved on Continue. Storage revision stays 13; engines and controller connections are outside this change.

Validation: production UI fixture per item, actual candidate render inspection, full requested host runner and Docker Elite development build with original synths. No device action or push.

Eighth-strip precedence: populated drum positions retain drum actions; empty bank-2 positions remain inactive. Ordinary eighth-strip cycling applies outside those contexts. FX Select retains FX press/release ownership. Extend app-side release-loss classification to all tracked note holds.

Visual proposal: [eight-strip mockup](mixer-ux-proposal.svg). This sketches caption and travel changes on the reused layout; it is not a firmware screenshot. Independent design review resolved the hold-loss, Unmute destination, FX ownership and bank-recapture findings before implementation.
