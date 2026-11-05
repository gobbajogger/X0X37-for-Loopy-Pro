# Open sourcing gap: "cloud only / never operate the owner's Mac" — 2026-11-05

Status: OPEN. This note records a missing source. It does not add, change or
repeal a rule, and it is not an owner quotation.

## Finding

No exact, dated owner quote of the form "cloud only" / "never operate the owner's
Mac" was found in the DnBOX release-plan ledger or in the cached issue #832
comments. `docs/rules/current-owner-contract.md` states the rule as current
policy; its exact owner-dated source is not located.

## Sources checked

- `/workspace/dnbox/release-plan.md` (coordinator box ledger, 462 lines), including
  owner entries t20u, t24u, t27u, t55u, t56u/t57u, t58u/t59u. Searched for "Mac",
  "cloud only", "cloud-only", "owner's Mac".
- `/workspace/c832a.json` (box cache of issue #832: 30 of 63 comments, oldest
  2026-11-03T16:38:53Z, newest 2026-11-04T11:06:27Z). Matches for "Mac" are
  agent/bot technical reports (for example a host-name note), not owner rule text.
  Later #832 comments were seen only as ledger summaries, not raw bodies.
- `/workspace/docs-audit/inventory.md` and
  `/workspace/docs-audit/consumption-receipt-20261105.md` (prior audits; both
  report the same gap).
- `docs/rules/current-owner-contract.md` and `docs/audit/dnbox-rules-20261105.md`
  at commit `6956b49d7cbced9713220bc33e5a6f7cfabd1dd6`.

## Closest documented wording (exact quotes; not owner quotes unless stated)

- `docs/rules/current-owner-contract.md` L17: "Work in the **cloud**, using a qualified supported executor and its actual tools."
- `docs/rules/current-owner-contract.md` L20: "Do not use or transfer this work to the owner's Mac. Platform names do not prove"
- `docs/rules/current-owner-contract.md` L69: "installation. Keep prior safe versions. Agents never install or flash. Production"
- `/workspace/dnbox/release-plan.md` L396 (t56u/t57u, coordinator summary of owner, 2026-11-05 ~00:12 CST): "- Owner: I own DnBOX start-to-finish via Dev/QA/existing roles; arrange authorized push/merge/NIGHTLY publication through owners; verify before claiming."
- `/workspace/dnbox/release-plan.md` L404 (t59u): "- t59u: owner confirms I own orchestration of the whole project and its smooth, reliable flow."
- `/workspace/docs-audit/inventory.md` L35: "Closest documented form is Work-routing lane text (Safari unused / agents do not access device) and t55u cloud orchestration ownership."
- `/workspace/docs-audit/inventory.md` L107 (summary of t55u–t57u): "no Mac-agent operating instructions in those statements."

None of these is an owner-dated statement of "cloud only" or "never operate the
owner's Mac". The release-plan lines are coordinator summaries.

## Request

An owner-dated confirmation (date, channel and exact wording) of the cloud-only /
no-owner-Mac rule is requested. When it exists, DnBOX Docs records its source and
chronology in the contract. Until then, treat the contract text as policy stated
in commit `6956b49d`, with this gap retained.
