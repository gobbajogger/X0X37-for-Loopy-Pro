import test from 'node:test';
import assert from 'node:assert/strict';
import {createOutcomeReporter,attachInstallerEvents} from '../installer/outcome-bridge.mjs';

const identity = {
  catalogId: 'nightly-79280e76-dev',
  model: 'elite',
  packageSha256: 'a'.repeat(64),
  clock: () => '2026-11-13T17:00:00Z',
  uuid: () => '12345678-abcd-4321-aaaa-abcdef987654',
};
function fixture(optIn=true) {
  const reports = [];
  const reporter = createOutcomeReporter({
    ...identity, consent: () => optIn,
    fetcher: async (_url, options) => {
      reports.push(JSON.parse(options.body));
      assert.equal(options.mode, 'same-origin');
      return {ok: true,status:202};
    },
  });
  return {reports,reporter};
}

test('explicit consent is required before ANY report is transmitted', async () => {
  const {reports,reporter} = fixture(false);
  assert.deepEqual(await reporter.report('transfer-started'),
                   {sent:false,reason:'no-consent'});
  assert.equal(reports.length, 0);
});

test('upload acknowledgement cannot claim restart or boot', async () => {
  const {reports,reporter} = fixture();
  assert.deepEqual(await reporter.report('restart-observed'),
                   {sent:false,reason:'restart-not-connected-to-transfer'});
  assert.deepEqual(await reporter.report('boot-observed', {confirmation:'manual'}),
                   {sent:false,reason:'boot-not-separately-confirmed'});
  await reporter.report('transfer-started');
  await reporter.report('transfer-completed');
  assert.deepEqual(await reporter.report('boot-observed',{confirmation:'manual'}),
                   {sent:false,reason:'boot-not-separately-confirmed'});
  assert.equal(reports.length, 2);
  assert.equal(reports[1].stage, 'transfer-completed');
});

test('restart and boot require distinct verified user callback and still stay untrusted',async()=>{
  const {reports,reporter} = fixture();
  await reporter.report('transfer-started');
  await reporter.report('transfer-completed');
  await reporter.report('restart-observed');
  assert.deepEqual(await reporter.report('boot-observed',{confirmation:'upload-ack'}),
                   {sent:false,reason:'boot-not-separately-confirmed'});
  const boot = await reporter.report('boot-observed',{confirmation:'manual'});
  assert.equal(boot.sent,true);
  assert.equal(boot.confidence,'anonymous-unverified');
  assert.equal(boot.authorizes_hardware_approval,false);
  assert.deepEqual(reports.map(r=>r.stage), [
    'transfer-started','transfer-completed','restart-observed','boot-observed',
  ]);
  for(const report of reports) {
    assert.equal(report.consent,true);
    assert.equal(report.catalog_id,identity.catalogId);
    assert.equal(report.model,'elite');
    assert.equal(report.package_sha256,identity.packageSha256);
    assert.deepEqual(Object.keys(report).sort(), [
      'catalog_id','consent','model','observed_at','package_sha256',
      'run_id','sequence','stage',
    ].sort());
  }
});

test('wrong model, missing package, or external telemetry endpoint rejected', ()=>{
  assert.throws(()=>createOutcomeReporter({...identity,model:'pro100',consent:()=>true}),TypeError);
  assert.throws(()=>createOutcomeReporter({...identity,packageSha256:'',consent:()=>true}),TypeError);
  assert.throws(()=>createOutcomeReporter({...identity,endpoint:'https://x.example.com',consent:()=>true}),TypeError);
});

test('CustomEvent interface detaches without changing uploader behavior', async()=>{
  const {reports,reporter}=fixture();
  const callbacks = new Map();
  const target = {
    addEventListener(event,cb){callbacks.set(event,cb)},
    removeEventListener(event,cb){if(callbacks.get(event)===cb)callbacks.delete(event)},
  };
  const detach = attachInstallerEvents(target,reporter);
  const callback = callbacks.get('dnbox:installer-outcome');
  assert.equal(typeof callback,'function');
  callback({detail:{stage:'transfer-started'}});
  await new Promise(resolve=>setImmediate(resolve));
  assert.equal(reports.length,1);
  detach();
  assert.equal(callbacks.has('dnbox:installer-outcome'),false);
});
