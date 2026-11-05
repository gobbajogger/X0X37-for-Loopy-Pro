# D6-N status decision — nightly-ce278129-dev

**Scope:** Push C dry-run only. No publication. Status choice is reserved for Sanity Checker / coordinator.

**Candidate:** `nightly-ce278129-dev`  
**Source commit:** `ce27812951555ff7ed9694bf99d20a65dd5f410b`  
**Source tree:** `a917fb6a2c738bb410601bd784278bf35c142f1c`  
**Catalog tip today:** versions.json already lists this entry; installer embedded catalog lagged at 28 entries (latest `nightly-8be16486-dev`).

## Plain disclosure (must accompany any status decision)

- **UNREVIEWED**
- **DEVICE NOT TESTED**
- Host suite: **391 ok / 6 FAIL** (including `original-release-ui`; also `smc-ui`, `offline-sequencer-browser`, `ui-feedback`, `ui-glyph-polish`, `scenario-plock`)
- `package_nightly --check`: **NOT_RUN**
- Firmware images show **"DnBOX DEV"**
- `display_version` **0.0.24.5** is a **carried catalog label only** (not the firmware's own string)
- **ROADMAP not proven**

These facts are already reflected in the versions.json description for this entry.

## Installer gate (fail-closed; do not bypass)

In `install/index.html`, `availableModels` / picker listing require `status === "dev-checked"` (and model `status` undefined or `dev-checked`). There is **no** separate documented `dev-selectable` flag. Entries with `status: "unreviewed"` remain in the catalog JSON but are **not listed / not selectable**.

Precedent for making a build selectable: commit `6427d068` added `nightly-8be16486-dev` to **both** `versions.json` and the embedded catalog with **`status: "dev-checked"`** (owner-authorized early publication while QA continued). That commit changed only catalog data (versions.json + install/index.html catalog blob); installer logic was untouched.

## Options for Sanity Checker / coordinator (this dry-run does **not** choose)

### Option A — keep `status: "unreviewed"` (dry-run commit content)

- Sync installer embedded catalog to versions.json (29 entries).
- **Result:** ce278129 is present in the catalog but **still not selectable** under the fail-closed gate.
- Matches current recorded status; disclosure above remains accurate.
- Patch: `pushc.patch` on branch `evidence/pushc-dryrun-ce278129-20261105`.

### Option B — set `status: "dev-checked"` (separate alternative diff only)

- Same catalog sync, plus change this entry's `status` from `unreviewed` to `dev-checked` (clearly separated, reviewable hunk).
- **Result:** ce278129 becomes selectable; disclosure text (label/description) should still state UNREVIEWED / DEVICE NOT TESTED / suite FAIL / etc. unless a later reviewed decision rewrites them.
- Does **not** change installer logic.
- Note: full `scripts/render-qualified-nightly.mjs` `validateCatalog` additionally expects `config: "dev"` and the exact hardware-test label for a *qualified* prepare path; that is a renderer qualification requirement, not the browser gate. Option B as shown is the minimal status flip the installer gate needs.
- Diffs: `alternative-status-dev-checked.diff` (install/index.html vs Option A) and `alternative-status-dev-checked-versions.diff` (versions.json).

## Decision

**UNDECIDED in this dry-run.** Coordinator / Sanity Checker must pick Option A or B (or a documented variant) before any real push.
