// SPDX-License-Identifier: GPL-3.0-only
// Test a private copy so malformed manifests never touch tracked files.
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';
const root = fileURLToPath(new URL('../', import.meta.url));
const required = [
  'scripts/verify-build-status.mjs', 'build-status.json', 'docs/build-status.json',
  'preview-catalog.json', 'docs/versions.json', 'docs/builds/index.html', 'docs/index.html'
];
function run(label, mutate, shouldPass = false) {
  const dir = fs.mkdtempSync(join(tmpdir(), 'x0x37-status-'));
  try {
    for (const name of required) {
      const target = join(dir, name);
      fs.mkdirSync(dirname(target), {recursive:true});
      fs.copyFileSync(join(root, name), target);
    }
    fs.writeFileSync(join(dir, 'package.json'), '{"type":"module"}\n');
    mutate(dir);
    const r = spawnSync(process.execPath, [join(dir, required[0])], {cwd: dir, encoding:'utf8', timeout:15000});
    assert.equal(r.error, undefined, label + ' spawned');
    if (shouldPass) assert.equal(r.status, 0, label + ': ' + r.stderr);
    else assert.notEqual(r.status, 0, label + ' was wrongly accepted');
    console.log('PASS ' + label);
  } finally {fs.rmSync(dir, {recursive:true, force:true});}
}
const edit = fn => dir => {
  const j = JSON.parse(fs.readFileSync(join(dir,'build-status.json'),'utf8'));
  fn(j);
  const s = JSON.stringify(j, null, 2) + '\n';
  fs.writeFileSync(join(dir,'build-status.json'),s);
  fs.writeFileSync(join(dir,'docs/build-status.json'),s);
};
run('baseline',()=>{},true);
for (const [name, fn] of [
  ['aggregate-device-flag',j=>j.builds[0].hardware_tested=true],
  ['forged-history-device-pass',j=>j.history[0].status='device-verified'],
  ['empty-build-models',j=>j.builds[0].models={}],
  ['script-URL',j=>j.history[0].url='javascript:alert(1)'],
  ['insecure-URL',j=>j.history[0].url='http://github.com/'],
  ['unsupported-model',j=>j.history[0].models=['FM1']],
  ['history-device-bool',j=>j.history[0].device_verified=true],
  ['history-installable',j=>j.history[0].installable=true],
  ['history-unknown-status',j=>j.history[0].status='stable'],
  ['history-bad-source-SHA',j=>j.history[0].source_commit='bad'],
  ['model-bad-hash',j=>j.builds[0].models.elite.sha256='bad'],
  ['firmware-link',j=>j.history[0].pkg='https://evil.example/evil.fwsc'],
]) run(name,edit(fn));
run('nonempty-preview',dir=>{
  const p=join(dir,'preview-catalog.json');
  const j=JSON.parse(fs.readFileSync(p,'utf8'));
  j.versions=[{version:'unapproved'}];fs.writeFileSync(p,JSON.stringify(j));
});
run('source-docs-mismatch',dir=>{
  const p=join(dir,'docs/build-status.json');
  fs.appendFileSync(p,' ');
});
console.log('PASS: 15 native positive/negative cases');
