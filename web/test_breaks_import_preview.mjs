// Source-backed regression: listening metadata must not alter stored PCM/ADPCM or source audio.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {cutLoop, prepareLoopImport, imaEncode, RATE, RATE_HALF} from './x0x_breaks.js';
const base = execFileSync('git',['show','8333eca5d9e5fb5c039edde0b988c5058275c22b:web/x0x_breaks.js'],{encoding:'utf8'});
const prior = await import('data:text/javascript;base64,'+Buffer.from(base).toString('base64'));
let checked=0;
for(const bars of [false,true]) for(const trim of [false,true])
for(const storageRate of [RATE,RATE_HALF]) for(const gritFactor of [1,1.5,2,3,4]) {
  const x=Array.from({length:44100*4},(_,i)=>i<44100/4||i>44100*3.75?0:.18*Math.sin(i*2*Math.PI*220/44100));
  const source=Buffer.from(new Float64Array(x).buffer);
  const old=prior.cutLoop(x,44100,bars,storageRate,trim,gritFactor);
  const now=prepareLoopImport(x,44100,{bars,trim,storageRate,gritFactor});
  assert.equal(now.rate,old.rate);
  assert.deepEqual(now.s,old.s);
  assert.deepEqual(now.adpcm,imaEncode(old.s));
  assert.equal(now.n,old.s.length);
  assert.equal(now.referenceRate,44100);
  assert.ok(Math.abs(now.reference.length/44100-now.n/now.rate)<.001);
  assert.equal(Buffer.compare(source,Buffer.from(new Float64Array(x).buffer)),0);
  checked++;
}
for(const gritFactor of [0,9,NaN]) assert.throws(()=>prepareLoopImport([.1,.2],44100,{gritFactor}));
assert.throws(()=>prepareLoopImport([.1,NaN],44100));
assert.throws(()=>prepareLoopImport([.1,.2],0));
console.log(`PASS ${checked} stored PCM/ADPCM/rate/length retention cases and invalid-input guards`);
