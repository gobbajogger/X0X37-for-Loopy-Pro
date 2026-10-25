// SPDX-License-Identifier: GPL-3.0-only
// Exact checked-out HTML test; mocks file input/DOM only, never Web MIDI or device.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const html = readFileSync(new URL("./x0x_installer.html", import.meta.url), "utf8");
const from = html.indexOf("let stockLoadSerial = 0;");
const to = html.indexOf("\nasync function run(", from);
assert.ok(from >= 0 && to > from, "local-selection source not found");
assert.match(html, /id="stockconfirm" disabled/);
const source = html.slice(from, to);
function defer() {
  let resolve;
  const promise = new Promise((r) => { resolve = r; });
  return { promise, resolve };
}
const tick = async () => { for (let j = 0; j < 12; j++) await Promise.resolve(); };
const input = (n) => ({ size:1, arrayBuffer: async () => Uint8Array.of(n).buffer });
function scenario() {
  const nodes = new Map(), hashes = new Map(), notices = [];
  const $ = (id) => {
    if (!nodes.has(id)) nodes.set(id, {
      disabled: id === "stockconfirm", checked: false, textContent: "", value: "",
      handlers: {}, classList: { add() {}, remove() {} },
      addEventListener(e, fn) { this.handlers[e] = fn; }
    });
    return nodes.get(id);
  };
  const ctx = createContext({
    $, Uint8Array, t: k=>k,
    productOf: (raw) => raw[0] === 3 ? "BAD_900" :
      raw[0] === 1 ? "SMK-37 Pro_9001001" : "SMK-37 Elite_9001002",
    logicalImage: (raw) => raw[0],
    sha256Hex: (buf) => {
      const req = defer();
      hashes.set(new Uint8Array(buf)[0], req);
      return req.promise;
    },
    sameKeyboard: (a, b) => a.startsWith("FM-1") === b.startsWith("FM-1"),
    sayK: (k) => notices.push(k), STOCK_SHA256_FM1: "unmatched"
  });
  const script = "let stock=null,writing=false,lastId=null;\n" + source + "\n" +
    "globalThis.TEST={" +
    "choose:f=>$('stockfile').handlers.change({target:{files:[f],value:'x'}})," +
    "drop:f=>$('stockdrop').handlers.drop({dataTransfer:{files:[f]},preventDefault(){}})," +
    "confirm:()=>{$('stockconfirm').checked=true;$('stockconfirm').handlers.change()}," +
    "writing:x=>{writing=x},device:x=>{lastId=x}," +
    "state:()=>({product:stock&&stock.product,enabled:!$('stockgo').disabled," +
    "confirmed:$('stockconfirm').checked,confirmDisabled:$('stockconfirm').disabled})};";
  new Script(script, { filename: "stock-html-source.js" }).runInContext(ctx);
  return { ui: ctx.TEST, hashes, notices };
}
let passed = 0;
{
  const x = scenario(), first = x.ui.choose(input(1));
  await tick();
  assert.ok(x.hashes.has(1));
  const second = x.ui.drop(input(2));
  await tick();
  x.hashes.get(2).resolve("second"); await second;
  x.hashes.get(1).resolve("first"); await first;
  assert.equal(x.ui.state().product, "SMK-37 Elite_9001002");
  assert.equal(x.ui.state().enabled, false);
  assert.equal(x.ui.state().confirmed, false);
  x.ui.confirm();
  assert.equal(x.ui.state().enabled, true);
  passed++;
}
{
  const x = scenario(), slow = defer();
  const first = x.ui.choose({ size:1, arrayBuffer: () => slow.promise });
  const second = x.ui.drop(input(2));
  await tick(); x.hashes.get(2).resolve("second"); await second;
  slow.resolve(Uint8Array.of(1).buffer); await first;
  assert.equal(x.ui.state().product, "SMK-37 Elite_9001002");
  assert.equal(x.hashes.has(1), false);
  passed++;
}
{
  const x = scenario(), p = x.ui.choose(input(3));
  await tick(); x.hashes.get(3).resolve("bad"); await p;
  assert.equal(x.ui.state().product, null);
  assert.equal(x.ui.state().enabled, false);
  assert.equal(x.ui.state().confirmDisabled, true);
  assert.ok(x.notices.includes("stockbad"));
  passed++;
}
{
  const x = scenario();
  await x.ui.choose({ size:1, arrayBuffer: async () => { throw Error("read failure"); } });
  assert.equal(x.ui.state().product, null);
  assert.ok(x.notices.includes("stockloadfail"));
  passed++;
}
{
  const x = scenario();
  x.ui.writing(true); await x.ui.choose(input(1));
  assert.equal(x.hashes.size, 0);
  passed++;
}
{
  const x = scenario();
  x.ui.device({ model: "FM-1_9001000", text: "FM-1" });
  const p = x.ui.choose(input(2));
  await tick(); x.hashes.get(2).resolve("match"); await p;
  assert.equal(x.ui.state().product, null);
  assert.ok(x.notices.includes("model"));
  passed++;
}
{
  const x=scenario();let reads=0;
  await x.ui.choose({size:16*1024*1024+1,arrayBuffer:async()=>{reads++;return Uint8Array.of(2).buffer}});
  assert.equal(reads,0);assert.equal(x.ui.state().product,null);
  assert.equal(x.ui.state().enabled,false);assert.equal(x.ui.state().confirmDisabled,true);
  assert.ok(x.notices.includes("stockbad"));passed++;
}
{
  for(const size of [undefined,0,-1,NaN,Number.MAX_SAFE_INTEGER+1]){
    const x=scenario();let reads=0;
    await x.ui.drop({size,arrayBuffer:async()=>{reads++;return Uint8Array.of(2).buffer}});
    assert.equal(reads,0);assert.equal(x.ui.state().product,null);
    assert.equal(x.ui.state().confirmDisabled,true);
    assert.ok(x.notices.includes("stockbad"));
  }passed++;
}
console.log("PASS " + passed + "/8 installer local-selection mock regressions (no hardware)");
