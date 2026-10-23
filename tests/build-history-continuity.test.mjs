// SPDX-License-Identifier: GPL-3.0-only
// Cross-revision audit for THIS populated catalog; the schema separately permits empty fresh history.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
const read = path => JSON.parse(readFileSync(new URL('../' + path, import.meta.url), 'utf8'));
const lock = read('tests/fixtures/history-appendonly-baseline-20261024.json');
const original = read('build-status.json').history;
assert.equal(lock.schema, 1);
assert.equal(lock.origin.digest, 'SHA256(UTF8(JSON.stringify(parsed row)))');
function check(history) {
  assert.ok(Array.isArray(history) && history.length >= lock.rows.length,'history must preserve baseline');
  for(let i=0;i<lock.rows.length;i++){
    assert.equal(history[i].id,lock.rows[i].id,'history ID deleted or reordered at '+i);
    assert.equal(createHash('sha256').update(JSON.stringify(history[i]),'utf8').digest('hex'),
      lock.rows[i].sha256,'historical row modified at '+i);
  }
}
const clone = () => structuredClone(original);
const mustReject = (name,edit) => {
  const rows=clone();edit(rows);assert.throws(()=>check(rows),{name:'AssertionError'},name);
};
check(original);
const failure=original.findIndex(row=>row.id==='producer-pr93-20261023');
assert.ok(failure>=0,'historical compiler failure missing');
mustReject('deleted native compiler-failure receipt', rows=>rows.splice(failure,1));
mustReject('deleted failure and forged unrelated failed row',rows=>{
  rows.splice(failure,1);rows.find(r=>r.status==='source-only').status='failed';
});
mustReject('compiler failure relabeled',rows=>rows[failure].status='source-only');
mustReject('compiler log SHA forged',rows=>rows[failure].failed_build.pro.log_sha256='a'.repeat(64));
mustReject('history reorder',rows=>{[rows[0],rows[1]]=[rows[1],rows[0]];});
mustReject('latest source receipt removed',rows=>rows.pop());
const appended=clone();
appended.push({...appended.at(-1),id:'continuity-synthetic-future-source',source_commit:'f'.repeat(40)});
check(appended);
console.log('PASS: ' + lock.rows.length + ' SHA256-pinned historical records; 6 malicious continuity negatives; safe append');
