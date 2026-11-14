# DnBOX whole-instrument design

[R7 PDF: actual-current / proposed workflow comparisons](r7/DnBOX-R7-owner-comparisons.pdf) · [Native PNGs](r7/native/) · [Interaction walkthrough](r7/workbench.html) · [Whole design and remaining work](r7/WHOLE-INSTRUMENT-DESIGN.md). The standard PDF and native images are the owner-facing presentation; HTML is the interaction study. Actual iOS-device opening remains untested.

R7 adds direct select-only GEN correction, preserving current generator controls and the native large held-note editor. It removes the SEQ-and-ARP detour: two view presses. It also fixes three prototype omissions in held-step readability, stopped modifiers/releases and physical-key context. These repairs preserve existing firmware behavior. Seven connected model paths and15 curated comparisons remain useful WIP, not completed whole-instrument acceptance or implementation.

Current before captures execute unchanged3c41/0.0.26 Elite; accepted d158 shares relevant UI files and was not executed here. R7 pixels and interactions are proposals. The original Settings component and historical25.x pairs remain separately scoped. Exact source, native-image and tested/not-run identities are in [R7 readiness](r7/READINESS-AND-LIMITS.json) and [manifest](r7/ARTIFACT-MANIFEST.json). The successor needs the existing independent affected assessment.

The full overhaul continues across FX/controller, valid BREAK, real connected use, running/poly capture, presets, recording/recovery interfaces, comparator adoption and whole audible musical acceptance. Earlier revisions remain frozen history; they are not the current solution. No firmware implementation, build, merge, device operation or release follows from this design delivery.
