# CLI-MODEL-01 revision 2

Candidate source tree: `20c60779d0bb3c086bd6f40a3348df44dfaac9a8`. Repair delta applies to reviewed tree `90b7b2f79196a8840fb174adff39ad0e5edd9a0f`; complete cli-safety.patch applies to original proposal `2e38c67d3dbf1b905085580229e395433aefd92d`. See identity.json for verified full pins and SHA-256.

The original installer checked the starting device but accepted any single newly appeared loader. Four real CLI synthetic transitions reproduced unsafe loader admission: Pro to Elite, Elite to Pro, and each model to FM-1. Both matching-model positive controls passed. red.log records four failures before the repair.

After discovery, install() now checks the loader identity with same_model(), closes a mismatching link and raises the model error before write() can send UPGRADE or package data. Discovery and its ambiguity refusal remain unchanged. Regression assertions require model refusal, only handshake packets to the loader, zero loader upgrade/data exchange, and closed links. Matching renamed Pro and Elite loaders remain positive controls.

The initial controller regression run found that shared FakeSMK always returned a Pro loader even for Elite. The normal fixture now retains the initial model; adversarial cases explicitly override the transition. Controller production code and assertions are unchanged; the initial fixture failure is retained. results.json and individual logs contain final focused results. Prior version/package evidence remains scoped and reusable because those inputs are unchanged. No firmware, backup/readback/controller/version production path changed.

Author verification only. Return this exact tree and repair delta to the same independent reviewer for CLI-MODEL-01 closure through parent #698/#806. No independent closure, hardware, merge, firmware publication or selected source commit is claimed.
