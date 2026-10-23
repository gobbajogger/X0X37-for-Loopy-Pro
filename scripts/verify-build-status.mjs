// SPDX-License-Identifier: GPL-3.0-only
// Ensure NIGHTLY status history stays noninstallable and matches generated docs.
import { readFileSync } from 'node:fs';
import assert from 'node:assert/strict';
const read = (p) => readFileSync(new URL('../' + p, import.meta.url), 'utf8');
const text = read('build-status.json');
assert.equal(read('docs/build-status.json'), text, 'source and docs build status mismatch');
const catalog = JSON.parse(text);
const preview = JSON.parse(read('preview-catalog.json'));
const published = JSON.parse(read('docs/versions.json'));
assert.equal(catalog.schema, 1);
assert.equal(catalog.kind, 'nightly-read-only-build-history');
assert.equal(catalog.installable, false);
assert.equal(catalog.policy.public_latest, null);
assert.deepEqual(catalog.policy.public_device_proven_versions, []);
assert.match(catalog.source.versions_blob_sha, /^[a-f0-9]{40}$/);
assert.deepEqual(preview.versions, []);
assert.deepEqual(preview.history, []);
assert.equal(preview.latest, null);
assert.deepEqual(published, preview);
assert.equal(catalog.counts.actual_build_generations, catalog.builds.length);
assert.equal(catalog.counts.source_or_historical_checkpoints, catalog.history.length);
const seen = new Set();
for (const build of catalog.builds) {
  assert.equal(seen.has(build.id), false, 'duplicate build ' + build.id);
  seen.add(build.id);
  assert.match(build.source_commit, /^[a-f0-9]{40}$/);
  assert.equal(build.installable_from_nightly_status, false);
  for (const [model, data] of Object.entries(build.models)) {
    assert.ok(['elite', 'pro'].includes(model));
    assert.match(data.sha256, /^[a-f0-9]{64}$/);
    assert.ok(Number.isInteger(data.size) && data.size > 0);
    assert.equal(data.device_verified, false);
  }
}
const ids = new Set();
for (const item of catalog.history) {
  assert.equal(ids.has(item.id), false, 'duplicate historical item ' + item.id);
  ids.add(item.id);
}
assert.doesNotMatch(text, /"pkg"\s*:|\.fwsc|ctl\.img|"download"\s*:/i);
console.log('PASS: ' + catalog.builds.length + ' nightly builds + ' + catalog.history.length + ' history; no unapproved installer entries');
