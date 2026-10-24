# PR8 R5 — existing R28 evidence URL ref repair

Producer follow-up to independent R4 review [5638658907](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638658907) and evidence commit `7318f2affd81c52827e5aa3c45c234440f4f55d3`. Parent/base is immutable PR8 R4 `02a5a05ee0aa9677884aadf1e0bb48e1b31d6cad`, root `eb903cc7a8ae658e99894b0e7b7b995eb9437c74`.

The only runtime/validator change is the existing Catalog R28 immediate Git-ref guard from `9d03fe036220c02b224a91a90c0bb09b66fd29fe`. Complete validator blob now equals its existing `9cd11cf06a1277238d5eafc470ab315177f99264`. It compares evidence_commit to the SHA immediately after commit/tree/blob; a different Git ref cannot be authenticated by repeating the claimed SHA later in the pathname. This repairs future malformed source-evidence admission. Current real records, firmware eligibility and browser behavior are unchanged.

Six default regression cases preserve the R28 route assertions: legitimate commit/blob/tree controls, wrong blob/tree ref with claimed SHA later in the path, and a mutable-ref control. The fixture row is explicitly synthetic and noninstallable. The R28 test's clone-last-row helper is adapted to an explicit future row because our frozen R25 audit tail has separate fixed attribution; cloning that audit would fail for an unrelated reason. No audit pin is weakened. Original R27 negatives/positive remain registered.

## Executed RED/GREEN

`final_matrix_extension.py` is copied byte-for-byte from the independent R4 evidence. Its existing `/workspace/pr8-independent-qa` source alias points to the canonical repair checkout. Actual validators execute from private temporary copies. No production/test guard is stubbed out.

- Before guard change, the supplied exact matrix is 5/6: matched-ref accepts; the all-f blob ref with all-1 claimed SHA later in its path incorrectly accepts; wrong-tree and mutable controls reject. Both unrelated existing browser/size groups pass. See `final-matrix-red.log`.
- Before guard change, updated default status regressions pass all three legitimate route controls, then fail on wrongly accepted nested-blob SHA. See `status-red.log`.
- Final build, full npm test, genuine default Chromium and supplied final matrix commands/results are in `consolidated-commands.json` with complete logs. Default tests include both nested-blob and nested-tree negatives and all prior registered guards.

Only validator, registered status tests and this evidence change from R4. Browser source/generated HTML, package preparation, model/binding/backup implementations and all117 immutable rows/two historical builds are byte-identical to independently reviewed R4. Existing R4 independent F1–F6/B/S checks remain attributable only to that R4 root; final acceptance of the new root belongs to independent QA. The default Chromium suite is rerun here. No upload/capability investigation is performed in this bounded turn.

## Exact next consumer and remaining gates

Keep PR8 draft/HOLD MERGE pending distinct independent exact-head review. Parent coordinates that reviewer and the existing sole Website Main Merger/Pages publisher. No protected ref, firmware, release, tag, scheduler or hardware action occurs.

The merger's newer [comment6385653457](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#issuecomment-6385653457) separately requires integration of Catalog's119-row union. Parent's explicit R5 scope says **no moving-history rebase**, so this narrow repair preserves its117-row input rather than claiming119-row integration. A selected combined tree must preserve both later records and all browser fixes/test registrations and receive fresh combined-root checks; do not overwrite newer history or treat this receipt as that integrated-tree approval. Real firmware qualification and immutable artifact publication remain separate gates; actual picker stays empty.
