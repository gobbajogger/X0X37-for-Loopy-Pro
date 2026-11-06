# LX-REQ-016 UI carry-forward (related LX-REQ-023)

Base c295af1554f9e80a29864cd013995eccad66cd2c, tree d98072ba1d689cb99bd16e4a6e1e682b22b04c70. Isolated detached worktree <isolated-candidate>. Parent delegates this patch only; existing UI/QA/integrator roles and sequencer mute lane remain assigned. No shared branch writes, merge, publication or devices.

Donors: 8be16486e33f65b31c76710e1d825b6aa701c2c9 and c32c54ac8ec68aaca53adccf7af66f155590f2dc. Reuse cc116395 + 25933a9b settings, display/snapshot portions of d7e2e833 peer names, de80f7b6 waveform cues and their tests. Do not replace complete ui.c/smc_mixer.c or take donor Chicago policy, DSP, transport, persistence or LCD dispatch changes.

Files: ui.c; main_fm1.c and host/x0x_host.c arrow-consumption callers; plat.h, plat_fm1.c, ble.c bounded peer snapshot; gfx.c clipped text; ui_original_wave.h; donor settings/peer tests and waveform test additions; test registrations. Resolve only contextual conflicts against the newer base. Expected state addition: waveform cache 156 bytes plus settings navigation and peer scroll; measure exact layout and distinguish native linker budgets from host sizes.

Proof: settings navigation/repeat/reset/scroll tests; peer names bounds/sanitizers/framebuffers; original synth waveform/program/preview tests; newer UI performance, question/reset, SMC, bounded-LCD regressions. Capture actual framebuffer outputs. Reuse existing donor presentation/source review receipts without claiming they qualify this combined candidate.

Policy read: main checkout AGENTS/workflow, nightly 66073c68dd80d51a7053432c2c161a8b720a1341 and coordination 4a35a6cdaa0f112db6b3f577542138afd170c20d. #860 latest QA closes actual capture verification but leaves design provenance and #859/#860 selection to existing UI/QA/integrator. No approved selection found. Preserve combined-base UI code; that selection is a separate unresolved integration decision.

Stop at a reviewable source patch, exact tree and test receipts with honest native RAM limits. Parent/existing integrator receives patch and routes independent QA and target qualification.
