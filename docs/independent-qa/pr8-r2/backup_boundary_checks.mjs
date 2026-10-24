import {readFileSync} from 'node:fs';
import {Script} from 'node:vm';
import assert from 'node:assert/strict';
const root=process.env.PR8_QA_SOURCE||'/workspace/pr8-independent-qa';
const src=readFileSync(root+'/web/x0x_breaks.js','utf8').replace(/^export\s+/gm,'').replace(/^import .*?;\r?\n/gm,'');
const exercise=`
(async()=>{
 let installed=0,closed=0,writes=[];
 const layout={count:1,slots:[{nz:1}]};
 readSlotLayout=async()=>layout;flashMap=()=>[[1,2]];packageSlotMap=()=>[[3,4]];atRisk=()=>[0];
 const device={id:{model:'SMK-37 Pro'},link:{close(){closed++}}};
 const up={find:async()=>device,install:async()=>{installed++;return 'synthetic'}};
 backupSlots=async()=>{throw Error('synthetic backup read failure')};
 try{await installKeepingSlots(up,new Uint8Array(1),'SMK-37 Pro_900','smk',{sameKeyboard:()=>true});throw Error('accepted failed backup')}catch(e){assert.equal(e.code,'slotbackup')}
 assert.equal(installed,0);assert.equal(closed,1);console.log('PASS actual installKeepingSlots blocks install when required backup fails');
 backupSlots=async()=>({layout,images:[new Uint8Array([1])]});archiveStamp=()=>({created:'synthetic',name:'test.x0xslots'});makeArchive=()=>new Uint8Array([1]);readArchive=()=>{throw Error('synthetic archive corruption')};
 await assert.rejects(installKeepingSlots(up,new Uint8Array(1),'SMK-37 Pro_900','smk',{sameKeyboard:()=>true}),/archive corruption/);
 assert.equal(installed,0);console.log('PASS actual installKeepingSlots blocks install when saved archive verification fails');
 writeSlotImage=async(l,k,img)=>writes.push(k);readSlotImage=async()=>new Uint8Array([4,6]);readSlotLayout=async()=>({slots:[{nz:1},{nz:1}]});
 await assert.rejects(restoreMapped({},[{k:0,j:1}],[new Uint8Array([4,5])],()=>{}),/read back differs/);
 assert.deepEqual(writes,[1]);console.log('PASS actual restoreMapped rejects mismatched readback');
 readSlotImage=async()=>new Uint8Array([4,5]);await restoreMapped({},[{k:0,j:1}],[new Uint8Array([4,5])],()=>{});
 console.log('PASS actual restoreMapped accepts equal readback');
})()`;
await new Script(src+'\n'+exercise).runInNewContext({assert,console,Uint8Array,DataView,Date,TextEncoder,TextDecoder,setTimeout});
