# Isolated next-owner combined candidate

This candidate lets the musician use the reviewed SMC screen and Pause lock together with probability-safe notes, longer TIEs, RANDOM timing, sustain Decay repair and silent cutoff navigation. It is a separate branch from the owner flash input. No active flash files, immutable artifacts or device were changed.

Runtime source: `ac11dc7cc363d78214e355a05c3b63601a2729ed`; full root `11a379224f058f5330b9ec59ae0bad19f81686d3`; firmware tree `679b553e337673ecac4374df77c654a565dac35c`. The final branch tip may add evidence only. The input map is in `source-inclusion.json`.

## Source reconciliation

The observed NIGHTLY was `08d3164a265123cb4c84e5104642fd93257d63d7`. The three actual PR heads matched the supplied inputs: PR168 `e75fb2a584aa22978f352697a5e36fc9f886c3c5`, PR188 `4baf1160f69d8bbc55ec36ca2dae22c22856f3f4`, PR189 `662ce39e43096fea1be159247ad46465034a0b28`. PR168's historical tooling/installer removals were excluded. Current NIGHTLY tools, installer, rules and safety tests remain intact. Only the seven mapped runtime files change.

PR188 retains its PR139/140/159/182/183 repairs and explicit int-return bass acceptance contract. PR168 owns smc_mixer.c/ui.c. PR189 owns main_fm1.c. The engine/SMC/UI runtime ownership does not overlap. No pointer casts or callback ABI shortcuts were added. Tests now model complete SMC Select press/release taps, configure the project migration fixture as a host build, serialize generated SMC assets, and complete the disconnected SMC live-recording adapter.

## Bounded size repair

Both initial actual native model builds overflowed XIP by 348 bytes; no application/package was emitted and RAM/pool checks were not reached. Measured changes against PR168 were seq_advance +498 B, do_start +12 B, s_bon +4 B, filter_adsr +4 B, smk_wheels -100 B and pref_desc -2 B. A first Clang minsize/noinline dispatcher trial recovered 304 B but still overflowed by 44 B. The accepted variant also avoids an overwritten noncommitting RANDOM lookahead and caches direction before callbacks. The post-callback swing direction read remains fresh. It recovers 400 B without removing controls or features. Native model records remain separate from this source statement.

## Behavior evidence

- Twelve affected sequencer/clock/DSP/migration selectors pass. Ten complete-host note/clock/probability/pattern/song scenarios pass. Live recording and SMK live recording pass.
- Independent exact-runtime review runs 18 affected selectors; production-linked cutoff navigation has 96 navigation/control audio pairs, with another 96 while SMC targets remain locked and the local UI changes. Navigation queues no wheel parameters and rendered audio hashes match throughout; release remains tied to its gesture owner. No restore on reentry.
- The RANDOM/chance/TIE/SLIDE matrix passes 18,336 cases and 232,256 assertions under optimized Apple Clang and Apple ASan+UBSan. Compiled orphan-guard and random-preview regressions fail at runtime as expected.
- Producer dispatcher differential covers 245,760 calls and compares complete normalized sequencer state, pattern bytes and sink events. Independent comparison covers all 256 raw direction values and callback-time direction edits. Both match the original combined runtime.
- SMC review preserves physical Note0x5D Pause lock, Note0x5E Play toggle and User MMC34 Stop, target-capture/loss handling, drum sound names and lossless raster output. Host mutants are separate evidence from device proof.

Earlier PR188 Linux ASan runs retained intermittent DEADLYSIGNAL/139 failures without useful source traces; same-image empty-main failure supports environment instability. Later reruns and Apple sanitizer passes do not establish universal Linux sanitizer acceptance. Exact unchanged-source evidence is reused only within its recorded scope. Missing TB-3PO reference comparison stays unclaimed.

## Failures and setup corrections

Actual failed fixture runs remain in private logs: Select-without-release; missing host pool definition; POSIX-only Darwin declarations; disconnected screen adapter missing symbols and one corrected draw signature. The corrected tests execute the asserted behavior; none was removed to obtain a pass. The first canonical Linux host run also encountered mkfifo on a shared mount; supported serial host-test mode fixes that environment setup, retaining the failed log. A container-owned Git safe-directory setting corrected initial orchestration access to the isolated clone.

## Known unresolved behavior and authority

The owner subsequently reported that installed PR168 faders appear to require two pickup stages. Dedicated isolated regression owner task88 is reproducing and fixing it. SMC/UI bytes in this candidate remain exactly PR168: no speculative pickup patch is included and no claim is made that the report is resolved. The parent decides whether and when to consume that later reviewed patch; it would invalidate affected source, review and model evidence.

The physical Pause lamp identity, ADC/audio/radio timing and device behavior remain unproven on this candidate. No GUI, hardware, flash, protected merge, main edit, Pages/picker promotion or public binary action is authorized here. The parent owns hardware sequencing and future integration/publication. Any private packages are DEV / HARDWARE TEST — DEVICE NOT TESTED, with the pickup report explicitly unresolved. Source/build acceptance does not certify all requested behavior on hardware.
