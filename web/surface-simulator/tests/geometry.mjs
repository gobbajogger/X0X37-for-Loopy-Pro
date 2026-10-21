import assert from 'node:assert/strict';
import {renderSmk,renderSmc,surfaceGeometry,fitProjection} from '../surfaces.js';
const close=(a,b,msg)=>assert(Math.abs(a-b)<1e-7,msg??`${a} ≠ ${b}`);
const transforms=s=>[...s.matchAll(/-?\d+(?:\.\d+)?/g)].map(m=>Number(m[0]));
for(const[kind,render]of[['smk',renderSmk],['smc',renderSmc]]){
 const g=surfaceGeometry[kind],svg=render();
 assert.equal(g.width,1000);assert(g.height>600&&g.height<700);
 for(const x of[0,20,143,500,1000])for(const y of[0,27,g.physicalHeight])for(const z of[0,30,63,90]){const p=g.project(x,y,z),q=g.unproject(p.x,p.y,z);close(x,q.x);close(y,q.y);}
 assert(svg.includes(`viewBox="0 0 ${g.width} ${g.height}"`));assert(!svg.includes('skew'));assert(!svg.includes('<image'));
 const ids=[...svg.matchAll(/data-control="([^"]+)"/g)].map(m=>m[1]);assert.equal(ids.length,kind==='smk'?85:61);assert.equal(new Set(ids).size,ids.length);
 const polygons=[...svg.matchAll(/<polygon points="([^"]+)"/g)].map(m=>m[1]);assert(polygons.length>300,'real multi-face solids, not a flat plane');
 for(const points of polygons)for(const pair of points.split(' ')){const[x,y]=pair.split(',').map(Number);assert(Number.isFinite(x)&&Number.isFinite(y));assert(x>=0&&x<=g.width&&y>=0&&y<=g.height,`${kind} bounds${x},${y}`);}
 assert.equal((svg.match(/<g data-pointer=/g)||[]).length,8);assert.equal((svg.match(/<g data-cap=/g)||[]).length,g.faders.length);
 assert((svg.match(/data-solid-side=/g)||[]).length>100,'visible vertical faces');
 assert(!/<rect[^>]*data-face/.test(svg));assert((svg.match(/<polygon[^>]*data-face/g)||[]).length>=32);
 for(let i=0;i<g.knobs.length;i++){const k=g.knobs[i];assert(g.knobTransform(i,215).endsWith(`rotate(215 ${k.x} ${k.y})`));assert(g.knobTransform(i,-10).includes('rotate(-10'));}
 for(let i=0;i<g.faders.length;i++){const f=g.faders[i];for(const value of[0,1,63.5,126,127]){const y=f.y+(1-value/127)*(f.height-f.capHeight),p=g.project(f.x,y+f.capHeight/2,f.z+f.capDepth);close(g.faderValue(i,p.x,p.y),value);const[dx,dy]=transforms(g.faderTransform(i,value)),a=g.project(f.x,f.defaultCapY,f.z),b=g.project(f.x,y,f.z);assert(Math.abs((b.x-a.x)-dx)<.0001);assert(Math.abs((b.y-a.y)-dy)<.0001);}assert.equal(g.faderTransform(i,-2),g.faderTransform(i,0));assert.equal(g.faderTransform(i,300),g.faderTransform(i,127));}
 assert(g.keyTransform(true).startsWith('translate(0 '));
 console.log(`${kind}: ${ids.length} controls; ${polygons.length} bounded projected polygons; inverse projection and full fader travel PASS`);
}
const fitted=fitProjection([[0,0,0],[10,0,0],[0,10,0],[10,10,10]],200,10);assert.equal(fitted.width,200);assert(fitted.height>20);
assert(renderSmk().includes('<g id="smk-screen" data-display="smk" transform="matrix('));
console.log('PASS: original axonometric solids, fitProjection, affine screen, movable knob/fader groups, control counts, and animation math.');
