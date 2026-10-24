// SPDX-License-Identifier: GPL-3.0-only
// Exact-source NIGHTLY product admission regression; no device, network or write.
// Run: node web/test_install_nightly_product_identity.mjs
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Script, createContext} from 'node:vm';

const source = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const start = source.indexOf('const NIGHTLY_PUBLISHER_APPROVALS =');
const end = source.indexOf('\nfunction versions()', start);
assert.ok(start >= 0 && end > start, 'publisher authorization function missing');
const excerpt = source.slice(start, end);
assert.match(excerpt, /NIGHTLY_PRODUCTS/);
const context = createContext({Object, Number});
new Script(excerpt + '\nglobalThis.evaluate = approvedNightlyModels;').runInContext(context);
const evaluate = (v, approvals) => Array.from(context.evaluate(v, approvals));
let passed = 0;
function check(name, v, approvals, expected) {
  assert.deepEqual(evaluate(v, approvals), expected, name);
  passed++;
}
function fixture(model, product) {
  const p = {product, size:651396, sha256:'a'.repeat(64),
    controller_sha256:'b'.repeat(64),
    pkg:'https://nightly.example.test/immutable/model.fwsc'};
  const v = {version:'checked-sha', status:'dev-checked',
    source_commit:'1'.repeat(40),firmware_tree_sha:'2'.repeat(40),
    qa_pass:true,source_licenses_approved:true,hardware_tested:false,
    models:{[model]:p}};
  const a = {prehardware_qa_pass:true,license_and_source_ok:true,
    backup_integrity_pass:true,model_and_controller_pass:true,
    immutable_http_pass:true,model,product,
    source_root_sha:v.source_commit,firmware_tree_sha:v.firmware_tree_sha,
    package_url:p.pkg,package_size:p.size,package_sha256:p.sha256,
    controller_sha256:p.controller_sha256,host_sha256:p.sha256};
  return {v,a,approvals:{[v.version+'/'+model]:a},p};
}
for (const [model,name] of [['elite','SMK-37 Elite'],['pro','SMK-37 Pro']]) {
  for (const suffix of ['_900','_9001','_9001100']) {
    const x = fixture(model,name+suffix);
    check(model+' approved '+suffix,x.v,x.approvals,[model]);
  }
  {
    const x=fixture(model,name+'_9001100');
    x.p.size++;
    check(model+' reject changed catalog package size',x.v,x.approvals,[]);
    x.p.size--; x.a.package_size++;
    check(model+' reject changed publisher package size',x.v,x.approvals,[]);
  }
  for (const suffix of ['_900x','_901','_900-foo','_9001a','_900 ','_900.']) {
    const x = fixture(model,name+suffix);
    check(model+' reject '+suffix,x.v,x.approvals,[]);
  }
}
{
  const x=fixture('elite','SMK-37 Pro_900');
  check('wrong model product',x.v,x.approvals,[]);
}
{
  const x=fixture('pro','SMK-37 Pro_900');
  check('no attestation',x.v,{},[]);
  x.v.qa_pass=false; check('QA pending',x.v,x.approvals,[]);
  x.v.qa_pass=true; x.a.immutable_http_pass=false;
  check('HTTP unverified',x.v,x.approvals,[]);
  x.a.immutable_http_pass=true; x.v.revoked=true;
  check('revoked',x.v,x.approvals,[]);
  x.v.revoked=false; x.p.fromRelease=true;
  check('unreviewed Release',x.v,x.approvals,[]);
  x.p.fromRelease=false; x.v.source_licenses_approved=false;
  check('license unresolved',x.v,x.approvals,[]);
  x.v.source_licenses_approved=true; x.p.controller_sha256='c'.repeat(64);
  check('controller hash mismatch',x.v,x.approvals,[]);
  x.p.controller_sha256=x.a.controller_sha256; x.p.sha256='d'.repeat(64);
  check('package hash mismatch',x.v,x.approvals,[]);
  x.p.sha256=x.a.package_sha256; x.v.firmware_tree_sha='f'.repeat(40);
  check('firmware subtree mismatch',x.v,x.approvals,[]);
  x.v.firmware_tree_sha=x.a.firmware_tree_sha; x.v.source_commit='f'.repeat(40);
  check('root source mismatch',x.v,x.approvals,[]);
  x.v.source_commit=x.a.source_root_sha; x.p.pkg='https://nightly.example.test/../model.fwsc';
  check('URL path traversal',x.v,x.approvals,[]);
}
console.log('PASS '+passed+' exact-source NIGHTLY product identity/approval assertions (synthetic, no device/HTTP)');
