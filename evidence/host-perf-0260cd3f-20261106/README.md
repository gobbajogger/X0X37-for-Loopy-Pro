# Diagnostic performance evidence

This evidence-only packet measures final reconstructed source tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb` against provisional integration baseline `48e92b76c2f476cf40771e00a04e1838136bb55d` (tree `00370c8cf385577395f616e9df3cbc45a0d23041`). It does not change firmware or establish release-gate acceptance.

[Report: every case and limitations](REPORT.md) | [raw timings](raw.jsonl) | [summary](summary.csv) | [methodology and binary identities](metadata.json) | [source closure](post-run-closure.json).

Candidate reconstruction: source commit `c295af1554f9e80a29864cd013995eccad66cd2c`, tree `d98072ba1d689cb99bd16e4a6e1e682b22b04c70`; apply `evidence/combined-v6-proposal-20261106/combined-v6.patch` from evidence commit `66cfa3d0724a039f29803c7770ae0d6233f7fada` (patch SHA-256 `95b6e01f904cfc20dbeac2bc2530842de6395a66d1f5e760be18d31dd0508bfd`) to obtain v6 tree `fba9b1f2df4ca7c075a0f757e60319d04c3a02f9`. Then apply `evidence/ui-ram-reduction-0260cd3f/ui-name-storage.patch` from evidence commit `d5c328862929b14a49104011a843115a7f42d27e` (patch SHA-256 `558d038601ff8afdc63097bdd1c5e92d5bf7bddf50bae6022d1c93d01a3add19`). All 5,120 final candidate and 4,935 baseline files/modes were verified after execution.

15 ABBA rounds and 15 AAAA same-binary controls, 30 metrics, CPU 2 pinned, every group starts below load 5. Diagnostic-only classification: 5 FASTER, 25 SAME, 0 SLOWER. The maximum-absolute-A/A-envelope formula is provisional; the project-specific classification receipt was not recovered. SAME means unresolved within this observed noise, not proven equivalence. No device timing, audio/dropout, native/package, release or publication claim.

Reproducer: `run_diagnostic.py` is the executed orchestration with only its absolute workspace root replaced by required environment variable `PERF_WORK_ROOT`. Set this to an isolated work root containing `candidate`, `integration-baseline`, and the two previously built executables at `evidence/candidate-build/test` and `evidence/baseline-build/test`. Build each using `NIGHT_UI_TEST_FILE=tests/host/night_ui_bench.c sh tests/host/night_ui_test.sh <output>`. Preserve source trees, exact fixture and flags; see `build-inputs.json`. Existing output is refused rather than mixed with a new run. Source bytes and source-specific build inputs are not bundled.

Only nonsecret technical text is included: raw timing stdout, JSON/CSV results, methodology and hashes. Compiler logs, local absolute paths, source/private-header manifests, generated source headers, SDK/toolchain inputs, executable/firmware binaries and captures are excluded. `sha256.json` binds the sanitized delivery files, not the original local report bytes.

CSV line endings are normalized to LF; numerical values are unchanged.
