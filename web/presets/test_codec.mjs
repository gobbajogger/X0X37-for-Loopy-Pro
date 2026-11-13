// SPDX-License-Identifier: GPL-3.0-only
// Focused W3 contract tests: node --test web/presets/test_codec.mjs
// Native golden fixtures copied byte-for-byte from tests/host/preset_record_test.c
// at 16f9f9ad, originally produced by the C encoder at 9bda58dd.
import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { crc32, parseDnpr, encodeDnpr, ensureExportable, sampleWarnings,
  fileNameFor, explicitModelHint, ensureDeviceModel, PresetSession, DNPR_ENGINES } from "./codec.mjs";
import { makeDnprZip, readDnprZip } from "./zip.mjs";

const GOLDEN_CHI = Uint8Array.from([0x44, 0x4e, 0x50, 0x52, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x7f, 0x21, 0x90, 0x92, 0x02, 0x01, 0x00, 0x00, 0xf9, 0x28, 0x35, 0x01, 0x00, 0x00, 0x00, 0x00, 0x47, 0x6f, 0x6c, 0x64, 0x65, 0x6e, 0x20, 0x43, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x41, 0x50, 0x54, 0x55, 0x52, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x11, 0x14, 0x17, 0x1a, 0x1d, 0x20, 0x23, 0x26, 0x29, 0x2c, 0x2f, 0x32, 0x35, 0x38, 0x3b, 0x3e, 0x41, 0x44, 0x47, 0x4a, 0x4d, 0x50]);
const GOLDEN_909 = Uint8Array.from([0x44, 0x4e, 0x50, 0x52, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00, 0x66, 0x8b, 0x3b, 0x20, 0x03, 0x05, 0x00, 0x00, 0xf9, 0x28, 0x35, 0x01, 0x01, 0x00, 0x02, 0x01, 0x4b, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x69, 0x6c, 0x65, 0x2e, 0x64, 0x6e, 0x70, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x04, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe, 0x00, 0x02, 0x04, 0x06, 0x01, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x43, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x22, 0x22, 0x22, 0x22, 0x4f, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x33, 0x33, 0x33, 0x33, 0x43, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x44, 0x44, 0x44, 0x44, 0x52, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
const sample = (i) => ({ source:1, index:i, crc:(0x11110000 + i) >>> 0, name:"S" + i });
const clone = bytes => Uint8Array.from(bytes);
const recalc = b => { const c=crc32(b,true); new DataView(b.buffer,b.byteOffset,b.byteLength).setUint32(16,c,true); return b; };
const expectCode = (f, code) => assert.throws(f, e => e.code === code, "Expected error " + code);
const make = (engine, nrefs = null) => {
  const spec = Object.values(DNPR_ENGINES).find(e => e.token === engine);
  const refs = Array.from({ length:nrefs ?? (spec.samples === -1 ? 0 : spec.samples) }, (_, i) => sample(i));
  return encodeDnpr({engine, name:"Test " + engine, sourceName:"OWNER", origin:2,
    params:Uint8Array.from({length:spec.params},(_,i)=>(3*i+7)&255), sampleRefs:refs, date:20261113});
};

test("native C golden Chicago and 909 files match exact JS wire encoding", () => {
  const c = encodeDnpr({engine:"chicago", name:"Golden Chi", sourceName:"CAPTURE", origin:2,
    date:20261113, params:Uint8Array.from({length:22},(_,i)=>17+i*3)});
  assert.deepEqual(c, GOLDEN_CHI); assert.equal(c.length,122); assert.equal(crc32(c,true),0x9290217f);
  const kit = encodeDnpr({engine:"909", name:"Kit", sourceName:"file.dnpr", origin:3,
    date:20261113, pluginFormat:1, pluginVersion:0x0102,
    params:Uint8Array.from({length:132},(_,i)=>(2*i)&255),
    sampleRefs:["CH","OH","CR","RD"].map((name,i)=>({source:1,index:i,crc:((i+1)*0x11111111)>>>0,name}))});
  assert.deepEqual(kit, GOLDEN_909); assert.equal(kit.length,296); assert.equal(crc32(kit,true),0x203b8b66);
  assert.deepEqual(ensureExportable(parseDnpr(GOLDEN_909)),GOLDEN_909);
});

test("A1 roundtrip: 7 engines, 808 at zero and 16 references, exact unchanged bytes", () => {
  for (const e of ["chicago","miami","juno","drone","909","808","break"]) {
    for (const n of e === "808" ? [0,16] : [null]) {
      const b = make(e,n), record = parseDnpr(b,{expectedEngine:e});
      assert.equal(record.engine,e); assert.equal(record.version,1); assert.equal(record.schema,1);
      assert.deepEqual(ensureExportable(record),b);
      const session = new PresetSession(2);
      const item = session.importMany([{name:"renamed-anything.dnpr",bytes:b}],{expectedEngine:e})[0];
      assert.deepEqual(session.exportOne(item.id).bytes,b);
    }
  }
});

test("A2 corruption, truncation, wrong magic and inconsistent length all refuse", () => {
  const original = make("chicago");
  for (const bad of [
    (()=>{let b=clone(original);b[16]^=1;return b;})(),
    original.slice(0,-1),
    (()=>{let b=clone(original);b[0]=88;return recalc(b);})(),
    (()=>{let b=clone(original);b[12]+=1;return recalc(b);})()
  ]) expectCode(()=>parseDnpr(bad),"BAD FILE");
  const bank = new PresetSession(4);
  expectCode(()=>bank.importMany([{name:"ok.dnpr",bytes:original},{name:"bad.dnpr",bytes:original.slice(0,-1)}]),"BAD FILE");
  assert.equal(bank.entries.length,0);
});

test("A3 newer format and schema reject with NEWER DATA, older unknown rejects BAD FILE", () => {
  const b=make("miami");
  for (const at of [4,8]) { const c=clone(b); c[at]=2; recalc(c); expectCode(()=>parseDnpr(c),"NEWER DATA"); }
  const old=clone(b);old[4]=0; recalc(old);expectCode(()=>parseDnpr(old),"BAD FILE");
});

test("A4 both engine ID and original engine are checked before target import", () => {
  const b=make("chicago");
  expectCode(()=>parseDnpr(b,{expectedEngine:"miami"}),"WRONG ENGINE");
  expectCode(()=>parseDnpr(make("miami"),{expectedEngine:"chicago"}),"WRONG ENGINE");
  expectCode(()=>parseDnpr(b,{expectedEngine:"909"}),"WRONG ENGINE");
  const inconsistent=clone(b); inconsistent[21]=2; recalc(inconsistent);
  assert.equal(parseDnpr(inconsistent).originalEngine,2); // preview only, do not relabel.
  expectCode(()=>parseDnpr(inconsistent,{expectedEngine:"chicago"}),"WRONG ENGINE");
  expectCode(()=>parseDnpr(inconsistent,{expectedEngine:"miami"}),"WRONG ENGINE");
});

test("A5 capacity preflight preserves every existing record and avoids partial ZIP staging", () => {
  const bank=new PresetSession(2), a=make("chicago"), b=make("miami");
  bank.importMany([{name:"a.dnpr",bytes:a}]);
  const before=clone(bank.entries[0].record.bytes);
  expectCode(()=>bank.importMany([{name:"b.dnpr",bytes:b},{name:"c.dnpr",bytes:b}]),"MEMORY FULL");
  assert.equal(bank.entries.length,1); assert.deepEqual(bank.entries[0].record.bytes,before);
});

test("A6 sample references warn when no device inventory is available or CRC differs", () => {
  for (const e of ["909","808","break"]) {
    const p=parseDnpr(make(e,e==="808"?2:null));
    assert.equal(sampleWarnings(p).length,p.sampleRefs.length);
    const inventory={};for(const ref of p.sampleRefs)inventory[ref.source+":"+ref.index]=ref.crc;
    assert.equal(sampleWarnings(p,inventory).length,0);
    inventory["1:0"]=0; assert.equal(sampleWarnings(p,inventory).length,1);
  }
  assert.deepEqual(sampleWarnings(parseDnpr(make("chicago"))),[]);
});

test("A7 user and imported non-factory records export byte-identical", () => {
  const user=parseDnpr(make("juno"));
  assert.deepEqual(ensureExportable(user),user.bytes);
  const imp=clone(make("drone"));imp[20]=3;recalc(imp);
  assert.deepEqual(ensureExportable(parseDnpr(imp)),imp);
  const zero=clone(make("drone"));zero.fill(0,20,32);recalc(zero);
  const p=parseDnpr(zero);assert.equal(p.origin,0);assert.equal(p.originalEngine,0);
  assert.deepEqual(ensureExportable(p),zero);
});

test("A8 factory or factory-derived exports always refused, without output", () => {
  for(const e of ["chicago","miami","juno","drone"]) {
    const user=clone(make(e));
    for(const origin of [1,2,3]) {
      const b=clone(user);b[20]=origin;b[22]=1;recalc(b);
      expectCode(()=>ensureExportable(parseDnpr(b)),"FACTORY EXPORT OFF");
    }
  }
  const bank=new PresetSession(1);const b=clone(make("chicago"));b[22]=1;recalc(b);
  const item=bank.importMany([{name:"derived.chicago.dnpr",bytes:b}])[0];
  expectCode(()=>bank.exportOne(item.id),"FACTORY EXPORT OFF");
});

test("BAD FILE semantic checks: flags, reserved, canonical strings, sample count and refs", () => {
  const c=make("chicago"), b=make("909");
  const mutations=[
    [c,10,1],[c,98,1],[c,32,65],[c,20,8],[c,22,255],
    [b,97,3],[b,100+132+1,1],[b,100+132,4]
  ];
  for(const [base,at,value] of mutations) {const changed=clone(base);changed[at]=value;recalc(changed);
    expectCode(()=>parseDnpr(changed),"BAD FILE"); }
});

test("file naming is bounded and does not depend on disk filename for record identity", () => {
  const b=make("miami"), p=parseDnpr(b,{fileName:"renamed.dnpr"});
  assert.equal(p.engine,"miami");assert.equal(p.modelInRecord,false);
  assert.equal(fileNameFor({...p,name:" Acid / Bass ? "}),"Acid-Bass.miami.dnpr");
  assert.equal(fileNameFor({...p,name:"?"}),"user-01.miami.dnpr");
  assert.ok(fileNameFor({...p,name:"A".repeat(250)}).length<=64);
  assert.equal(explicitModelHint("foo.elite.chicago.dnpr"),"elite");
  assert.equal(explicitModelHint("foo.chicago.dnpr"),null);
});

test("model mismatch hints fail closed, device requires actual identity for attestation", () => {
  const b=make("chicago");
  expectCode(()=>parseDnpr(b,{targetModel:"pro",fileName:"bass.elite.chicago.dnpr"}),"WRONG MODEL");
  assert.equal(parseDnpr(b,{targetModel:"elite",fileName:"bass.elite.chicago.dnpr"}).modelInRecord,false);
  expectCode(()=>ensureDeviceModel("elite",null),"DEVICE UNVERIFIED");
  expectCode(()=>ensureDeviceModel("elite","pro"),"WRONG MODEL");
  assert.equal(ensureDeviceModel("pro","pro"),true);
});

test("ZIP stored records preserve every byte, format is not a firmware bank", async () => {
  const files=[{name:"A.chicago.dnpr",bytes:GOLDEN_CHI},{name:"B.909.dnpr",bytes:GOLDEN_909}];
  const archive=makeDnprZip(files);
  const got=await readDnprZip(archive);
  assert.equal(got.length,2);
  for(let i=0;i<files.length;i++) {assert.equal(got[i].name,files[i].name);assert.deepEqual(got[i].bytes,files[i].bytes);}
  assert.equal(parseDnpr(got[1].bytes).engine,"909");
});

test("ZIP rejects duplicate names, traversal, CRC errors and tampered member counts", async () => {
  const a={name:"A.chicago.dnpr",bytes:GOLDEN_CHI};
  expectCode(()=>makeDnprZip([a,a]),"BAD FILE");
  expectCode(()=>makeDnprZip([{name:"../unsafe.dnpr",bytes:GOLDEN_CHI}]),"BAD FILE");
  const zip=makeDnprZip([a]);const bad=clone(zip);
  const at=bad.indexOf(0x44,30); assert.ok(at>0);bad[at]^=1;
  await assert.rejects(()=>readDnprZip(bad),e=>e.code==="BAD FILE");
  const invalid=clone(zip); invalid[invalid.length-12]=2;
  await assert.rejects(()=>readDnprZip(invalid),e=>e.code==="BAD FILE");
});

test("unsupported third-party plugin bytes are not presented as DNPR or converted", () => {
  expectCode(()=>parseDnpr(Uint8Array.from([65,85,80,82,69,83,69,84])),"BAD FILE");
});

test("file UI is explicitly offline and does not invoke device or network APIs", () => {
  const html=readFileSync(new URL("./index.html",import.meta.url),"utf8");
  const app=readFileSync(new URL("./app.mjs",import.meta.url),"utf8");
  assert.match(html,/DnBOX/);assert.match(html,/color-scheme:\s*dark/);
  assert.match(html,/role="status"/);assert.match(html,/type="file"/);
  assert.match(app,/importMany/);assert.match(app,/download/);
  assert.doesNotMatch(app,/\bfetch\s*\(|requestMIDIAccess|navigator\.usb|navigator\.serial|localStorage|sessionStorage|\.send\s*\(/);
});