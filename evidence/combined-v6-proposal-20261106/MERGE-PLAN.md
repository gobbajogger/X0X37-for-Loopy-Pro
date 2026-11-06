# UI + mute/BREAK isolated combined proposal

Exact tree fba9b1f2df4ca7c075a0f757e60319d04c3a02f9, based on source commit c295af1554f9e80a29864cd013995eccad66cd2c / tree d98072ba1d689cb99bd16e4a6e1e682b22b04c70.
UI donor tree40c9273a6bfbf99c4e799ad26abc5b4ecb68013e from evidence commitd9a7b3aa7d21f733ad725090a9bd59afaa438542.
Mute/BREAK donor tree3b21f19e1b8dc6d4fa9000c76c368a23379a23d1 from evidence commitc8ad9b7a411aef2b5656e1556e06eee4a21a4b6b.
Parent confirms independent acceptance of both exact donors; author reports in packets predate that acceptance.

Both authenticated packets separately reconstruct their exact trees from d98072ba. Combined changes26 paths; only shared path is tests/run_tests.sh. Git three-way merge retained both sets of insertions and class-arm extensions without conflict. All25 non-runner files equal their selected donor bytes. All258 existing declarations remain exact;3 UI+4 mute selectors make265. G-class generators remain locked; mutating mutant fixture remains serial. Existing scheduler11 checks pass,2 dependency mutants rejected,7 added classes checked. Required controller subtree5bf6aec63e21f527cb962dfbb4698656ee648d80 and ABI unchanged, retaining verified a334208b/61552B/564a8f55 provenance. No controller binary change/substitution.

Apply combined-v6.patch only to exact d98072ba; SHA25695b6e01f904cfc20dbeac2bc2530842de6395a66d1f5e760be18d31dd0508bfd. Independent fresh-index reconstruction matches fba9b1f2. Do not overwrite the existing integration branch without its single-writer lease/authorization. No remote/shared-source writes occurred in this task.

Actual combined checks:12 scripts exit0. Settings494; peers61; BLE name bounds; original-release-ui23176 normal andASan/UBSan,40 PNG pairs byte-identical; waveform truth; LCD deferred-DMA dispatch; synth-bus-mute144; BREAK production/fade plus abrupt-cut detection; seq-mute42; M7a runtime57cases; panic route ASan/UBSan; chain-longest including2 killedmutants. Logs and exact commands in verification.json. Original independent donor tests/captures and mutant evidence are reused within their actual scope; no duplicate donor audit. Scoped non-author composition review found no integration issue.

MUTE preserves internal synth progression and bus gain silence while MIDI is suppressed; OFF still releases voices. BREAK unmute resumes on an actual admitted step; held-audition override preserved. Waveform/peer/settings UI additions do not rewrite panic/LCD/engine controls. Prior whole-engine audio identity is scoped to its old tree, not a claim of unchanged output for newly restored mute policy.

Native fit NOT_RUN. Parent reports current base only28B static RAM headroom. UI donor host-size estimate is+240B (+172pool,+68ordinaryBSS), not a native linker result. No fit claim or headroom extrapolation. The separate1024B RAM proposal is explicitly NOT_ADOPTED: needs independent review and native proof. Existing native Build owner must build this exact combined tree and measure RAM/pool/stack before merge. Do not redo unchanged CLI/controller tests solely due tree ID.

Required part-OFF historical baseline/cross-firmware execution remains NOT_RUN in this runtime-only check. Existing legacyformat2 migration and old smc_peer_names fixture failures retain donor baseline status, not newPASS. Full unfiltered suite/native/device qualification not claimed. Preserve existing owner/QA workflow, source selection and applicable checks; queued backlog is not added scope. No merge, firmware publication or devices.
