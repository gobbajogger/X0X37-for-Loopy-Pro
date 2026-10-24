# Supplementary PR8 history-response QA — NEEDS CHANGES

Candidate remains unchanged: PR8 `b0d3c2b88587948a428305210d8b76ebd4d40535`, complete root `5b5305ae79d109c87c5f5004fb81b2d3780f65cf`. No rebase to moving PR3 R26 was performed. The five precise transformations in the existing merger [R26 receipt](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/3#issuecomment-6384551244) were independently replayed against served, uninstrumented PR8 HTML with an intercepted JSON response in genuine Chromium154.0.8037.57.

**Applicability is now demonstrated on PR8, not inferred from another lane's verdict.** R26 source `234cd59437137002565b22376b0d2a7a24e8053e` has history HTML blob `ecad15f48e897d0f0fe87b7df8b060f7067a35cc`; accepted PR8 has `72f18fcbd4568ab45e70ac2d3beecee6af70ccad`. PR8 changed CSS, but the entire runtime module is byte-identical: shared SHA256 `a80994108b9928af9646c568e323693bc4109989bac4ae98062da4a021a5cdf1`.

The earlier independent acceptance verified installer malformed-selection fail-closed behavior, history layout and checked repository data, but missed these separate history HTTP-response cases. It is superseded for overall full-site acceptance by this supplementary **NEEDS CHANGES** disposition. Earlier command passes remain truthful; F1 stale installer download and F2 history overflow remain resolved. Closing their old inline threads does not clear these new runtime findings or authorize a merge.

## Actual commands and observations

`python3 history_response_negatives.py` exited **1**. The five named negative assertions all fail; both 320/375 positive controls pass. One additional negative using the actual complete two-build response also fails. The base fixture copies PR8's actual response and limits the build list to one build while retaining all117 history records to match the merger's cardinality; it is independently constructed, not claimed byte-identical to the merger's archived synthetic fixture. Mutations are exactly those reported in the receipt.

| Case | Error banner | Build/history cards remaining | Fail-closed expectation |
| --- | --- | --- | --- |
| Valid response375px | No | 1/117 | PASS; no overflow/download links |
| Valid response320px | No | 1/117 | PASS; no overflow/download links |
| `history[-1]=null` | Yes | 1/117, last history partial | FAIL |
| `history[0]=null` | Yes | 1/1, history partial | FAIL |
| `builds[0].models.pro=null` | Yes | 1/0, build partial | FAIL |
| `schema=999` | No | 1/117 | FAIL; accepted |
| `history[0].status='DEVICE VERIFIED'` | No | 1/117 | FAIL; forged tag displayed |
| Actual full two-build response with `schema=999` | No | 2/117 | FAIL; accepted |

**F3/P2 — non-atomic history population retains partial records after malformed nested input.** The runtime creates/attaches cards before validating each record/model; the catch only sets an error message and never clears them. Validate the complete presentation shape before any visible commit, or use detached fragments and clear all previously rendered content on failure.

**F4/P2 — history runtime accepts unsupported schema and arbitrary status claims.** It checks kind/installable/top-level arrays and one build flag, but not schema or history status. Require supported schema, allowed status and nested presentation field types before displaying records; preserve truthful future append behavior through reviewed validator changes.

These are read-only presentation/runtime defects under corrupted/untrusted HTTP fixtures. No firmware link, working download admission, remote attack or physical operation was demonstrated. The source generator's separate provenance validator is not alleged to accept these forged records. Installer qualification and real firmware/controller/hardware gates remain distinct. Repairing browser validation does not require new firmware compilation or public firmware hosting.

## Parent-controlled repair handoff

Overall website merge remains NO-GO. Parent coordinates existing Catalog Sync/PR3 history owner and PR8 producer for a bounded browser-history repair on an owned isolated source lane, without an unnecessary moving-R26 rebase. Start from exact PR8 head/root above; consume this script/receipt/logs and live AGENTS/GPL/build rules. Preserve117 immutable rows, source/generated parity, safe empty catalog, all seven registered suites, R2 installer and wrapping fixes. Do not edit another owner's branch by inference. Producer should add these meaningful real-browser negatives, retain valid nonempty history controls, deliver authorized source updates and fresh head/root evidence. Independent Website QA then rechecks the final repaired full root with native test/build/test and actual Chromium regressions. Only after that acceptance may the existing sole Website Main Merger proceed through normal fresh expected-head checks. Parent retains dispatch; this reviewer creates no successor.

Original own threads F1 `PRRT_kwDOVUCQlc6v4gnY` and F2 `PRRT_kwDOVUCQlc6v4gnb` may be resolved with the R2 acceptance link because those exact findings remain verified repaired. No new/unrelated finding is resolved. No review deletion, protection change, source repair, merge, main/nightly/gh-pages write, publication, firmware/vendor upload, device or scheduler action is part of this work.
