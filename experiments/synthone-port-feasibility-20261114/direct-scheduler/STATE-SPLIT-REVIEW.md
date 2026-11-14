# Independent runtime/oracle state-split acceptance

Accepted for the bounded Synth One source/host state split. No source/runtime blocker or outstanding documentation finding remains in this reviewed boundary. This receipt does not authorize or qualify firmware integration, merge, publication, release or device use.

## Accepted identity

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Baseline: #939 commit `46b06ada4009e5047cd3bd4a153b8e3b76c46713`, root tree `721e6afd3bc68dd6782a28ef7c90eb0a9e3709ce`.
- Accepted `SOURCE-MANIFEST.json` SHA-256: `2b7cdc90cd7f3399c0b3dd08ed32e3ec997e0d1a006caea77c19dca1c902e48f`.
- Original independent report SHA-256: `4bd0e3f83ee312108d19689b6568997e2bfe6f8cfc92b5e89e06a4aaa86a4ed6`.
- Exact documentation readback addendum SHA-256: `06fe052d06f53c416466f840298f4ed645ee1f727ef428eb311b851dfe64b3fb`.

## Reviewed boundary and checks

Common `State` retains graph/lifecycle. Runtime owns only compact ports; handwritten `OracleState` owns original flags/ports and no-op delay tokens. Final concrete wrappers validate complete-object spans. Exact-output restore snapshots, partial/internal alias rejection, address-stable self-bindings, independent instances and distinct fresh/reinit/restore behavior are retained.

All 150 generated oracle flags were independently matched to authenticated original metadata, with 46 selected indices checked against the compact map. Typed canonical snapshots preserve original parameter order; the oracle retains its independent original port loop. Actual port c1/c2 retention and parameter-144 clamped-current-output semantics were inspected and exercised. Generated `oracle/oracle.cpp` and the runtime event/render tail remain byte-identical to the baseline.

Fresh independent host O2 and Os full fixtures each passed 54 cases, 18,162 frames, 19,302 canonical snapshots and 648 events, with zero intercepted heap calls during actions and zero failures. The same full fixture passed with ASan, UBSan and float-cast-overflow enabled and leak detection disabled. All ten original mutations compiled/linked before failing genuine runtime assertions; the relocated inline oracle mutation rebuilt every local translation unit. Separate runtime/oracle shortened-span mutations also compiled/linked and failed the concrete-storage rejection checks. Additional independent O2 and sanitizer probes each passed 764,050 checks covering coefficient retention, exact aliases, late-derived-storage overlap, overlap from before the object, all nine rate-pair reinitializations and self-binding/fresh-state comparisons.

P3 documentation finding closed: README now distinguishes historical 48-case evidence from the current 54-case fixture and records the completed oracle-storage separation. The matching runner correction is comment-only, confirmed by AST comparison. Exact readback found only those prose/comment changes plus the corresponding source/result identity records; compiled C++ inputs are unchanged. Post-correction O2 consumer smoke receipts were verified: 3 cases, 144 frames, 189 snapshots, 18 events, exit 0, stderr empty. No unchanged full-suite replay was claimed for that correction.

## Limitations

Default LeakSanitizer execution fails explicitly because it cannot operate under ptrace in this executor. LSan is not PASS. No native target build, combined target flash/RAM/CPU or workload qualification, complete-engine/global-FX fidelity, AU/AEArray/MIDI integration, firmware integration, device test/write, merge, release or publication is qualified. Borrowed immutable waveform/tuning lifetime and serialized-call preconditions remain. No owner bank or recorded audio was used; fixture PCM was compared in memory, not exported.

The measured object sizes are host-ABI observations only, not target savings. Source preservation and downstream integration retain their separate owners and action-specific gates.
