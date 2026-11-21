# Integration baseline

## Stock firmware and five-zone BREAK qualification

The five original 22,050 Hz BREAK zones now have a native build and package-composer entry point, `--starting-breaks`. Fixtures for Elite and Pro recovered all 109,394 bytes of the bank through the firmware reader and rejected a one-byte application overrun. The bank retains its original ADPCM and reserves 1,024 erased bytes below Bluetooth. Both current stock applications reject it for lack of space. [Implementation](https://github.com/gobbajogger/DnBOX/commit/c5ca95ec4b040ec1d638f1db0d9be42c609bb386)

| Combined stock model | Application | Free XIP | Pool above reserve | Available for bank | Bank shortfall |
|---|---:|---:|---:|---:|---:|
| Elite | 546,468 B | 6,204 B | 832 B | 5,168 B | 104,226 B |
| Pro | 546,460 B | 6,212 B | 832 B | 5,184 B | 104,210 B |

The native builds used integration `cfbdb7ae` and Sound `f013ecc6`, including FWUI #975. The resulting tree was `b469f59b5bbeb963e6d4ec899d03399fabb6fc71`. Both stock packages passed the model, CRC and partition checks. All eight application, ELF, loader and package fingerprints matched the previous build. All 16 recorded firmware merge SHAs were reachable from consolidated `46874497`. [Build and timing receipt](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7064436580)

| Heavy playback run | Blocks | Render-thread misses | Wall misses | Render-thread peak | Wall peak |
|---|---:|---:|---:|---:|---:|
| Ordinary load | 32,768 | 18 | 701 | 75,169.046 µs | 111,621.122 µs |
| After local builds and tests paused | 32,768 | 43 | 4,203 | 136,213.218 µs | 1,991,462.852 µs |

All 32 PCM and 32 meter/scope digests matched the reference in both runs. Both kits and all five parts played. Muting Bass B failed the coverage check as expected. CPU 6 recorded 18.31% steal time in the second run, and a fixed-work control missed 109 deadlines. The 909 optimization trial reproduced the audio but increased the median end-to-end time by 0.96% across 16 paired cases. It was excluded from firmware. Neither stock sweep passed I3. The alternate CPU also missed its qualification control in both 4,096-block runs. [Per-case timing, PCM and meter hashes](../../evidence/integ-starting-breaks/heavy-playback.tar.xz) · [Alternate CPU controls](../../evidence/integ-starting-breaks/alternate-cpu.tar.xz)

[Native build fingerprints](../../evidence/integ-starting-breaks/native-builds.json) · [Source reachability](../../evidence/integ-starting-breaks/lineage.json) · [Stock bank composer and native reader](../../evidence/integ-starting-breaks/stock-fit.tar.xz) · [Focused checks and controls](../../evidence/integ-starting-breaks/focused.tar.xz) · [Combined Git source bundle](../../evidence/integ-starting-breaks/combined.bundle)

The six scheduler-capacity, four pool-budget and seven roadmap cases passed. The generated roadmap matched consolidated. The local LeakSanitizer control returned a tracing error before it could report the deliberate leak. The earlier stock Debian run detected the leak and rejected corrupted metadata. [Stock sanitizer evidence](../../evidence/integ-45b09b2d/compact-stock-linux.tar.xz)

Consolidated advanced to `d4bdf88eefd437ecaf21837eba1011f1756bc57a` with FWUI #988. Integration merged it at `9859767f`. All 17 recorded firmware merges match consolidated first-parent history. The roadmap generator and its seven tests passed against that base.

The current two-parent firmware build candidate `336ce3cc486c3960061d7b18f07160641eb5bc93` combines `9859767f` with Sound #973 `f013ecc6` and contains FWUI #975. It retains every tracked path from both parents. We rebuilt Elite and Pro on this tree and checked the model, all 20 block CRCs and the stock partitions. All eight application, ELF, loader and package hashes matched the preceding build. The SMK-native bank check on each fresh application reproduced the 104,226 / 104,210-byte deficits in the table. [Current combined source bundle](../../evidence/integ-starting-breaks/combined-988.bundle) · [Builds, digests, stock fit and 17 ledger rows](../../evidence/integ-starting-breaks/base-988-builds.tar.xz) · [Matching Elite package](https://github.com/gobbajogger/DnBOX/blob/959bf9489f949713997410d81dba798aaa73e6cf/artifacts/integ-candidates/c51d9d86/x0x-smk37-elite.fwsc) · [Matching Pro package](https://github.com/gobbajogger/DnBOX/blob/959bf9489f949713997410d81dba798aaa73e6cf/artifacts/integ-candidates/c51d9d86/x0x-smk37.fwsc)

On the integ branch alone, native `--starting-breaks` builds at `c5ca95ec` reached the packing step with 112 bytes free on Elite and 128 on Pro. Both rejected the 109,394-byte original bank. Normal builds on lane `983a02aa` passed both package checks. Four composer case groups, 16 parser checks, 16 build-composition tests and 13 build-plan tests passed at that SHA. [Native bank attempts and prior combined rebuild](../../evidence/integ-starting-breaks/native-rebuild.tar.xz) · [Lane stock builds](../../evidence/integ-starting-breaks/lane-stock-builds.tar.xz) · [Focused checks](../../evidence/integ-starting-breaks/focused-983a02aa.tar.xz)

The 808 filter trial preserved all 64 PCM and meter hashes, removed six static floating-point multiplies across three hot routines and reduced linked text by 160 bytes per board. Its 32,768-block matrix recorded 77 render-thread and 4,936 wall misses. In four reversed-order pairs per workload, the automated case improved in both orders. The standard case was 1.68% faster with stock first and 0.32% slower with the candidate first. The trial missed its predeclared adoption criterion and the stock DSP source was retained. [Patch, target disassembly and full workload](../../evidence/integ-starting-breaks/808-filter-trial.tar.xz) · [Paired end-to-end results](../../evidence/integ-starting-breaks/808-all-engine-pairs.tar.xz)

Sound #973 and nightly Remote #169 remain open. Signed-in vote verification, final consolidated builds, the parallel host and strict emulator gates, and post-merge cleanup remain pending.

## Earlier combined qualification

The [tested combination](https://github.com/gobbajogger/DnBOX/commit/c51d9d8647fb72ae0628af6fef7fb8cfbcbd40e9) includes FWUI #975 and the engine and 909 sample changes that Sound #973 incorporated from #985. The [subsequent two-parent comparison](https://github.com/gobbajogger/DnBOX/commit/3571c7dc29722356e486701fbe566da13cd36f04) retains all 6,918 paths from integration `711df7b3` and Sound `2fe5930c`. The 78 stock translation units, 71 playback translation units and 166 compiler dependencies matched the build, and five regenerated stock headers matched byte-for-byte. Consolidated is `46874497`. [Source comparison and input proof](../../evidence/integ-c51d9d86/source-equivalence.tar.xz)

| Item | Latest result |
|---|---|
| I2 | Fresh Elite and Pro builds passed their stock package checks and matched independent GitHub binary downloads. Elite produced a 546,468 B application and a 651,732 B package. Pro produced a 546,460 B application and a 651,730 B package. Free XIP was 6,204 / 6,212 B and each model had 832 B above the pool reserve. Sound #973 remains open. [Packages and build logs](https://github.com/gobbajogger/DnBOX/tree/959bf9489f949713997410d81dba798aaa73e6cf/artifacts/integ-candidates/c51d9d86) |
| I3 | Two complete 32,768-block heavy playback runs recorded 13 and 15 render-thread CPU misses at 11,609 µs. Peaks were 88,185.258 and 125,209.990 µs. The ordinary-load run recorded 866 wall misses and the repeat recorded 2,731. All 32 PCM and 32 meter/scope digests matched across the new runs. All 64 changed from the previous Sound source. Every workload exercised five parts and both drum kits. Muting Bass B failed as expected. The same-CPU fixed-work controls missed eight times during browser qualification and 36 times after all local builds and browsers ended. Zero-dropout qualification remains open. [Raw traces, mutations and controls](../../evidence/integ-c51d9d86/heavy-playback.tar.xz) |
| I4 | Stock Debian LeakSanitizer passed the compact-program test, detected the deliberate leak, and rejected the corrupted metadata. [Guest transcript](../../evidence/integ-b5207372/compact-stock-linux.txt) |
| I5 | The five original BREAK samples require 109,394 B. Stock budgets after alignment and reserve are 5,168 B Elite and 5,184 B Pro, short by 104,226 B and 104,210 B. All 31 nonempty subsets failed on both models. The exact-capacity, one-byte-over, full five-zone fixture, production chooser and native preview checks passed. [Packer and reader evidence](../../evidence/integ-c51d9d86/break-fit.tar.xz) |
| I6–I7 | The owner submitted [#984](https://github.com/gobbajogger/DnBOX/issues/984). The production inbox refreshed at zero and the issue closed with zero opening-post reactions. Five-width fixture voting passed. The signed-in increment, final consolidated builds, parallel 501-case host and strict emu360 gates, and post-merge cleanup remain open. |

### Independent rebuild and playback sweeps

A second checkout combined the same two parents at local commit `45b09b2d`. Its complete Git tree matches [`3571c7dc`](https://github.com/gobbajogger/DnBOX/commit/3571c7dc29722356e486701fbe566da13cd36f04). Both newly built packages matched the published candidate bytes. Stock package checks passed on both models, along with 13 scheduler admission cases, six capacity checks, seven roadmap cases, four pool checks and 14 overlay selector cases. The first selector attempt timed out and the unchanged repeat passed. [Build fingerprints](../../evidence/integ-45b09b2d/builds.json) · [Native and focused logs](../../evidence/integ-45b09b2d/native-logs.tar.xz)

The two stock heavy sweeps each covered 32,768 blocks. At 11,609 µs the render thread missed 63 and 57 deadlines, peaking at 123,918.891 and 61,728.367 µs. All 32 audio and 32 meter traces matched across the repeats. The fixed-work control on the same CPU missed 81 times. Neither run qualifies I3. [Matrices, control, masks and trace hashes](../../evidence/integ-45b09b2d/heavy-playback.tar.xz)

The stock BREAK packer reproduced both deficits in the table. An additional lossless experiment compressed the application and five-zone bank in independent 1 KiB pages. The results still exceeded the stock region by 25,399 B on Elite and 25,453 B on Pro before decoder space. The ten public preview URLs matched the expected audio hashes and the missing-zone control returned 404. [Exact inputs, fit tests and preview checks](../../evidence/integ-45b09b2d/break-fit.tar.xz)

### Replay of late blocks and sanitizer checks

The Miami/Chicago probe located eight late blocks. On replay of the same workload, those same block positions took 633.424–1,411.873 µs. No late position repeated across the paired runs. The production binary and output digests matched the measurement controls. I3 remains open at the original 11,609 µs threshold. [Per-block observations and controls](../../evidence/integ-45b09b2d/late-block-replay.tar.xz)

The compact-program test ran in an untraced stock Debian 6.12.117 guest as UID 1000. The clean sanitizer control exited zero and the deliberate 123-byte leak exited 23. The original wrapper passed two sets of 796 exact value/name checks and rejected corrupted metadata. The FREE PLAY framebuffer wrapper also passed the positive render and all three mutations with cached GCC objects at its original deadline. [Sanitizer output and source hashes](../../evidence/integ-45b09b2d/compact-stock-linux.tar.xz) · [Framebuffer results](../../evidence/integ-45b09b2d/freeplay.tar.xz)

The fresh local combination `b51a352f` retains every path from integration `cfbdb7ae` and Sound `f013ecc6`. Its eight production build inputs, including the complete firmware, tools, assets and BLE trees, match the independently built `45b09b2d` source. The new Sound test registration is included. The combined suite, strict emulator run and branch cleanup remain pending the final lane merge. [Input comparison](../../evidence/integ-b51a352f/inputs.json) · [Reproducible combined source bundle](../../evidence/integ-b51a352f/combined.bundle)

At `b51a352f`, the fresh Elite native build completed and matched all four earlier output hashes. Its stock parser passed and its package SHA-256 is `765777742a9cf6b0dac086c0ddd248b9037b661a42100c8bfd1c869d69e7b67d`. The new production Miami stereo test passed at three control settings on presets 0, 1 and 29. The Pro compiler produced its main object, DSP objects and loader and reached linking before the cloud executor disconnected. The last complete Pro build on the identical production inputs passed and produced package SHA-256 `c67d8c231bd45b9581da67fe508ddf7142a4f1c26ef1eecfb921d25c04475bc6`. A terminal Pro result at the newer commit remains open. The branch audit verified all 488 historical tip witnesses and all eight protected PR heads. Both consolidated tips still match the preceding audit. Cleanup follows the remaining lane merges. [Protected ref readback](../../evidence/integ-b51a352f/protected-refs.json)

### Cloud replay on the updated Sound source

Stock Sound source `5837c73f` reproduced all eight firmware fingerprints in Sound's `2fe5930c` manifest. Both package checks passed. The five original inputs used by the native sample reader reproduced both shortfalls above. The Juno runtime passed its six-voice sustain, release and render checks. The unchanged BREAK probability suite passed its original-law check and both corruption controls.

A further 32,768-block heavy sweep on that source recorded 11 thread-CPU misses and 1,206 wall-clock misses at 11,609 µs. The worst thread-CPU and wall times were 75,212.934 and 207,355.472 µs. All 32 audio and all 32 meter/scope hashes matched both independent combined-source runs at `c51d9d86`. The muted bass B check returned the expected exit 3. An independent fixed-work run on the same CPU recorded 35 misses in 32,768 blocks of 850,000 iterations. Zero-dropout timing remains open.

On integration `711df7b3`, the unchanged FREE PLAY wrapper passed its positive and all three framebuffer controls. The final run took 130.688 seconds overall. The four cases took 12.695, 44.582, 29.678 and 43.676 seconds against 100 seconds each. The cold and partially warmed attempts exceeded a case deadline. On the passing run GCC compiled the mutated unit separately for each control, linked four distinct executables and ran each one.

A fresh ref audit found all 488 historical tips and verified that all eight protected PR heads still match their archive refs. At that check firmware had 12 branches and three open drafts. Nightly had seven branches and two open drafts. The final host and emulator gates and cleanup depend on the remaining lane merges. [Native build and timing output, sample fit, FREE PLAY and ref checks](../../evidence/integ-stock-replay/sound-5837c73f.tar.xz)

## Earlier stock verification at 2c16e15f

Integration `2c16e15f` contains consolidated `46874497`, including FWUI #975 and its qualification #986. On this exact source, native Elite and Pro packages passed the stock parser and matched all eight #986 application, ELF, loader and package hashes. The scheduler, four FREE PLAY framebuffer cases, overlay registration, roadmap, issue forms, ten original input checks and all 16 recorded merge ancestors passed. [Focused results](../../evidence/integ-c28ae27c/firmware-focused.tar.xz) · [Stock #976 builds and bank-fit tests](../../evidence/integ-c28ae27c/firmware-native.tar.xz)

The [combined source](https://github.com/gobbajogger/DnBOX/commit/c28ae27c2c6a1afb70086c32a0ad321927e3dc25) has parents `2c16e15f` and Sound #973 `63278e33`. Both models rebuilt from that commit. Every output hash matched the earlier combined build. Elite produced a 651,732-byte package and Pro a 651,730-byte package. Both passed the board, 20-block CRC and partition checks, and each matched the corresponding binary fetched from GitHub. Available XIP was 6,172 B Elite and 6,180 B Pro, with 780 B of pool margin on each. The `2d9f05d6` ledger update retained all eight build-input trees. Sound #973 remains open. [Build manifest](../../evidence/integ-c28ae27c/native-builds.json) · [Raw build and download logs](../../evidence/integ-c28ae27c/combined-native.tar.xz) · [Verified package downloads](https://github.com/gobbajogger/DnBOX/tree/0f5171501677793ddd3a5318cc00bba6199bb9ca/artifacts/integ-candidates/b5207372)

The fourth independent fixed-work control recorded seven misses in 32,768 blocks on CPU 0, with a 44,665.406 µs peak. The preceding three controls recorded 21, 8 and 105 misses. The two heavy playback runs still stand at 42 and 294 misses. The five original samples require 109,394 B. The combined install permits 5,136 B on Elite and 5,152 B on Pro after alignment and reserve. The standalone #976 builds also rejected every nonempty subset. [Later fixed-work controls](../../evidence/integ-c28ae27c/fixed-work-controls.tar.xz)

## Qualification before Sound incorporated #985

Firmware [#976](https://github.com/gobbajogger/DnBOX/pull/976) incorporates consolidated `46874497`. [FWUI #975](https://github.com/gobbajogger/DnBOX/pull/975) merged at `3e307cfa`, and [#986](https://github.com/gobbajogger/DnBOX/pull/986) records its focused tests, stock builds and independent screen review. The merge retains the positive FREE PLAY check and the three distinct controls for inactive mode, missing label and unexpected cue. All tracked paths from both parents are present. Firmware and nightly use the same rules blob `d66eb78b94ef207660565a186322fc0025e85506`. The scheduler test fix is `7f87de10`. The [focused log](../../evidence/integ-b5207372/focused.txt) records the results.

| Item | Result |
|---|---|
| I0 and I1 | All 16 recorded firmware merge commits are reachable. The full host suite and strict emulator gate are reserved for the final lane merge. |
| I2 | The [combined source](https://github.com/gobbajogger/DnBOX/commit/b5207372b5d342575b70b651b7dd413527c2f161) includes integration, Sound `63278e33` and FWUI `9dd6f2df`. Both native packages passed model identity, CRC and stock partition checks. Elite used 546,500 B of application space and Pro used 546,492 B. XIP free was 6,172 B and 6,180 B. Both retained 780 B above the pool reserve. Recomposing this integration revision with Sound `63278e33` reproduces every production build input from `b5207372`, including firmware tree `4f0bb9df6e55cbf38295132ba041b028417f8495`. The FWUI source is now on consolidated. [Sound #973](https://github.com/gobbajogger/DnBOX/pull/973) remains open. The final qualification follows the last lane merge. [Packages, manifest and build logs](https://github.com/gobbajogger/DnBOX/tree/0f5171501677793ddd3a5318cc00bba6199bb9ca/artifacts/integ-candidates/b5207372) |
| I3 | Two 32,768-block, stock zero-loop sweeps recorded 42 and 294 thread-CPU misses at 11,609 µs. Peaks were 48,802.468 and 357,656.158 µs. All 32 PCM and 32 meter/scope digests matched between runs and against the prior three-lane build. The muted-part control failed as expected. Across independent 32,768-block fixed-work controls, CPU 3 missed 21 deadlines, CPU 6 missed eight, and CPU 5 missed 105 with a peak of 42,060.443 µs. Zero-dropout timing remains open. [Raw playback runs, CPU 3 and CPU 6 controls, and reproduction scripts](../../evidence/integ-b5207372/heavy-playback.tar.xz) |
| I4 | Passed at `7f87de10` in a stock Debian 6.12.117 guest as uid 1000 with no tracer. The independent control detected the deliberate 123-byte leak. The unchanged test passed both sets of 796 binary32/name checks and rejected corrupted metadata, exit 0. [Guest output](../../evidence/integ-b5207372/compact-stock-linux.txt) |
| I5 | The five original samples form a 109,394 B bank. Preserving alignment and the 1,024 B reserve leaves 5,136 B on Elite and 5,152 B on Pro. The stock packer rejected all 31 nonempty subsets on each model. The native reader accepted exact-capacity controls and all five zones in a sufficiently large fixture. The chooser and decoder passed. All ten live preview URLs served the expected audio, and the unknown zone returned HTTP 404. Embedding the originals in the stock applications remains open. [Fit measurements](../../evidence/integ-b5207372/break-fit.tar.xz) · [Live preview digests](../../evidence/integ-b5207372/live-previews.json) |
| I6 | The roadmap generator passed `--check` against consolidated `46874497`. The signed-in owner submitted [test proposal #984](https://github.com/gobbajogger/DnBOX/issues/984) through the GitHub app. The inbox rendered the issue and refreshed its live count at zero. The issue closed with zero opening-post thumbs-up reactions. The mounted five-size voting fixture passed. A signed-in opening-post vote and refreshed increment remain open. |
| I7 | FWUI #975 has merged. Cleanup follows the remaining lane merges, including Sound #973. |
| C1–C4 | The scheduler test now waits for both workers to start and fails if they run serially. All 13 admission cases, the serialized negative, four capacity tests, 14 selector cases, seven roadmap cases, both forms, all ten original input checks and the FREE PLAY framebuffer check passed. The cold framebuffer attempt exceeded its 100-second limit. A cached GCC rerun preserved that limit and rejected both mutations. The combined Juno runtime and both BREAK retrigger configurations also passed. |

## Independent stock replay

Two complete 32,768-block runs of the stock zero-loop heavy scene missed 359 and 189 thread-CPU deadlines at 11,609 µs. The peaks were 66,750.254 and 82,883.582 µs. Both runs matched all 32 reference audio digests and all 32 meter/scope digests. The muted-bass negative returned the expected mask and exit code. Fixed-instruction controls missed 293 deadlines on CPU 6 and 345 on CPU 2. I3 remains open.

New stock builds of combined source `b5207372b5d342575b70b651b7dd413527c2f161` produced byte-identical Elite and Pro packages. All eight application, ELF, loader and package hashes matched the retained manifest. Both packages passed their model identity, CRC and stock partition checks. A newly compiled C reader verified both capacity boundaries and decoded all five zones in the larger control. The production packer rejected every nonempty subset on each stock model. The full five-zone bank is 104,258 B over the aligned Elite limit and 104,242 B over the Pro limit. [Build logs, per-case timing measurements, controls and packer output](../../evidence/integ-stock-replay/stock-replay.tar.xz)

On integration `2d9f05d6b7368b9e240d3b019934ebf9c1efc080`, all six scheduler capacity tests, 13 admission cases, 14 overlay selector cases and seven roadmap tests passed. The generated roadmap matched consolidated `468744971720a81a536308e3bd84bd8766a5ed0f`. Sound [#973](https://github.com/gobbajogger/DnBOX/pull/973) remains open. The full 501-case host suite and strict emu360 qualification both depend on the final merge.

## Independent cloud gate

| Item | Measured cloud result |
|---|---|
| 501-case host run | All 16 inputs passed preflight at pre-merge #976 `2643d690`. The saved per-case inventory contains 326 results. The last live checkpoint at 12:49:19 UTC recorded 329 results, with 303 PASS, 26 FAIL and 172 outstanding. The cloud executor became unavailable before a terminal verdict could be recovered. Independent Debian guest replays have separate receipts. [All 501 cases](../../evidence/integ-cloud-3c6a5a04/host-status.tsv) · [Guest replays](../../evidence/integ-cloud-3c6a5a04/replays.tsv) |
| Combined builds | Fresh builds at combined trees `3c6a5a04` and `53585ca1` produced identical applications, ELFs and stock packages for both models. The latter includes Sound `63278e33`, FWUI `9dd6f2df` and integration `bdef4bca`. Both passed board identity, all 20 blocks, CRCs and stock partition checks. [Hashes](../../evidence/integ-cloud-3c6a5a04/refreshed-build-identity.json) |
| Heavy playback | Two 32,768-block sweeps on the same firmware measured 88 and 14 thread-CPU misses and 819 and 244 wall-clock misses. All 64 PCM and meter digests matched. [Initial run](../../evidence/integ-cloud-3c6a5a04/heavy-summary.json) · [Quiet repeat](../../evidence/integ-cloud-3c6a5a04/heavy-quiet-summary.json) |
| Strict 360 MHz emulator | Twelve physical-input sweeps with `FM1_STRICT=1` peaked at 11,446 µs Elite and 11,461 µs Pro. All twelve completed 648 half-buffers and recorded one raw late half and one xrun during startup. No subsequent half ran late. Two local alternatives reached zero on individual scenes but changed later playback or saved project state. [Per-model sweeps](../../evidence/integ-cloud-3c6a5a04/strict-summary.json) |
| Stock BREAK | The exact originals occupy 109,394 bytes. The stock packer found room for 5,136 bytes Elite and 5,152 bytes Pro. Both native builds rejected the bank. [Packer results](../../evidence/integ-cloud-3c6a5a04/pack-fit.json) · [Read-only-data bounds](../../evidence/integ-cloud-3c6a5a04/addressable-bound.json) |
| Feature vote and cleanup | Account `gobbajogger` submitted and closed the live test proposal. Production code refreshed the real row at zero votes and removed it after closure. The signed-out cloud browser could not cast the opening-post vote. Live refs accounted for all 488 historical tips and all eight protected PR heads. [Proposal](../../evidence/integ-cloud-3c6a5a04/feature-proposal-receipt.json) · [Reachability](../../evidence/integ-cloud-3c6a5a04/i7-live-witnesses.json) |
| Nightly | Exact `b3868da0` failed two full gates. Nightly #170 at `b6c3ea4d` contains the current consolidated base, the installer USB retry correction, 28 reviewed viewport references and 84 of 116 reference slots. All 74 firmware links in its generated catalog yielded the advertised bytes. The four packages also present in GitHub Releases matched those assets by byte count, CRC32 and SHA-256. [Release asset comparisons](../../evidence/integ-cloud-3c6a5a04/nightly-release-asset-checks.json) |

An independent stock-Linux replay also passed the complete sound-reset, panic-notes and drum-fill checks. The optimization replay did not reach a recorded terminal verdict. [Last cloud checkpoint](../../evidence/integ-cloud-3c6a5a04/cloud-run-checkpoint.json)

The combined post-merge run requires the final consolidated SHA. The candidate firmware and these measurements are recorded in the [recovery index](../LIBRARY-INDEX.md#combined-firmware-cloud-qualification).

## Previous checkpoints

Firmware draft [#976](https://github.com/gobbajogger/DnBOX/pull/976) incorporates the consolidated merge of [#978](https://github.com/gobbajogger/DnBOX/pull/978) at `8e4c1a10dbc6c84d2c6d8c4bf086d1ef699f3374`. The firmware and nightly consolidated rules match byte for byte (Git blob `bb02f20c3761abeac2cfda7f4f7153eaa6454ff9`). The branch ruleset allows `lane/integ`.

| Item | State and evidence |
|---|---|
| I0 | **Open.** The completed firmware baseline at `76040daa` recorded 478 PASS, 23 FAIL and 0 NOT_RUN. The explicitly requested 501-case host gate at `2643d690` has no terminal result. The prior stock-Linux checkpoint ends after `composed-features`. A fresh Debian 6.12.117 guest passed all 16 preflight checks, the clean and deliberate-leak sanitizer controls, and the sandboxed Chromium DOM check. Separate exact-source Elite and Pro rebuilds reproduced all six original package, application and loader hashes. [Full baseline](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058434593) · [501-case inventory](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058435097) · [Original checkpoint](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059118916) · [Fresh control and package evidence](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061534954) |
| I1 | **Pass for the consolidated ledger through `1c82772b`.** Each of the 14 recorded merges is reachable from this consolidated snapshot. The two latest are #980 and #981. [Source ledger](../../CONSOLIDATED-STATE.md) · [Reachability check](../../evidence/integ-cloud-2643d690/consolidated-reachability.txt) |
| I2 | **Both pre-merge models built at `0fbe180f`.** Elite application 546,496 B, Pro 546,488 B. XIP free 6,176 B and 6,184 B respectively. Both retain 780 B above the 8,192 B pool reserve. Final builds on consolidated follow integration of the outstanding source lanes and require a complete passing host suite. [Packages, checksums and link maps](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061499375) |
| I3 | **Open.** The two stock zero-loop runs at `0fbe180f` each recorded six thread-CPU misses in 32,768 blocks against 11,609 µs. PCM and meter output matched for all 32 bank/workload combinations. An independent fixed-instruction CPU control also crossed the limit eight times. Validate the full workloads on a timing-qualified runner and retain the measured failures. [Raw audio and control evidence](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061556315) |
| I4 | **Pass at consolidated `76040daa` and lane `f394ac03`.** Stock Linux ran the original test untraced as uid 1000. It detected the deliberate 123-byte leak, passed both sets of 796 checks and rejected corrupted data. [Consolidated log](../../evidence/integ-baseline-76040daa/compact-programs-linux.log) · [Lane log](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7056736948) |
| I5 | **Preview and restore pass. Stock embedding remains open.** The canonical five-zone bank uses 109,394 B. The stock native packer allows 5,152 B on each combined model after alignment and the 1,024 B XIP reserve, leaving both 104,242 B short. Native small-pack, chooser and engine previews passed. [Exact bank, packer and build evidence](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061499375) |
| I6 | **Open.** Roadmap requests 26.8, 26.9, 26.11 and 26.12 are recorded. The mounted feature inbox read and refreshed its simulated vote. The live inbox has no open proposals. Authenticated submission, a thumbs-up on the opening issue body, and a corresponding refresh still need a signed-in voting session. [Nightly browser receipt](https://github.com/gobbajogger/DnBOX-nightly/pull/170#issuecomment-7061416989) |
| I7 | The owner retained GitHub's automatic redirects. The [read-only inventory](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058900972) traced 488 historical tips and eight protected PR heads. Cleanup follows integration of all lanes. |
| C1 | Focused checks passed at `09d9ffab`, including the unmodified FREE PLAY wrapper with both mutation controls and its original 100 s per invocation. The governance and evidence update at this head requires its own receipt. [Focused checks and cold-run timeouts](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061534954) |
| C2 | Both consolidated governance merges are retained in branch ancestry. This update preserves the tracked paths from both parents. |
| C3 | The firmware integration draft changes tests, GitHub forms, Markdown and evidence. Its diff contains no product screen code. The nightly integration changes screens and requires independent review at the applicable sizes. [Rule 9](../../RULES.md) |
| C4 | The earlier official gate on `2643d690` passed the full tracked-file limit scan, and its stock Linux control rejected a 50 MiB tracked blob. The current governance and evidence update changes Markdown. [Gate output](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059118916) · [Qualification](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059719750) |
| C5, C6 | The draft remains open while the outstanding integration and acceptance items are measured. |

The [recovery index](../LIBRARY-INDEX.md#consolidated-firmware-baseline-evidence) identifies the two baseline archives and verification manifests. The owner reported disabling `gates.yml` in both repositories. At 02:32 UTC on 21 November 2026 GitHub still reported both workflows active and the permissions endpoint required administrative access. This preserves the observation made under the earlier Rule 2.

## Cloud qualification on 21 November 2026

The consolidated ledger now includes the two owner-approved rules PRs #980 and #981 through `1c82772b`. The check at integration PR head `8911d62c` found zero deleted files and five consolidated commits outside its ancestry. The six changed paths since the initial tested PR head `2643d690` are Markdown files. The [cloud evidence](../../evidence/integ-cloud-2643d690/) names each input and records separate replay results.

| Item | Measured result |
|---|---|
| I0 and I4 | The full 501-case suite started on the pre-merge head `2643d690` after all 16 preflight checks passed. A separate stock Debian 6.12 guest running as uid 1000 rejected the 123-byte LeakSanitizer control, passed both sets of 796 program checks and rejected the corrupted data. |
| I2 | The stock three-lane source tree `12266ef6` includes Sound `e5426681`, FWUI `b7b91c8b` and integration `8911d62c`. The Elite and Pro package checks passed. XIP free was 6,192 / 6,200 bytes and pool margin was 780 bytes on both models. |
| I3 | Two heavy playback sweeps on this tree reported 3 and 8 thread-CPU misses in 16,384 blocks each. All 64 PCM and meter digests matched the prior candidate. Twelve strict 360 MHz emulator scenes peaked at 11,481 µs Elite and 11,434 µs Pro. Each recorded one late half at boot and zero additional late halves during playback. |
| I5 | The native stock packer accepted a maximum 5,168-byte payload budget per model. The unchanged 109,394-byte bank is 104,226 bytes over budget on both. The live site returned the expected SHA-256 for all five previews at both URL forms. The unknown-zone control returned HTTP 404. |
| I6 | The generated roadmap, proposal form and installer receipt form passed their focused checks. The production GitHub inbox reader received HTTP 200 and an empty proposal list. A signed-in submission, opening-post vote and refreshed count are still unverified. |
| Nightly | Two fresh full local gate attempts on `b3868da0` exited 1. The homepage wait was fixed on #170 at `b62b3c1b`. The four approved viewport images matched. Nightly `061e9521` fixes the downloaded installer’s Release status link and scopes the product name check to exclude the owner’s policy text. |

The post-merge target and the I7 cleanup depend on the remaining lane merges.

## Earlier three-lane candidate at 0fbe180f

[Combined diagnostic `0fbe180f`](https://github.com/gobbajogger/DnBOX/commit/0fbe180f279ac638c85934ec0b3739120085cf4e) joins integration `09d9ffab`, Sound `ad499684` and FWUI `a0d49bb8`. The tree is `b82bb101e2f4006166fcbaec9ce205ead23a977e`. The later Sound changes through `4c1f7589` affect governance, documentation and experimental sources. The new combined SuperJ6 production code was present in both native builds and all three host matrices. The two final post-merge models remain outstanding while [Sound #973](https://github.com/gobbajogger/DnBOX/pull/973) and [FWUI #975](https://github.com/gobbajogger/DnBOX/pull/975) are drafts.

The stock [Elite and Pro packages](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061499375) were independently downloaded and compared to the actual link outputs. Elite is 651,732 bytes and Pro is 651,730. The five original sample payloads produced bank SHA-256 `8df1316157fc8ce8dcb87cfd267fea83790702ccd1963ae91c0de60ca255b582`. The production packer rejected that bank on both models.

The first two sweeps peaked at 61,321.907 and 63,474.118 µs. A separately instrumented full sweep observed 13 misses and reproduced every output. Its longest measured block took 119,149.607 µs. The fixed-instruction scalar control on the same CPU peaked at 98,447.186 µs. The current measurements do not establish a firmware cause for the individual misses. [Full matrices, controls and trace fingerprints](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061556315)

Nightly's requested `1039b686` passed its full local gate. The current nightly draft incorporates later Web and governance merges. The mounted installer and simulated vote passed on the newer code. The independent screen review and visual regressions have their own receipts on [nightly #170](https://github.com/gobbajogger/DnBOX-nightly/pull/170). [Original full gate](https://github.com/gobbajogger/DnBOX-nightly/pull/170#issuecomment-7059670134) · [Focused browser run](https://github.com/gobbajogger/DnBOX-nightly/pull/170#issuecomment-7061416989)

## Earlier three-lane candidate at 67b9f56c

The [combined diagnostic commit](https://github.com/gobbajogger/DnBOX/commit/67b9f56c4370b19ffa7e4508cf5b81df090c775f) has three parents, integration `2643d690`, Sound `dea023ce` and FWUI `3e67b59c`. Independent source review found all 6,641 file paths from the three parents, both scheduler changes, the positive FREE PLAY render, and its inactive-mode, missing-label and unexpected-label controls. The three-lane production and test sources remain on that separate diagnostic commit. Both native builds later completed. [Build and map receipt](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061401959). Both source PRs are drafts.

| Item | Verified result and remaining work |
|---|---|
| Requested 501-case run on `2643d690` | On `2643d690` the stock Linux qualification passed at 08:08 UTC. The full gate began at 08:22. Its last recoverable 9,647-byte log ends with `composed-features` passing. The next case, `juno6-runtime`, had opened its output. Resume the same allocation from the verified seventh checkpoint to retain the original process and exact SHA. The eighth interval yielded a partial memory stream and no terminal result. Both recovered 9,647-byte gate logs have SHA-256 `586e7d53c8cb8ae5dc6980f5998c3c0cb817bdcf27d8522c8f56a42fca1268e0`. [Qualification](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059719750) · [Checkpoint and official log](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059118916) |
| I2 | The combined production firmware tree is `d4a415429c08c1a1bca275da32016a323b6ecc56`. Across 621 build-input paths, the compiled source, tools and assets match the [previous clean replay](https://github.com/gobbajogger/DnBOX/blob/3e67b59c8e4dd3d5994680d857ec4e2ea755ab6d/docs/lanes/fwui/current/synthone-editor/native-replay-result.json). That replay matched all 16 app, loader, package and ELF fingerprints for two models with the optional patch off and on. The subsequent [native build receipt](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7061401959) verifies both packages. Final builds follow acceptance of the source lanes. |
| I3 | The two stock zero-loop sweeps each covered 32,768 blocks. Their PCM and meter traces match for all 32 bank/workload combinations. One sweep recorded one thread-CPU miss and the other recorded eight. The missed combinations differ between runs. Only per-case summaries survive, so those records cannot locate the missed block or establish a cause. Repeat on the current combined code with the stock header and the 11,609 µs limit. [First run](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058637831) · [Second run](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058821910) |
| I5 | All five original ADPCM inputs reproduced the 109,394-byte native bank, SHA-256 `8df1316157fc8ce8dcb87cfd267fea83790702ccd1963ae91c0de60ca255b582`. With the default patch setting, the aligned application budget after the 1,024 B XIP floor is 8,016 B Elite and 8,032 B Pro. The deficits are 101,378 B and 101,362 B respectively. The linked models retain 780 B above the pool reserve. The smallest verified lossless representation measured 97,776 B before its decoder. No measured application layout fits the originals. [Model sizes](https://github.com/gobbajogger/DnBOX/blob/3e67b59c8e4dd3d5994680d857ec4e2ea755ab6d/docs/lanes/fwui/current/synthone-editor/native-replay.json) · [Lossless measurements](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059044211) |
| I6 | The live inbox resolved the firmware repository and returned zero open proposals. Authenticated submission, an opening-post vote and a refreshed count remain open. The GitHub connection used for verification supports issue-comment and PR reaction writes. The opening issue body uses a distinct route. |
| I7 | Cleanup follows acceptance of every lane. Sound and FWUI remain open. |

### First consolidated Local run

The gate registered 500 host checks. Before interruption 247 passed and 26 failed. The other 227 produced no terminal result. The Elite package prerequisite was absent when registration started. Both stock packages were available for the repeat. [Baseline log and retest discussion](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7055474361).

| Failure in the interrupted run | Separate exact-source retest or cause |
|---|---|
| `gate-trap`, `dev-validate`, `dev-validate-composed`, `offline-browser-safety`, `offline-sequencer-browser`, `offline-touchosc-lua` | Passed after correcting checkout/temp placement, reference inputs, the browser environment and the Lua 5.1 runtime. |
| `fxbus-coef-cache`, `chicago-peaking-coef-cache`, `chicago-ens-hoist`, `chicago-modulation-hoist` | Passed once the runner had the pinned historical Git objects. |
| `compact-programs` | Passed on stock Linux with LeakSanitizer active, recorded under I4. |
| `composed-midi-epoch`, `chicago-fx2-inserts`, `original-sound-cell`, `sound-reset`, `panic-notes`, `drum-fill`, `cq-reclaim`, `optimization-equivalence`, `ble-midi-receive`, `ble-capture-replay`, `ble-device-setting-flow`, `ble-toggle-persistence-gate`, `ble-role-switch`, `chicago-span-hold`, `fma-exact-identity` | The sandbox runtime could not attach the sanitizer helper. Repeat on a Linux runner that passes both the clean and deliberate-leak controls. |

The repeat at the exact consolidated SHA also reported `panic-clear`, `original-release-integration` and `original-release-ui` as failures on this runner. Each log identifies the sanitizer helper’s failed attach. The candidate’s original-release-integration executable completed on stock Linux, including all 12 expected images. Nine complete host scripts also passed in that guest. The complete repeat is recorded below. [Sanitizer evidence](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7056160192).

A separate FREE PLAY framebuffer test passed the positive render and then exited 128 fetching an unpublished Git object. The repaired test exercises missing and unexpected cue mutations through the native renderer. Its missing-cue result matched the reachable historical predecessor in 32 of 32 comparisons, across 154,880 RGB565 bytes (SHA-256 `6e617d6c451e7b62e3bbd37fc4528f7f8279cec4abf9dbee2d3643872e115a07`). The complete repaired wrapper subsequently exited zero.

The first no-loss step failed because the checkout lacked a parent object. The missing history was restored. The generator scheduler also repeatedly tried to create a lock directory while another job held it. A focused test observed 32 rejected attempts across eight waiters. The runner now uses one blocking FIFO token. All eight waiters completed, and independent capacity, registration and admission tests passed. Host compilers, sanitizer settings, timing limits and firmware code are unchanged.

### Consolidated repeat and stock Linux qualification

The full Local gate at `76040daa5b52d686a8b7b544296ba5d79610a95d` exited 1. The only failed stage was host tests. Pool accounting, clean tree, setup, no-loss, repository limits, both stock builds, flash map, budgets, UI defect checks and the retired-engine scan passed. The offline runner used its pinned base and skipped the network fetch. The [45,892-byte gate log](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058434593) has SHA-256 `2c9b8e660bac970b0f96ec8fd87ea9a79fda1f42cf5b6ad0a1d869195a35aa7a`.

The consolidated repeat finished all 501 registered host cases. It recorded **478 PASS, 23 FAIL, 0 NOT_RUN and 0 skipped**. Its 25,592-byte host log has SHA-256 `41d85ab2843ed2cb41c70d5cd3ff21bcb0ff6497756961f82b2f87946e527d49`. This run covered all 500 originally registered cases and the additional Elite package check, which passed. Of the first run's 227 unknowns, 220 passed and seven failed. Ten of the first run's 26 failures passed on the repeat. Across the two runs there were 33 distinct failing cases. [501-case crosswalk](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058435097) · [Individual failure diagnoses](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058435595).

| Outcome in the consolidated repeat | All affected host cases |
|---|---|
| LeakSanitizer could not attach on this host. | `ble-capture-replay`, `ble-device-setting-flow`, `ble-midi-receive`, `ble-role-switch`, `ble-toggle-persistence-gate`, `chicago-fx2-inserts`, `chicago-span-hold`, `compact-programs`, `composed-midi-epoch`, `cq-reclaim`, `drum-fill`, `fma-exact-identity`, `optimization-equivalence`, `original-release-integration`, `original-release-ui`, `original-sound-cell`, `panic-clear`, `panic-notes`, `pattern-preset-notelock`, `sound-reset`, `ui-theme` |
| Missing-curve mutation exceeded its 120-second wrapper deadline. | `ui-drum-curve-axes` |
| Missing historical source object `6c418a43ff6fc5f4e1fad3bbf71e63cfd64a1987`. Git returned 128 and the GitHub commit lookup returned 422. | `ui-freeplay-state` |

The [separate drum-curve and panic checks](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058413614) used the same consolidated source. Both original wobble configurations of `panic-clear` reported zero failures under stock Linux with sanitizers active. The normal drum-curve renderer passed all 34,294 checks. Its compiled negative rejected the missing curve at `ink>100`. The original Python wrapper later passed in full in 50 seconds with its original 120-second deadline. The integration candidate's FREE PLAY test uses actual missing-cue and unexpected-cue controls and passed native framebuffer verification. [Curve readback](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058273800) · [Independent FREE PLAY controls](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7057455036).

A separate low-memory stock Linux guest executed the real consolidated `pattern-preset-notelock`, `original-release-integration`, `original-release-ui` and `ui-theme` binaries and their original output checks. It compared all 370 normal/sanitized PNG pairs and both copies of every expected digest. These results distinguish the sanitizer attach failures from firmware behavior. [Stock Linux raw results](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058333600).

At lane `f394ac03`, a stock Debian guest passed the real leak and Chromium controls. The original limit check admitted a small Git blob and rejected a tracked 52,428,800-byte blob. With those checks working, the unmodified gate scanned 5,060 Git objects and entered host compilation. The virtual machine received SIGKILL before the first host case completed. Both the guest and external copies of the partial gate log contain 9,597 identical bytes, SHA-256 `b4b915973855f8d259781dbbf13deee6204351dc4245618a95e82182259405c4`. C1 requires the complete gate on a capable runner. [Strict controls](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058009663) · [Recovered log](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058191242).

The firmware production and host-test inputs at `07a4dfcd` matched `f394ac03`. Both [fresh stock builds and their strict preflight](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058440978) passed at `07a4dfcd` (16/16). Pro retained 1,188 bytes of XIP free and Elite retained 1,180 bytes. Both preserved the 780-byte margin above the 8,192-byte pool reserve. On `f394ac03`, the original release UI passed 35,689 checks with 99 captures in both normal and sanitized executions. All paired PNG bytes and both stdout streams matched. [Stock UI replay](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058261667).

### Owner release and link update

GitHub’s Latest endpoint and the v0.0.26 tag both identify the released source as `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. The owner reports successful startup and restart on a real Elite and Pro. README, CHANGELOG, the release guide and roadmap distinguish this published result from integration builds. The published release description already links both canonical installers. All checked GitHub releases in both repositories use canonical links. The docs link migration preserves issue, commit and comment destinations under the renamed repositories. [Published release](https://github.com/gobbajogger/DnBOX/releases/tag/v0.0.26).

An uncached read of the deployed installers and their embedded catalogs on 21 November 2026 UTC confirmed that the stable channel listed 0.0.25.1 (`79280e76`). Nightly listed `nightly-94197fc8-dev`. Both nightly model checksums matched the v0.0.26 release. Its older hardware-test label predates the owner’s startup and restart results. [Stable catalog](https://gobbajogger.github.io/DnBOX/versions.json) · [Nightly catalog](https://gobbajogger.github.io/DnBOX-nightly/versions.json) · [Release and checksums](https://github.com/gobbajogger/DnBOX/releases/tag/v0.0.26).

The candidate `c125477e` built stock Elite and Pro packages byte-identical to the consolidated firmware. A separate composition of Sound `933a6f40` and FWUI `6763b056` had no three-way merge conflicts. Its diagnostic tree `ecde45dde6621a82e750a7ee223a8a5ce9f84a69` linked with XIP margins of 9,104 B (Pro) and 9,092 B (Elite) and a 780 B pool margin on each. Synth One is partial on this tree. In the Chicago/Miami host stress sweep, wall time exceeded the deadline on 22 of 3,072 blocks. All-engine timing remains open. [Native builds and measured peaks](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7055761401).

### Current stock build and sample recovery

The offline combination of Sound `03f92a7d`, FWUI `11bc3f3a` and integration `d6244db5` produced tree `5155fc01b14e4223c3a787fc3f373449bcf47ced`. Both stock builds passed. The real five-loop pack contains 109,394 bytes. Its placement calculation retains 1,024 bytes of flash headroom.

| Model | Stock image | Free XIP | Extra flash the pack needs | Pool above the 8,192 B reserve |
|---|---:|---:|---:|---:|
| Elite | 546,152 B | 6,520 B | 103,906 B | 780 B |
| Pro | 546,144 B | 6,528 B | 103,890 B | 780 B |

Both BREAK velocity tests and all seven startup scenarios passed on the combined sources, including recovery before the first display frame. The source PRs remain open. The [build logs, pack calculation and startup test](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058641850) identify the individual source commits. All 32 bank-pair and workload combinations finished using the stock zero-loop header. The compiler include trace verifies the generated input. All 32 PCM files and 32 meter/scope files match the independent stock run. The stock replay recorded eight missed thread-CPU deadlines and ten wall-clock misses in 32,768 blocks. The worst thread time was 92,119.090 µs. The independently timed stock run recorded one thread-CPU miss and fourteen wall-clock misses. [Stock replay, compiled input and raw timing records](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058821910) · [Independent stock run](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058637831).

The stock Elite and Pro packages each ran the same 30-second 360 MHz simulator test. The 140 BPM scene combined Chicago, Juno, 909 and a user-slot CRAMEN break. With observation enabled and disabled, each model produced identical PCM, RAM and virtual flash. All 774 measured intervals on each model contained a verified BREAK contribution. [Elite trace](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058994446) · [Matched Pro trace and both model controls](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059101065).

| Measurement from 20 to 29 seconds | Elite | Pro |
|---|---:|---:|
| Completed 512-frame audio intervals | 774 | 774 |
| Missed interrupt deadlines | 0 | 0 |
| Firmware late or xrun counter increments | 0 | 0 |
| Slowest firmware interval | 8,624 µs | 8,637 µs |
| Minimum interrupt-completion slack, lower bound | 2,973.046 µs | 2,955.635 µs |

The canonical five-sample bank took 109,394 B. Three lossless alternatives measured between 97,776 B and 102,688 B before a decoder. With the 1 KiB safety floor retained, native placement leaves 5,488 B on Elite and 5,504 B on Pro for samples. [Compression, round-trip verification and stock placement](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059044211).

The host suite also generates a two-loop factory header. In that configuration, four blocks exceeded both deadlines. Its PCM and meter/scope outputs match the earlier two-loop fixture. [Two-loop diagnostic](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058763565).

The full [stock Linux gate on `d6244db5`](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058541652) began at 06:20 UTC after both model builds. Its original limits scan passed. The guest finished the host simulator and began compiling `composed-features`. Supervision ended after the 07:12:31 UTC host sample. A read-only recovery matched both copies of the 9,623-byte gate log, verified the 1,200,840-byte simulator and found zero completed host results. The interruption had no recorded exit status. [Original output and disk recovery](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7059195374). The two model builds, all six image/loader/package hashes, strict prerequisites and pool limits were also [rechecked on the documentation commit `d1d6fbba`](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058778835). Separately, the consolidated BLE role-switch, BLE MIDI receive and composed MIDI epoch wrappers passed unmodified outside the tracer. [Complete wrapper and scoped binary replays](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058615700).

### Earlier Sound and FWUI composition

Sound `7e68fc39` and FWUI `5f80e1d6` composed against `76040daa` without a conflict. Adding integration `810867e1` required reconciling their two fixes to the FREE PLAY pixel test. The local tree `6308068178d2c1f320fde0962c4061bd52a2f3e6` preserves the inactive mode, missing cue and unexpected cue checks. Its stock Pro and Elite builds passed. Pro had 6,944 B of XIP free, Elite had 6,936 B, and both retained 780 B above the 8,192 B pool reserve. The exact five-sample bank still requires an additional 103,474 B (Pro) or 103,490 B (Elite) with the 1 KiB floor. The compiled Synth One INIT host test covered MIDI audio, note ownership, sustain, pitch, release, voice stealing and panic.

In the 3,072-block Chicago/Miami diagnostic, the chorus workload exceeded the 512-frame period (11,609.977 µs) once on CPU (14,541.589 µs). Wall time exceeded the period on 251 blocks across the three workloads. The host was shared. Device timing and all-engine acceptance remain open. The composed FREE PLAY test exceeded its unmodified 100-second compile timeout before producing a positive result. The earlier composition above remains a historical measurement. Both source PRs are still drafts. [Exact inputs, full resolution and per-workload results](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7056513397).

### Stock fit and heavy playback with four bass banks

Sound `e2e403f9`, FWUI `4c05722d` and integration `f394ac03` produced local tree `3ba62ac5e7b2b8c779c17be80e185bf702612b0b`. The stock Pro application was 546,128 bytes with 6,544 bytes of free XIP. Elite was 546,136 bytes with 6,536 bytes free. Both retained 780 bytes above the 8,192-byte pool reserve. Keeping the 1 KiB XIP floor leaves the owner's exact BREAK bank short by 103,874 bytes on Pro and 103,890 on Elite. The later Sound `96c9f154` composition had identical firmware, tools, tests and build inputs. [Model builds, size checks and packing rejection](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7057455036) · [Later source comparison](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058041362).

The sequencer, drum voices, owner's BREAK audio, both bass parts, MIDI and effects ran together over every ordered pairing of Chicago, Miami, Synth One INIT and Juno. Each pairing ran 1,024 blocks for a master-filter sweep and 1,024 for tempo and continuous control. One pairing completed on a same-condition retry after the first attempt exceeded its original timeout; the silenced-B negative failed as expected. Eleven of the resulting 32,768 blocks exceeded 11,609 µs on the render thread. The worst was 262,311 µs for Miami/Miami. On two repeats each of the two worst pairings there were zero CPU misses over 4,096 blocks and the PCM matched exactly. The original misses remain recorded. Hardware timing and the final merged sources are still outstanding. [Complete workload results](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7057716886).

### Five BREAK samples

The five canonical ADPCM payloads occupy 109,274 bytes, plus 120 bytes in native bank structures. The resulting pack SHA-256 is `8df1316157fc8ce8dcb87cfd267fea83790702ccd1963ae91c0de60ca255b582`. Native placement rejected both stock images. The aligned space available is 1,184 B on Pro and 1,168 B on Elite. Retaining 1 KiB free requires another 109,234 B on Pro and 109,250 B on Elite. The tested offline export is 110,844 bytes and uses three zones in slot 1 and two in slot 2. The live exporter produced SHA-256 `3a7f19858d23309dac7375b88d0537cae7854ad65680edd1f95d4dbc0beed4c6`. User-selected restore is available. A default delivery needs either additional measured space or a separately approved slot-initialization scheme which preserves existing user recordings.

### Installer receipts and Local gates

The install-report issue form records the build ID, model, firmware SHA-256, port name, startup result, restart result, browser and notes. Each startup and restart field accepts success, failure or an untested result. The dated Rule 2/C4 note distinguishes the owner’s reported disablement of `gates.yml` from the state returned by the public GitHub API. Repository-wide settings were inaccessible to the public reader.

The [receipt form and installer reader](https://github.com/gobbajogger/DnBOX-nightly/pull/170#issuecomment-7058014195) agreed on both released packages in 90 manual report fixtures and eight generated forms. Mismatched models, package checksums and cached port versions produced zero successful installs across 270 controls. The form is staged on the lane branches.

## Earlier evidence

Move: M0 (clean start and green gate)   PR: #955   Head: this commit
LEFT items PASS: 0/2; failing: L15 L16
Gate at head: M0 start checks pending; corrected limits self-match and size-loop false failures; full green gate remains follow-up
ETA merge: start files after M0 checks; green gates 2026-11-17 22:00 VLAT

# Lane: integ   (copy to docs/lanes/integ.md; update in every push)
Owner agent: <name>   Branch: lane/integ-<topic>   Draft PR: #<n>   Head: <sha8>   Updated: <YYYY-MM-DD HH:MM VLAT>

## Checklist (from CHECKLISTS.md)
| ID | Check | Status (PASS/FAIL) | Evidence (SHA + log path or PR comment link) |
|---|---|---|---|
| C1 | Local gate at head | FAIL | |
| <S1…> | | FAIL | |

## Now / next
- Now: <item, riskiest first>
- Next: <item>

## Blockers (each with its fix and the route being tried; never "waiting")
- <blocker> → <fix> (since HH:MM)

## Top 3 risks for the next hour
1.
2.
3.

## Deletes (files this lane removes on purpose, with reason; the gate's no-loss step reads this)
- <path> — <reason>

## Conflicts noted (RULES 23)
- <conflict> → reading used: <…>

## Owner-directed start-only split
This PR now contains only the supplied prep manifest, source ledger and removal list. All unrelated product/test/environment changes are removed from its effective diff by a normal successor commit, never by force push. Complete broad work remains recoverable at backup/integ-before-start-split-20261116 (246ea3718c6c72ba80bbd1aafcd5356f871c78c8).

Latest owner direction permits these two start-only merges after meaningful touched-file Local checks; this is a specific sequencing exception, not a changed RULES.md v2 text or a claim that the full before/after application gate and viewport review passed. Independent QA records exact scoped results and known defects on the PR. All lane finish lines remain open.

The gate scripts are exact supplied ZIP bytes, not later corrected versions. Known follow-up defects include firmware fail-fast/budget attribution and gate environment/prerequisite handling; the touched-file gate must not execute unsafe browser routes or treat the original scripts' possible false PASS as qualification. Full Local remains unqualified.

Removal-list exceptions retained: AGENTS.md, cloud-handoff/pipeline.json, docs/NEW.md. AGENTS.md is replaced, not deleted; other retained paths are existing test dependencies.

## Exact removals
Each path below is an obsolete brief/rule/handoff explicitly listed in the supplied remove manifest; the original bytes remain in the inherited baseline and backup history.
- `.agents/skills/x0x37-check` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-device-bug` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-docs` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-install` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-integrate` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-owner-decisions` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-shared-work` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-task` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-workflow` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-writing/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-writing/references/strict.md` — supplied removal list; historical instructions, retained in Git history.
- `.agents/skills/x0x37-writing/references/terms.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/environment.json` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/references/builds.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/references/cursor-runtime.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/references/handoff.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/references/provenance.md` — supplied removal list; historical instructions, retained in Git history.
- `.cursor/skills/loopyx0x37-classic-build/references/stages.md` — supplied removal list; historical instructions, retained in Git history.
- `STATUS.md` — supplied removal list; historical instructions, retained in Git history.
- `audit/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `audit/artifact-comparison.json` — supplied removal list; historical instructions, retained in Git history.
- `audit/current-device-diag.txt` — supplied removal list; historical instructions, retained in Git history.
- `audit/differential-results.json` — supplied removal list; historical instructions, retained in Git history.
- `audit/smc_differential.py` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/START-HERE.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-complete/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-complete/built-packages.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-complete/elite-build.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-complete/input-and-artifact-identity.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-complete/pro-build.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/CONTINUE.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/ble-midi-receive-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/dev-validation.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/docs-check.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/focused.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/gate-configs-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/gate-configs-repaired.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/gate-reason-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/input-identity.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/install-gate-guard-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/installer-ctl-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/installer-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/installer-slots-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/overflow-baseline.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/overflow-fixed.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/regressions.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/smc-reachability-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/smk-update-contract-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/toolchain-redirect.headers` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/toolchain-retry.error` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/toolchain-retry.headers` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/toolchain.headers` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/transfer-integrity.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage2-continuation/validation-inputs-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/SUCCESSOR.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/elite-build.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/isolation-fixture-probe.patch` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/isolation-probe.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/packages.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/pro-build.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/runner.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/app/smk-clip-isolation.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/docs-check.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/evidence-sha256.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/input-and-artifact-identity.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/REPORT.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/ble-central-diag-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/ble-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/ble-midi-receive-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/central-baseline.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/central-guard-probe.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/clip-baseline.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/clip-extractor-probe.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/clip-extractor-probe.py` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/diag-size-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/diagnostic-fixture-repairs.patch` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/focused.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/inlog-tool-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/receive-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-buttons-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-clip-visibility-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-fader-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-filter-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-mapping-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-part-select-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-protocol-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smc-reachability-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smk-apc40-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smk-apc40-map-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smk-apc40-violet-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/smk-inlog-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/receive/ubsan-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/dev-validate.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/gate-configs-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/gate-reason-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/install-gate-guard-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/install-guards-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/install-head-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/installer-ctl-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/installer-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/installer-slots-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/installer-targets-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/report.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/runner.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/smk-update-contract-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/safety/validation-inputs-detail.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud-stage3-investigation/stage2-repair-prompt.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud_setup_20261020/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud_setup_20261020/firmware-setup-check.sh` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud_setup_20261020/prior-draft.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/cloud_setup_20261020/validation.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/checkpoint-manifest.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/fm1-stage2-checkpoint.patch` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/initial-inventory.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/merge-resolutions.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/delivery_convergence_20261020/smc-stage2-checkpoint.patch` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/checkpoint.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/gate-reason.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/install-slots.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/update-contract.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/validation-inputs.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/baseline-overflow.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/cloud-transition.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/inlog-tool.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/overflow_probe.c` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/reachability-linux.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/reachability.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/smc_receive_repair_20261020/receive-sanitizers.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-20/transfer-sha256.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/baseline-isolation.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/baseline-receive-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/baseline-visibility.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/ble-midi-receive.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/cc-strict` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/docs-check.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/evidence-sha256.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/failfast-original-fixture.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/focused.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/input-and-checks.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/isolation-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/smc-clip-visibility.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/smk-clip-isolation.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/stage3-prompt.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/strict-results.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/thread-capability.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage2-fixture-repair/visibility-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/HANDOFF.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/evidence-sha256.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/REVIEW.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/cc-strict` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/isolation-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/normal.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/results.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/smc-clip-visibility.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/smk-clip-isolation.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/extraction/visibility-strict.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/receive/ble-midi-receive.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/receive/failfast-original-fixture.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/receive/focused.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/receive/results.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/receive/review.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/root/docs-check.log` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/root/reuse-and-integrity.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/root/thread-capability.json` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/2026-10-21/cloud-stage3-fixture-retest/stage4-prompt.txt` — supplied removal list; historical instructions, retained in Git history.
- `cloud-handoff/firmware-release-labels/NATIVE_BUILD_INPUT.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/QUICKCHECK-SCOPE.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/fm1-reference-handoff.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/installer-ctl-selection-pending-20261024-r18.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/ble-two-peer-central-port.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build-25-m1-port-20261106/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build-25-m1-port-20261106/independent-qa.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build-25-m1-port-20261106/independent-qa.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build-25-m1-port-20261106/validation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/closure.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/old-fixture-mutant.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/optimization-equivalence-original.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/reproduce_usb_timeout.py` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/usb-v2-after-mutant.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/usb-v2-baseline.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/build25-usb-recut/qa-closure-20261106/v2-fixture-mutant.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/chicago-opt.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/engines-ui-dev-plan.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/engines-ui.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/evidence/pr691-goldens-f9ffd12e-20261030.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/miami-one-build-rebase.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/miami-one-build-table-fastpath.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/miami-opt2.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/miami-pr691-cloud-validation-20261030.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/miami-pr691-golden-input-check-20261030.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/midi-queue-order-original.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/missing-features-plan.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-host-validation.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/host-suite.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/inputs.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/native-metrics.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/post-review-test-repairs.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/production-inputs.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/recut-manifest.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build-optimization-recut/review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/one-build.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/orchestrator.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/pr705-cloud-qualification-20261030.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/pr705-cloud-test-output-20261030.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/pr705-historical-baseline-host-results-20261030.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/pr705-source-recovery-evidence-20261030.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/pr705-v121-native-retest-20261030.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/lanes/q3-editor-slots-evidence.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/live-a.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/mbootloader-usrtrim-install-plan.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/one-build-integration-plan.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/remaining-feature-host-plan.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/remaining-features-20261030.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/rules/firmware-nightly-promotion.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/rules/local-proof.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/rules/workflow-history.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/rules/workflow.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf011-break-q16-native-recheck-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf013-full-stereo-native-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf014-panel-calibration.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf015-tb3po-git-handoff-v1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf015-tb3po-mutation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf016-bass303-fenv-idempotence.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bf017-motion-release-pending.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf018-motion-clear-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf020-bd-pan-motion-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf021-project-object-collision-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf022-legacy-master-highpass-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf023-motion-pattern-pending-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf024-motion-switch-order-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf025-clip-key-note-ownership-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf025-clip-strum-ownership-r3-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf025-runner-exec-r2-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf026-strum-emitted-pitch-ownership-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf027-motion-idle-transport-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf028-human-reset-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf029-motion-queued-transport-20261025-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf030-motion-external-clock-wait-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf031-tb3po-logical-bars-20261025-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf032-tb3po-accent-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-bf033-random-tie-preview-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-pr139-r3-orphan-tie-native-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-pr140-r2-random-temporal-swing-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-pr140-r3-random-clock-source-handoff-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/bugfinder-pr159-r2-bass-sink-test-compat-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/classic-qa-pr103-r15-wire-fault-matrix-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/classic-qa-pr118-smc-host-overflow-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/clock_handover_interaction.c` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/clock_handover_interaction.sh` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/combined-seq-synth-reliability-20261025.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/controller-padding-first-nightly-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-atomic-20261023-patched.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-atomic-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-atomic-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-darwin-dirfd-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-directory-durability-20261024-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-linkless-20261024-pass.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-linkless-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-linkless-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-snapshot-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-backup-snapshot-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-ctl-readback-framing-20261024-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-install-backup-noclobber-20261023-patched.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-install-backup-noclobber-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-install-backup-noclobber-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-ota-finish-ack-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-ota-finish-ack-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-ota-loader-resume-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-pr110-controller-address-echo-7bit-20261024-r2.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-pr76-per-slot-restore-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-pr96-cli-mode-20261024-pass.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-pr96-cli-mode-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-pr96-cli-mode-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-restore-max-matching-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-restore-max-matching-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-restore-status-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-slot-echo-7bit-20261024-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-slot-timeout-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-slot-timeout-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-slot-write-ack-integrity-20261024-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-smp-info-invariants-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-smp-info-invariants-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-smp-info-preflight-20261023-patched.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-smp-info-preflight-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-smp-info-preflight-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-stack-replay-20261024-pr91-darwin.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-stack-replay-20261024-pr91.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-stack-replay-20261024-pr96-darwin.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/data-safety-stack-replay-20261024-pr96.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/aggregate-normal.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/baseline-project-runtime-sanitized.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/baseline-regression.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/candidate-regression.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/candidate-sanitized.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/handoff.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/independent-review-receipt.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/independent-review.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/independent-runs.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/producer-dispatch-green.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/producer-dispatch-red.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/producer-dispatch-sanitized.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/producer-focused-suites-darwin.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/producer-focused-suites.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/project-protocol-sanitized.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/editor-ota-dispatch-race-20261025/project-runtime-sanitized.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/first-nightly-fulltree-recovery-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/first-nightly-fulltree-recovery-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-ctl-selection-pending-20261024-r18.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-controller-binding-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-download-guard-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-model-prewrite-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-ota-loader-model-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-postreboot-model-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-product-identity-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-publisher-release-origin-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-nightly-revocation-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r20-size-binding-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r21-unreviewed-stock-links-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r22-postwrite-sample-result-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r23-bounded-stream-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r24-nostream-deny-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r25-approved-package-byte-budget-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r26-stale-selection-20261025.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-pr102-r27-local-file-size-20261025.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/installer-verified-blob-download-20261024-r11.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-archive75-ctl49-smc79-critical-path-20261024-r7.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-data-safety-backup-snapshot-20261024-r5.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-data-safety-loader-resume-20261024-r3.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-data-safety-replay-20261024-r2.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-data-safety-restore-ota-finish-20261024-r4.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-pr107-dirsync-critical-path-20261024-r8.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-small-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-small-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-smc50-slot76-critical-path-20261024-r6.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-303-smc79-63-73-critical-path-20261024-r9.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-smc40-smc43-datasafety108-first-candidate-20261024-r10.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/integration-smc58-solo-overflow-first-candidate-20261024-r12.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-break-reverse-checkpoints-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-ima-decode-benchmark-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-adversarial-full-renderer-20261024-r6.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-aligned-live-r24-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-callback-tail-20261025-r10.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-checkpoint-aligned-20261025-r23.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-chunk-partition-20261025-r18.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-decoder-work-budget-20261025-r11.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-direct-nightly-compare-20261025-r19.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-dual-voice-20261025-r22.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-fast-reverse-advance-20261025-r13.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-fractional-tempo-20261025-r14.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-live-set-stress-20261025-r15.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-low-rate-long-loops-20261025-r21.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-musical-tempo-work-20261025-r12.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-rch-buffer-host-tradeoff-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-real-renderer-latency-r25-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-renderer-native-regression-20261024-r5.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/performance-pr93-seeded-live-property-20261025-r17.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/private-combined-candidate-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/private-combined-candidate-20261025/build-receipt.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/private-combined-candidate-20261025/package-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/private-combined-candidate-20261025/source-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/project-persistence-reconciliation-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/qa-r22-host-harness-recovery-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/qa-r22-host-harness-recovery-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/safe-merge-pr103-pr110-7bit-r15.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/safe-merge-pr103-r15-cli-mode-r16.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/safe-merge-pr103-r17-smp-info-fixture-source-handoff.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/safe-merge-pr103-slot109-20261024-r11.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/safe-merge-pr103-smc58-early-snapshot-r14.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-auto-idle-wrap-20261025-r33.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-accepted-play-20261025-r39.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-accepted-stop-20261025-r38.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-crosssource-pending-20261025-r37.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-epoch-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-handover-20261024-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-handover-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-handover-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-handover-interaction-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-handover-threeway-20261024-r2.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-internal-transition-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-local-stop-20261025-r40.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-owner-boundary-20261025-r30.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-owner-linked-native-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-source-tag-test.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-source-tag.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-source-tag.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-clock-uptime-rollover-20261025-r32.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-engine-event-partition-20261024-r26.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-external-dropout-resume-20261025-r28.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-first-f8-coalesced-20261025-r36.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-first-f8-reconnect-20261025-r35.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-fixture-abi-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-internal-idle-mode-transition-20261025-r34.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-link-disconnect-handoff-20261025-r31.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-local-echo-external-tick-20261024-r23.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-long-notes-20261025.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-midi-lifetime-stress-20261025-r27.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-mixed-origin-f8-grid-20261024-r25.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-mixed-origin-f8-partition-20261024-r24.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-pingpong-length-resize-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-pingpong-length-resize.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-pingpong-length-resize.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-pingpong-linked-firstnightly-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-pr90-firstnightly-linked-mid13-20261024-r2.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-rec-55-65-native-linked-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-stop-restart-burst-20261025-r29.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-coalesce-20261023-after.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-coalesce-20261023-before.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-coalesce-20261023-suite.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-coalesce-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-coalesce-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-epoch-output-linked-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-mixed-origin-output-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-noteoff-linked-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023-before.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023-clang-san.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023-gcc-ubsan.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023-gcc.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/seq-transport-origin-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/caption-dispatch.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/independent-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/knob-framebuffer.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/lock-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/native-size.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/negative-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/ui-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-action-feedback-20261025/validation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-acl-start-flags-20261023-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-acl-start-flags-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-acl-start-flags-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-acl-zero-credit-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-acl-zero-credit-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-att-mailbox-retry-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-att-mailbox-retry-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-disconnect-failure-recovery-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-disconnect-failure-recovery-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-hci-completed-packets-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-hci-completed-packets-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-hci-mailbox-backpressure-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-link-epoch-reset-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-link-epoch-reset-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-stack-reconciled-20261024.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-stack-reconciled-20261024.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-sysex-length-preflight-20261023.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-ble-sysex-length-preflight-20261023.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/artifact-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/baseline-repro.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/elite-build.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/elite-package-receipt.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/evidence-manifest.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/final-affected-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frame-manifest.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/00-prior-pickup.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/01-main-pickup.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/03-picked-fader.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/04-delay-knob.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/07-clip-field.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/08-mute-action.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/09-part-pan.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/10-808-seq-bank2-pan.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/11-list-on-fader.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/frames/12-parked-knob.png` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/input-reuse.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/late-base-observation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/pro-build.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/pro-package-receipt.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/repro.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/routing-map-comparison.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/runtime.txt` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/source-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-distinct-ui-20261025/source-review.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-fader-pickup-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-fader-pickup-20261025.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-full-control-ui-20261025.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/build-receipt.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/compiled-inputs.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/elite-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/handoff.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/independent-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/object-symbol-sizes.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/pro-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-frame-equivalence.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-late-ack-negative.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-lock.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-pause-loss.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-solo-overflow.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/consolidated-ui.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/qa/delivery-frame-equivalence.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pause-lock-sound-names-20261025/size-specialist/size-native.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-pr58-solo-early-snapshot-20261024-r1.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/independent-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/lock-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/native-size.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/ui-negative.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/ui-tests.log` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-selected-lane-20261025/validation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-spatial-lock-select-20261025/README.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-spatial-lock-select-20261025/independent-review.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/scheduler-handoffs/smc-spatial-lock-select-20261025/validation.json` — supplied removal list; historical instructions, retained in Git history.
- `docs/smc-mapping-handoff-2026-10-20.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/smc-reachability-handoff-2026-10-20.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/song-actions-handoff-2026-10-20.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/ui-theme-proposal-20261030.md` — supplied removal list; historical instructions, retained in Git history.
- `docs/ui-theme-proposal-20261030.svg` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage2-fixture-repair/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage3-fixture-retest/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage4-dev-checked/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage4-validation/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage5-complete-set/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `local-handoff/2026-10-21/stage6-dev-installed/HANDOFF.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-check/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-check/references/build-environment.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-check/references/commits.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-check/references/validation.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-device-bug/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-docs/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-docs/references/checks.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-install/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-install/references/artifacts.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-install/references/evidence.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-integrate/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-integrate/references/release-gate.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-owner-decisions/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-shared-work/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-task/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-task/references/ownership.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-task/references/routing.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-workflow/SKILL.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-workflow/references/handoff.md` — supplied removal list; historical instructions, retained in Git history.
- `skills/x0x37-workflow/references/stages.md` — supplied removal list; historical instructions, retained in Git history.

M0 limits correction: exclude the gate scanner itself from its PSRAM-setting diff and explicitly succeed for small tracked blobs; object-read failures still fail closed. Original supplied limits falsely rejected its own regex and the final small file. Environment and full-gate repairs remain separate.

## M5 roadmap source candidate (2026-11-17)

Owner: integ. Target: integration/dnbox-consolidated. Exact base: 7b17a64e11731de50f4fe46ef7ebe3948a730a10.
Roadmap generation and regression tests implemented; positive local ancestry required for every Complete entry against explicit DNBOX_GATE_BASE_SHA. No move declared Complete. Preserved historical ROADMAP content.
Local checks: generator tests 7 PASS; current views PASS; existing roadmap-evidence self-test 42 PASS. These checks do not qualify the complete M5 composition. Actions not used.
M5 remains WORKING: compact-program LSan outside tracer, deployed five BREAK previews/selection, and genuine feature-list item120 voting runtime acceptance remain open. No device operation or publication claimed.


### Hygiene review corrections after #957/#958

Owner: integ hygiene-review worker. Consolidated input: `ec4f4e56abc37d677475244d73588fc42effa415`. Correct docs PR target, required gate-log exception, workflow prohibition, missing documentation links and rename wording. Reuses `lane/integ-hygiene-review-fixes`; no firmware or workflow changes. Documentation assertions PASS; full gate and independent final-head review NOT_RUN. Next: independent review and applicable integration gates; no delivery claim.
- tests/host/roadmap_evidence_check.sh: evidence-only check, stale per DOT-BRIEF.
- tests/host/roadmap_evidence_check_test.sh: self-test of the removed evidence check.
- tests/host/run_host_with_evidence.py: evidence-only host-run wrapper, stale per DOT-BRIEF.
- tests/host/mboot_nightly_contract/check_source_closure.py: source-closure receipt check, stale per DOT-BRIEF.
- tests/host/mboot_readonly_contract/check_source_closure.py: source-closure receipt check, stale per DOT-BRIEF.
- tests/ui_defects/first_slice/evaluate.py: judged saved render receipts; replaced by judge.py, which renders and judges the checkout itself.
- firmware/src/dsp/drone_string.h: Drone engine removed (sound item 2); slot 2 is Synth One and old Drone projects load as Synth One init.
- tests/host/drone_slot_test.c: stale Drone test; replaced by tests/host/synthone_slot_test.c.
- tests/host/drone_string_test.c: stale Drone test; replaced by tests/host/legacy_slot2_map_test.c (old Drone projects load as Synth One init).
### BREAK and compact-program checks, 2026-11-18

- On the public [Starting breaks page](https://gobbajogger.github.io/DnBOX-nightly/starting-breaks/), Chromium played each of the five previews after choosing None. All five responses returned HTTP 200 and matched the corresponding repository WAV bytes. Playback kept all five boxes clear. The page made zero requests to the private audio route.
- Choosing All and downloading produced `DnBOX-starting-breaks.x0xslots`, 110844 bytes, SHA256 `2d5dca29b235104ffbf1efe372ba14c35f3d50c92c4630c0d3f9c1d4cfca2117`. The checks exercised the public browser and download paths. Device restoration and audible hardware playback remain unverified.
- At firmware commit `aa5b0691b686c8f543b90b94445975f9aa267aca`, `python3 tests/program_values_compact_test.py` passed both sets of 796 exact metadata checks. LeakSanitizer rejected the genuine 123-byte leak and the test rejected the corrupt metadata control. All ten active original synth inputs matched the pinned manifest.
- The combined firmware gate, stock-device fit, and independent review remain open. The public BREAK result applies to the site observed on 2026-11-18. GitHub reported deployed Pages build `241196130538b5e6da7738f0c2ebbbdc57fce02e` and branch tip `2de2355715f979ca1da7aeca1632c4bfe254ad24`.

### Prepared BREAK archive restore

The public five-sample archive at SHA256 `2d5dca29b235104ffbf1efe372ba14c35f3d50c92c4630c0d3f9c1d4cfca2117` passed parsing and preview but failed verified restore on the first slot. It carries 32 zero bytes between its 480-byte header and the sample payload at offset 512. Sending only the short header left those bytes erased. The CLI and browser installer's restore paths now use the full header area when it contains programmed bytes. They keep the short header for archives with erased padding.

On the SMK host fixture for `523d486e`, the firmware's editor and slot scanner read both original sample images back exactly with a full header (60,237 and 50,061 bytes). With short headers, both images differed in 32 bytes. The CLI and browser installer regression suites pass with nonuniform padding. All 31 possible nonempty selections from the corrected website exporter passed CLI restore and backup against the simulated SMK flash layout.

In the firmware host UI, the full archive exposed three BR1 zones and two BR2 zones. Browsing each played a distinct preview while the pattern selection stayed at zero until SEL. The five measured 350 ms peaks were −9.5, −10.8, −15.7, −12.7 and −13.3 dBFS. The idle control measured −180 dBFS. The measurements cover user-slot imports. The default firmware package and physical keyboard were outside this check.

The full gate at `523d486e` passed the firmware builds, flash map, budget and UI checks. Its sole failed host test was `ext-midi-rec-smk`: a concurrent missing-header probe removed the shared drum tables during compilation. The negative probe now runs the real test runner in a private build directory, and the external MIDI test uses the generated-header lock. Both cases pass together with eight runner jobs. Re-running the negative probe preserved the hashes of all three parent drum and BREAK headers.
