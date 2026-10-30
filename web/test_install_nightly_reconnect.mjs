// SPDX-License-Identifier: GPL-3.0-only
// Exact installer scan/run bodies against mock Web MIDI; no device or firmware touched.
// Run: node web/test_install_nightly_reconnect.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const html = readFileSync(process.argv[2] || new URL("./x0x_installer.html", import.meta.url), "utf8");
function extract(anchor) {
  const i = html.indexOf(anchor), j = html.indexOf("\n}", i);
  assert.ok(i >= 0 && j > i, "missing exact source function: " + anchor);
  return html.slice(i, j + 2);
}
const scan = extract("async function scan(allowPostWrite = false) {");
const run = extract("async function run(img, product, opts = {}) {");
assert.match(scan, /if \(writing && !allowPostWrite\) return;/);
assert.match(run, /await scan\(true\)/);
assert.match(run, /!window\.confirm\(t\("otaconfirm"\)\)/);
assert.match(html, /otaconfirm: "RECOVERY MODE/);
assert.match(html, /otaconfirm: "復旧モード/);
assert.match(run, /if \(writing\) return;/);
assert.match(html, /if \(writing \|\| ctlNeedsResolution \|\| \$\("go"\)\.disabled \|\|\s*!hasVerifiedNightlySelection/);
let passed = 0;
for (const model of ["SMK-37 Elite_9001100", "SMK-37 Pro_9001100"]) {
  for (const mode of ["normal", "resume", "resume-cancel", "missing", "failure", "denied", "pending", "pending-stale", "revoked"]) {
    let grant;
    let approvalActive = mode !== "revoked";
    const permission = new Promise(resolve => { grant = resolve; });
    const state = { writes: 0, postScan: 0, connected: null, lockHeld: true,
      messages: [], midiCalls: 0, otaPrompts: 0 };
    const ui = Object.fromEntries(["go", "stockgo", "stockconfirm", "conn", "bar"]
      .map(id => [id, { disabled: false, checked: true, textContent: "", value: 0 }]));
    let ctx;
    const mock = {
      $: id => ui[id] || (ui[id] = {}),
      midiAccess: async () => {
        state.midiCalls++;
        if (mode === "denied") throw Object.assign(Error("denied"), { code: "denied" });
        return mode.startsWith("pending") ? permission : {};
      },
      Updater: class {
        async find(predicate) {
          if (state.writes > 0) {
            if (!ctx.isWriting()) state.lockHeld = false;
            state.postScan++;
            if (mode === "missing") return null;
            const id = { model: model.split("_")[0], text: "USB keyboard" };
            return predicate(id) ? { id, name: "Keyboard", link: { close() {} } } : null;
          }
          const ota = { model: "ota-" + model.split("_")[0], text: "ota" };
          return ["resume", "resume-cancel"].includes(mode) && predicate(ota)
            ? { id: ota, link: { close() {} } } : null;
        }
        async resume() { state.writes++; return true; }
      },
      installKeepingSlots: async () => {
        state.writes++;
        if (mode === "failure") throw Object.assign(Error("test write failure"), { code: "lost" });
        return { move: null };
      },
      sameKeyboard: () => true, confirmNightlyModel: () => true, t: key => key,
      sayK: key => { state.messages.push(key); },
      log() {}, led() {}, suggestModel() {},
      setConnected: on => { state.connected = on; },
      currentVersion: () => ({ models: { elite: {}, pro: {} } }),
      isCatalogChecked: () => mode !== "revoked",
       hasVerifiedNightlySelection: () => mode !== "revoked",
      sampleImage: new Uint8Array([1]),
      stock: null, TEXT: { en: { lost: "lost", otabackup: "Prior backup needed", selectionstale: "Changed" } }, lang: "en",
      window: { confirm: () => { state.otaPrompts++; return mode !== "resume-cancel"; } },
      Blob, URL, setTimeout,
      document: { createElement: () => ({ click() {}, remove() {} }),
        body: { appendChild() {} } }
    };
    ctx = createContext({ renderConnectionAction() {}, refreshJourney() {}, ...mock });
    new Script(`let connectionState="idle",writing=false,lastId=null,image=sampleImage,stock=null,chosenModel="elite",ctl=null,ctlLoadSerial=0,scanSerial=0,ctlNeedsResolution=false;
${scan}
${run}
globalThis.start=run; globalThis.manualScan=scan; globalThis.isWriting=()=>writing;`).runInContext(ctx);
    const first = ctx.start(new Uint8Array([1]), model, { selectionGuard: () => approvalActive });
    if (mode.startsWith("pending")) {
      assert.equal(ctx.isWriting(), true, "permission request left write unlocked");
      assert.equal(ui.stockgo.disabled, true, "second install button was enabled");
      await ctx.manualScan();
      const second = ctx.start(new Uint8Array([1]), model, { foreign: true });
      if (mode === "pending-stale") approvalActive = false;
      grant({});
      await Promise.all([first, second]);
    } else await first;
    assert.equal(ctx.isWriting(), false, model + " " + mode + ": leaked write lock");
    assert.equal(state.otaPrompts, ["resume", "resume-cancel"].includes(mode) ? 1 : 0,
      model + " " + mode + ": unexpected recovery confirmation");
    if (mode === "denied") {
      assert.equal(state.writes, 0, "permission refused after write started");
      assert.equal(state.messages.at(-1), "denied");
      assert.equal(ui.go.disabled, false, "permission denial retry blocked");
      assert.equal(state.postScan, 0);
    } else if (mode === "resume-cancel") {
      assert.equal(state.writes, 0, "cancelled loader recovery performed a write");
      assert.equal(state.postScan, 0, "cancelled loader recovery rescanned after a write");
      assert.equal(state.messages.at(-1), "otabackup", "missing actionable recovery refusal");
      assert.equal(ui.go.disabled, false, "rejected loader recovery left retry disabled");
    } else if (mode === "revoked" || mode === "pending-stale") {
      assert.equal(state.writes, 0, "stale/revoked selection wrote firmware");
      assert.equal(state.postScan, 0, "stale selection started post-write scan");
      assert.equal(state.messages.at(-1), "selectionstale");
      assert.equal(ui.go.disabled, mode === "revoked");
    } else if (mode === "failure") {
      assert.equal(state.postScan, 0, "failed write masked by rescan");
      assert.equal(state.messages.at(-1), "lost");
    } else {
      assert.equal(state.writes, 1, "duplicate or missing write");
      assert.ok(state.postScan > 0, "no post-write rescan");
      assert.equal(state.lockHeld, true, "post-write rescan not under lock");
      assert.equal(state.connected, mode === "missing" ? false : true);
      assert.equal(ui.go.disabled, mode === "revoked",
        "revoked catalog entry left Install available");
    }
    passed++;
    console.log("PASS reconnect " + model.split("_")[0] + " " + mode);
  }
}
console.log("PASS " + passed + " exact-source Web MIDI mock scenarios");


// ---- Partial/total skipped restoration must NOT claim complete sample recovery.
//      Execute exact source run() against synthetic Elite/Pro, no MIDI hardware. ----
assert.match(html, /slotspartial: "Installed, but some samples were NOT restored/);
assert.match(html, /slotspartial: "インストールは完了しましたが/);
let restorePassed = 0;
for (const model of ["SMK-37 Elite_9001100", "SMK-37 Pro_9001100"]) {
  for (const mode of ["all", "partial", "total", "stock", "backup-error"]) {
    const records = [], ui = {}, name = "before.x0xslots";
    let writes = 0, scans = 0;
    const move = mode === "stock" ? { newMap: [], skipped: [], name }
      : mode === "all" ? { newMap: [{}], skipped: [], name }
      : mode === "partial" ? { newMap: [{}], skipped: ["slot 2: no room"], name }
      : { newMap: [{}], skipped: ["slot 1: no room", "slot 2: no room"], name };
    const mock = {
      $: id => ui[id] ||= { value: 0, checked: true, disabled: false },
      midiAccess: async () => ({}),
      Updater: class { async find() { return null; } },
      installKeepingSlots: async () => {
        writes++;
        if (mode === "backup-error")
          throw Object.assign(Error("backup could not finish"), { code: "slotbackup" });
        return { move };
      },
      scan: async () => { scans++; },
      sayK: (key, suffix = "") => { records.push({ key, suffix }); },
      sameKeyboard: () => true,
      log() {}, led() {}, t: key => key,
      currentVersion: () => ({ models: { elite: {}, pro: {} } }),
      isCatalogChecked: () => false,
       hasVerifiedNightlySelection: () => false,
      TEXT: { en: { slotbackup: "Backup error" } }, lang: "en",
      Blob, URL, setTimeout,
      document: { createElement: () => ({}), body: { appendChild() {} } },
    };
    const ctx = createContext({ renderConnectionAction() {}, refreshJourney() {}, ...mock });
    new Script(`let connectionState="idle",writing=false,lastId=null,image=null,stock=null,chosenModel="elite",ctl=null,ctlLoadSerial=0,scanSerial=0,ctlNeedsResolution=false;
${run}
globalThis.runRestoreTest=run;`).runInContext(ctx);
    await ctx.runRestoreTest(new Uint8Array([1]), model, { selectionGuard: () => true });
    const wanted = { all: "slotsdone", partial: "slotspartial", total: "slotspartial",
      stock: "slotstock", "backup-error": "slotbackup" }[mode];
    assert.equal(writes, 1, "write not called exactly once");
    assert.equal(scans, mode === "backup-error" ? 0 : 1, "failed backup masked by scan");
    assert.equal(records.at(-1)?.key, wanted, model + " " + mode);
    if (mode === "partial" || mode === "total") {
      assert.ok(records.at(-1).suffix.includes("before.x0xslots"));
      assert.ok(records.at(-1).suffix.includes("slot 2: no room"));
    }
    restorePassed++;
  }
}
console.log("PASS " + restorePassed + "/10 sample-restore disposition scenarios (Elite/Pro)");
