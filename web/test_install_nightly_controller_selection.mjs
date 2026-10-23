// SPDX-License-Identifier: GPL-3.0-only
// Browser installer controller file races; exact source, mock File reads, no MIDI/device.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const html = readFileSync(process.argv[2] || new URL("./x0x_installer.html", import.meta.url), "utf8");
const i = html.indexOf("let ctlLoadSerial = 0;");
const j = html.indexOf('\nwindow.addEventListener("beforeunload"', i);
assert.ok(i >= 0 && j > i, "guarded controller selection source absent");
assert.match(html, /async function run\(img, product, opts = \{\}\) \{\s*\/\/[^\n]*\n\s*if \(writing\) return;\s*writing = true;\s*ctlLoadSerial\+\+;/, "firmware writer must invalidate pending controller reads");
const selectedSource = html.slice(i, j);
function defer() {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve=yes; reject=no; });
  return {promise, resolve, reject};
}
const event = promise => ({target:{files:[{arrayBuffer:()=>promise}],value:"controller.img"}});
function fixture() {
  const handlers={},notes=[];
  const ctx=createContext({handlers,notes,Uint8Array,
    $:()=>({addEventListener:(name,fn)=>{handlers[name]=fn}}),
    ctlProblem:raw=>raw[0]===0?"invalid CRC":null,
    sayK:name=>notes.push(name)});
  new Script('let ctl=null,writing=false;\n'+selectedSource+'\n'+
    'globalThis.caseState={choose:e=>handlers.change(e),ctl:()=>ctl&&ctl[0],write:x=>writing=x,begin:()=>{writing=true;ctlLoadSerial++},notes:()=>notes};')
    .runInContext(ctx);
  return ctx.caseState;
}
let passed=0;
async function check(name, fn) { await fn(); passed++; console.log("PASS "+name); }
await check("latest valid file replaces earlier slow read",async()=>{
  const x=fixture(),a=defer(),b=defer(),p=x.choose(event(a.promise)),q=x.choose(event(b.promise));
  b.resolve(Uint8Array.of(2).buffer);await q;a.resolve(Uint8Array.of(1).buffer);await p;
  assert.equal(x.ctl(),2);
});
await check("bad latest file blocks old valid controller",async()=>{
  const x=fixture(),a=defer(),p=x.choose(event(a.promise));
  await x.choose(event(Promise.resolve(Uint8Array.of(0).buffer)));
  a.resolve(Uint8Array.of(1).buffer);await p;assert.equal(x.ctl(),null);
  assert.equal(x.notes().at(-1),"ctlbad");
});
await check("late read error cannot overwrite newer success",async()=>{
  const x=fixture(),a=defer(),p=x.choose(event(a.promise));
  await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));
  a.reject(Error("stale read error"));await p;assert.equal(x.ctl(),2);
  assert.equal(x.notes().at(-1),"ctlok");
});
await check("controller read completing during flash is ignored",async()=>{
  const x=fixture(),a=defer(),p=x.choose(event(a.promise));
  x.write(true);a.resolve(Uint8Array.of(2).buffer);await p;assert.equal(x.ctl(),null);
});
await check("write-start invalidation survives flash completion",async()=>{
  const x=fixture(),a=defer(),p=x.choose(event(a.promise));
  x.begin();x.write(false);a.resolve(Uint8Array.of(2).buffer);await p;assert.equal(x.ctl(),null);
});
await check("controller selection forbidden while flashing",async()=>{
  const x=fixture(),e=event(Promise.resolve(Uint8Array.of(2).buffer));x.write(true);
  await x.choose(e);assert.equal(x.ctl(),null);assert.equal(e.target.value,"");
});
await check("valid single controller accepted",async()=>{
  const x=fixture();await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));
  assert.equal(x.ctl(),2);assert.equal(x.notes().at(-1),"ctlok");
});
await check("failed read can retry same file",async()=>{
  const x=fixture(),e=event(Promise.reject(Error("read fail")));
  await x.choose(e);assert.equal(e.target.value,"");
  await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));assert.equal(x.ctl(),2);
});
console.log("PASS "+passed+"/8 controller selection regressions; no hardware/HTTP");
