# Nightly / Public firmware catalog admission policy

Owner decision: 2026-10-23. This is a *development-channel rule*, not a stable-release authorization.

## Channel membership

- **Nightly:** full immutable status history of known development builds, superseded revisions, failed builds, missing artifacts and source-only checkpoints. Non-installable records must be visibly labeled and must never offer a flash link. Firmware only becomes a downloadable NIGHTLY / HARDWARE TEST candidate after a separately authorized exact-byte build, controller, model/CRC, license and installer/data-preservation safety gate.
- **Production/public installer:** only **working DEV** firmware with a matching per-model physical-device PASS and attributable evidence for the exact same source revision, package SHA-256, model, controller image and device; plus passing non-hardware validations, installer backup/readback checks, independent review and explicit owner authorization for public distribution. A checksum, build PASS or \`DEV CHECKED\` alone is insufficient. Per-model verification does not confer approval on any other model.
- Failed, source-only, merely compiled, device-untested, unknown and unapproved versions are **not public picker options**. A superseded version can be offered only with explicit verified rollback approval. An empty public picker is the correct fail-closed result when no device-verified build exists.

## Read-only production planning helper

\`python3 tools/catalog_public_gate.py --catalog /path/to/verified-site-versions.json --approved-device-evidence /path/to/independently-reviewed-device-approvals.json --out /path/to/public-plan.json\`

The input approvals are **not** automatically trusted. The designated publisher must independently authenticate the referenced device report bytes, their digest, model identity, source/build record, controller, successful physical test and owner publication permission. \`tools/catalog_public_gate.py\` emits a *plan only*, never edits a hosted site.

Run regression tests with \`python3 -B tests/catalog_public_gate_test.py\`. A valid empty approvals map \`{}\` produces \`latest: null\` and no eligible versions for the currently available hardware-untested DEV packages. Before an approved production publish, rebuild both \`versions.json\` and the HTML's embedded catalog from the **same** filtered plan, verify exact parity, inspect browser selection and negative cases, and confirm old firmware binary bytes and unrelated assets remain unchanged. The existing Pages branches must move only by an authorized nonforced fast-forward after checking their live heads. NEVER remove old firmware files as a shortcut to hide them from the picker, relax backup/readback/model/controller guards, or rewrite history.

## Verified starting situation

At inspected production \`gh-pages\` commit \`dcb41f2ead9e2166ec3dddcd658e445d22a5fac9\`, catalog blob \`e573799f5d870005c7e170b71dcce435a585ce74\`, versions \`dev-20261022-1638-e894804\` and \`dev-20261022-1629-116d903\` both declare \`hardware_tested:false\` for Elite and Pro. Therefore **zero currently verifiable public-eligible versions**. This source branch does not update the public site, does not delete the older packages, and cannot itself attest hardware success.

Nightly complete read-only inventory is developed independently in \`gobbajogger/x0x37-for-Loopy-Pro_nightly\`, branch \`lane/catalog-nightly-preview-verified-only-20261023\`. Its installable catalog must remain empty under nightly \`AGENTS.md\` absent candidate-specific approval; this separation is intentional, not a synchronization failure.
