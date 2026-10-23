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
  ['build-forged-device-status',j=>j.builds[0].status='device-verified'],
  ['build-forged-stable-status',j=>j.builds[0].status='stable'],
  ['build-forged-hardware-proven',j=>j.builds[0].status='hardware-proven'],
  ['empty-build-id',j=>j.builds[0].id=''],
  ['empty-history-id',j=>j.history[0].id=''],
  ['cross-build-history-duplicate-id',j=>j.history[0].id=j.builds[0].id],
  ['empty-build-models',j=>j.builds[0].models={}],
  ['script-URL',j=>j.history[0].url='javascript:alert(1)'],
  ['insecure-URL',j=>j.history[0].url='http://github.com/'],
  ['unsupported-model',j=>j.history[0].models=['FM1']],
  ['history-device-bool',j=>j.history[0].device_verified=true],
  ['history-installable',j=>j.history[0].installable=true],
  ['history-unknown-status',j=>j.history[0].status='stable'],
  ['history-bad-source-SHA',j=>j.history[0].source_commit='bad'],
  ['model-bad-hash',j=>j.builds[0].models.elite.sha256='bad'],
  ['build-unreviewed-qa-forgery',j=>j.history.find(x=>x.status==='build-succeeded-unreviewed').build_pipeline.qa='QA_PASSED'],
  ['build-unreviewed-http-forgery',j=>j.history.find(x=>x.status==='build-succeeded-unreviewed').build_pipeline.live_http='LIVE_HTTP_VERIFIED'],
  ['build-unreviewed-installable-forgery',j=>j.history.find(x=>x.status==='build-succeeded-unreviewed').build_pipeline.installability='READY_FOR_FLASH'],
  ['build-unreviewed-silenced-failure',j=>j.history.find(x=>x.status==='build-succeeded-unreviewed').build_pipeline.followup_native_checks_fail=[]],
  ['build-unreviewed-missing-pro',j=>delete j.history.find(x=>x.status==='build-succeeded-unreviewed').producer_reported_artifacts.pro],
  ['build-unreviewed-forged-rehash',j=>j.history.find(x=>x.status==='build-succeeded-unreviewed').artifact_bytes_independently_verified=true],
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

run('empty-history-and-builds-allowed-without-device-firmware-or-QA',edit(j=>{
  j.history=[]; j.builds=[];
  j.counts.actual_build_generations=0;
  j.counts.source_or_historical_checkpoints=0;
  j.policy.public_latest=null; j.policy.public_device_proven_versions=[];
}),true);
run('approved-GitHub-source-evidence-remains-allowed',edit(j=>{
  j.history[0].evidence_url='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/58b5b45a2dd90581de7a674c331ca5cbe8a67c7e';
}),true);
for(const [label,mutation] of [
 ['nested-history-download-bin',j=>j.history[0].download_url='https://example.net/firmware.bin'],
 ['nested-history-picker',j=>j.history[0].picker_url='https://example.net/file'],
 ['nested-history-artifact-text',j=>j.history[0].artifact_url='https://example.net/readme.txt'],
 ['nested-history-assets',j=>j.history[0].assets={url:'https://example.net/file'}],
 ['nested-reported-check-download',j=>j.history.find(x=>x.reported_checks).reported_checks.download_url='https://example.net/file'],
 ['nested-build-pipeline-release',j=>j.history.find(x=>x.build_pipeline).build_pipeline.release_url='https://example.net/file'],
 ['nested-failed-check-link',j=>j.history.find(x=>x.build_pipeline).build_pipeline.followup_native_checks_fail[0].download_url='https://example.net/file'],
 ['nested-reported-package-link',j=>j.history.find(x=>x.producer_reported_artifacts).producer_reported_artifacts.elite.download_url='https://example.net/file'],
 ['nested-controller-link',j=>j.history.find(x=>x.producer_reported_artifacts).producer_reported_artifacts.private_controller.download_url='https://example.net/file'],
 ['nested-offline-controller-link',j=>j.history.find(x=>x.reported_offline_artifacts).reported_offline_artifacts.controller.download_url='https://example.net/file'],
 ['nested-failed-native-build-link',j=>j.history.find(x=>x.failed_build).failed_build.elite.download_url='https://example.net/file'],
 ['nested-build-model-link',j=>j.builds[0].models.elite.picker_url='https://example.net/file'],
 ['top-level-binary-link',j=>j.download_url='https://example.net/file'],
 ['unrecognized-build-release-checked',j=>j.builds[0].status='release-checked'],
 ['unrecognized-build-lifecycle',j=>j.builds[0].lifecycle='live'],
 ['duplicate-history-ids',j=>j.history[1].id=j.history[0].id],
 ['unapproved-download-on-github',j=>j.history[0].url='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v1/firmware.bin'],
 ['unapproved-binary-blob',j=>j.history[0].url='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/main/firmware.bin'],
 ['forged-GitHub-host',j=>j.history[0].url='https://github.com.evil.invalid/gobbajogger/X0X37-for-Loopy-Pro/pull/1'],
 ['github-query-forced-download',j=>j.history[0].url='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/1?download=1'],
 ['wrong-source-repository',j=>j.history.find(x=>x.source_repo).source_repo='https://example.net/sources'],
 ['unknown-source-download-field',j=>j.source.download_url='https://example.net/file'],
 ['versioning-picker-url',j=>j.versioning.picker_url='https://example.net/file'],
 ['unreviewed-controller-redistribution',j=>j.history.find(x=>x.producer_reported_artifacts).producer_reported_artifacts.private_controller.redistribution_permitted=true],
 ['failed-build-forged-artifact',j=>j.history.find(x=>x.failed_build).failed_build.elite.artifact_created=true]
]) run(label,edit(mutation));
run('synthetic-malicious-Release-does-not-become-an-installable-catalog',dir=>{
  const p=join(dir,'preview-catalog.json');
  const j=JSON.parse(fs.readFileSync(p,'utf8'));
  j.versions=[{model:'elite',source_commit:'f'.repeat(40),sha256:'f'.repeat(64),
    status:'DEVICE_VERIFIED',download_url:'https://evil.invalid/unapproved.bin'}];
  fs.writeFileSync(p,JSON.stringify(j));
});


 // Historical hardware observations and PRODUCTION permission are distinct.
 // These synthetic fixtures never add firmware or authorize installation.
const officialReport='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/'+'b'.repeat(40);
function setProof(j,{eliteApproved=false,proVerified=false,proApproved=false}={}){
  const b=j.builds[0];b.hardware_tested=true;
  for(const model of ['elite','pro']){
    if(model==='pro'&&!proVerified)continue;
    const m=b.models[model];m.device_verified=true;m.physical_evidence={
      report_sha256:'a'.repeat(64),report_url:officialReport,
      controller_sha256:'c'.repeat(64),owner_publication_approved:model==='elite'?eliteApproved:proApproved
    };
  }
}
run('recorded-Elite-device-PASS-without-production-permission',edit(j=>setProof(j)),true);
run('both-model-public-proof-synthetic-shape-only',edit(j=>{
  setProof(j,{eliteApproved:true,proVerified:true,proApproved:true});
  j.policy.public_latest=j.builds[0].id;j.policy.public_device_proven_versions=[j.builds[0].id];
}),true);
for(const [label,mutate] of [
 ['hidden-description-download-any-extension',j=>j.history[0].description={download_url:'https://example.net/firmware.bin'}],
 ['hidden-policy-artifact',j=>j.policy.nightly={artifact_url:'https://example.net/file.txt'}],
 ['hidden-reported-check-picker',j=>j.history.find(x=>x.reported_checks).reported_checks.result={picker_url:'https://example.net/file'}],
 ['hidden-build-pipeline-Release',j=>j.history.find(x=>x.build_pipeline).build_pipeline.builder_selected_host_checks_pass={release_url:'https://example.net/file'}],
 ['hidden-generic-link',j=>j.history[0].description={notes:'https://example.net/file'}],
 ['hidden-blob-URI',j=>j.history[0].description={asset_uri:'blob:unapproved'}],
 ['mislabelled-Elite-build-model',j=>j.builds[0].models.elite.product='SMK-37 Pro_900'],
 ['public-single-model-cannot-imply-Pro',j=>{setProof(j,{eliteApproved:true});j.policy.public_latest=j.builds[0].id;j.policy.public_device_proven_versions=[j.builds[0].id]}],
 ['public-list-needs-each-model-approval',j=>{setProof(j,{eliteApproved:false,proVerified:true,proApproved:true});j.policy.public_latest=j.builds[0].id;j.policy.public_device_proven_versions=[j.builds[0].id]}]
])run(label,edit(mutate));

console.log('PASS: status-history allowlist and malicious-nesting fixtures; no firmware artifacts');
