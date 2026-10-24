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
const meta = cut("function isCatalogChecked(", "\nasync function sha256Hex(");
const releases = cut("async function mergeGitHubReleases()", '\n$("ctlfile").addEventListener("change"');
const loader = cut("async function loadSelectedPackage()", "\nfunction fillVersions()");
const chooser = cut("function fillVersions()", "\nfunction confirmWrite(");
assert.doesNotMatch(releases, /fetch\s*\(/, "GitHub Releases must not be imported");
assert.match(loader, /if \(!isCatalogChecked\(ver, pkg\)\)/, "missing selected-package guard");
assert.match(loader, /if \(hex !== pkg\.sha256\)/, "package SHA comparison required");
assert.match(loader, /if \(product !== pkg\.product\)/, "exact model/product required");
assert.match(chooser, /\$\("download"\)\.removeAttribute\("href"\)/, "stale href persists");
assert.match(chooser, /\$\("download"\)\.hidden = true/, "empty picker must hide link");
assert.match(html, /id="stockfile"[^>]*accept="\.fwsc"/, "local arbitrary .fwsc import lost");

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
  package_url:pkg.pkg, package_sha256:pkg.sha256,
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
    removeAttribute(name) {this[name] = null;}
  }]));
  let requests = 0;
  const context = createContext({
    console, Object, Array, Number, document:{}, dom,
    fetchCount: () => requests,
    fetch: async () => { requests++; throw Error("unapproved release fetch"); }
  });
  new Script(`let catalogLive={versions:[]}, chosenModel="elite";
    const $ = (id) => dom[id];
    function currentVersion() { return versions()[0] || null; }
    ${pieces}
    ${meta}
    ${releases}
    globalThis.setCatalog = (v) => { catalogLive={latest:v.version,versions:[v]}; };
    globalThis.setModel = (m) => { chosenModel=m; };
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
test("explicit fully QA-checked Elite can download without owner hardware PASS", () => {
  checked.setCatalog(good);
  assert.equal(checked.available().length,1);
  assert.deepEqual({...checked.present()}, {hidden:false,href:pkg.pkg});
});
test("unapproved Pro cannot borrow Elite proof", () => {
  checked.setModel("pro");
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
  checked.setModel("elite");
});
test("switch from good to bad clears previously displayed href", () => {
  checked.setCatalog(good);
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
test("revoked package clears a previously approved direct Download link", () => {
  checked.setCatalog(good);
  assert.equal(checked.present().hidden, false);
  checked.setCatalog({...good,models:{elite:{...pkg,revoked:true}}});
  assert.deepEqual(Array.from(checked.available()), []);
  assert.deepEqual({...checked.present()}, {hidden:true,href:null});
});
test("explicitly nonrevoked approved package still appears", () => {
  const explicit = {...good,revoked:false,fromRelease:false,
    models:{elite:{...pkg,fromRelease:false,revoked:false}}};
  assert.equal(Array.from(checked.evaluation(explicit,{[key]:{...approval,revoked:false}})).length, 1);
});
reject("unapproved Release import", v => v.models.elite.fromRelease=true);
reject("QA not passed", v => v.qa_pass=false);
reject("source license not verified", v => v.source_licenses_approved=false);
reject("source root mismatch", v => v.source_commit="f".repeat(40));
reject("firmware subtree mismatch", v => v.firmware_tree_sha="f".repeat(40));
reject("model swap", v => v.models.elite.product="SMK-37 Pro_9001100");
reject("package SHA mismatch", v => v.models.elite.sha256="c".repeat(64));
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
  const dom = {version: select, noversion: {}, versionrow: {},
    modeldetails: {}, go: {}, pkgmeta: {}, stock: {}, stocklinks: {},
    download: {removeAttribute(name) {this[name] = null;}}};
  const context = createContext({dom, eligible, latest,
    document: {createElement: () => ({value: "", textContent: ""})}});
  new Script(`let loadSerial=0, image=null, selectedProduct=null, catalogLive={latest};
    const $ = (id) => dom[id];
    const versions = () => eligible;
    const refreshModelUi = () => {};
    const loadSelectedPackage = () => {};
    const say = () => {}, t = () => "";
    ${chooser}
    fillVersions();`).runInContext(context);
  return {selected: select.value, options: select.options.map((o) => o.value),
    hidden: dom.download.hidden};
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
function makeRace(){
  const pending=new Map(),errors=[],go={disabled:true},version={version:"synthetic",models:racePkgs};
  const ctx=createContext({
    $:()=>go,refreshMeta(){},currentVersion:()=>version,
    isCatalogChecked:(v,p)=>!!v&&Object.values(v.models).includes(p),
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
  return {ctx,pending,errors};
}
{
  const r=makeRace(),first=r.ctx.choose("elite"),second=r.ctx.choose("pro");
  r.pending.get(racePkgs.pro.pkg).resolve({ok:true,arrayBuffer:async()=>new Uint8Array([2]).buffer});
  await second;
  r.pending.get(racePkgs.elite.pkg).resolve({ok:true,arrayBuffer:async()=>new Uint8Array([1]).buffer});
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
  r.pending.get(racePkgs.pro.pkg).resolve({ok:true,arrayBuffer:async()=>new Uint8Array([2]).buffer});
  await second;
  assert.equal(r.ctx.result().selectedProduct,racePkgs.pro.product);
  console.log("PASS stale HTTP error does not hide current selection");
}

