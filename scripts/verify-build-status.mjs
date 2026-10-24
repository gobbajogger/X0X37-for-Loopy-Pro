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
// The empty source-only picker is an exact schema, not an extensible download surface.
// Both source and generated picker files must use the approved provenance values.
const approvedPickerKeys = ['schema', 'repo', 'pages', 'latest', 'versions', 'history'];
assert.deepEqual(Object.keys(preview).sort(), approvedPickerKeys.sort(),
  'unapproved NIGHTLY picker field (including hidden download/approval metadata)');
assert.equal(preview.schema, 1, 'unsupported NIGHTLY picker schema');
assert.equal(preview.repo, 'https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly',
  'NIGHTLY picker must identify the correct public browser-source repository');
assert.equal(preview.pages, '', 'NIGHTLY picker must not advertise an unapproved hosting target');
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

 // A history record may describe a build, but may not introduce any installer picker,
 // download, or artifact URL. Only explicitly reviewed source-evidence fields exist.
 const shape=(value,allowed,at)=>{
   assert.ok(value && typeof value==='object' && !Array.isArray(value),at+': object required');
   const names=new Set(allowed.split(' '));
   for(const key of Object.keys(value)) assert.ok(names.has(key),at+': forbidden field '+key);
 };
 const officialRepo='https://github.com/gobbajogger/X0X37-for-Loopy-Pro';
 // Navigational history may link to a moving PR/branch; actual evidence of
 // test, build or physical PASS must identify immutable Git source bytes.
 const navigationURL=(value,at)=>{
   assert.equal(typeof value,'string',at+': source URL must be a string');
   assert.ok(value.startsWith(officialRepo+'/'),at+': untrusted source');
   const tail=value.slice(officialRepo.length+1);
   const valid=(
     /^pull\/[0-9]+$/.test(tail) ||
     /^commit\/[a-f0-9]{40}$/.test(tail) ||
     /^tree\/[A-Za-z0-9._/-]+$/.test(tail) ||
     /^blob\/[A-Za-z0-9._/-]+\.(?:md|json)$/.test(tail)
   );
   assert.ok(valid,at+': unapproved source navigation route');
   assert.ok(!tail.split('/').includes('..') && !tail.includes('%') &&
      !tail.includes('?') && !tail.includes('#') && !tail.includes('\\'),at+': unsafe source path');
 };
 const evidenceURL=(value,at)=>{
   navigationURL(value,at);
   const tail=value.slice(officialRepo.length+1);
   assert.ok(
     /^commit\/[a-f0-9]{40}$/.test(tail) ||
     /^tree\/[a-f0-9]{40}(?:\/[A-Za-z0-9._/-]+)?$/.test(tail) ||
     /^blob\/[a-f0-9]{40}\/[A-Za-z0-9._/-]+\.(?:md|json)$/.test(tail),
     at+': immutable Git commit SHA is required for evidence');
 };

 // Read-only status cannot carry download/picker/Release links, even when an
 // ostensibly scalar status/diagnostic field is replaced with nested JSON.
 const evidenceKeys=new Set(['url','evidence_url','handoff_source','validation_record','report_url']);
 const repoKeys=new Set(['repo','source_repo']);
 const checkNestedLinks=(value,at='catalog')=>{
   if(Array.isArray(value)){value.forEach((entry,i)=>checkNestedLinks(entry,at+'['+i+']'));return;}
   if(!value||typeof value!=='object')return;
   for(const [key,child] of Object.entries(value)){
     const path=at+'.'+key;
     if(/(?:url|uri|href|link)$/i.test(key) ||
        /^(?:download|downloads|pkg|picker|assets?|release_assets|firmware_file|binary_file)$/i.test(key)){
       assert.ok(evidenceKeys.has(key),path+': forbidden artifact/download/picker link field');
       if(child!=null)(key==='url'?navigationURL:evidenceURL)(child,path);
       else assert.ok(['url','evidence_url','handoff_source'].includes(key),path+': missing evidence');
     }
     if(typeof child==='string'&&/https?:\/\//i.test(child)){
       assert.ok(evidenceKeys.has(key)||repoKeys.has(key),path+': unapproved URL value');
       if(repoKeys.has(key))assert.equal(child,officialRepo,path+': unapproved source repo');
       else (key==='url'?navigationURL:evidenceURL)(child,path);
     }
     checkNestedLinks(child,path);
   }
 };
 checkNestedLinks(catalog);
 assert.equal(typeof catalog.policy.nightly,'string','nightly policy must be text');
 assert.equal(typeof catalog.policy.public,'string','public policy must be text');

 shape(catalog,'schema kind installable source builds history policy counts versioning','manifest');
 shape(catalog.source,'repo ref commit versions_blob_sha','source');
 assert.equal(catalog.source.repo,officialRepo,'source repo');
 assert.equal(catalog.source.ref,'gh-pages','source ref');
assert.match(catalog.source.commit,/^[a-f0-9]{40}$/,'source commit requires pinned SHA');
 shape(catalog.policy,'nightly public public_device_proven_versions public_latest','policy');
 shape(catalog.counts,'actual_build_generations source_or_historical_checkpoints','counts');
 shape(catalog.versioning,'firmware_product_version build_id channel_rules web_preview_package_version web_preview_package_version_is_firmware_version','versioning');
 shape(catalog.versioning.firmware_product_version,'format monotonic historical_assignment prerelease_iteration','firmware version');
 shape(catalog.versioning.build_id,'legacy_format legacy_ids_are_firmware_product_versions model_artifact_identity_requires','build-id');
 shape(catalog.versioning.channel_rules,'nightly production','channel rules');
 assert.equal(catalog.versioning.web_preview_package_version_is_firmware_version,false);
 for(const build of catalog.builds){
   shape(build,'id title built_at source_commit status lifecycle hardware_tested installable_from_nightly_status models','build');
   assert.equal(build.status,'dev-checked','unreviewed or unsupported build status');
   assert.ok(['current','superseded'].includes(build.lifecycle),'unknown build lifecycle');
   for(const model of Object.values(build.models||{})){
     shape(model,'product size sha256 validation_record device_verified physical_evidence','build model');
     evidenceURL(model.validation_record,'model validation record');
     if(model.physical_evidence!==undefined){
       assert.equal(model.device_verified,true,'unverified model cannot attach physical PASS evidence');
       shape(model.physical_evidence,'report_sha256 report_url controller_sha256 owner_publication_approved','hardware evidence');
       evidenceURL(model.physical_evidence.report_url,'hardware report');
     }
   }
 }
 const approvedHistoryFields='id title source_repo source_ref source_commit source_tree source_time date status models firmware_product_version build_id installable device_verified reported_checks description missing_gate url evidence_commit evidence_url handoff_source source_identity_note reported_package_sha256 reported_offline_artifacts offline_source exact_remote_native_build artifact_distribution_approved firmware_subtree producer_reported_artifacts artifact_bytes_independently_verified build_target_tests_independently_reexecuted build_pipeline failed_build';
 for(const item of catalog.history){
   shape(item,approvedHistoryFields,'history');
   if(item.source_repo!==undefined)assert.equal(item.source_repo,officialRepo,'historical source repo');

    // Metadata booleans never grant distribution, exact-source build or full QA.
    for (const flag of ['artifact_distribution_approved','exact_remote_native_build',
      'artifact_bytes_independently_verified','build_target_tests_independently_reexecuted']) {
      if (Object.prototype.hasOwnProperty.call(item,flag))
        assert.equal(typeof item[flag],'boolean','history '+item.id+': '+flag+' must be boolean');
    }
    if(item.artifact_distribution_approved!==undefined)
      assert.equal(item.artifact_distribution_approved,false,'read-only history cannot approve distribution');
    if(item.exact_remote_native_build!==undefined)
      assert.equal(item.exact_remote_native_build,false,'no independent exact-full-root compiler attestation');
    if(item.build_target_tests_independently_reexecuted!==undefined)
      assert.equal(item.build_target_tests_independently_reexecuted,false,'no independent full-root QA receipt');
    if(item.artifact_bytes_independently_verified===true){
      if(item.id==='integration-pr103-model-builds-20261024'){
      // Preserve the one REAL private byte rehash, tied to the prior committed
      // 23/23 audit and exact source/model/controller SHA, never to installability.
      const r4=JSON.parse(read('docs/scheduler-handoffs/catalog-sync-20261024.json')).closed_loop_cycle_20261024_r4;
      const proof=r4?.catalog_private_zip;
      const source=r4?.consumed_firmware?.[0];
      assert.equal(item.id,'integration-pr103-model-builds-20261024','unattributed independent-byte claim');
      assert.equal(item.status,'evidence-only','independent byte audit remains noninstallable evidence');
      assert.equal(source?.sha,item.source_commit,'private audit source commit mismatch');
      assert.equal(source?.root_tree,item.source_tree,'private audit full root mismatch');
      assert.equal(source?.firmware_subtree,item.firmware_subtree,'private audit firmware subtree mismatch');
      assert.equal(proof?.scoped,'CATALOG_PRIVATE_BYTE_REHASH_ONLY__NO_NATIVE_COMPILER_OR_FIRMWARE_CRC_PARSER_QA');
      assert.equal(proof?.exit_code,0,'private-byte audit must exit zero');
      assert.equal(proof?.checks_pass,23,'private-byte audit pass count');
      assert.equal(proof?.total_checks,23,'private-byte audit total count');
      assert.match(proof?.producer_manifest_sha256,/^[a-f0-9]{64}$/,'private-byte audit manifest SHA256');
      assert.equal(proof?.controller_redistribution_cleared,false,'controller redistribution remains unapproved');
      for(const model of ['elite','pro','private_controller']){
        assert.equal(item.producer_reported_artifacts?.[model]?.sha256,proof?.sha256?.[model],
          model+': private-byte audit SHA256 mismatch');
        assert.equal(item.producer_reported_artifacts?.[model]?.bytes,proof?.bytes?.[model],
          model+': private-byte audit byte length mismatch');
      }

      } else if(item.id==='integration-pr103-smc79-native-build-20261024-r13'){
        // Real private bytes, not full-root QA, lawful hosting or device verification.
        const r13=JSON.parse(read('docs/scheduler-handoffs/catalog-sync-20261024.json')).closed_loop_cycle_20261024_r13?.catalog_private_zip;
        assert.equal(r13?.scoped,'CATALOG_PRIVATE_BYTE_REHASH_27_PASS__NO_FULL_ROOT_QA');
        assert.equal(r13?.exit,0);assert.equal(r13?.checks_pass,27);
        assert.equal(r13?.zip_sha256,'2b4ead6559dc003b0394edd99633ce2aa46b838dea9a2635c16261a9fe2913ee');
        assert.equal(r13?.results_sha256,'72aa5b6b115a8121aa4989a6f4f26c05679e7d8b6116a3ef5d99ad4b2d704acb');
        assert.equal(r13?.compiled_source,'56c5d971b6d1c1864cd0f6153f4399950c223c2d');
        assert.equal(r13?.current_source,item.source_commit);
        assert.equal(r13?.current_root,item.source_tree);
        assert.equal(r13?.firmware_subtree,item.firmware_subtree);
        assert.equal(r13?.receipt_blob_sha,'f4f865e770ace19af2d484ccc4a5b56699140ae4');
        assert.equal(r13?.receipt_blob_sha,item.build_pipeline?.evidence_manifest_blob_sha);
        assert.equal(r13?.full_root_QA,false);
        assert.equal(r13?.controller_redistribution_cleared,false);
        for(const model of ['elite','pro','private_controller']){
          assert.equal(item.producer_reported_artifacts?.[model]?.sha256,r13?.artifacts?.[model]?.sha256,model+' hash');
          assert.equal(item.producer_reported_artifacts?.[model]?.bytes,r13?.artifacts?.[model]?.bytes,model+' size');
        }
        assert.equal(item.artifact_distribution_approved,false);
        assert.equal(item.build_target_tests_independently_reexecuted,false);
      } else assert.ok(false,'unattributed private-byte audit');
    }

   for(const key of ['url','evidence_url','handoff_source'])if(item[key]!=null)(key==='url'?navigationURL:evidenceURL)(item[key],'history.'+key);
   assert.equal(item.models.length,new Set(item.models).size,'duplicate historical model');
   if(item.reported_checks!==undefined)shape(item.reported_checks,'origin command result','reported_checks');
   if(item.reported_package_sha256!==undefined){
     shape(item.reported_package_sha256,'elite pro','reported hashes');
     for(const [model,sha] of Object.entries(item.reported_package_sha256))
       assert.match(sha,/^[a-f0-9]{64}$/,model+': reported historical package SHA256');
   }
   if(item.offline_source!==undefined){
     shape(item.offline_source,'synthetic_commit synthetic_tree firmware_nightly_tree identical_to_firmware_nightly','offline projection');
     assert.equal(item.offline_source.identical_to_firmware_nightly,false,'unverified source equality');
     for(const key of ['synthetic_commit','synthetic_tree','firmware_nightly_tree'])
       assert.match(item.offline_source[key],/^[a-f0-9]{40}$/,'offline '+key);
   }
   if(item.reported_offline_artifacts!==undefined){
     shape(item.reported_offline_artifacts,'elite pro controller','offline artifacts');
     for(const [model,artifact] of Object.entries(item.reported_offline_artifacts)){
       shape(artifact,model==='controller'?'size sha256 crc32':'size sha256','offline artifact');
       assert.ok(Number.isInteger(artifact.size)&&artifact.size>0,'offline artifact size must be positive integer');
       assert.match(artifact.sha256,/^[a-f0-9]{64}$/,'offline artifact SHA256');
       if(model==='controller')assert.match(artifact.crc32,/^[a-f0-9]{8}$/,'offline controller CRC32');
     }
   }
   if(item.producer_reported_artifacts!==undefined){
     shape(item.producer_reported_artifacts,'elite pro private_controller','reported artifacts');
     for(const [model,artifact] of Object.entries(item.producer_reported_artifacts)){
       shape(artifact,model==='private_controller'?'bytes sha256 crc32 redistribution_permitted':'product bytes sha256','reported artifact');
       assert.ok(Number.isInteger(artifact.bytes)&&artifact.bytes>0,'producer artifact byte count');
       assert.match(artifact.sha256,/^[a-f0-9]{64}$/,'producer artifact SHA256');
       if(model==='private_controller'){
         assert.match(artifact.crc32,/^[a-f0-9]{8}$/,'private controller CRC32');
         assert.equal(artifact.redistribution_permitted,false,'controller redistribution not cleared');
       } else {
         assert.equal(artifact.product,model==='elite'?'SMK-37 Elite_900':'SMK-37 Pro_900','producer model/product mismatch');
       }
     }
   }
   if(item.build_pipeline!==undefined){
     assert.equal(item.status,'build-succeeded-unreviewed','non-build status cannot claim QA, live HTTP or firmware build pipeline');
     shape(item.build_pipeline,'build qa installability live_http hardware source_scope builder_selected_host_checks_pass followup_native_checks_pass followup_native_checks_fail evidence_commit evidence_manifest_blob_sha package_bytes_independently_received_and_rehashed','reported build pipeline');
     for(const failure of item.build_pipeline.followup_native_checks_fail)shape(failure,'name exit_code','reported host check');
   }
   if(item.failed_build!==undefined){
     assert.equal(item.status,'failed','typed compiler-failure receipt may not be relabeled');
     shape(item.failed_build,'scope firmware_subtree pool_used_bytes pool_capacity_bytes pool_free_bytes pool_minimum_bytes elite pro controller','failed build');
     assert.ok(item.failed_build.pool_free_bytes<item.failed_build.pool_minimum_bytes,'unexpected pool eligibility');
     for(const key of ['pool_used_bytes','pool_capacity_bytes','pool_free_bytes','pool_minimum_bytes'])
       assert.ok(Number.isInteger(item.failed_build[key])&&item.failed_build[key]>=0,'invalid pool '+key);
     assert.equal(item.failed_build.pool_capacity_bytes-item.failed_build.pool_used_bytes,item.failed_build.pool_free_bytes,'failed source pool accounting mismatch');
     assert.match(item.failed_build.firmware_subtree,/^[a-f0-9]{40}$/,'failed build firmware subtree');
     for(const model of ['elite','pro']){
       shape(item.failed_build[model],'exit_code artifact_created log_sha256','failed '+model);
       assert.equal(item.failed_build[model].artifact_created,false,'failed build cannot claim package');
       assert.ok(Number.isInteger(item.failed_build[model].exit_code)&&item.failed_build[model].exit_code!==0,'failed model exit must be a nonzero integer');
       assert.match(item.failed_build[model].log_sha256,/^[a-f0-9]{64}$/,'failed model log SHA256');
     }
     shape(item.failed_build.controller,'exit_code size_bytes sha256 crc32 redistribution_allowed','failed controller');
     assert.equal(item.failed_build.controller.redistribution_allowed,false);
     assert.ok(Number.isInteger(item.failed_build.controller.exit_code),'failed-source controller exit must be integer');
     assert.ok(Number.isInteger(item.failed_build.controller.size_bytes)&&item.failed_build.controller.size_bytes>0,'failed-source controller size');
     assert.match(item.failed_build.controller.sha256,/^[a-f0-9]{64}$/,'failed-source controller SHA256');
     assert.match(item.failed_build.controller.crc32,/^[a-f0-9]{8}$/,'failed-source controller CRC32');
   }
   // Bind the original real PR93 native failed-build receipt to immutable Git
   // source and compiler logs; older 'failed' non-build evidence remains legal.
   if(item.id==='producer-pr93-20261023'){
     assert.equal(item.status,'failed','PR93 compiler failure cannot be hidden by relabeling');
     assert.ok(item.failed_build,'PR93 historical compiler-failure receipt is required');
     assert.equal(item.source_commit,'74389d4bb4abc2f8a36b91bc16d1822b63264986','PR93 failed source commit');
     assert.equal(item.source_tree,'aeacf2c81bd19a33dd50ba0bf3a6e72b1131271a','PR93 failed full root tree');
     assert.equal(item.failed_build.firmware_subtree,'3fe98ec515b3abafd50a62280632839c2910dd80','PR93 failed firmware subtree');
     assert.equal(item.failed_build.elite.exit_code,1,'PR93 Elite compiler failed');
     assert.equal(item.failed_build.pro.exit_code,1,'PR93 Pro compiler failed');
     assert.equal(item.failed_build.elite.log_sha256,'9bdf740b6aeac19d6cb9264fb5143fee986296b49913fca9165eb81b341213db','PR93 Elite failure log');
     assert.equal(item.failed_build.pro.log_sha256,'0bfc208e37a7d72390ac6b740a94524f53ba4eb1f13ffec702b7dbc0ea17d17b','PR93 Pro failure log');
     assert.equal(item.failed_build.controller.exit_code,0,'PR93 private controller succeeded separately');
   }
 }

const seen = new Set();
const allowedBuildStatuses = new Set(['dev-checked']);
for (const build of catalog.builds) {
  assert.ok(typeof build.id === 'string' && build.id.trim().length > 0, 'blank build id');
  assert.equal(seen.has(build.id), false, 'duplicate build ' + build.id);
  seen.add(build.id);
  assert.ok(allowedBuildStatuses.has(build.status), 'unsupported build status ' + build.status);
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
    assert.equal(data.product,model==='elite'?'SMK-37 Elite_900':'SMK-37 Pro_900','build model/product mismatch');
    assert.match(data.sha256, /^[a-f0-9]{64}$/);
    assert.ok(Number.isInteger(data.size) && data.size > 0);
    assert.ok(typeof data.device_verified === 'boolean', 'device verified must be explicit');
    if (data.device_verified) {
      assert.equal(build.hardware_tested, true, 'device PASS cannot contradict version status');
      assert.equal(build.status, 'dev-checked', 'device PASS cannot upgrade a failed/unbuilt version');
      assert.ok(data.physical_evidence && /^[a-f0-9]{64}$/.test(data.physical_evidence.report_sha256), 'missing attributable hardware report digest');
      assert.ok(/^https:\/\//.test(data.physical_evidence.report_url), 'missing physical report link');
      assert.ok(/^[a-f0-9]{64}$/.test(data.physical_evidence.controller_sha256), 'missing controller digest');
      assert.ok(data.physical_evidence.owner_publication_approved === undefined || typeof data.physical_evidence.owner_publication_approved === 'boolean', 'recorded owner approval must be boolean if supplied');
    }
  }
  if (publicProven.has(build.id)) {
    for(const [model,data] of Object.entries(build.models)){
      assert.equal(data.device_verified,true,model+': public build needs same-model hardware proof');
      assert.equal(data.physical_evidence?.owner_publication_approved,true,model+': public model needs separate production approval');
    }
  }
}
for (const id of publicProven) assert.ok(seen.has(id), 'unknown public build ' + id);
const ids = new Set();
const historicalStatuses = new Set([
  'source-only', 'not-hosted', 'evidence-only', 'failed', 'reference',
  'validation-evidence-only', 'offline-projection-artifact-unattributed', 'build-succeeded-unreviewed'
]);
for (const item of catalog.history) {
  assert.ok(typeof item.id === 'string' && item.id.trim().length > 0, 'blank historical id');
  assert.equal(seen.has(item.id), false, 'duplicate build/history id ' + item.id);
  assert.equal(ids.has(item.id), false, 'duplicate historical item ' + item.id);
  ids.add(item.id);
  assert.ok(historicalStatuses.has(item.status), 'unverified or invalid history status ' + item.status);
  assert.match(item.source_commit, /^[a-f0-9]{40}$/, 'history requires immutable source SHA');
  if (item.source_tree !== undefined) assert.match(item.source_tree, /^[a-f0-9]{40}$/);
  if (item.firmware_subtree !== undefined) assert.match(item.firmware_subtree,/^[a-f0-9]{40}$/,'firmware subtree must be exact Git tree SHA');
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
    if(item.id==='integration-pr103-smc79-native-build-20261024-r13') assert.deepEqual(x.followup_native_checks_fail.map(f=>f.name),["mixed-historical-seed:ble-midi-receive","mixed-historical-seed:smc-reachability","mixed-historical-seed:smc-mapping","mixed-historical-seed:smc-drum-mutes","mixed-historical-seed:smc-clip-visibility"]);
    for (const failure of x.followup_native_checks_fail) {
      assert.ok(typeof failure.name === 'string' && failure.name.length > 0);
      if(item.id==='integration-pr103-smc79-native-build-20261024-r13') assert.equal(failure.exit_code,null,'older mixed-test exits unrecorded');
      else assert.ok(Number.isInteger(failure.exit_code) && failure.exit_code !== 0);
    }
    assert.equal(x.package_bytes_independently_received_and_rehashed,item.artifact_bytes_independently_verified===true,'private bytes proof status');
    if(item.id!=='integration-pr103-smc79-native-build-20261024-r13') assert.equal(item.artifact_bytes_independently_verified,false,'old native build lacked independent byte readback');
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
