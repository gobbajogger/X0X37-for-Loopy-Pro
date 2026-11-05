# Authorized project delivery

**Current rules / release boundary:** [RULES.md](../../RULES.md) (source of truth). This file is only a short adapter for this production checkout.

- Adopting workflow text does not promote firmware or grant device access.
- Resolve development `nightly` to its full SHA before reading its workflow skills; also apply this checkout's RULES.md.
- Route merge/publish candidates to the existing sole integration and publication owners. `tools/merge_project_prs.py` is read-only and grants no merge authority.
- Historical records: [workflow-history.md](workflow-history.md). Evidence selection: [local-proof.md](local-proof.md).
