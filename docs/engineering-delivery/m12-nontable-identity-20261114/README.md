# M12 non-table identity source stage

The table-only adapter could not select the canonical COMP1 request without inventing a historical table row. This additive identity selector lets a separately accepted policy select exact public Request/source metadata while preserving every evidence obligation. It does not establish that the browser companion works or that the instrument is complete.

Base: #926 `0e2b7b78d223e10af1e9172db6dee657a8d590cb`. Isolated branch: `wip/m12-nontable-identity-20261114`. Only the standalone adapter, its real-Git CLI tests and source-stage documentation change. Existing checker/runner and product policy remain untouched.

The [usage guide](../m12-adapter-20261114/README.md) describes the selector and immutable-input trust boundary. The [specification](../../superpowers/specs/2026-11-14-m12-nontable-identity-design.md), [plan](../../superpowers/plans/2026-11-14-m12-nontable-identity.md) and [verification receipt](VERIFICATION.json) record the exact source-stage scope.

## Executed proof

- Baseline 40 focused tests passed. The new 51-test suite was observed RED because the selector was absent, then GREEN after implementation.
- A malformed empty preserved-source identity reproduced false acceptance in one additional test. Its repair and the accepted/forged non-source COMP1 receipt checks produced 53/53 focused tests PASS before independent review. Syntax and diff-whitespace checks passed.
- The actual CLI exercised public #887 metadata at `9ec6810ab41777feb4c44f048b1df3d550c77f86` with disposable synthetic candidate, policy and evidence. It parsed 187 canonical records and all 122 numbered requests. Synthetic source-only COMP1 passed; omitted required evidence stayed NOT_RUN; wrong source digest and wrong queue document were rejected. This is parser/policy-consistency proof only.

## Independent review and repair

Independent Sol review found three concrete P2 gaps: indented/orphan table provenance allowed fallback, indented duplicate source fields escaped uniqueness checks, and Unicode/control line splitting authenticated a prefix instead of the exact source value. The author reproduced all three through real-CLI regressions: the 57-test suite was RED with 14 false-acceptance failures, then GREEN after one repair pass. Syntax, whitespace and the unchanged public COMP1 exercise passed again.

The new selector now conservatively refuses visible preserved-source markers and pipe-leading table-shaped lines, including unrelated tables. Such a future record needs explicit provenance review instead of fallback. Existing strict table selection is not broadened. LF/CRLF delimit logical lines; other Unicode/control separators remain exact UTF-8 value bytes. A narrow same-reviewer check accepted the repaired exact source closure, verified all original adversarial outcomes and passed the four repair tests. See the [review receipt](REVIEW.md). No source delivery is asserted by this local receipt. No product policy, companion payload, firmware/native/device check, release hook, package/preview/publisher integration, merge or publication was used or performed. Deferred #119, declined 26.7 and undefined M5 remain held. Downstream owners retain actual policy and acceptance-boundary work.
