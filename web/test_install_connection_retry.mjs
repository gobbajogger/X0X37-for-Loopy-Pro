// SPDX-License-Identifier: GPL-3.0-only
// Adapted from the installer connection-state regression registered by bbe325d48.
// Exercise current scan/connection controls; no MIDI interface or device writes.
// Run: node web/test_install_connection_retry.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const html = readFileSync(new URL("./x0x_installer.html", import.meta.url), "utf8");
function extract(anchor) {
  const start = html.indexOf(anchor), end = html.indexOf("\n}", start);
  assert.ok(start >= 0 && end > start, "missing source function: " + anchor);
  return html.slice(start, end + 2);
}
const source = ["function led(state) {", "function setConnected(on, detail) {",
  "function renderConnectionAction() {", "async function scan(allowPostWrite = false) {"]
  .map(extract).join("\n");

function setup() {
  const ui = Object.fromEntries(["led", "conn", "connDetail", "connbtn", "stockconfirm"]
    .map(id => [id, { textContent: "", className: "", disabled: false, checked: true }]));
  const state = { permissionError: null, calls: 0, find: async () => null };
  const labels = { connect: "Connect", retry: "Try again", rescan: "Rescan",
    scanning: "Scanning", connected: "Connected", notconnected: "Not connected",
    looking: "Looking", notfound: "No keyboard found", connhelp: "Connect a keyboard", error: "Error" };
  const ctx = createContext({
    $: id => ui[id], t: key => labels[key], refreshJourney() {}, suggestModel() {},
    navigator: { requestMIDIAccess() {} }, isSecureContext: true,
    midiAccess: async () => { if (state.permissionError) throw state.permissionError; return {}; },
    Updater: class { async find(predicate) { state.calls++; return state.find(predicate); } },
  });
  new Script(`let connected=false, writing=false, lastId=null, connectionState="idle", scanSerial=0;
${source}
globalThis.scan=scan;
globalThis.setWriting=value=>{ writing=value; renderConnectionAction(); };
globalThis.identity=()=>lastId;
`).runInContext(ctx);
  return { ctx, ui, state };
}
function device(model, onClose = () => {}) {
  return { id: { model, text: model + "_9001100" }, name: "USB MIDI",
    link: { close: onClose } };
}

for (const model of ["SMK-37 Elite", "SMK-37 Pro"]) {
  const { ctx, ui, state } = setup();
  // Keep a reset diagnostic visible, then allow a fresh user-initiated scan.
  state.find = async () => { throw Error("MIDI connection reset"); };
  await ctx.scan();
  assert.equal(ui.connbtn.textContent, "Try again");
  assert.equal(ui.connbtn.disabled, false);
  assert.equal(ui.connDetail.textContent, "MIDI connection reset");
  assert.equal(ctx.identity(), null);

  let release;
  const pending = new Promise(resolve => { release = resolve; });
  state.find = async () => pending;
  const scanning = ctx.scan();
  assert.equal(ui.connbtn.textContent, "Scanning");
  assert.equal(ui.connbtn.disabled, true);
  let closed = 0;
  release(device(model, () => { closed++; }));
  await scanning;
  assert.equal(ui.connbtn.textContent, "Rescan");
  assert.equal(ui.connbtn.disabled, false);
  assert.equal(ui.conn.textContent, "Connected");
  assert.equal(ctx.identity().model, model);
  assert.equal(closed, 1, "successful probe leaked its MIDI listener");

  // Permission failure must clear the previously connected identity and remain retryable.
  state.permissionError = Error("MIDI permission denied");
  await ctx.scan();
  assert.equal(ctx.identity(), null);
  assert.equal(ui.connDetail.textContent, "MIDI permission denied");
  assert.equal(ui.connbtn.disabled, false);
  state.permissionError = null;
  state.find = async () => null;
  await ctx.scan();
  assert.equal(ui.connbtn.textContent, "Try again");
  assert.equal(ui.led.className, "led err");

  ctx.setWriting(true);
  const calls = state.calls;
  await ctx.scan();
  assert.equal(state.calls, calls, "manual scan probed MIDI during installation");
  assert.equal(ui.connbtn.disabled, true);
  ctx.setWriting(false);

  // A superseded successful probe must close its link without replacing the fresh result.
  let staleRelease, entered;
  const started = new Promise(resolve => { entered = resolve; });
  const stale = new Promise(resolve => { staleRelease = resolve; });
  state.find = async () => { entered(); return stale; };
  const oldScan = ctx.scan();
  await started;
  state.find = async () => null;
  await ctx.scan();
  const detail = ui.connDetail.textContent;
  let staleClosed = 0;
  staleRelease(device(model, () => { staleClosed++; }));
  await oldScan;
  assert.equal(staleClosed, 1, "superseded probe leaked its MIDI listener");
  assert.equal(ctx.identity(), null, "superseded scan replaced the fresh identity");
  assert.equal(ui.connDetail.textContent, detail);
  assert.equal(ui.connbtn.textContent, "Try again");
  console.log("PASS connection retry, writer exclusion and stale scan: " + model);
}
