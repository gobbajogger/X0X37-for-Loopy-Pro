# DnBOX R3: workflow first

**The owner rejected a8986ac2's design direction.** This is a new post-rejection study, not approval to implement that direction and not whole-design acceptance.

Start with [the six-page PDF](DnBOX-R3-workflow-comparison.pdf). It embeds the native images and is a standard PDF intended for iOS viewing; actual iOS-device opening has not been tested. The separate PNGs open directly:

- [Drum sequencer](native/drum.png): percussion pictures, full grid, retained length/rate and mute route.
- [Chicago overview](native/chicago.png): four neighboring groups and fixed knob columns.
- [Home](native/home.png) and [external WAIT](native/wait.png): retain pattern/part context and PERFORM.
- [Song neighbors](native/song-neighbors.png): retain the table and both BAR control pages.
- [808 sound-switch snapshot](native/808variant.png): CONGA/CLAVE/MARAC identity pictures.

The [interactive workbench](workbench.html) is for browser validation. Downloading HTML is no longer the only owner-facing delivery. It is an input/display model with no firmware, audio or storage execution. Native frame inventory remains separate in frame-manifest.json; it is not a contact sheet of design alternatives.

## What changes and what stays

Recognizable drum pictures replace abbreviations in the existing row-label space. A larger selected-instrument picture/full name uses only the formerly unmapped K7/K8 display area; those knobs remain unmapped. Every row, step, length/rate control and direct key/pad/mute route remains.

The focused parameter replacement is withdrawn. Keep the current four-row/four-column overview, meaningful glyph/readout families, sparse physical columns, K2/arrow wrap, PARA cycle, SCALE deep list and PATCH return. Chicago8/Miami6/Juno5/Drone3 full cycles include SOUND. Keep Home's sixteen pattern tiles, five lanes and all four performance controls. Keep Song neighbors, both BAR pages, BREAK controls, MIX MASTER/WAVE and FX sends. No improvement over current parameter access is claimed merely from restoring it.

The current source also defines a useful coordinated light interface. The37 keybed has no LEDs; sixteen RGB pads show selected-part content, playhead inversion and held-input priority. Mute identity remains on the LCD. REC is separate from PLAY; SMK PLAY-red is unused. Home pad cue, LCD cue and WAIT have different source frame phases. No new held/mute/REC pad colour or host-success meaning is invented.

## Exact evidence and limits

Current source: `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`, tree `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d`. Detailed engine/drum/light maps and exact file blobs are delivered beside this README. The revision retains all39 R22 journeys and261 routing rows, with35 source-derived access baselines. Planned retained-route count delta is zero; actual task speed, discovery and insignificant-tradeoff claims remain unqualified.

The four PDF reference captures execute historical package0.0.25/source79280e76, requested/displayed as25.1. Their package hash is `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a`. They are not current-candidate pixels. The parent reports fresh current Elite/Pro AudioWorklet exercise, including clip LOAD advertising masked SCALE YES and held Chicago silence on Global entry. The exact stage1534 report/launch/capture bytes are not accessible in this workspace yet; this packet does not claim their byte-level intake or a corrected runtime.

The final proposal has33 scoped model checks passing, with all native text bounds inside240×240. These checks cover page topology, held-lock versus base scope, direct performance control retention and source-derived light snapshots. PDF parsing/rendering and all embedded native comparison-image identities were checked. These are distinct from independent emulator review, physical light brightness, listening, persistence or whole acceptance. Other scalar/enum fixtures and controller/recording/storage transactions are incomplete models.

The existing parent reconciles the admitted fresh reviewer; no duplicate reviewer is launched. That reviewer must exercise this exact revised proposal with legitimate current firmware and compare actual equivalent states and control sequences without earlier scores or target ratings. Root needs the supported stage1534 result route and sanitized current LCD captures; do not rebuild or rerun the completed review merely to recover its evidence.

Settings component, seven CP1 cases and shared #891 ownership remain reserved. CP2 must select one whole-idea/repeated-take strategy before CP3 lifecycle. Comparator adoption, DF1, combined resources, actual correction/repeat/recovery and physical qualification remain open. No source edit/build, device operation, release or merge is included. Library's prepared-upload HTTP401 remains preserved and was not retried.

Barlow font files embedded in the workbench come from the pinned project assets; their OFL notice is included. Rasterization is a browser proposal, not packed-firmware font parity. Historical/rejected artifacts remain in Git history; they are not this R3 solution.
