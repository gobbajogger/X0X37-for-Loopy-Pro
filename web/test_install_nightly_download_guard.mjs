// SPDX-License-Identifier: GPL-3.0-only
// Exact web/x0x_installer.html source excerpts; no network, MIDI or writes.
// Run: node web/test_install_nightly_download_guard.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script, createContext } from "node:vm";

const path = process.argv[2] || new URL("./x0x_installer.html", import.meta.url);
const html = readFileSync(path, "utf8");
function cut(start, end) {
  const i = html.indexOf(start), j = html.indexOf(end, i);
  assert(i >= 0 && j > i, `source excerpt missing: ${start}`);
  return html.slice(i, j);
}
const admission = cut("const NIGHTLY_PUBLISHER_APPROVALS =", "\nfunction currentVersion()");
const downloadGuard = cut("let verifiedDownload = null;", "\nfunction led(");
const meta = cut("function isCatalogChecked(", "\nasync function sha256Hex(");
const releases = cut("async function mergeGitHubReleases()", '\n$("ctlfile").addEventListener("change"');
const loader = cut("async function readApprovedPackage(", "\nfunction fillVersions()");
const chooser = cut("function fillVersions()", "\nfunction confirmWrite(");
assert.doesNotMatch(releases, /fetch\s*\(/, "GitHub Releases must not be imported");
assert.match(loader, /if \(!isCatalogChecked\(ver, pkg\)\)/, "missing selected-package guard");
assert.match(loader, /if \(hex !== pkg\.sha256\)/, "package SHA comparison required");
assert.match(loader, /if \(product !== pkg\.product\)/, "exact model/product required");
assert.match(loader, /URL\.createObjectURL\(new Blob\(\[raw\]/, "download must serve verified package bytes");
assert.doesNotMatch(meta, /link\.href\s*=\s*pkg\.pkg/, "unverified remote href must not be exposed");
assert.match(meta, /link\.href = verifiedDownload\.href/, "only verified Blob may be linked");
assert.match(chooser, /if \(!list\.length\) \{[\s\S]*discardVerifiedDownload\(\)/, "empty picker must revoke verified Blob URL");
assert.match(html, /id="stockfile"[^>]*accept="\.fwsc"/, "local arbitrary .fwsc import lost");


assert.match(html, /id="stocklink" href="https:\/\/www\.m-vave\.com\/download"/,
  "official manufacturer landing page lost");
assert.ok(!html.includes("yms-file-store.oss-cn-hongkong.aliyuncs.com/software/firmware/"),
  "unreviewed vendor firmware HREF exposed");
const modelStockUi = cut("function refreshModelUi()", "\nfunction isCatalogChecked(");
assert.ok(modelStockUi.includes('$("stocklinks").textContent = "";'),
  "model picker must not inject unreviewed firmware links");
assert.ok(chooser.includes('$("stocklinks").textContent = "";'),
  "empty catalog must not inject unreviewed firmware links");

const product = "SMK-37 Elite_9001100";
const pkg = {product, size:651396, pkg:"https://nightly.example.test/sha256/elite-abc.fwsc",
  sha256:"a".repeat(64), controller_sha256:"b".repeat(64)};
const good = {version:"dev-sha-pinned",status:"dev-checked",
  source_commit:"1".repeat(40), firmware_tree_sha:"2".repeat(40),
  qa_pass:true, source_licenses_approved:true, hardware_tested:false,
  models:{elite:pkg}};
const approval = {
  prehardware_qa_pass:true, license_and_source_ok:true,
  backup_integrity_pass:true, model_and_controller_pass:true,
  immutable_http_pass:true, model:"elite", product,
  source_root_sha:good.source_commit, firmware_tree_sha:good.firmware_tree_sha,
  package_url:pkg.pkg, package_size:pkg.size, package_sha256:pkg.sha256,
  controller_sha256:pkg.controller_sha256, host_sha256:pkg.sha256
};
const key = `${good.version}/elite`;
let checks = 0;
function test(label, fn) {
  fn();
  checks++;
  console.log("PASS " + label);
}
function runtime(publisher = {}) {
  const pieces = admission.replace("Object.freeze({})", `Object.freeze(${JSON.stringify(publisher)})`);
  const dom = Object.fromEntries(["download", "pkgmeta"].map((k) => [k, {
    hidden:true, href:null, download:null, textContent:"",
    removeAttribute(name) {this[name] = null;},
    addEventListener() {}
  }]));
  let requests = 0;
  const context = createContext({
    console, Object, Array, Number, document:{}, dom,
    URL: {revokeObjectURL() {}},
    fetchCount: () => requests,
    fetch: async () => { requests++; throw Error("unapproved release fetch"); }
  });
  new Script(`let catalogLive={versions:[]}, chosenModel="elite";
    const $ = (id) => dom[id];
    function currentVersion() { return versions()[0] || null; }
    ${downloadGuard}
    ${pieces}
    ${meta}
    ${releases}
    globalThis.setCatalog = (v) => { catalogLive={latest:v.version,versions:[v]}; };
    globalThis.setModel = (m) => { chosenModel=m; };
    globalThis.setVerified = (p, version) => { verifiedDownload = {pkg:p,model:"elite",version,href:"blob:verified"}; };
    globalThis.available = () => versions().map(v=>Object.keys(v.models));
    globalThis.evaluation = (v,a) => approvedNightlyModels(v,a);
    globalThis.present = () => {refreshMeta();return {hidden:dom.download.hidden,href:dom.download.href};};
    globalThis.tryRelease = () => mergeGitHubReleases();
    globalThis.reqCount = () => fetchCount();`).runInContext(context);
  return context;
}
const noApproval = runtime();
test("unreviewed all-green catalog still hides direct download", () => {
  noApproval.setCatalog(good);
  assert.deepEqual(Array.from(noApproval.available()),[]);
  assert.deepEqual({...noApproval.present()}, {hidden:true,href:null});
});
const fakeRelease = structuredClone(good);
fakeRelease.version = "unreviewed-release";
fakeRelease.models.elite.fromRelease = true;
fakeRelease.models.elite.pkg = "https://github.com/unknown/releases/download/x.fwsc";
noApproval.setCatalog(fakeRelease);
await noApproval.tryRelease();
test("unapproved GitHub release asset cannot appear or request network", () => {
  assert.equal(noApproval.reqCount(),0);
  assert.equal(noApproval.present().hidden,true);
});
const checked = runtime({[key]:approval});
test("approved Elite appears only after verified bytes, not before host SHA check", () => {
  checked.setCatalog(good);
  assert.equal(checked.available().length,1);
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
  checked.setVerified(pkg,good.version);
  assert.deepEqual({...checked.present()}, {hidden:false,href:"blob:verified"});
});
test("unapproved Pro cannot borrow Elite proof", () => {
  checked.setModel("pro");
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
  checked.setModel("elite");
});
test("switch from good to bad clears previously displayed href", () => {
  checked.setCatalog(good);
  checked.setVerified(pkg,good.version);
  assert.equal(checked.present().hidden,false);
  const bad={...good, models:{elite:{...pkg,sha256:"0".repeat(64)}}};
  checked.setCatalog(bad);
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
});
function reject(name, mutate, customApproval = {[key]:approval}) {
  const v = structuredClone(good);
  mutate(v);
  test("reject " + name,() => {
    assert.equal(Array.from(checked.evaluation(v,customApproval)).length,0);
  });
}
// A model/release or publisher proof can be revoked without revoking its parent
// version. Optional and non-boolean markers must not bypass direct-download gating.
reject("per-model revoked=true", v => { v.models.elite.revoked=true; });
reject("per-model revoked string", v => { v.models.elite.revoked="true"; });
reject("catalog revoked string", v => { v.revoked="true"; });
reject("catalog Release marker", v => { v.fromRelease=true; });
reject("nonboolean imported-Release marker", v => { v.models.elite.fromRelease="true"; });
reject("revoked publisher tuple", v => {}, {[key]:{...approval,revoked:true}});
reject("Release-marked publisher tuple", v => {}, {[key]:{...approval,fromRelease:true}});
reject("nonboolean Release-marked publisher tuple", v => {}, {[key]:{...approval,fromRelease:"true"}});
test("revoked package clears a previously approved direct Download link", () => {
  checked.setCatalog(good);
  checked.setVerified(pkg,good.version);
  assert.equal(checked.present().hidden, false);
  checked.setCatalog({...good,models:{elite:{...pkg,revoked:true}}});
  assert.deepEqual(Array.from(checked.available()), []);
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
});
test("explicitly nonrevoked approved package still appears", () => {
  const explicit = {...good,revoked:false,fromRelease:false,
    models:{elite:{...pkg,fromRelease:false,revoked:false}}};
  assert.equal(Array.from(checked.evaluation(explicit,{[key]:{...approval,revoked:false,fromRelease:false}})).length, 1);
});
reject("unapproved Release import", v => v.models.elite.fromRelease=true);
reject("QA not passed", v => v.qa_pass=false);
reject("source license not verified", v => v.source_licenses_approved=false);
reject("source root mismatch", v => v.source_commit="f".repeat(40));
reject("firmware subtree mismatch", v => v.firmware_tree_sha="f".repeat(40));
reject("model swap", v => v.models.elite.product="SMK-37 Pro_9001100");
reject("package SHA mismatch", v => v.models.elite.sha256="c".repeat(64));
reject("catalog package size changed without publisher approval", v => { v.models.elite.size++; });
reject("catalog package size enlarged to unbounded budget", v => { v.models.elite.size=Number.MAX_SAFE_INTEGER; });
reject("publisher package size mismatch", v => {}, {[key]:{...approval,package_size:pkg.size+1}});
reject("publisher package size absent", v => {}, {[key]:{...approval,package_size:undefined}});
reject("controller SHA mismatch", v => v.models.elite.controller_sha256="d".repeat(64));
reject("unapproved HTTP origin", v => v.models.elite.pkg="https://other.test/a.fwsc");
reject("relative Pages package without immutable approval", v => v.models.elite.pkg="../firmware/a.fwsc");
reject("traversal link", v => v.models.elite.pkg="https://nightly.example.test/../b.fwsc");
reject("revoked build", v => v.revoked=true);
reject("missing host byte check", v => {}, {[key]:{...approval,immutable_http_pass:false}});
reject("missing backup integrity", v => {}, {[key]:{...approval,backup_integrity_pass:false}});
reject("wrong approved model", v => {}, {[key]:{...approval,model:"pro"}});
test("default embedded publisher list cannot self-authorize", () => {
  assert.deepEqual(Array.from(noApproval.evaluation(good,{})),[]);
});
// NIGHTLY_ELIGIBLE_LATEST_FALLBACK_BEGIN
// Filtered version lists must not select an unapproved catalog.latest.
function eligiblePickerResult(eligible, latest, current = "") {
  const select = {
    _value: current, options: [],
    get value() { return this._value; },
    set value(v) { this._value = this.options.some((o) => o.value === v) ? v : ""; },
    set innerHTML(_markup) { this.options = []; this._value = ""; },
    append(option) { this.options.push(option);
      if (this.options.length === 1) this._value = option.value; }
  };
  const revoked = [];
  const dom = {version: select, noversion: {}, versionrow: {},
    modeldetails: {}, go: {}, pkgmeta: {}, stock: {}, stocklinks: {},
    download: {hidden: true, href: null, addEventListener() {},
      removeAttribute(name) {this[name] = null;}}};
  const context = createContext({dom, eligible, latest,
    URL: {revokeObjectURL: (href) => revoked.push(href)},
    document: {createElement: () => ({value: "", textContent: ""})}});
  new Script(`let loadSerial=0, image=null, selectedProduct=null, catalogLive={latest}, chosenModel="elite";
    const $ = (id) => dom[id];
    const versions = () => eligible;
    const refreshModelUi = () => {};
    const refreshNightly = () => {};
    const loadSelectedPackage = () => {};
    const say = () => {}, t = () => "";
    ${downloadGuard}
    ${chooser}
    if (!eligible.length) {
      verifiedDownload={href:"blob:stale-approved",pkg:{},model:"elite",version:"before"};
      dom.download.href=verifiedDownload.href;
      dom.download.hidden=false;
    }
    fillVersions();
    globalThis.blobAlive=verifiedDownload!==null;`).runInContext(context);
  return {selected: select.value, options: select.options.map((o) => o.value),
    hidden: dom.download.hidden, href: dom.download.href,
    revoked, blobAlive: context.blobAlive};
}
const eligibleList = [{version: "dev-approved-1"}, {version: "dev-approved-0"}];
test("unapproved newest catalog entry falls back to eligible version", () => {
  const r = eligiblePickerResult(eligibleList, "source-only-newer");
  assert.equal(r.selected, "dev-approved-1");
  assert.deepEqual(r.options, ["dev-approved-1", "dev-approved-0"]);
});
test("missing latest catalog entry falls back to eligible version", () => {
  assert.equal(eligiblePickerResult(eligibleList, "deleted-latest").selected, "dev-approved-1");
});
test("approved newest catalog entry remains selected", () => {
  assert.equal(eligiblePickerResult(eligibleList, "dev-approved-0").selected, "dev-approved-0");
});
test("manual eligible choice survives catalog refresh", () => {
  assert.equal(eligiblePickerResult(eligibleList, "dev-approved-1", "dev-approved-0").selected, "dev-approved-0");
});
test("empty eligible catalog keeps picker blank and direct link hidden", () => {
  const r = eligiblePickerResult([], "source-only-newer");
  assert.equal(r.selected, "");
  assert.equal(r.options.length, 0);
  assert.equal(r.hidden, true);
  assert.equal(r.href, null);
  assert.deepEqual(r.revoked, ["blob:stale-approved"]);
  assert.equal(r.blobAlive, false);
});

// Cached verified bytes must remain bound to the selected, independently
// approved version/model/package. A blank or programmatically replaced DOM
// selection must never flash an old image under a new package's approval.
const exactSelectedVersion = cut("function currentVersion()", "\nfunction modelKeyForProduct(");
const exactGoHandler = cut('$("go").addEventListener("click", async () => {', "\nfunction confirmNightlyModel(");
const publisherAdmission = admission.replace("Object.freeze({})",
  `Object.freeze(${JSON.stringify({[key]:approval})})`);
function attemptedNightlyWrite({selection=good.version, cached=pkg,
  cachedVersion=good.version, cachedProduct=product, cachedHref="blob:verified"}={}) {
  const handlers={}, calls=[];
  const dom = {version:{value:selection}, go:{disabled:false,
    addEventListener(event,fn){handlers[event]=fn;}}};
  // Product go-handler reads break-pack radio via document.querySelector;
  // default null => choice "none" (no pack insert) for these selection-guard cases.
  const ctx=createContext({dom,good,pkg,cached,cachedVersion,cachedProduct,
    cachedHref,calls,Uint8Array,document:{querySelector:()=>null}});
  new Script(`const $=(id)=>dom[id];
    let catalogLive={latest:good.version,versions:[good]},chosenModel="elite";
    let loadSerial=0,image=new Uint8Array([17]),selectedProduct=cachedProduct;
    let verifiedDownload=cached&&{pkg:cached,model:"elite",
      version:cachedVersion,href:cachedHref};
    let writing=false,ctlNeedsResolution=false,ctl=null,ctlHash=null;
    const confirmWrite=()=>true;
    const ownedRun=(raw,prod)=>{calls.push({product:prod,byte:raw[0]});};
    const insertCtl=(raw)=>raw;
    const sayK=()=>{},t=()=>"?";
    ${publisherAdmission}
    ${exactSelectedVersion}
    ${meta}
    ${exactGoHandler}
  `).runInContext(ctx);
  handlers.click();
  return calls;
}
test("exact approved loaded Blob permits the normal install click", () => {
  assert.equal(attemptedNightlyWrite().length,1);
});
test("removed/version-blank DOM selection refuses the previously loaded image", () => {
  assert.equal(attemptedNightlyWrite({selection:""}).length,0);
});
test("another eligible selected version cannot authorize previously cached bytes", () => {
  assert.equal(attemptedNightlyWrite({cachedVersion:"older-version"}).length,0);
});
test("different cached package cannot borrow the new version approval", () => {
  assert.equal(attemptedNightlyWrite({cached:{...pkg,product:"SMK-37 Elite_9001000"}}).length,0);
});
test("mismatched cached product identity cannot be flashed", () => {
  assert.equal(attemptedNightlyWrite({cachedProduct:"SMK-37 Pro_9001100"}).length,0);
});
test("no verified Blob never creates a usable install candidate", () => {
  assert.equal(attemptedNightlyWrite({cached:null}).length,0);
});
test("remote non-Blob href never creates a usable install candidate", () => {
  assert.equal(attemptedNightlyWrite({cachedHref:"https://release.test/unchecked.fwsc"}).length,0);
});
function manipulatedDownloadHref(visible) {
  let vetoes=0;const revoked=[];
  const dom={version:{value:good.version},
    download:{hidden:false,href:visible,download:"x.fwsc",
      removeAttribute(k){this[k]=null;},
      addEventListener(_event,fn){this.clickHandler=fn;}}};
  const ctx=createContext({dom,good,pkg,URL:{revokeObjectURL:(url)=>revoked.push(url)}});
  new Script(`const $=(id)=>dom[id];
    let catalogLive={latest:good.version,versions:[good]},chosenModel="elite";
    ${publisherAdmission}
    ${exactSelectedVersion}
    ${downloadGuard}
    ${meta}
    verifiedDownload={pkg,model:"elite",version:good.version,href:"blob:verified"};
  `).runInContext(ctx);
  dom.download.clickHandler({preventDefault(){vetoes++;}});
  return {vetoes,revoked,visible:dom.download.href,hidden:dom.download.hidden};
}
test("unaltered verified Blob download remains permitted", () => {
  const r=manipulatedDownloadHref("blob:verified");
  assert.equal(r.vetoes,0);
  assert.equal(r.visible,"blob:verified");
  assert.deepEqual(r.revoked,[]);
});
test("altered direct remote Download HREF is vetoed and verified URL revoked", () => {
  const r=manipulatedDownloadHref("https://release.test/unapproved.fwsc");
  assert.equal(r.vetoes,1);
  assert.equal(r.visible,null);
  assert.equal(r.hidden,true);
  assert.deepEqual(r.revoked,["blob:verified"]);
});

// NIGHTLY_ELIGIBLE_LATEST_FALLBACK_END

console.log(`PASS ${checks} NIGHTLY installer source-excerpt assertions; device/host HTTP not tested`);

// Request order regression, controlled HTTP/model fixture (no real device/HTTP).
assert.match(html,/let loadSerial = 0;/);
assert.match(chooser,/function fillVersions\(\) \{\s*loadSerial\+\+;/);
const racePkgs={
  elite:{product:"SMK-37 Elite_9001100",pkg:"https://nightly.example.test/elite.fwsc",sha256:"a".repeat(64)},
  pro:{product:"SMK-37 Pro_9001100",pkg:"https://nightly.example.test/pro.fwsc",sha256:"b".repeat(64)}
};
// Use real streamed responses; no body means deny without arrayBuffer allocation.
function streamedResponse(data) {
  let sent = false;
  return {ok:true,body:{getReader:()=>({
    async read(){if(sent)return {done:true};sent=true;return {done:false,value:data};},
    async cancel(){},releaseLock(){}
  })}};
}
function makeRace({eliteSize=1,proSize=1}={}){
  const pending=new Map(),errors=[],created=[],go={disabled:true},
    version={version:"synthetic",models:{
      elite:{...racePkgs.elite,size:eliteSize},
      pro:{...racePkgs.pro,size:proSize}
    }};
  const ctx=createContext({
    $:()=>go,refreshMeta(){},currentVersion:()=>version,
    verifiedDownload:null, discardVerifiedDownload(){},
    URL:{createObjectURL:()=>{created.push("blob:verified-race");return "blob:verified-race";},revokeObjectURL(){}},Blob,
    isCatalogChecked:(v,p)=>!!v&&Object.values(v.models).includes(p),
    hasVerifiedNightlySelection:(v,p)=>!!v&&Object.values(v.models).includes(p),
    ctlNeedsResolution:false,
    sayK:k=>errors.push(k),say(){},MODELS:{elite:"Elite",pro:"Pro"},
    sha256Hex:async raw=>new Uint8Array(raw)[0]===1?"a".repeat(64):"b".repeat(64),
    productOf:raw=>new Uint8Array(raw)[0]===1?racePkgs.elite.product:racePkgs.pro.product,
    logicalImage:raw=>new Uint8Array(raw)[0],
    fetch:url=>new Promise((resolve,reject)=>pending.set(url,{resolve,reject})),Uint8Array
  });
  new Script("let loadSerial=0,chosenModel='elite',image=null,selectedProduct=null,writing=false;\n"
    +loader+"\n"
    +"globalThis.choose=m=>{chosenModel=m;return loadSelectedPackage()};\n"
    +"globalThis.result=()=>({image,selectedProduct,disabled:$('go').disabled});")
    .runInContext(ctx);
  return {ctx,pending,errors,created};
}
{
  const r=makeRace(),first=r.ctx.choose("elite"),second=r.ctx.choose("pro");
  r.pending.get(racePkgs.pro.pkg).resolve(streamedResponse(new Uint8Array([2])));
  await second;
  r.pending.get(racePkgs.elite.pkg).resolve(streamedResponse(new Uint8Array([1])));
  await first;
  assert.equal(r.ctx.result().image,2);
  assert.equal(r.ctx.result().selectedProduct,racePkgs.pro.product);
  assert.equal(r.ctx.result().disabled,false);
  console.log("PASS latest approved model wins in-flight race");
}
{
  const r=makeRace(),first=r.ctx.choose("elite"),second=r.ctx.choose("pro");
  r.pending.get(racePkgs.elite.pkg).reject(Error("stale failure"));
  await first;
  assert.equal(r.errors.includes("loadfail"),false);
  assert.equal(r.ctx.result().disabled,true);
  r.pending.get(racePkgs.pro.pkg).resolve(streamedResponse(new Uint8Array([2])));
  await second;
  assert.equal(r.ctx.result().selectedProduct,racePkgs.pro.product);
  console.log("PASS stale HTTP error does not hide current selection");
}

// Approved catalog length must bind fetched bytes before SHA/product/Blob.
for (const [actual,declared] of [[1,2],[2,1]]) {
  const r=makeRace({eliteSize:declared});
  const attempt=r.ctx.choose("elite");
  r.pending.get(racePkgs.elite.pkg).resolve(streamedResponse(new Uint8Array(actual).fill(1)));
  await attempt;
  assert.equal(r.ctx.result().image,null);
  assert.equal(r.ctx.result().selectedProduct,null);
  assert.equal(r.ctx.result().disabled,true);
  assert.equal(r.ctx.verifiedDownload,null);
  assert.equal(r.created.length,0);
  assert.ok(r.errors.includes("badpkg"));
  console.log("PASS mismatch: bytes="+actual+", approved size="+declared+" forbids Blob/Install");
}
{
  const r=makeRace();
  const attempt=r.ctx.choose("elite");
  r.pending.get(racePkgs.elite.pkg).resolve(streamedResponse(new Uint8Array([1])));
  await attempt;
  assert.equal(r.ctx.result().image,1);
  assert.equal(r.ctx.result().selectedProduct,racePkgs.elite.product);
  assert.equal(r.ctx.result().disabled,false);
  assert.equal(r.created.length,1);
  assert.ok(r.ctx.verifiedDownload.href.startsWith("blob:"));
  console.log("PASS size-matched eligible model still yields verified Blob");
}
{
  const r=makeRace(),attempt=r.ctx.choose("elite");
  r.pending.get(racePkgs.elite.pkg).resolve({ok:false,status:404});
  await attempt;
  assert.equal(r.ctx.result().image,null);
  assert.equal(r.ctx.result().disabled,true);
  assert.equal(r.created.length,0);
  assert.ok(r.errors.includes("loadfail"));
  console.log("PASS failed HTTP fetch still forbids Blob/Install");
}

// R23: streaming source guard. The production Fetch body is bounded before SHA
// or product parsing, and stale/error responses never authorize an old package.
const streamHelper = cut("async function readApprovedPackage(", "\nasync function loadSelectedPackage()");
assert.match(loader, /const raw = await readApprovedPackage\(r, pkg\.size, isLatest\)/,
  "approved download must use bounded stream reader");
const streamContext = createContext({ Uint8Array });
new Script(`${streamHelper}\nglobalThis.streamRead = readApprovedPackage;`).runInContext(streamContext);
const packets = (...values) => Uint8Array.from(values);
function streamResponse(parts, state = {}) {
  let index = 0;
  return { body: { getReader: () => ({
    async read() {
      if (state.failAt === index) throw new Error("network cut");
      return index < parts.length ? { done: false, value: parts[index++] } : { done: true };
    },
    async cancel() { state.canceled = true; },
    releaseLock() { state.released = true; }
  }) } };
}
let streamChecks = 0;
{
  const state = {};
  const raw = await streamContext.streamRead(streamResponse([packets(1,2), packets(3,4,5)], state), 5, () => true);
  assert.deepEqual([...raw], [1,2,3,4,5]);
  assert.equal(state.canceled, undefined);
  assert.equal(state.released, true);
  streamChecks++;
}
{
  const state = {};
  const raw = await streamContext.streamRead(streamResponse([packets(1,2,3),packets(4,5)],state),4,()=>true);
  assert.equal(raw,null);assert.equal(state.canceled,true);streamChecks++;
}
{
  const raw = await streamContext.streamRead(streamResponse([packets(1,2)]),4,()=>true);
  assert.equal(raw,null);streamChecks++;
}
{
  const state={};let read=0;
  const raw=await streamContext.streamRead(streamResponse([packets(1),packets(2)],state),2,()=>++read===1);
  assert.equal(raw,null);assert.equal(state.canceled,true);streamChecks++;
}
{
  const state={failAt:1};
  await assert.rejects(streamContext.streamRead(streamResponse([packets(1)],state),2,()=>true),/network cut/);
  assert.equal(state.canceled,true);assert.equal(state.released,true);streamChecks++;
}
{
  let calls=0;
  const raw=await streamContext.streamRead({body:null,arrayBuffer:async()=>{calls++;return packets(7,8).buffer;}},2,()=>true);
  assert.equal(raw,null);assert.equal(calls,0);streamChecks++;
}
{
  let calls=0;
  const raw=await streamContext.streamRead({body:{},arrayBuffer:async()=>{calls++;return packets(7,8).buffer;}},1,()=>true);
  assert.equal(raw,null);assert.equal(calls,0);streamChecks++;
}
{
  const state={};
  const raw=await streamContext.streamRead(streamResponse([packets(1,2,3,4)],state),2,()=>true);
  assert.equal(raw,null);assert.equal(state.canceled,true);streamChecks++;
}
checks += streamChecks;
console.log("PASS " + streamChecks + "/8 approved firmware streaming bound/abort/no-stream regressions (source-exact synthetic)");
