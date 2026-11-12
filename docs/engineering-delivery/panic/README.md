# M1 panic completion

This note records the host behavior added on `lane/dev-m1-panic-complete-20261105` on top of the chord helper in `firmware/src/app/panic_chord.h`. The ROADMAP row is M1 in PR #812 (`496c87269349b70f9a900bd95b996d765aa51a80`): keyboard FX+SEQ, SMC BANK L+R in DAW only, no panic in MMC, all notes off across parts, transport stays running. L6 is only an alias of M1.

The helper already defines the chord. This lane wires that helper and clears sounding notes. It does not change the `.x0x` project format or `settings_t`.

## What the player gets

Holding FX and SEQ together asks every sounding part to release. Holding SMC BANK L and BANK R together does the same only while the controller is in DAW mode. Playback, tempo and the playheads keep going. The next new sequencer note still plays. A single FX, SEQ, BANK L or BANK R press keeps its existing action.

## ASSUMPTION

The ROADMAP text does not specify these details. Each one is the smallest behavior that still releases stuck notes without stopping the pattern.

- ASSUMPTION: Keyboard FX and SEQ still act on release. A finished chord consumes both releases, so neither release changes the screen. A key that is released before the other key is pressed still performs its normal action. There is no undo of a navigation that already happened.
- ASSUMPTION: SMC BANK L (`0x2E`) and BANK R (`0x2F`) still act on press. The first press keeps its current action, including drum select on a drum SEQ page. The second press is the panic and does not repeat that action. There is no undo of the first press.
- ASSUMPTION: Channel arrows `0x30` and `0x31` are not part of the chord.
- ASSUMPTION: An MMC bank command is one event, not a held key. It never panics and it does not leave a bank key looking held. User mode and MMC stay ineligible, including a raw Note On while the User preset is selected.
- ASSUMPTION: If a bank key is already held when the mode leaves DAW, panic stays blocked until both bank keys are up, even if the mode returns to DAW.
- ASSUMPTION: Connecting or disconnecting the SMC clears that chord. A project lock also clears it. A chord that was already held when the lock ends does not fire until the next press or release.
- ASSUMPTION: Panic sends Note Off only for bass notes the sequencer currently owns on its outputs. It does not send CC 123 and it does not send MIDI Stop (`0xFC`).
- ASSUMPTION: The 909 and 808 stop their sounding voices. Reverb and delay tails are left to ring. Pots, mixer levels, patterns and the project are not edited.
- ASSUMPTION: A tie that was holding the killed note does not start again. The next NOTE step still plays. Tempo, clock, song position and playheads stay where they are.
- ASSUMPTION: Note-ons already waiting in the audio command queue (drum, bass, record and break press) are dropped in that block. Parameter changes and note-offs in the same block still apply. MIDI already waiting in that block is silenced before the render. A step that comes due after that still plays.
- ASSUMPTION: If the existing record or stop pre-roll starts a step in the same block, that new note is silenced too. Later steps still play.
- ASSUMPTION: The Chicago damper pot stays as it is. Notes that the damper was holding are released anyway. A later live note can still be held by the damper.
- ASSUMPTION: A running arp is stopped. The arp on/off pot stays, so a later key can arp again.
- ASSUMPTION: On an SMK build, panic also requests the existing clip-key all-notes-off. That request does not stop the sequencer. A host link without the clip-key object uses an empty stand-in.
- ASSUMPTION: The keyboard chord works in every screen, including clip mode. HOME-hold and STOP-hold do not panic.
- ASSUMPTION: There is one keyboard chord state and one SMC chord state.

## Not run here

Device, native Elite, native Pro, controller and package runs are NOT_RUN. A host pass is not a device pass.
