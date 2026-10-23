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
console.log(`PASS ${checks} NIGHTLY installer source-excerpt assertions; device/host HTTP not tested`);
