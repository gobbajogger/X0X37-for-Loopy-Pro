// SPDX-License-Identifier: GPL-3.0-only
// Geometry from firmware79280e76 ui.c/gfx.c. Pixels always come from the actual emulator.
export const WIDTH=240,HEIGHT=240;
export const ROWS=['BD','SD','LT','MT','HT','RS','CP','CH','OH','CR','RD','AC'];
export function nativeFrame(canvas){
 if(canvas.width!==1920||canvas.height!==880)throw Error('Unqualified panel geometry');
 const src=canvas.getContext('2d').getImageData(262,44,480,480).data;
 const rgba=new Uint8ClampedArray(240*240*4);
 for(let y=0;y<240;y++)for(let x=0;x<240;x++){
  const p=(y*2*480+x*2)*4,q=(y*240+x)*4;
  for(let k=0;k<4;k++){
   const v=src[p+k];
   if(src[p+4+k]!==v||src[p+480*4+k]!==v||src[p+481*4+k]!==v)throw Error('Native LCD pixel scaling changed');
   rgba[q+k]=v;
  }
 }
 return rgba;
}
const region=(id,x,y,w,h,label)=>({id,x,y,w,h,label});
export function layout(qualified){
 if(!qualified)return [region('header',0,0,240,20,'Native header; layout unclassified'),region('main-top',0,20,240,69,'Native upper display'),region('main-bottom',0,89,240,69,'Native lower display'),region('parameters',0,158,240,82,'Native parameter strip')];
 const out=[region('part-mode',0,0,130,20,'909 SEQ header'),region('pattern',130,0,60,20,'Pattern 1, first 16-step page'),region('tempo',190,0,24,20,'Actual firmware tempo'),region('transport',214,0,26,20,'Actual transport and recording indicators')];
 for(let r=0;r<12;r++){
  out.push(region('label-'+r,0,20+r*11,27,11,ROWS[r]+' row'));
  for(let c=0;c<16;c++){
   const x=27+c*13+(c>>2),next=c===15?240:27+(c+1)*13+((c+1)>>2);
   out.push({...region(`cell-${r}-${c}`,x,20+r*11,next-x,11,`${ROWS[r]} step ${c+1}`),row:r,step:c+1});
  }
 }
 out.push(region('grid-footer',0,152,240,6,'Native grid boundary'));
 for(let k=0;k<4;k++)out.push(region('parameter-'+k,k*60,158,60,82,`Actual parameter region ${k+1}`));
 return out;
}
export function colorRuns(rgba,x,y,w){
 let start=0,parts=[],last='';
 const color=i=>{const p=((y*240)+x+i)*4;return `rgb(${rgba[p]} ${rgba[p+1]} ${rgba[p+2]})`;};
 for(let i=0;i<=w;i++){
  const c=i===w?'':color(i);
  if(i&&c!==last){parts.push(`${last} ${start}px ${i}px`);start=i;}
  last=c;
 }
 return `linear-gradient(to right,${parts.join(',')})`;
}
export function equalRect(a,b,x,y,w,h){
 for(let yy=y;yy<y+h;yy++)for(let xx=x;xx<x+w;xx++)for(let k=0;k<4;k++)if(a[(yy*240+xx)*4+k]!==b[(yy*240+xx)*4+k])return false;
 return true;
}
export function qualify909(frame,references){
 // Exact observed signatures: mode+save flag, pattern/page, full row-label lane,
 // and REC-off indicator. A request to select909 is never an acknowledgement.
 return references.some(ref=>equalRect(frame,ref,0,0,130,20)&&equalRect(frame,ref,130,0,60,20)&&equalRect(frame,ref,0,20,27,138)&&equalRect(frame,ref,228,0,12,20));
}
export function qualify909Family(frame,references,rowLanes){
 const context=references.some(ref=>equalRect(frame,ref,0,0,130,20)&&equalRect(frame,ref,130,0,60,20)&&equalRect(frame,ref,228,0,12,20));
 if(!context)return false;
 return rowLanes.some(lane=>{
  for(let y=0;y<138;y++)for(let x=0;x<27;x++)for(let k=0;k<4;k++)if(frame[((20+y)*240+x)*4+k]!==lane[(y*27+x)*4+k])return false;
  return true;
 });
}
export function observedMessage(frame,messages){
 return messages.find(message=>{
  for(let i=0;i<240*20*4;i++)if(frame[i]!==message.header[i])return false;
  return true;
 })?.text||null;
}
export function readBD(frame,off,on){
 const result=[];
 for(let c=0;c<16;c++){
  const x=27+c*13+(c>>2),y=21;
  // Compare the unchanged outer shape. The playhead touches only the inner8x5.
  const match=ref=>equalRect(frame,ref,x,y,12,2)&&equalRect(frame,ref,x,y+7,12,2);
  const isOn=match(on),isOff=match(off);
  result.push(isOn&&!isOff?true:isOff&&!isOn?false:null);
 }
 return result;
}
export class HTMLScreen{
 constructor(root){this.root=root;this.qualified=null;this.regions=[];this.changes=[];this.previous=null;}
 render(rgba,qualified){
  const before=performance.now();this.changes=[];
  if(this.qualified!==qualified){
   this.root.replaceChildren();this.regions=layout(qualified).map(r=>{
    const node=document.createElement('section');node.className='lcd-region';node.dataset.region=r.id;
    if(r.row!==undefined){node.dataset.row=ROWS[r.row];node.dataset.step=String(r.step);}
    node.setAttribute('role','img');node.setAttribute('aria-label',r.label);
    Object.assign(node.style,{left:r.x+'px',top:r.y+'px',width:r.w+'px',height:r.h+'px'});
    const lines=Array.from({length:r.h},()=>{const line=document.createElement('i');line.setAttribute('aria-hidden','true');node.append(line);return line;});
    this.root.append(node);return {...r,node,lines,cache:[]};
   });this.qualified=qualified;this.previous=null;
  }
  for(const r of this.regions){let changed=false;
   for(let y=0;y<r.h;y++){
    if(this.previous&&equalRect(rgba,this.previous,r.x,r.y+y,r.w,1))continue;
    const css=colorRuns(rgba,r.x,r.y+y,r.w);if(r.cache[y]!==css){r.lines[y].style.backgroundImage=css;r.cache[y]=css;changed=true;}
   }
   if(changed)this.changes.push(r.id);
  }
  this.previous=rgba.slice();this.renderMs=performance.now()-before;return this.changes;
 }
}
