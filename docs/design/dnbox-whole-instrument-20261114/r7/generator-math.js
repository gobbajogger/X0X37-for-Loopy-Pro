/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-model port of public DnBOX tb3po.c at3c41, which credits schwung-tb3po.
 * Pattern mathematics only, not firmware/audio execution. */
const GEN_DEGREES=[[0,2,3,5,7,8,10],[0,1,3,5,7,8,10],[0,2,3,5,7,8,11],[0,3,5,7,10],[0,2,3,5,7,9,10],[0,2,4,5,7,9,11]];
function genRng(seed){let r=seed>>>0;return {next(){let x=r||1;x^=x<<13;x^=x>>>17;x^=x<<5;r=x>>>0;return r;},float(){return (this.next()&0xffffff)/0x1000000;},get state(){return r;}};}
function genNewSeed(entropy){const r=genRng((Math.imul(entropy>>>0,2654435761)+0x9e3779b9)>>>0);r.next();return r.next()||1;}
function genPitch(cfg,degree,oct){const scale=GEN_DEGREES[cfg.scale]??GEN_DEGREES[0];return clamp(12*(cfg.base+1)+cfg.root+scale[degree]+12*oct,0,127);}
function genDegree(r,scale){return Math.min(scale.length-1,Math.trunc(Math.fround(r.float()*scale.length)));}
function genOctave(r,range){return Math.min(range-1,Math.trunc(Math.fround(r.float()*range)));}
function genKind(r,cfg){let accent=r.float()<Math.fround(cfg.accent/100),slide=r.float()<Math.fround(cfg.slide/100);if(slide)accent=false;return {gate:'NOTE',accent,slide,end:0};}
function genGenerateData(b){
 const cfg=b.gen,scale=GEN_DEGREES[cfg.scale]??GEN_DEGREES[0],range=clamp(cfg.range,1,3),len=clamp(b.length,1,64),r=genRng(cfg.seed||1);
 b.notes=Array.from({length:64},()=>({gate:'REST',note:genPitch(cfg,0,0),velocity:100,rawVelocity:0,accent:false,slide:false,end:0}));
 for(let i=0;i<len;i++){
  if(r.float()>Math.fround(cfg.density/100))continue;let degree;if(i%4===0&&r.float()<Math.fround(.35))degree=0;else degree=genDegree(r,scale);const oct=genOctave(r,range);
  b.notes[i]={...b.notes[i],note:genPitch(cfg,degree,oct),...genKind(r,cfg)};
 }
 let any=false;for(let i=0;i<len;i++){if(b.notes[i].slide&&b.notes[(i+1)%len].gate==='REST'){b.notes[i].slide=false;b.notes[i].accent=false;}if(b.notes[i].gate!=='REST')any=true;}
 if(!any){b.notes[0].note=genPitch(cfg,0,0);b.notes[0].gate='NOTE';}return r.state;
}
function genMutateData(b,seed){
 const cfg=b.gen,scale=GEN_DEGREES[cfg.scale]??GEN_DEGREES[0],range=clamp(cfg.range,1,3),len=clamp(b.length,1,64),r=genRng(seed||cfg.seed||1),had=b.notes.slice(0,len).some(n=>n.gate!=='REST');
 for(let i=0;i<len;i++){
  if(r.float()>=Math.fround(.25))continue;
  if(r.float()<Math.fround(.5)){if(r.float()>Math.fround(cfg.density/100))Object.assign(b.notes[i],{gate:'REST',accent:false,slide:false,end:0});else Object.assign(b.notes[i],genKind(r,cfg));}
  else{const degree=genDegree(r,scale),oct=genOctave(r,range);b.notes[i].note=genPitch(cfg,degree,oct);}
 }
 let any=false;for(let i=0;i<len;i++){if(b.notes[i].slide&&b.notes[(i+1)%len].gate==='REST')b.notes[i].slide=false;if(b.notes[i].gate!=='REST')any=true;}
 if(had&&!any)Object.assign(b.notes[0],{note:genPitch(cfg,0,0),gate:'NOTE',accent:false,slide:false,end:0,velocity:100,rawVelocity:0});return r.state;
}
