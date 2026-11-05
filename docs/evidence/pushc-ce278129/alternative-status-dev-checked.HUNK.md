# Option B — explicitly reviewable status hunk (NOT applied in dry-run commit)

Against Option A / current versions.json entry for `nightly-ce278129-dev`:

```diff
--- versions.json (Option A / recorded)
+++ versions.json (Option B alternative)
       "version": "nightly-ce278129-dev",
       "label": "UNREVIEWED · DEVICE NOT TESTED",
-      "status": "unreviewed",
+      "status": "dev-checked",
       "config": "X0X_DEV_PADCAL=0",
```

The same one-field change is the only semantic delta inside the embedded `const catalog = …` blob in `install/index.html` (full unified diffs are large because the catalog is one line).

Machine diffs:
- `alternative-status-dev-checked-versions.diff`
- `alternative-status-dev-checked.diff`
- Full Option B files: `versions.alternative-dev-checked.json`, `install-index.alternative-dev-checked.html`

Installer gate effect: with `status=dev-checked`, `availableModels` returns elite/pro and the picker lists the entry. Disclosure fields (label/description) stay UNREVIEWED / DEVICE NOT TESTED / 391 ok / 6 FAIL / etc. unless separately rewritten.
