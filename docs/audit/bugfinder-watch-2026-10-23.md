# LoopyX0X37 hourly bugfinder — initial audit, 2026-10-23

**Scope:** first manually triggered audit run; documentation and evidence only. No source fix, firmware, installer package, published site, or device write is included in this report.

- Repository: `gobbajogger/X0X37-for-Loopy-Pro` (public at inspection).
- Live `main`: [`c9718ae833eb64a5191c350922593df8a36017c9`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/c9718ae833eb64a5191c350922593df8a36017c9); inspected 2026-10-23. This ref matched the Project bootstrap's 2026-10-23 main record; no earlier **successful hourly bugfinder** checkpoint was established, so this is the baseline rather than a claim of changes since a previous run.
- Source used for local, non-hardware probes: Project Sources bootstrap `source-at-7537a46` (exact tree `152ca944e9cd3d0913906a1d3fa9d8d2826ea639`, cloned from an offline synthetic Git bundle). **It is not current main.** Every reused module/test claimed below was checked against the live GitHub blob SHA; exceptions are labelled.
- Existing overlapping owners: [PR #20](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/20) (installer build registry and hosted-package stale-fetch guard), [PR #18](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/18) (dual SMC), [PR #19](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/19) (SMC popup), [PR #22](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/22) (optimization audit). Do not compete in those active work lanes.

## BF-001 — HIGH — local `.fwsc` file selections can finish out of order

**Status:** NEW, source-backed with controlled async interleaving reproduction; **not yet fixed**, not a physical/browser pass. **Risk:** the user can select local package B after package A, but an earlier pending A read may finish last and enable the install button for A. The user must still explicitly confirm the displayed product, so this is a wrong-selection hazard, not evidence of an unprompted flash.

**Source:** [`web/x0x_installer.html` lines 576–615](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/web/x0x_installer.html#L576-L615). Both file-change and drop handlers `await f.arrayBuffer()` before calling `acceptStock`, while `acceptStock` asynchronously `await sha256Hex(...)` then writes the global `stock`, metadata and enabled state. There is no selection generation/token. The installed-package click handler later passes `stock.image` and `stock.product` directly to `run` at [lines 689–702](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/web/x0x_installer.html#L689-L702).

**Controlled reproduction:** Start file A and suspend its `arrayBuffer`; pick file B and resolve B immediately; then resolve A. The isolated Node interleaving probe uses the unchanged, live-main-matching `web/fm1pkg.js` (`014f5fa3490f476554a17b08e941bde04c7eff74`) to encode two valid product markers. Observed final state: `selected: SMK-37 Pro_9001000 | expected: SMK-37 Pro_9001001`; the Install button is enabled for A. This probe models the exact statement order but **does not execute the complete current main browser page**. Its repro script SHA-256 is `b1e2d84236c844fb0b90033374ea3a55d1e79338ede3d9a16f81846407e8d17f` and observed log SHA-256 is `717ae2482f27471e0488acada4e1cac9063cb91b4134268bd4683bac8fc66f8e` for the exact model-only probe and its output, embedded below.

**Suggested fix:** increment an independent local-file selection generation **in each change/drop handler before awaiting `arrayBuffer()`**; invalidate `stock` and disable Install on new selection; check that generation after both file read and digest and before any state or error/status update; keep the selected product bound to the confirmed immutable file. Add deferred-file-read and deferred-hash races for last-selection-wins, plus cancellation and invalid-package cases in a current-main browser harness. Audit the similarly async `ctl.img` file selector separately. PR #20's stale hosted-`fetch()` guard has different scope: do not duplicate or overwrite its work.

## BF-002 — MEDIUM — scoped quickcheck omits project web regressions

**Status:** NEW, source-backed and reproduced on the matching selection logic in the historical local snapshot; **not yet fixed**. This is a coverage defect, not proof that project import itself fails.

**Source:** [`tools/quickcheck_plan.py` lines 18–35](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/tools/quickcheck_plan.py#L18-L35) routes all `web/` changes to `breaks-web breaks-page install-slots-web`, but omits `projects-web` and `projects-page`. Its [`select()`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/tools/quickcheck_plan.py#L98-L154) indexes only `tests/`, not source references in `web/test_projects*.mjs`; therefore the project suites are not inferred from changes to `web/x0x_projects.js`, `web/x0x_projects.html`, or shared `web/x0x_breaks.js`. The actual suites are exposed by [`tests/run_tests.sh`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/tests/run_tests.sh#L295-L299).

**Reproduction:** Call the offline snapshot's `select(root, ['web/x0x_projects.js'], False)`, and again with `web/x0x_projects.html` and `web/x0x_breaks.js`, using the existing test runner. Each returns `breaks-page, breaks-web, install-slots-web, locked-settings, smk-flash` and **neither** project suite. The routing statements and selection algorithm were checked against main; the entire historical planner file is **not** byte-identical to main, so this is scoped logical evidence, not a full exact-main CI run.

**Suggested fix:** add explicit project suite routes for the project JS/HTML and their shared `x0x_breaks.js` dependency (and modified project tests). Extend `tests/host/test_quickcheck_plan.py` with a fixture that advertises both project commands; assert both suites are selected by production-module edits, and that unrelated UI changes remain scoped. Execute the affected selector tests and project web/page tests on the exact resulting head.

## BF-003 — LOW — retained editor test references a file absent from `main`

**Status:** NEW as an audit observation; no source repair approved because it is unclear whether the editor is retired or must be restored.

**Evidence:** [`web/test_web.mjs` line 28](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/web/test_web.mjs#L28) reads `web/editor.html`, but [the live `web/` directory](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/c9718ae833eb64a5191c350922593df8a36017c9/web) contains no `editor.html`. On the historical snapshot, `node web/test_web.mjs` exits 1 with `ENOENT` at that exact line. `web/test_web.mjs` and its missing path are the same at the checked main. This test is **not** registered in the current `tests/run_tests.sh`, so it is a stale optional suite, not an additional claim that current default CI runs it. `web/make_site.py` still references the same missing source.

**Disposition:** decide whether the editor is still supported; either restore its source and tests with the correct licence/provenance, or retire the obsolete test/site-builder links. Do not silently skip a required test as a "fix".

## Deduplication: known findings retained, not counted as new

The previous 2026-10-21 independent project audit already documented **F02** (installer re-enumeration can move from the intended keyboard to another loader) and **F03** (unknown sample-slot probe can be treated as no slots before firmware change). The remote main blobs for `tools/fm1_install.py` (`77dbe2cfab79a78c953abf7c273dbf6156a35f7e`) and `web/fm1ota.js` (`014f5fa3490f476554a17b08e941bde04c7eff74`) remain identical to the historical source used in that audit. These high-risk items are **not retested here**, and no repair is claimed. The F02 device-target issue is independent of the BF-001 stale file selection issue.

## Tests, CI and limitations

**Executed in this turn, on Linux x86-64 using the offline snapshot:**

| Test | Result | Evidence boundary |
| --- | --- | --- |
| `node web/test_projects.mjs` | PASS 29/29 | Both source test and `x0x_projects.js` / `x0x_breaks.js` match live-main blobs. |
| `node web/test_projects_page.mjs` | PASS 7/7 | Both test and project page sources match live-main blobs. |
| `node web/test_breaks.mjs` | PASS | Test and `x0x_breaks.js` match live-main blobs. |
| `node web/test_breaks_page.mjs` | PASS against historical page | Current-main `x0x_breaks.html` differs; this is **not** current-main page coverage. |
| `python3 tests/install_slots_test.py` | PASS | Test and CLI installer sources match live-main blobs. Does not cover BF-001. |
| Isolated `installer_local_file_race_repro.mjs` | **BUG REPRODUCED** | Controlled interleaving model only; not the full current browser page. |
| Direct scoped planner `select()` probes | **BUG REPRODUCED** | Relevant routing logic matches current-main source, full files differ. |
| `node web/test_web.mjs` | FAIL: `ENOENT web/editor.html` | The file is absent from both the local and current remote trees. |
| `python3 tests/host/test_quickcheck_plan.py` | **INCOMPLETE: command timed out** | No PASS asserted. |

**GitHub Actions:** current-main Pages workflow [run 39816499153](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39816499153) finished with a failed `build` job and skipped `deploy`; PR run [39820154010](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39820154010) failed its focused SMC host job. Both have actual jobs, unlike earlier zero-job startup failures. Job-log retrieval returned `BlobNotFound`, and step summaries were empty; **root causes remain unverified**. A separate dynamic Pages deployment [39822941019](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/39822941019) reports success; this does not prove firmware/build or installer safety.

**Not executed or claimed:** current-main full checkout validation, complete host test matrix, Elite/Pro target firmware builds, memory/flash budgets, `.fwsc`/controller matching, installer preview, physical keyboard/SMC testing, site publication, or stable release. The native sandbox could not resolve `github.com` for a live Git clone; available Project ZIP contains only historical source. SDK/toolchain ZIPs were present but not extracted or used because this audit adds no firmware code.

## Next action and hourly policy

1. Assign the BF-001 installer fix to the current web/installer owner, coordinating with draft PR #20 rather than making an incompatible second edit.
2. Fix BF-002 in a separate narrow code lane with a red/green planner regression and exact-head host tests, without touching firmware or user data.
3. Decide BF-003 retirement/restoration before changing that legacy test.
4. Each subsequent hour compare the live head/PRs with this baseline, deduplicate against these IDs and the 2026-10-21 audit, rotate modules, and only add materially new evidence. No empty commits.

**Publication scope:** audit report branch only; `main` and `gh-pages` unchanged, no forced push, device or public release. Findings are not "fixed" merely because this report exists.

## Appendix — executable asynchronous-file interleaving model

This is a **logic model**, not copied from or run inside the live browser DOM. Save it at `evidence/bugfinder/installer_local_file_race_repro.mjs` of a source checkout and run `node evidence/bugfinder/installer_local_file_race_repro.mjs`. It imports `web/fm1pkg.js` from that checkout. The result `BUG REPRODUCED` is expected until the browser selection logic is repaired; a zero exit here **does not mean production passed**.

```js
// Isolated interleaving probe modeled after current main web/x0x_installer.html lines 576-615.
// Uses the exact unchanged web/fm1pkg.js productOf/logicalImage functions.
// This reproduces a scheduling bug; it does not mock WebMIDI or flash hardware.
import assert from 'node:assert/strict';
import { webcrypto } from 'node:crypto';
import { productOf, logicalImage } from '../../web/fm1pkg.js';

function makePackage(identity) {
  const raw = new Uint8Array(960); raw.fill(0x7D);
  for (let i = 0; i < 20; i++) {
    const ch = i < identity.length ? identity.charCodeAt(i) : null;
    raw[i * 0x30 + 0x2F] = ch === null ? 0x7D : (ch + i + 1) & 255;
  }
  assert.equal(productOf(raw), identity);
  return raw;
}
function deferred() {
  let resolve;
  const promise = new Promise((r) => { resolve = r; });
  return { promise, resolve };
}
const state = { stock: null, disabled: true, text: '', message: '', writing: false, checked: true };
const sayK = (m) => { state.message = m; };
const sameKeyboard = () => true;
let lastId = null;
const STOCK_SHA256_FM1 = 'not-the-digest-of-these-test-packages';
async function sha256Hex(buf) {
  const hex = new Uint8Array(await webcrypto.subtle.digest('SHA-256', buf));
  return [...hex].map((n) => n.toString(16).padStart(2,'0')).join('');
}
// Statement ordering matches acceptStock(raw) on main c9718ae, lines 576-599.
async function acceptStock(raw) {
  state.stock = null;
  state.disabled = true;
  state.text = '';
  try {
    const product = productOf(raw);
    const model = product.split('_')[0];
    const hex = await sha256Hex(raw.buffer);
    const isFm1Stock = hex === STOCK_SHA256_FM1;
    if (!isFm1Stock && !/^(FM-1|SMK-37|MKE-P37|STARRYKEY|ota-)/i.test(model)) {
      sayK('stockbad'); return;
    }
    if (lastId && !sameKeyboard(lastId.model.replace(/^ota-/i, ''), model)) {
      sayK('model'); return;
    }
    state.stock = { image: logicalImage(raw), product, foreign: true, fm1Stock: isFm1Stock };
    state.text = `${product} · ${(raw.length / 1024).toFixed(0)} KB`;
    state.disabled = state.writing || !state.checked;
    sayK('stockok');
  } catch { sayK('stockbad'); }
}
// Statement ordering matches the stockfile change handler on main, lines 600-604.
async function onStockFile(file) {
  await acceptStock(new Uint8Array(await file.arrayBuffer()));
}
const one = makePackage('SMK-37 Pro_9001000');
const two = makePackage('SMK-37 Pro_9001001');
const slowA = deferred();
const first = onStockFile({arrayBuffer: () => slowA.promise});
const second = onStockFile({arrayBuffer: async () => two.buffer});
await second;
assert.equal(state.stock.product, 'SMK-37 Pro_9001001');
slowA.resolve(one.buffer);
await first;
assert.equal(state.stock.product, 'SMK-37 Pro_9001000');
assert.equal(state.disabled, false);
console.log('BUG REPRODUCED: first selection A finished after later selection B; older A is enabled for Install.');
console.log('selected: '+state.stock.product+' | expected: SMK-37 Pro_9001001');
```
