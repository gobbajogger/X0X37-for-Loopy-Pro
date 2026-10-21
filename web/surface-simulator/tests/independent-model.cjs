const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');const ctx={};vm.createContext(ctx);vm.runInContext(fs.readFileSync(__dirname+'/../data.js','utf8')+'\n'+fs.readFileSync(__dirname+'/../model.js','utf8')+'\nglobalThis.M=SurfaceModel;',ctx);const M=ctx.M;const results=[];
function test(name,fn){try{fn(new M);results.push({name,pass:true});}catch(e){results.push({name,pass:false,error:e.message});}}
test('SMC PLAY idempotent',m=>{m.nav('play');m.nav('play');assert.equal(m.play,true)});
test('External encoder resets pickup',m=>{m.fader(0,90);assert(m.pick[0].picked);m.encoder(0,1);assert(!m.pick[0].picked);assert.equal(m.pick[0].last,null)});
test('Drum bank selects visible kit',m=>{m.part=4;m.setView('808 MIX');m.nav('bank-right');assert.equal(m.part,1);assert.equal(m.selected[1],8);assert.equal(m.target(3),null)});
test('Held solo release original bank target',m=>{m.setView('909 MIX');m.strip('solo',0);m.nav('bank-right');assert.equal(m.solo[0],'drum:0:0');m.strip('solo',0,false);assert.equal(m.solo[0],null)});
test('FX mute changes value and pickup',m=>{m.fader(5,40);m.strip('mute',5);assert.equal(m.main[5],0);assert(!m.pick[5].picked);m.strip('mute',5);assert.equal(m.main[5],40)});
test('Select FX retains part',m=>{m.part=2;m.strip('select',6);assert.equal(m.part,2)});
test('APC release original note after off and page move',m=>{m.toggleClip();m.loadHost();m.pad(0);let n=m.heldPads.get(0);m.cells[n].velocity=0;m.viewport=1;m.pad(0,false);assert.equal(m.log[0].bytes[1],n);assert.equal(m.log[0].bytes[0],128)});
test('Down from drum MIX goes SONG',m=>{m.setView('909 MIX');m.nav('down');assert.equal(m.view,'SONG')});
test('Drum sound bank changes drum not part',m=>{m.part=0;m.setView('909 SOUND');m.nav('bank-right');assert.equal(m.part,0);assert.equal(m.selected[0],1)});
test('Stage parameter change leaves APC pad held',m=>{m.toggleClip();m.loadHost();m.pad(0);m.clipPage='SCALE';m.editStage(1,1);assert(m.heldPads.has(0))});
console.log(JSON.stringify({checks:results.length,passed:results.filter(r=>r.pass).length,results},null,2));if(results.some(r=>!r.pass))process.exitCode=1;
