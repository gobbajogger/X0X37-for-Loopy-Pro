# Nightly development build 0.0.24.1

Owner-accepted DEV / HARDWARE TEST. Device not yet tested. Source `3d8131f7` reverts #809, and is byte-for-byte the first-parent source tree `e3e82141`.
Elite and Pro SDK V1.2.1 builds use X0X_DEV_PADCAL=0 and strict _900 identities. Both pass package_nightly --check with controller a334208b, and each has 8,328 B of pool headroom (minimum 8,192 B).
Full host suite remains at 348 passing / 33 failing / 2 not-run, unchanged from published 0.0.24 and expressly accepted by the owner for this candidate. Audio identity passes (17 scenarios / 50 files). Source framebuffer unchanged.
These are firmware test packages, not physical-device validation. Firmware's ABOUT version is 0.0.24 while the nightly catalog entry is 0.0.24.1. No installation or flashing was performed.
See manifest.json and source evidence at https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/build/0.0.24.1/docs/evidence/build-0.0.24.1
