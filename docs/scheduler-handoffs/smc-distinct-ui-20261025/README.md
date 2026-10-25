# Distinct SMC controls and quiet page navigation

Reason: populated mixer knobs duplicated faders, while unchanged absolute fader
packets could reopen the temporary screen after SMK page navigation. The musician
needs useful independent controls and a clear indication of the touched destination.

Purpose/design: preserve every fader, channel, acceleration and button/transport
mapping. HOME/MIX PARTS knobs add part pan, reverb decay, delay feedback and filter
resonance; drum MIX/SEQ knobs find the matching voice Pan descriptor in the active
bank. GLOBAL/PERFORM K3 and MIX/MASTER K4 use resonance. Other ordinary rows already
owned by any fader park with ON FADER n feedback instead of a substituted parameter.
Specialized clip editors retain their existing field ownership.

The focused destination/value now occupies the main display, with real page or
kit/bank context, explicit pickup direction/target versus ACTIVE, and a smaller
quiet overview. It uses existing fonts/rendering and bounded stack buffers, with
no graphics allocation and the same 1,200 ms inactivity timeout.

Production-path reproduction: panel navigation and smc_frame alone are silent.
After navigation, an unchanged absolute packet previously reached unconditional
smc_show/smc_ui_begin even without changing a parameter. Pickup history is reset
on target changes, so it cannot establish fresh physical activity. One physical
history bit per strip now survives pickup refresh; repeated samples still seed
normal pickup crossing history but cannot open/extend the screen. Nonzero relative
turns and real changed fader samples retain feedback. Hardware echo is a hypothesis,
not an observed cause of a particular physical-device packet.

Expected benefit: independent mix dimensions, readable touched-control feedback,
and no overlay flash from the reproduced unchanged-sample navigation sequence.
Host/frame/build results are recorded alongside this file; device behavior needs
separate owner testing. No hardware, raw capture, publication or protected merge
is performed by this source/build stage.

## Inputs and boundaries

Accepted base NIGHTLY: 93449136baae02c9715ba20edc5c43176786b635, refreshed before
editing. PR150 remains at 025fba5a777b74012bf0cc1746fda207482423bc; its unbound Pause
lock is not integrated or activated. Future integration must snapshot
smc_knob_target independently from smc_target and preserve full pref_t identity,
kit/bank/sequence/song context and the physical-history/presentation distinction.
Do not copy fader identities into knob slots. Re-run exact locked/unlocked maps,
parked-row and navigation-provenance checks on the combined source. Physical Pause
input and LED binding require their separately owned evidence.

PR122 b037edac22f914bd14c6ca7aaa2988a530abd13d routing changes remain separate.
Current direct BLE SMC topology is unchanged; no draft multi-peer proposal is
imported. No controller/ABI/build/package-source changes are intended.

Workflow: reviewed trial e2482a5039527195e346925c751ac3e3a333ada9,
docs/engineering-delivery/SKILL.md SHA256
2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3.
Native method/input evidence is reused from task17; unchanged matching controller
SHA256 956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a
is reused after exact closure checks, then fresh applications are composed and
checked per model. CTL_NO_CACHE=1 remains set (installer cache protection, not a
compiler-cache flag). This candidate is BUILD_ONLY until the relevant later gates.
