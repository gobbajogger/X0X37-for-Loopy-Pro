// SPDX-License-Identifier: GPL-3.0-only
// Revision-13 ABI from x0x.h, engine.h, pattern.h and bass_identity.h at
// bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2. Opaque bytes remain in the pinned file.
import {parseProject} from '../x0x_projects.js';
import {crc32} from '../x0x_breaks.js';
import {createDraft,validateDraft,copy} from './model.js';

const HEADER=32,SETTINGS=40,SOUND=2712,PATTERN=656,PATTERNS=HEADER+SETTINGS+SOUND;
const fail=(message,code='project')=>Object.assign(new Error(message),{code});
export function requireProjectModel(model) {
  if(!['SMK-37 Elite','SMK-37 Pro'].includes(model))
    throw fail('Choose the verified SMK-37 Elite or Pro package model, not a USB port name.','model');
  return model;
}
function projectBytes(input) {
  if(!(input instanceof Uint8Array)&&!(input instanceof ArrayBuffer))throw fail('Choose a raw revision-13 project file.');
  const raw=new Uint8Array(input instanceof ArrayBuffer?input.slice(0):input);
  if(raw.length!==36452)throw fail('Only the exact revision-13 project layout is supported.','version');
  const view=new DataView(raw.buffer);
  if(view.getUint32(20,true)!==13||view.getUint32(24,true)!==13)
    throw fail('Only revision-13 projects are supported; no legacy migration is performed.','version');
  parseProject(raw,36452);
  if(view.getUint32(32,true)!==0x50305830||view.getUint32(36,true)!==1||raw[50]!==2||raw[51]!==13)
    throw fail('Project settings do not match the revision-13 layout.','version');
  return raw;
}
function enginesOf(raw) {
  const view=new DataView(raw.buffer),engines=[];
  for(let voice=0;voice<2;voice++) {
    const cell=HEADER+SETTINGS+2*12*32+voice*32;
    const tag=HEADER+SETTINGS+7*12*32+voice*12;
    const id=view.getUint32(tag,true),kind=id===0x4f434831?0:id===0x4f4d4931?1:-1;
    const count=kind===0?22:18,factories=kind===0?55:30,mask=view.getUint32(cell+22,true);
    if(kind<0||view.getUint16(tag+4,true)!==2||raw[tag+6]!==count||raw[tag+7]!==1||
       raw[tag+11]||raw[tag+10]>1||view.getUint16(tag+8,true)>=factories||
       raw[cell+26]!==79||raw[cell+27]!==83||raw[cell+28]!==2||raw[cell+29]!==kind||
       raw[cell+30]>=factories||raw[cell+31]||mask>>>count||
       view.getUint16(tag+8,true)!==raw[cell+30])
      throw fail('Unsupported or conflicting original-engine identity/schema in project.','identity');
    for(let i=0;i<22;i++) {
      const maximum=i>=count?0:kind===0&&i===17?6:127;
      if(raw[cell+i]>maximum)throw fail('Invalid original-engine control in project.','identity');
    }
    engines.push(kind===0?'Chicago':'Miami');
  }
  if(engines.filter(x=>x==='Miami').length>1)throw fail('The project requires two Miami contexts.','identity');
  return engines;
}
function readDraft(raw) {
  const d=createDraft(),view=new DataView(raw.buffer);d.engines=enginesOf(raw);
  for(let p=0;p<32;p++) {
    const at=PATTERNS+p*PATTERN,target=d.patterns[p];
    for(let kit=0;kit<2;kit++) {
      const o=at+kit*100,k=target.drum[kit];
      for(let voice=0;voice<11;voice++)k.hits[voice]=[view.getUint32(o+voice*8,true),view.getUint32(o+voice*8+4,true)];
      k.accent=[view.getUint32(o+88,true),view.getUint32(o+92,true)];
      k.length=raw[o+96];k.rate=raw[o+97];
    }
    for(let voice=0;voice<2;voice++) {
      const o=at+200+voice*208,b=target.bass[voice],g=b.generator;
      for(let s=0;s<64;s++)b.steps[s]={note:raw[o+s*3],flags:raw[o+s*3+1],velocity:raw[o+s*3+2]};
      b.length=raw[o+192];b.rate=raw[o+193];b.direction=raw[o+194];b.transpose=raw[o+195]-24;
      [g.density,g.accent,g.slide,g.octaves,g.root,g.scale,g.baseOctave,g.autoBars]=raw.slice(o+196,o+204);
      g.seed=view.getUint32(o+204,true);g.rng=0;
    }
    const o=at+616,b=target.breaks,gates=view.getUint32(o,true);
    // Current local draft describes eight A slices. Packed A/B count modes and
    // higher stored intents must never be projected/clamped into that schema.
    if(gates>65535||raw[o+18]!==0||raw.slice(o+20,o+36).some(x=>x>8))
      throw fail('This BREAK slice/count intent cannot be represented by the current local draft.','representation');
    b.gates=gates;b.settings=Array.from(raw.slice(o+4,o+16));
    b.loopA=raw[o+16];b.loopB=raw[o+17];b.sliceMode=raw[o+18];
    b.slices=Array.from(raw.slice(o+20,o+36));
  }
  return validateDraft(d);
}

export class ProjectDraft {
  #raw;
  #baseline;
  #model;
  constructor(bytes,{model}={}) {
    this.#model=requireProjectModel(model);this.#raw=projectBytes(bytes);
    this.#baseline=readDraft(this.#raw);
  }
  get model() {return this.#model;}
  get draft() {return copy(this.#baseline);}
  exportDraft(doc) {
    validateDraft(doc);
    if(JSON.stringify(doc.engines)!==JSON.stringify(this.#baseline.engines))
      throw fail('Engine assignment changes need the original sound-cell adapter; project unchanged.','representation');
    if(Object.keys(doc.sound).length)
      throw fail('Local sound controls are not mapped to this original project representation.','representation');
    const out=new Uint8Array(this.#raw),view=new DataView(out.buffer);
    for(let p=0;p<32;p++) {
      const at=PATTERNS+p*PATTERN,target=doc.patterns[p];
      for(let kit=0;kit<2;kit++) {
        const o=at+kit*100,k=target.drum[kit];
        for(let voice=0;voice<11;voice++)for(let word=0;word<2;word++)view.setUint32(o+voice*8+word*4,k.hits[voice][word],true);
        view.setUint32(o+88,k.accent[0],true);view.setUint32(o+92,k.accent[1],true);
        out[o+96]=k.length;out[o+97]=k.rate;
      }
      for(let voice=0;voice<2;voice++) {
        const o=at+200+voice*208,b=target.bass[voice],g=b.generator;
        if(g.rng!==this.#baseline.patterns[p].bass[voice].generator.rng)
          throw fail('Generator RNG is a local runtime field and cannot be exported to a project.','representation');
        for(let s=0;s<64;s++)out.set([b.steps[s].note,b.steps[s].flags,b.steps[s].velocity],o+s*3);
        out.set([b.length,b.rate,b.direction,b.transpose+24],o+192);
        out.set([g.density,g.accent,g.slide,g.octaves,g.root,g.scale,g.baseOctave,g.autoBars],o+196);
        view.setUint32(o+204,g.seed,true);
      }
      const o=at+616,b=target.breaks;
      if(!Number.isInteger(b.loopA)||b.loopA<0||b.loopA>255||!Number.isInteger(b.loopB)||b.loopB<0||b.loopB>255)
        throw fail('Project loop slot indices must be explicit bytes; no slot remapping is performed.','representation');
      if(b.sliceMode!==0)throw fail('This local BREAK mode is not a revision-13 count-mode conversion.','representation');
      view.setUint32(o,b.gates,true);out.set(b.settings,o+4);out[o+16]=b.loopA;out[o+17]=b.loopB;
      out.set(b.slices,o+20);
    }
    view.setUint32(12,crc32(out.subarray(HEADER)),true);parseProject(out,36452);
    return out;
  }
}
export const openProject=(bytes,options)=>new ProjectDraft(bytes,options);
