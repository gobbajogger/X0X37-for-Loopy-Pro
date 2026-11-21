# Firmware integration qualification inputs

The original firmware PR head is `2643d690fa13b56ed4c2c5355ad2321d724d2e9f`. The 16 preflight checks, individual board builds, compact-value guest and retained-binary guest outputs use this source. The guest ran as uid 1000 on Debian 6.12.117. The compact-value and retained-binary logs are separate replays. The full native gate retains its own case markers.

The focused integration checks and Git ancestry audit used firmware head `8911d62c6d1ea5f95fe043bf7681a10ef82b85f9`. The original FREE PLAY negative control timed out in the cold GCC build. A separate invocation using cached compilation of the same translation units, flags and inputs passed the original positive and both negative controls with the original timeout.

The combined source, package and heavy-playback JSON files identify the three-lane pre-merge tree `12266ef6a63c55c1a3d0058714d2f69afe73bf89`. The stock BREAK budget and storage map describe that tree. The `976-elite` and `976-pro` logs describe the standalone PR.

The live site served all ten expected BREAK preview URLs. The feature inbox returned an empty list. Opening-post voting requires a signed-in write and a subsequent readback.
