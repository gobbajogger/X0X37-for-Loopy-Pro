import assert from 'node:assert/strict';
import {layout,nativeFrame,colorRuns,qualify909,qualify909Family,observedMessage} from './site/firmware-lesson/screen.mjs';
import {readFileSync} from 'node:fs';
const checks=[];
for(const semantic of [false,true]){
 const cells=new Uint8Array(240*240),regions=layout(semantic);
 for(const r of regions){assert(r.x>=0&&r.y>=0&&r.x+r.w<=240&&r.y+r.h<=240);for(let y=r.y;y<r.y+r.h;y++)for(let x=r.x;x<r.x+r.w;x++)cells[y*240+x]++;}
 assert(cells.every(n=>n===1));checks.push({name:`${semantic?'Semantic':'Fallback'} regions tile every native pixel exactly once`,result:'PASS',regions:regions.length});
}
assert.throws(()=>nativeFrame({width:300,height:150}),/Unqualified panel geometry/);checks.push({name:'Wrong native panel shape rejected before readback',result:'PASS'});
const rgba=new Uint8ClampedArray(240*240*4);for(let i=3;i<rgba.length;i+=4)rgba[i]=255;
assert.match(colorRuns(rgba,0,0,240),/^linear-gradient\(to right,rgb\(0 0 0\) 0px 240px\)$/);checks.push({name:'Uniform native colour has exact hard boundaries',result:'PASS'});
const refs=JSON.parse(readFileSync(new URL('./site/firmware-lesson/references.json',import.meta.url)));
const native=Uint8ClampedArray.from(Buffer.from(refs.signatures[0],'base64'));
assert(qualify909(native,[native]));
for(const [name,rect] of [['mode',[0,0,130,20]],['pattern/page',[130,0,60,20]],['row/overlay',[0,20,27,138]],['REC indicator',[228,0,12,20]]]){
 const changed=native.slice(),[x,y,w,h]=rect;for(let yy=y;yy<y+h;yy++)for(let xx=x;xx<x+w;xx++)changed[(yy*240+xx)*4]^=255;
 assert(!qualify909(changed,[native]));checks.push({name:`Unmatched ${name} cannot inherit909semantics`,result:'PASS'});
}
const decode=s=>Uint8ClampedArray.from(Buffer.from(s,'base64'));
const signatures=refs.signatures.map(decode),lanes=refs.rowLanes.map(decode);
for(const row of ROW_NAMES()){
 const frame=Uint8ClampedArray.from(readFileSync(new URL(`./evidence/row-family/${row}.rgba`,import.meta.url)));
 assert(qualify909Family(frame,signatures,lanes));
}
checks.push({name:'All11actual drum-row captures enable safe BD return in recognized909context',result:'PASS'});
const p5=Uint8ClampedArray.from(readFileSync(new URL('./review/workflow/pattern-detent-5-rgba.bin',import.meta.url)));
assert(!qualify909Family(p5,signatures,lanes));checks.push({name:'Actual Pattern5 cannot inherit Pattern1 return-helper semantics',result:'PASS'});
const messages=refs.saveMessages.map(m=>({...m,header:decode(m.header)}));
for(const [name,file] of [['SAVED','save-after-stop-450ms-rgba.bin'],['SAVES WHEN STOPPED','save-while-running-350ms-rgba.bin']]){
 const frame=Uint8ClampedArray.from(readFileSync(new URL('./review/workflow/'+file,import.meta.url)));
 assert.equal(observedMessage(frame,messages),name);checks.push({name:`Actual captured ${name} gets exact text equivalent`,result:'PASS'});
}
assert.equal(observedMessage(native,messages),null);checks.push({name:'An ordinary header is never called a saved/waiting reply',result:'PASS'});
function ROW_NAMES(){return ['BD','SD','LT','MT','HT','RS','CP','CH','OH','CR','RD'];}
console.log(JSON.stringify({result:'PASS',checks},null,2));
