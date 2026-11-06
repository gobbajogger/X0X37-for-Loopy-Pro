# Exploratory UI host-render comparison

Completed 15 interleaved ABBA rounds and 15 same-binary A/A control groups for all 15 existing UI cases (full and steady draws: 30 metrics). This is a diagnostic comparison with a provisional baseline, not the authoritative release performance gate.

Baseline commit: `48e92b76c2f476cf40771e00a04e1838136bb55d`; tree `00370c8cf385577395f616e9df3cbc45a0d23041`. Candidate tree: `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`. Prior source-selection blocker is superseded for this exploratory run by the parent instruction. Both complete source closures were verified; no firmware source fixes were made.

Matrix: unchanged `tests/host/night_ui_bench.c`, using the unchanged `tests/host/smc_ui_test.c` fixture and build runners. Production renderer, 200 draws per full/steady measurement, process CPU clock, GCC 14.2.0 with project runner flags (`-O1 -g`, no fast math, contraction off). One warm-up per executable, then serial runs pinned to CPU 2. ABBA and AAAA group order alternates across rounds. Both build scripts also execute one unmeasured setup run; those compiler/build logs are retained locally and excluded from this timing packet. All 122 recorded invocations exited 0 and passed 60 cache/nonmutation checks each. Highest one-minute load at a group start: 0.970; all 30 starts below 5.

## Classification limitation

The existing matrix was recovered and executed. An existing project A/A classification formula was NOT found in the checked source, pinned ROADMAP or inspected #809/#821/#698 receipts; no exact classifier filename was supplied. Historical `tests/host/ui_frame_identity_test.py` is absent from both measured trees. The results below therefore use the explicitly declared conservative exploratory formula in metadata.json, not an attributed project formula. That missing classification receipt remains a limitation for reproducing the requested project-specific classification.

Each ABBA round averages the two A observations and the two B observations. Delta is 100*(B/A-1). Each AAAA control compares its inner-pair mean to its outer-pair mean. The noise envelope is the largest absolute control delta across 15 rounds for that metric. The diagnostic label is SAME if the absolute median paired delta is within the envelope; otherwise positive is SLOWER and negative is FASTER. SAME means unresolved within observed noise, not demonstrated equivalence. A/A outliers widen this conservative envelope; raw data are retained for reanalysis under the actual project formula.

Diagnostic counts: FASTER 5, SAME 25, SLOWER 0.

## Every measured case

Times below are milliseconds per draw, with min–max and IQR across 15 paired round means. Delta is the median paired percent change, not the ratio of the separate medians.

| Case / metric | A median [min–max], IQR ms | B median [min–max], IQR ms | Delta % | A/A max abs % | Diagnostic |
|---|---:|---:|---:|---:|---|
| HOME/full_ms | 0.189775 [0.187943–0.192041], 0.001215 | 0.184438 [0.181770–0.194784], 0.001992 | -2.764 | 7.561 | SAME |
| HOME/steady_ms | 0.078986 [0.077872–0.082856], 0.000557 | 0.078348 [0.077418–0.080475], 0.000858 | -0.422 | 13.584 | SAME |
| 909/full_ms | 0.212633 [0.209539–0.220467], 0.003182 | 0.205742 [0.203101–0.208436], 0.000903 | -3.310 | 7.054 | SAME |
| 909/steady_ms | 0.169170 [0.166841–0.249412], 0.002310 | 0.157907 [0.154824–0.161523], 0.002243 | -6.746 | 3.207 | FASTER |
| 808/full_ms | 0.196989 [0.193294–0.282340], 0.002542 | 0.189923 [0.186949–0.194513], 0.001425 | -3.536 | 4.343 | SAME |
| 808/steady_ms | 0.153398 [0.150946–0.155021], 0.001702 | 0.140818 [0.138690–0.147861], 0.001750 | -8.022 | 4.542 | FASTER |
| MIA/full_ms | 0.316156 [0.311478–0.333767], 0.005154 | 0.285211 [0.281334–0.288079], 0.001904 | -9.847 | 4.202 | FASTER |
| MIA/steady_ms | 0.271346 [0.268683–0.277902], 0.003574 | 0.234163 [0.230585–0.245503], 0.003397 | -14.037 | 4.657 | FASTER |
| MIAMI_SOUND/full_ms | 0.226541 [0.224723–0.231855], 0.001914 | 0.222425 [0.218303–0.229014], 0.001808 | -2.111 | 6.738 | SAME |
| MIAMI_SOUND/steady_ms | 0.183028 [0.181984–0.186857], 0.000682 | 0.171294 [0.168775–0.178079], 0.002746 | -6.370 | 11.138 | SAME |
| CHI/full_ms | 0.266631 [0.262794–0.272318], 0.002860 | 0.264794 [0.262253–0.278427], 0.003603 | -0.653 | 4.741 | SAME |
| CHI/steady_ms | 0.223977 [0.220597–0.231104], 0.001837 | 0.211818 [0.209595–0.216410], 0.002204 | -5.785 | 5.988 | SAME |
| CHICAGO_SOUND/full_ms | 1.232785 [1.220403–1.239437], 0.008731 | 1.235421 [1.220407–1.251400], 0.009524 | +0.278 | 2.841 | SAME |
| CHICAGO_SOUND/steady_ms | 1.188449 [1.172538–1.204665], 0.012383 | 1.180486 [1.160683–1.252009], 0.014233 | -0.796 | 2.175 | SAME |
| BRK/full_ms | 0.203379 [0.200842–0.212215], 0.002785 | 0.199946 [0.196825–0.207244], 0.002421 | -2.026 | 10.507 | SAME |
| BRK/steady_ms | 0.089749 [0.088417–0.097994], 0.001467 | 0.090413 [0.088562–0.093635], 0.000936 | +0.353 | 4.674 | SAME |
| FX/full_ms | 0.194310 [0.191730–0.200667], 0.001656 | 0.191609 [0.186965–0.196057], 0.003660 | -2.219 | 4.973 | SAME |
| FX/steady_ms | 0.097616 [0.096585–0.098694], 0.000821 | 0.098828 [0.097253–0.105022], 0.002806 | +1.241 | 5.565 | SAME |
| MIX/full_ms | 0.187713 [0.185901–0.193679], 0.001937 | 0.182946 [0.179677–0.194366], 0.003043 | -2.269 | 3.882 | SAME |
| MIX/steady_ms | 0.083961 [0.083091–0.086567], 0.001632 | 0.084859 [0.083374–0.104617], 0.003467 | +0.568 | 3.252 | SAME |
| WAVE/full_ms | 0.185796 [0.184376–0.188447], 0.000603 | 0.181268 [0.177851–0.202976], 0.003993 | -2.310 | 4.750 | SAME |
| WAVE/steady_ms | 0.079057 [0.078034–0.086605], 0.001041 | 0.077568 [0.076979–0.081838], 0.001337 | -1.331 | 14.570 | SAME |
| SONG/full_ms | 0.171332 [0.168789–0.174238], 0.002793 | 0.165411 [0.163081–0.170257], 0.002082 | -3.056 | 7.648 | SAME |
| SONG/steady_ms | 0.068492 [0.067414–0.076025], 0.001346 | 0.068350 [0.067567–0.077869], 0.000529 | -0.292 | 3.980 | SAME |
| DRUM_SEQ/full_ms | 0.192763 [0.190790–0.195066], 0.000992 | 0.188064 [0.184924–0.194802], 0.002825 | -2.135 | 2.881 | SAME |
| DRUM_SEQ/steady_ms | 0.094445 [0.092957–0.095465], 0.000975 | 0.094365 [0.092692–0.095773], 0.001412 | +0.029 | 9.320 | SAME |
| SYNTH_SEQ/full_ms | 0.191879 [0.190407–0.194670], 0.001429 | 0.186821 [0.184740–0.197643], 0.002042 | -2.661 | 2.113 | FASTER |
| SYNTH_SEQ/steady_ms | 0.076657 [0.075924–0.078168], 0.000895 | 0.076854 [0.075848–0.081746], 0.001317 | -0.060 | 4.748 | SAME |
| GLOBAL/full_ms | 0.200183 [0.197192–0.209535], 0.001500 | 0.191438 [0.188580–0.199390], 0.004830 | -4.033 | 5.892 | SAME |
| GLOBAL/steady_ms | 0.089849 [0.088347–0.092561], 0.001383 | 0.085424 [0.084108–0.090473], 0.001242 | -5.564 | 11.071 | SAME |

## Evidence and bounds

`raw.jsonl`: every invocation and all 30 metrics. Per-invocation `.log` files retain original stdout. `round-load.jsonl`: UTC and load at every group start. `summary.json`: medians, ranges, IQRs and every per-round paired/A/A delta. `summary.csv`: flat comparison. `metadata.json`: binary SHA-256 and methodology. `build-inputs.json`: source trees, full tracked-leaf verification counts, generated-input and fixture hashes, compiler identity. `run_diagnostic.py`: orchestration source.

Both executable hashes are retained in metadata.json. No device LCD transfer, DMA, ISR, key/MIDI latency, audio callback budget, dropout result, native firmware/package result, or whole-product zero-SLOWER acceptance is established. Host HAL radio is stubbed by the existing fixture. No Mac, process kills, source fixes, pushes, merges, publication or device actions.
