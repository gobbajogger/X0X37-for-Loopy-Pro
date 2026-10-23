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
assert.ok(Array.isArray(catalog.policy.public_device_proven_versions), 'public-proven list is required');
const publicProven = new Set(catalog.policy.public_device_proven_versions);
assert.equal(publicProven.size, catalog.policy.public_device_proven_versions.length, 'duplicate public eligibility');
assert.equal(catalog.policy.public_latest === null, publicProven.size === 0, 'missing or unexpected public latest');
if (catalog.policy.public_latest !== null) {
  assert.ok(publicProven.has(catalog.policy.public_latest), 'latest must have real physical evidence');
}
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
    assert.ok(typeof data.device_verified === 'boolean', 'device verified must be explicit');
    if (data.device_verified) {
      assert.equal(build.hardware_tested, true, 'device PASS cannot contradict version status');
      assert.equal(build.status, 'dev-checked', 'device PASS cannot upgrade a failed/unbuilt version');
      assert.ok(data.physical_evidence && /^[a-f0-9]{64}$/.test(data.physical_evidence.report_sha256), 'missing attributable hardware report digest');
      assert.ok(/^https:\/\//.test(data.physical_evidence.report_url), 'missing physical report link');
      assert.ok(/^[a-f0-9]{64}$/.test(data.physical_evidence.controller_sha256), 'missing controller digest');
      assert.equal(data.physical_evidence.owner_publication_approved, true, 'owner approval required');
    }
  }
  if (publicProven.has(build.id)) {
    assert.ok(Object.values(build.models).some(m => m.device_verified), 'public admission needs at least one model PASS');
  }
}
for (const id of publicProven) assert.ok(seen.has(id), 'unknown public build ' + id);
const ids = new Set();
for (const item of catalog.history) {
  assert.equal(ids.has(item.id), false, 'duplicate historical item ' + item.id);
  ids.add(item.id);
}
assert.doesNotMatch(text, /"pkg"\s*:|\.fwsc|ctl\.img|"download"\s*:/i);

const page = read('docs/builds/index.html');
const home = read('docs/index.html');
assert.match(home, /href="builds\/"/, 'nightly landing page must link build history');
assert.match(page, /fetch\('\.\.\/build-status\.json'/, 'read-only page must fetch the published status manifest');
assert.match(page, /role="status" aria-live="polite"/, 'history loading and errors need live status');
assert.match(page, /textContent/, 'history page must render JSON via textContent');
assert.doesNotMatch(page, /href=["'][^"']*\.fwsc/i, 'history must never link directly to firmware packages');
const js = page.match(/<script type="module">([\s\S]*?)<\/script>/)?.[1];
assert.ok(js, 'build history page must have its renderer');
new (await import('node:vm')).Script(js, {filename:'nightly-build-history.js'});

console.log('PASS: ' + catalog.builds.length + ' nightly builds + ' + catalog.history.length + ' history; no unapproved installer entries');
