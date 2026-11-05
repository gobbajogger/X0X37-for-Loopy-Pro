# Push C dry-run SUMMARY — nightly-ce278129-dev

**Timezone note:** times below that come from git/GitHub as UTC are converted; box-local stamps are Asia/Vladivostok (UTC+10).

## Hard rules observed

- No push, force-push, merge, publish, GitHub write, secrets, binaries added, openbeta ledger, or firmware byte changes.
- Throwaway clone only under `/workspace/pushc-dryrun/`.
- Installer `dev-checked` fail-closed gate **not** removed or weakened.

## Base (gh-pages tip before dry-run)

- **commit:** `3c427179774016fc29d7baf7748e635c5423a11b`
- **tree:** `2e43d9dd6cd175b9502efb6ac3e1c8d9649c2174`
- Message: `nightly: publish nightly-ce278129-dev (UNREVIEWED · DEVICE NOT TESTED)` (versions.json only; installer catalog still 28).

## Dry-run commit (local branch, not pushed)

- **branch:** `evidence/pushc-dryrun-ce278129-20261105`
- **commit:** `b92d5fbe0397630f0775fb0d5330cb5708d6bd91`
- **tree:** `910dba511d7d6afd2d16dc91ad891d27cf4d94ba`
- **author/committer:** gobbajogger <255244738+gobbajogger@users.noreply.github.com>
- **trailers:** none

### diff --stat (vs base)

```
 install/index.html | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

- `versions.json` **unchanged** in the dry-run commit (already had 29 entries / latest ce278129 / status unreviewed).
- Non-catalog prefix/suffix of `install/index.html` are **byte-identical** to base.
- The 28 older catalog entries remain **byte-identical** to the previous embedded catalog / versions.json[1:].

## Mechanism finding

1. **Browser gate (what makes an entry selectable):** `availableModels` / picker filter require `ver.status === "dev-checked"` (model package status undefined or `dev-checked`). See install/index.html around the `availableModels` definition and the versions list filter. There is **no** alternate `dev-selectable` flag.
2. **Precedent `6427d068` (nightly-8be16486-dev):** changed only `versions.json` + embedded catalog in `install/index.html`; set the new entry to `status: "dev-checked"` with owner-authorized early publication wording. Installer logic unchanged.
3. **Why status=dev-checked is required for selectability:** with any other status (including `unreviewed`), `availableModels` returns `[]`, so the entry is omitted from the selectable list. Embedding alone is not enough.
4. **Renderer:** `scripts/render-qualified-nightly.mjs` `renderInstaller` embeds catalog via `JSON.stringify(...).replace(/</g, '\\u003c')` into `web/x0x_installer.html`'s `/*CATALOG*/`. Full CLI `validateCatalog` **rejects** `status !== "dev-checked"` (and expects `config: "dev"` + hardware-test label). Current on-disk `publicCatalog` would also **strip** `display_version`, which would mutate the 28 older entries — so this dry-run applied the renderer's **stringify embed formula** to the existing `versions.json` **as recorded**, replacing only the catalog literal inside the **current** `install/index.html` (logic byte-identical). Full template re-render was not used because `/workspace/nightly/web/x0x_installer.html` is not byte-identical to the live installer shell.

## Option A vs B (status not chosen here)

| | Option A (committed dry-run) | Option B (alternative diffs only) |
|--|--|--|
| ce278 `status` | `unreviewed` | `dev-checked` |
| Embedded catalog count | 29 | 29 |
| Selectable in installer? | **NO** (gate fail-closed) | **YES** |
| Files | `pushc.patch` | `alternative-status-dev-checked*.diff` |

## Delivery check (github.io) — Asia/Vladivostok session

- `install/index.html`: HTTP **200**, size 596990 (matches pre-dry-run live tip; dry-run not published).
- `versions.json`: HTTP **200**, size 68298.
- Elite fwsc: HTTP **200**, sha256 `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` (match).
- Pro fwsc: HTTP **200**, sha256 `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` (match).

## Headless installer UX parse (dry-run tree)

- Embedded catalog versions: **29**; latest `nightly-ce278129-dev`; status `unreviewed`.
- Selectable via gate simulation: **28** (all prior `dev-checked`); **ce278129 NOT selectable**.
- Intended disclosure text is present on the ce278129 entry (UNREVIEWED · DEVICE NOT TESTED; 391 ok / 6 FAIL; package_nightly --check NOT_RUN; DnBOX DEV; display_version carried label).

## sha256

- pushc.patch: `335912e1e9d484242066660f5b26d3ed52dd8ca3aa672ed9f9118d8ff3e9845f`
- install/index.html (dry-run result): `c46ceb06fbf73f7c3c9254f7842bcf190e78c90344a325ee2b5a7b7058eb87ed`
- versions.json (unchanged): `59222a60b019ece72a19f0ff5bfa6f10f37720336af9621afcde030f7a5f2581`

## Paths

- Clone: `/workspace/pushc-dryrun/nightly-gh-pages`
- Patch: `/workspace/pushc-dryrun/pushc.patch`
- Alternative status diffs: `/workspace/pushc-dryrun/alternative-status-dev-checked.diff`, `/workspace/pushc-dryrun/alternative-status-dev-checked-versions.diff`
- D6-N draft: `/workspace/pushc-dryrun/D6-N-status-decision.md`
- This summary: `/workspace/pushc-dryrun/SUMMARY.md`
- Delivery downloads: `/workspace/pushc-dryrun/delivery-check/`

## Exact commands used (abridged)

```bash
mkdir -p /workspace/pushc-dryrun
git clone --branch gh-pages --single-branch https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git nightly-gh-pages
cd nightly-gh-pages
git config user.name gobbajogger
git config user.email 255244738+gobbajogger@users.noreply.github.com
# inspect 6427d068, gate, versions.json vs embedded catalog
# embed: node JSON.stringify(versions.json).replace(/</g,'\\u003c') into install/index.html catalog slot only
git checkout -b evidence/pushc-dryrun-ce278129-20261105
git add install/index.html
git commit --no-verify -m 'Push C dry-run: sync installer embedded catalog ...'
git format-patch -1 --stdout > /workspace/pushc-dryrun/pushc.patch
curl github.io install + elite/pro fwsc; sha256sum
# NO git push
```

## Open questions

1. Coordinator must choose **Option A** (catalog sync, still not selectable) vs **Option B** (also set `status=dev-checked` to make selectable) before any real push.
2. If Option B: whether to also normalize `config` to `dev` / hardware-test label for renderer `validateCatalog` compatibility, or only flip status for the browser gate.
3. No pre-existing `D6-N-status-decision.md` was found in the nightly/main trees; this draft is new under `/workspace/pushc-dryrun/`.
