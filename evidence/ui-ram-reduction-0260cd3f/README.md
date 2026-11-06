# Incremental UI static-RAM reduction on exact combined v6

Evidence-only delivery for #698. The patch transforms source tree fba9b1f2df4ca7c075a0f757e60319d04c3a02f9 into 0260cd3f78a1b3717ef988d54aca767d65b6a2bb. The evidence branch itself does not alter firmware paths.

Read REPORT.md and manifest.json before applying ui-name-storage.patch. Native fit is unverified. The measured 47-byte reduction is a 32-bit host object result, not a native linker receipt. Existing Build must link the exact candidate; independent QA retains review ownership. The old peer fixture failure remains documented.

Only nonsecret source patch, extracted source reproducers, manifests and sanitized technical receipts are included. No raw captures, private data, SDK/toolchain inputs, object files, executables or firmware binaries.
