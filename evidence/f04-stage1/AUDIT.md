# Stage 1 audit of validated source a03cfab3

Audited tree: `a03cfab3a423ab5c65a6546afd4993fe51754ee7` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Git tree (`source_tree`): `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`.
Repair commit present in tip: `4f18983a` (F03 fail-closed slot probe).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f04-bf001-stage1-fb90`.
This thread: `bc-b24569bd-4f94-5a38-abb3-ed5ce76d4012` (next-cycle Stage-1 audit after F03 Stage 7 publish).
Prior cycle audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Evidence-bearing base: remote tip of `cursor/f03-slot-probe-stage7-fb90` (docs/evidence only; packages authenticate `a03cfab3`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `08d3164a…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live HTML | `nightly-a03cfab3-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Elite blank (hosted Stage-4) | size 651396, sha256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` **MATCH** |
| Pro blank (hosted Stage-4) | size 651392, sha256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` **MATCH** |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip | `4b3c5994eabd14aa1a1a575b2211800a478112b4` |
| Catalog `source_commit` / `source_tree` | `a03cfab3…` / `85e159c5…` **MATCH** |

Cited blobs at `a03cfab3`:

| File | Blob |
| --- | --- |
| `web/x0x_installer.html` | `c98b67815b2b824f306f2bd867dc7d04886fe7a6` |
| `tools/quickcheck_plan.py` | `52f1f06faddea0f6bf6c2c89c9b41c50906a66fd` |
| `web/test_web.mjs` | `7fbe81bf4905600c92613db7321449106f3e7a31` |

BF definition source (not on tip; open docs-only PR): [PR #26](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/26) / `docs/audit/bugfinder-watch-2026-10-23.md` on `lane/bugfinder-reports-20261023`.

## Selected defect

**BF-001. Local `.fwsc` (and related) file selections can finish out of order so an older package replaces the most recently chosen one.**

Oldest deferred defect from `evidence/f03-stage2/DEFERRED.md` that is **not** already fixed in `a03cfab3`, now with a recorded definition (PR #26). F03 is present in this SHA and is not selected.

## Evidence

File-change and drop handlers await `arrayBuffer()` then call `acceptStock`, which awaits `sha256Hex` before writing global `stock`. There is no selection generation token:

```603:641:web/x0x_installer.html
async function acceptStock(raw) {
  stock = null;
  $("stockgo").disabled = true;
  $("dropmeta").textContent = "";
  try {
    const product = productOf(raw);
    const model = product.split("_")[0];
    const hex = await sha256Hex(raw);
    ...
    stock = { image: logicalImage(raw), product, foreign: true, fm1Stock: isFm1Stock };
    ...
  } catch (err) { sayK("stockbad"); }
}
$("stockfile").addEventListener("change", async (e) => {
  const f = e.target.files && e.target.files[0];
  if (!f) return;
  await acceptStock(new Uint8Array(await f.arrayBuffer()));
});
...
drop.addEventListener("drop", async (e) => {
  const f = e.dataTransfer.files && e.dataTransfer.files[0];
  if (!f) return;
  await acceptStock(new Uint8Array(await f.arrayBuffer()));
});
```

The optional `ctl.img` selector has the same unprotected async pattern (`:588–598`). PR #26’s controlled Node interleaving model reproduced last-finisher-wins for stock; draft fix PR #31 is **not** an ancestor of `a03cfab3`.

## Affected scope

- `web/x0x_installer.html`: `acceptStock`, `stockfile` change, drop handler; preferably `ctlfile` with the same generation rule.
- Host / Node race regression (no real MIDI, no device write) proving last selection wins.
- Out of scope: firmware, controller image bytes, package application bytes, production `main`, production Pages, device flash, BF-002, BF-003, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets.
- Do **not** stack-merge draft PR #31 (depends on #20/#24, not in tip). Reimplement the generation-token fix cleanly on the F03 tip lineage.

## Proposed fix

1. Increment a local-selection generation **before** awaiting `arrayBuffer()` on each file/drop (and ctl) selection.
2. Clear `stock` / disable Install on new selection; require a fresh confirmation after a successful read.
3. After read and digest, apply state only if the generation still matches (newest selection wins; stale completions no-op).
4. Add a host regression: slow-then-fast interleaving must leave the newer package selected; cover invalid/stale paths.
5. Implement on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f04-bf001-stage1-fb90`.

No firmware build required. Development configuration if ever needed: `X0X_DEV_PADCAL=1`.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and the deferred-order rule select BF-001 as the next defect without asking. PR #26 already records the finding and suggested fix outline. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Not approved in this repair: BF-002, BF-003 (needs retire-vs-restore decision), withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE`, stacking PR #31 as-is.

## Missing decisions

None block the generation-token repair for local stock (and preferably ctl) selection. BF-003 still needs a retire-versus-restore decision before any Stage-2 change to `web/test_web.mjs` / editor — that decision is **not** required for BF-001. Optional: whether ctl hardening must ship in the same Stage-2 commit (recommended yes; same race class).

## Defects considered and not selected

### Already fixed in `a03cfab3`

| Item | Disposition |
| --- | --- |
| F03 fail-closed slot layout probe | Present (`4f18983a`). Closed on this SHA. |
| F02 fail-closed loader attach | Present (ancestor). Closed on this SHA. |

### Deferred list (`evidence/f03-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| BF-001 | **Selected.** Defined in PR #26; still present on tip. |
| BF-002 | Younger. Still present (`tools/quickcheck_plan.py` `web/` route omits `projects-web` / `projects-page`). Defer. |
| BF-003 | Younger. Stale `web/test_web.mjs` → missing `editor.html`. Needs retire/restore decision. Defer. |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial/interrupted-loader resume by F02 design; not a commanded-transition wait. |
| Longer `SLOT_PROBE` budgets | Optional hardening only after F03 fail-closed; still 1.0s/2 native and 1000ms/2 web. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F03 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live. Nightly FF REFUSED is divergence, not a package/host FAIL. Optional Pillow host gaps are not-run, not FAIL.

## Stage 2 limit

Implement only BF-001, with the generation-token local-selection repair above, on a new stage-2 branch from the pushed tip of `cursor/f04-bf001-stage1-fb90`. Do not take BF-002–003 or other deferred items in that same change. Do not merge stacked PR #31; reimplement on this tip.
