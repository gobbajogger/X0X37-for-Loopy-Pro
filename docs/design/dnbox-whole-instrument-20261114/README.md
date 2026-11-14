# DnBOX whole-instrument design review

Download `walkthrough.html` and open it in a browser. It is a self-contained, keyboard-accessible design walkthrough with native 240×240 proposals, curated comparisons and musical journeys. GitHub source preview does not execute HTML.

This preserves the corrected design for review by the existing firmware integration owner. It changes no firmware. The inventory contains 91 proposal frames, 22 design journeys, 12 curated views and eight actual-before/proposed-after pairs; these counts do not establish whole-design acceptance. Actual historical captures and simulated proposals are labelled separately inside the walkthrough.

## Provenance and validation

Design source baseline: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. R22 input SHA-256: `75c34c301d4667a116f6607a0429dfb2af8e2ecaa02991f5301bef9206410fc2`. Original reviewed HTML SHA-256: `e5924da7e2b065344c2cba547efa54ce72be17e2a0c760bb329884792139c54d`. This export substitutes the internal Settings thread identifier with its role name; it does not alter images, scripts, styles or interactions. See `export-manifest.json` for exact export identities and `independent-review.md` for the original narrow review, whose hash refers to the original HTML.

Previously recorded checks: 152 affected design/source/artifact checks and 103 browser states passed. The independent successor review resolved Home disclosure and USB-power findings in its affected scope. Those checks are reused; no new emulator, render or firmware tests were run for this publication. The private full verification archive remains separate.

Historical URL display label was 0.0.25.1 Elite, but the registry identifies package 0.0.25 and source `79280e763341d03ba43266d3e25dad66f12de860`. Package SHA-256: `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a`. Emulator advertised v0.5.1; WASM SHA-256: `1a996196b256c5f61a9386de62c48edcd2c1af536eef1ef2866caf258876588b`. No candidate firmware execution, listening acceptance or physical tests are claimed.

## Remaining decisions and limitations

Seven external CP1 cases remain unresolved: B6-ACCESS-BASE-STATUS, B6-ACCESS-ALT-STATUS, G1-PENDING-SOUND-BASE, G1-PENDING-SOUND-ALT-A, G1-PENDING-SOUND-ALT-B, KAR-SUCCESS and KAR-WRITE-FAILED. CP2 needs an explicit whole-idea/repeat-takes capture strategy; CP3 depends on that strategy. Refusal and recovery fixtures alone do not establish musical success.

Settings R2 was subsequently received and hash-verified as a separate private supplement; the frozen walkthrough still accurately records its earlier intake state. R2 is not visually merged or included here. Its six comparisons and 105 producer checks do not close whole-design acceptance. Settings ownership remains reserved. The LOAD footer still needs the normal cancel plus start/stop consequence made explicit; it currently says SEQ PLAY escapes; no load.

The separate same-hardware comparator adoption map remains unreceived. DF1 still needs complete case/control mapping, the eight-step Drone versus sixteen-step requirement resolved, sound-neighbour overview and typography/header/PERFORM continuity integrated, and CP1 integration. Actual whole-design owner consumption is unverified. Physical LEDs, power sensors/charging, hardware resources, tactile/glare, clock phase, host feedback, persistence and complete performed-idea/correction/repeat-take/recovery remain unproven.

This is a review-only design export for an isolated draft PR. No runtime implementation, merge, firmware binary, device operation or release is included.
