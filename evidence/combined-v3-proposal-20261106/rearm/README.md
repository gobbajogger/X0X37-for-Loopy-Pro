# V3-PANIC-REARM-01 repair proposal

Unselected source tree: `2e38c67d3dbf1b905085580229e395433aefd92d`.
Apply `V3-PANIC-REARM-01.patch` to exact prior source tree `4ae2dbf21cc85da975cfed750c807e10ad591a06`. Fresh-index reconstruction matches the new tree. Patch SHA256: `43da7f25746944fe90d574709a4ebb47b9c953ae3877fa7c88c8e8e1addccd81`.

Independent QA reported that a mode-inhibited bank release swallowed during performance testing leaves stale ownership and prevents the next fresh DAW chord. Producer reproduced eight assertion failures under ASan/UBSan before the repair, covering NoteOff and zero-velocity NoteOn. The minimal six-line production addition routes validated BANK L/R releases to panic ownership retirement while ordinary blocked actions remain suppressed. The full expanded panic route suite passes under ASan/UBSan afterward, including both previous stale-state regressions and mode-change inhibition. Playback continues.

Scoped non-author reviewer `/root/review_combined` found no additional issue. This is not closure by the original external QA reviewer: **V3-PANIC-REARM-01 external QA closure remains PENDING**. Existing full combined/native/DEV/audio/resource/UI/installer and source-selection gates remain open. All252 runner registrations and prior baseline/version selectors are byte-identical.

This evidence branch is not a selected source branch. No shared source write, merge, firmware publication or device operation. Files are UTF-8 patch, nonsecret regression logs and receipts; no credentials, raw captures or SDK/toolchain binaries.
