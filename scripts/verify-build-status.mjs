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
  assert.equal(typeof build.hardware_tested, 'boolean', 'hardware status must be explicit');
  assert.ok(build.models && typeof build.models === 'object' && !Array.isArray(build.models) &&
    Object.keys(build.models).length > 0, 'firmware generation requires a model');
  assert.equal(build.hardware_tested,
    Object.values(build.models).some(m => m.device_verified === true),
    'aggregate hardware PASS requires attributable per-model PASS');
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
const historicalStatuses = new Set([
  'source-only', 'not-hosted', 'evidence-only', 'failed', 'reference',
  'validation-evidence-only', 'offline-projection-artifact-unattributed', 'build-succeeded-unreviewed'
]);
for (const item of catalog.history) {
  assert.equal(ids.has(item.id), false, 'duplicate historical item ' + item.id);
  ids.add(item.id);
  assert.ok(historicalStatuses.has(item.status), 'unverified or invalid history status ' + item.status);
  assert.match(item.source_commit, /^[a-f0-9]{40}$/, 'history requires immutable source SHA');
  if (item.source_tree !== undefined) assert.match(item.source_tree, /^[a-f0-9]{40}$/);
  if (item.status === 'build-succeeded-unreviewed') {
    const x = item.build_pipeline;
    assert.ok(x && typeof x === 'object');
    assert.equal(x.build, 'BUILD_SUCCEEDED_UNREVIEWED');
    assert.equal(x.qa, 'QA_PENDING');
    assert.equal(x.installability, 'NOT_INSTALLABLE');
    assert.equal(x.live_http, 'NOT_VERIFIED');
    assert.equal(x.hardware, 'DEVICE_NOT_TESTED');
    assert.equal(x.source_scope, 'FIRMWARE_SUBTREE_MATCH_FULL_ROOT_NOT_ATTESTED');
    assert.match(x.evidence_commit, /^[a-f0-9]{40}$/);
    assert.match(x.evidence_manifest_blob_sha, /^[a-f0-9]{40}$/);
    assert.ok(Number.isInteger(x.builder_selected_host_checks_pass) && x.builder_selected_host_checks_pass > 0);
    assert.ok(Number.isInteger(x.followup_native_checks_pass) && x.followup_native_checks_pass >= 0);
    assert.ok(Array.isArray(x.followup_native_checks_fail) && x.followup_native_checks_fail.length > 0);
    for (const failure of x.followup_native_checks_fail) {
      assert.ok(typeof failure.name === 'string' && failure.name.length > 0);
      assert.ok(Number.isInteger(failure.exit_code) && failure.exit_code !== 0);
    }
    assert.equal(x.package_bytes_independently_received_and_rehashed, false);
    assert.equal(item.artifact_bytes_independently_verified, false);
    assert.match(item.firmware_subtree, /^[a-f0-9]{40}$/);
    for (const model of ['elite','pro']) {
      const pkg = item.producer_reported_artifacts?.[model];
      assert.ok(pkg);
      assert.match(pkg.sha256, /^[a-f0-9]{64}$/);
      assert.ok(Number.isInteger(pkg.bytes) && pkg.bytes > 0);
    }
  }
  assert.ok(Array.isArray(item.models) && item.models.every(m => m === 'elite' || m === 'pro'),
    'history models must be Elite/Pro or empty');
  assert.equal(item.installable ?? false, false, 'history record cannot be flashable');
  assert.equal(item.device_verified ?? false, false, 'history item cannot assert unsupported physical PASS');
  assert.ok(item.url == null || (typeof item.url === 'string' &&
    /^https:\/\/github\.com\//.test(item.url)), 'history URL must be a trusted HTTPS GitHub link');
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
