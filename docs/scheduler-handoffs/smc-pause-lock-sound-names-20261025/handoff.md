# Captured Pause lock and drum SEQ sound names

Physical Pause in the retained owner gesture window is NoteOn channel1 note0x5D (press127, release0). Seven pairs, first and last0x5D, with one accidental0x5E pair; thirty retained baseline records overlap and host/device elapsed time agrees within about12ms. This identifies the current owner input route, not a named preset or LED address.

The physical NoteOn0x5D route now toggles captured SMC target-page lock once per press. Held positive duplicates and release do not retoggle, and no page or persistent overlay opens. Play remains play/pause. User MMC34 keeps transport STOP because the retained capture established NoteOn input only. Separate producer-owned 32-bit Pause release-loss generations prevent unrelated dropped Solo/Select/Play releases from rearming held Pause. Lock survives link loss/reconnect, while project reset clears it.

Drum SEQ strip titles read the existing authoritative engine voice-name mapping for the displayed/captured kit and bank. 909 bank2 is OH/CR/RD;808 bank2 is CY/OH/CH. Empty positions remain--. Drum MIX and other mode labels retain their existing presentation. Selected-title highlights, action captions, knob arcs, mappings and omitted footer are preserved. Knob-bank remapping remains design-only.

## Source checks

Independent ASan/UBSan production UI5893, lock703 and Pause-loss397 checks pass; feedback1877 expectations pass. Eighteen affected native-host regressions cover mapping/buttons/faders/filter, queues/link/malformed-data/sender isolation, Solo overflow, both drum paths, clip visibility, performance abort, record feedback, part selection and reachability. Four new physical-Pause runtime mutations and four existing interaction mutations are detected; the UI suite detects fourteen runtime mutations. Compiler failures do not qualify as negative controls. Independent loss/backpressure tests cover both queues, both release forms, unrelated losses, actual lost Pause release,32-bit generation wrap and retry/cache behavior. Direct fader capture is independently equivalent for4080 assertions because those targets are only engine refs orNONE.

Value formatting is shared to reduce code without changing pixels. Independent before/after renders match all25 native frames under the identical generated firmware font. Previews are privately retained; framebuffer/source checks do not establish physical LCD or LED behavior.

## Native gate and next owner

This source is BUILD_BLOCKED: both native Elite and Pro DEV links exceed XIP by108bytes. No application/bundle qualifies for flash, and post-link RAM/pool budget checks did not complete. No behavior was removed to fit. The next source owner must recover the remaining108bytes of code space with equivalent behavior and repeat exact affected checks and model budgets before delivering any candidate to the separate hardware owner.

The existing supported Mackie output route emits90 5D 7F/00. It now requests512ms blink phases while following, steady127 when locked; successful queue transmission is not proof of the physical Pause lamp address. After a valid exact candidate is ready, the authorized native hardware owner must observe that the Pause glyph actually responds, check at least two unlocked blink cycles, steady lock over the same interval, unlock/release/duplicate-press behavior, Play, target capture after keyboard navigation, and reconnect state replay. Record exact model, app/controller/package bytes and preset scope. Do not repeat the already retained input gesture merely to rediscover0x5D.

No Actions tests, device/GUI access, flash, merge, public binary or permission changes occurred. Controller source is unchanged; no new controller pairing/installability claim is made. This handoff does not authorize those later actions.
