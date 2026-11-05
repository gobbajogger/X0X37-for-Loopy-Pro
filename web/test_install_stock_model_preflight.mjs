// SPDX-License-Identifier: GPL-3.0-only
// Real Git-source stock-model aliases and stale-selection negatives (mock UI only).
// Run: node web/test_install_stock_model_preflight.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";
const html = readFileSync(new URL("./x0x_installer.html", import.meta.url), "utf8");
const ota = readFileSync(new URL("./fm1ota.js", import.meta.url), "utf8");
function cut(source, begin, end) {
  const i = source.indexOf(begin), j = source.indexOf(end, i);
  assert.ok(i >= 0 && j > i, "missing source: " + begin);
  return source.slice(i, j + end.length);
}
const guard = cut(html, "function confirmStockModel(product) {", "\n}\n");
const handler = cut(html, '$("stockgo").addEventListener("click", () => {', "\n});");
const family = ota.match(/^const SMK_FAMILY = .*;$/m);
const same = ota.match(/^export const sameKeyboard = .*;.*$/m);
assert.ok(family && same);
assert.match(handler, /confirmStockModel\(stock\.product\) \|\| !confirmWrite\(stock\.product\)/);
assert.match(html, /stockalias: "MODEL NAME DIFFERENCE/);
assert.match(html, /stockalias: "機種名が異なります/);
assert.match(html, /const NIGHTLY_PUBLISHER_APPROVALS = Object\.freeze\(\{\}\)/);
assert.match(html, /stockfile" accept="\.fwsc"/);
const cases = [
  {n:"exact Elite",d:"SMK-37 Elite",p:"SMK-37 Elite_9001100",w:1,c:1},
  {n:"exact Pro",d:"SMK-37 Pro",p:"SMK-37 Pro_9001100",w:1,c:1},
  {n:"exact FM-1",d:"FM-1",p:"FM-1_0000016",w:1,c:1},
  {n:"Elite -> Pro accepted",d:"SMK-37 Elite",p:"SMK-37 Pro_9001100",w:1,a:1,c:1},
  {n:"Elite -> Pro declined",d:"SMK-37 Elite",p:"SMK-37 Pro_9001100",ack:false,w:0,a:1},
  {n:"Pro -> Elite accepted",d:"SMK-37 Pro",p:"SMK-37 Elite_9001100",w:1,a:1,c:1},
  {n:"Pro -> Elite declined",d:"SMK-37 Pro",p:"SMK-37 Elite_9001100",ack:false,w:0,a:1},
  {n:"Starrykey alias declined",d:"STARRYKEY-37 Play",p:"SMK-37 Pro_9001100",ack:false,w:0,a:1},
  {n:"OTA alias declined",d:"ota-SMK-37 Elite",p:"SMK-37 Pro_9001100",ack:false,w:0,a:1},
  {n:"family switched after selection",d:"FM-1",p:"SMK-37 Elite_9001100",w:0,e:"model"},
  {n:"reverse family switch",d:"SMK-37 Pro",p:"FM-1_0000016",w:0,e:"model"},
  {n:"no scan manual import",d:null,p:"SMK-37 Elite_9001100",w:1,c:1},
  {n:"final dialog cancelled",d:"SMK-37 Elite",p:"SMK-37 Elite_9001100",ok:false,w:0,c:1},
  {n:"alias approved but final cancelled",d:"SMK-37 Elite",p:"SMK-37 Pro_9001100",ok:false,w:0,a:1,c:1},
  {n:"checkbox unchecked",d:"SMK-37 Elite",p:"SMK-37 Elite_9001100",checked:false,w:0},
  {n:"writer locked",d:"SMK-37 Elite",p:"SMK-37 Elite_9001100",busy:true,w:0},
  {n:"no loaded file",d:"SMK-37 Elite",p:null,w:0},
];
let passed=0;
for (const v of cases) {
  let click, write=0, regular=0, alias=0, errors=[];
  const stockgo={addEventListener(event,cb){assert.equal(event,"click");click=cb;},disabled:false};
  const stockconfirm={checked:v.checked!==false};
  const ctx=createContext({
    Uint8Array, $:id=>({stockgo,stockconfirm})[id],
    t:k=>({stockalias:"MODEL NAME DIFFERENCE ",stockaliasTo:" PACKAGE ",stockaliasEnd:" Check model."})[k],
    window:{confirm(prompt){assert.match(prompt,/MODEL NAME DIFFERENCE/);alias++;return v.ack!==false;}},
    confirmWrite(){regular++;return v.ok!==false;},
    ownedRun(){write++;},sayK(k){errors.push(k);},
    currentDevice:v.d?{model:v.d,text:v.d}:null,
    currentStock:v.p?{product:v.p,image:new Uint8Array([1])}:null,
    busy:v.busy===true
  });
  new Script("let writing=busy,lastId=currentDevice,stock=currentStock;\n"+
    family[0]+"\n"+same[0].replace(/^export /,"")+"\n"+guard+"\n"+handler).runInContext(ctx);
  assert.ok(click,v.n+": handler missing");
  click();
  assert.equal(write,v.w,v.n+": write");
  assert.equal(alias,v.a||0,v.n+": extra alias confirmation");
  assert.equal(regular,v.c||0,v.n+": existing final confirmation");
  if(v.e) assert.equal(errors.at(-1),v.e,v.n+": blocked status");
  passed++;
}
console.log("PASS "+passed+"/"+cases.length+" exact-source stock model preflight tests (mock UI, no device)");


// Published NIGHTLY DEV packages must freshly verify/confirm the physical Elite/Pro
// model before any running-app prewrite or loader resume. No real MIDI/flash here.
const breaks = readFileSync(new URL("./x0x_breaks.js", import.meta.url), "utf8");
const helperStart = breaks.indexOf("export async function installKeepingSlots(");
assert.ok(helperStart >= 0);
// Stop at the next section — slicing to EOF pulled encodeBreakPack's export
// into the non-module vm.Script and threw SyntaxError.
const helperEnd = breaks.indexOf("\n\n// A break pack:", helperStart);
assert.ok(helperEnd > helperStart, "installKeepingSlots end not found");
const helperBody = breaks.slice(helperStart, helperEnd).replace(/^export /, "");
assert.match(html, /!foreign && !confirmNightlyModel\(ota\.id\.model, product\)/);
assert.match(html, /confirmModel: foreign \? null : \(id, p\) => confirmNightlyModel\(id\.model, p\)/);
assert.match(html, /nightlymodel: "DEV FIRMWARE/);
assert.match(html, /nightlymodel: "DEV ファームウェア/);
const prewriteCases = [
  ["alias declined", "SMK-37 Pro", false, [0,1,1,"modelcancel"]],
  ["same model declined", "SMK-37 Elite", false, [0,1,1,"modelcancel"]],
  ["alias acknowledged", "SMK-37 Pro", true, [1,1,1,null]],
  ["same model acknowledged", "SMK-37 Elite", true, [1,1,1,null]],
  ["device disappeared", null, true, [0,0,0,"notfound"]],
  ["other family rejected", "FM-1", true, [0,1,0,"model"], false],
  ["manual stock remains possible", "SMK-37 Pro", null, [1,1,0,null]]
];
let prewritePass = 0;
for (const [label, model, accepted, expected, familyOK] of prewriteCases) {
  let writes=0, closed=0, asked=0, code=null;
  const ctx = createContext({readSlotLayout:async()=>({slots:[]})});
  new Script(helperBody+"\nglobalThis.perform=installKeepingSlots;").runInContext(ctx);
  const dev = model ? {id:{model,text:model},link:{close(){closed++;}}} : null;
  const up = {find:async()=>dev,install:async()=>{writes++;return "simulated";}};
  const hooks = {sameKeyboard:()=>familyOK!==false,
    confirmModel:accepted===null?null:()=>{asked++;return accepted;}};
  try {await ctx.perform(up,Uint8Array.of(1),"SMK-37 Elite_9001100","smk",hooks);}
  catch(e){code=e.code||e.message;}
  assert.deepEqual([writes,closed,asked,code],expected,label);
  prewritePass++;
}
const promptStart=html.indexOf("function confirmNightlyModel(deviceModel, product) {");
const promptEnd=html.indexOf("\n}",promptStart)+2;
assert.ok(promptStart>=0&&promptEnd>promptStart);
let modelPrompt="";
const promptContext=createContext({
  sameKeyboard:(a,b)=>a===b||(/^SMK-37/i.test(a)&&/^SMK-37/i.test(b)),
  t:k=>({nightlymodel:"Package: ",nightlymodelReported:" device: ",nightlymodelEnd:" physical keyboard?",model:"Wrong family"})[k],
  window:{confirm:s=>{modelPrompt=s;return false;}}
});
new Script(html.slice(promptStart,promptEnd)+"\nglobalThis.verifyModel=confirmNightlyModel;").runInContext(promptContext);
assert.equal(promptContext.verifyModel("ota-SMK-37 Pro","SMK-37 Elite_9001100"),false);
assert.match(modelPrompt,/SMK-37 Pro/);
assert.match(modelPrompt,/SMK-37 Elite_9001100/);
prewritePass++;
let badCode=null;
try{promptContext.verifyModel("FM-1","SMK-37 Elite_9001100");}catch(e){badCode=e.code;}
assert.equal(badCode,"model");
prewritePass++;
console.log("PASS "+prewritePass+"/9 NIGHTLY physical-model prewrite mocked source cases");
