// SPDX-License-Identifier: GPL-3.0-only
// Browser installer controller file races; exact source, mock File reads, no MIDI/device.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const html = readFileSync(process.argv[2] || new URL("./x0x_installer.html", import.meta.url), "utf8");
const i = html.indexOf("let ctlLoadSerial = 0;");
const j = html.indexOf('\nwindow.addEventListener("beforeunload"', i);
assert.ok(i >= 0 && j > i, "guarded controller selection source absent");
assert.match(html, /async function run\(img, product, opts = \{\}\) \{\s*\/\/[^\n]*\n\s*if \(writing\) return;\s*writing = true;\s*scanSerial\+\+;[^\n]*\n\s*ctlLoadSerial\+\+;/, "firmware writer invalidates both async scans and controller reads");
const selectedSource = html.slice(i, j);
function defer() {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve=yes; reject=no; });
  return {promise, resolve, reject};
}
const event = promise => ({target:{files:[{arrayBuffer:()=>promise}],value:"controller.img"}});
function fixture() {
  const handlers={},notes=[],nodes={};
  const $=id=>nodes[id] ||= {value:"",disabled:false,addEventListener:(name,fn)=>{handlers[name]=fn}};
  const ctx=createContext({handlers,notes,Uint8Array,$,
    ctlProblem:raw=>raw[0]===0?"invalid CRC":null,
    sha256Hex:async buf=>"sha-"+new Uint8Array(buf)[0],
    sayK:name=>notes.push(name)});
  new Script('let ctl=null,writing=false;\n'+selectedSource+'\n'+
    'globalThis.caseState={choose:e=>handlers.change(e),clear:()=>handlers.click(),ctl:()=>ctl&&ctl[0],hash:()=>ctlHash,clearDisabled:()=>$(\"ctlclear\").disabled,write:x=>writing=x,begin:()=>{writing=true;ctlLoadSerial++},notes:()=>notes};')
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
await check("controller hash recorded for latest selected file",async()=>{
  const x=fixture();await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));
  assert.equal(x.hash(),"sha-2");assert.equal(x.clearDisabled(),false);
});
await check("controller cleared; no Bluetooth selection remains",async()=>{
  const x=fixture();await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));
  x.clear();assert.equal(x.ctl(),null);assert.equal(x.hash(),null);
  assert.equal(x.clearDisabled(),true);assert.equal(x.notes().at(-1),"ctlcleared");
});
await check("clear invalidates outstanding file read",async()=>{
  const x=fixture(),slow=defer(),p=x.choose(event(slow.promise));
  x.clear();slow.resolve(Uint8Array.of(2).buffer);await p;
  assert.equal(x.ctl(),null);assert.equal(x.hash(),null);
});
await check("cannot clear active controller during write",async()=>{
  const x=fixture();await x.choose(event(Promise.resolve(Uint8Array.of(2).buffer)));
  x.write(true);x.clear();assert.equal(x.ctl(),2);assert.equal(x.hash(),"sha-2");
});
const goBegin=html.indexOf('$("go").addEventListener("click", () => {');
const goEnd=html.indexOf("\nfunction confirmNightlyModel(",goBegin);
assert.ok(goBegin>=0&&goEnd>goBegin,"DEV Install handler missing");
const goSource=html.slice(goBegin,goEnd);
assert.match(goSource,/ctlHash !== p.controller_sha256/,"SHA authorization check missing");
assert.match(html,/ctlunapproved: "SHA-256/,"English warning missing");
assert.match(html,/ctlunapproved: "この機種/,"Japanese warning missing");
function goCase({hash,approvedHash="sha-2",withCtl=true,approved=true}) {
  let callback,writes=0,inserts=0,errors=[];
  const ctx=createContext({
    Uint8Array,$:()=>({disabled:false,addEventListener:(_e,f)=>{callback=f}}),
    currentVersion:()=>({models:{elite:{controller_sha256:approvedHash}}}),
    isCatalogChecked:()=>approved,confirmWrite:()=>true,
    insertCtl:img=>{inserts++;return img;},run:()=>{writes++;},
    sayK:k=>errors.push(k),t:()=> "SHA mismatch"
  });
  new Script('let image=new Uint8Array([1]),selectedProduct="SMK-37 Elite_9001100",'
    +'writing=false,chosenModel="elite",ctl='+(withCtl?'new Uint8Array([2])':'null')
    +',ctlHash='+JSON.stringify(hash)+';\n'+goSource).runInContext(ctx);
  assert.ok(callback,"DEV Install callback missing");callback();
  return {writes,inserts,errors};
}
await check("unapproved ctl never reaches insertion or firmware write",async()=>{
  const x=goCase({hash:"sha-1"});
  assert.equal(x.writes,0);assert.equal(x.inserts,0);assert.equal(x.errors.at(-1),"ctlbad");
});
await check("matching approved ctl preserves Bluetooth insertion",async()=>{
  const x=goCase({hash:"sha-2"});
  assert.equal(x.writes,1);assert.equal(x.inserts,1);
});
await check("omitted ctl preserves non-Bluetooth install",async()=>{
  const x=goCase({hash:null,withCtl:false});
  assert.equal(x.writes,1);assert.equal(x.inserts,0);
});
await check("changing model-approved ctl SHA blocks earlier selection",async()=>{
  const x=goCase({hash:"sha-2",approvedHash:"sha-3"});
  assert.equal(x.writes,0);assert.equal(x.inserts,0);
});
await check("revoked catalog cannot install even with matching ctl",async()=>{
  const x=goCase({hash:"sha-2",approved:false});
  assert.equal(x.writes,0);assert.equal(x.inserts,0);
});
console.log("PASS "+passed+"/17 exact-source controller selection/prewrite regression cases; no hardware");
