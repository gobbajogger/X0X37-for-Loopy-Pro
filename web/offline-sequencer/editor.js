// SPDX-License-Identifier: GPL-3.0-only
import {DraftStore,createDraft,parseDraft,copy,getPart,PARTS,KITS,RATES,gate,bit,setBit,pitchName,validateDraft,diffDraft} from './model.js';
import {notesOf,putNote,removeNote,applyPlan} from './editing.js';
import {SafeDraftStorage,SelectionEpoch} from './draft-guards.js';
import {partKind,takeFullPart,replaceFullPart,clearFullPart} from './part-operations.js';
import {generatePlan} from './generator.js';
import {selectNotes,takeNotes,groupPlan} from './note-groups.js';
import {snapPitch,snapStep,scaleContains} from './musical-grid.js';
import {createLibraryController} from './song-library.js';
import {mountLibrary} from './library-ui.js';
const $=s=>document.querySelector(s), E=(tag,attrs={},children=[])=>{const x=document.createElement(tag);for(const [k,v] of Object.entries(attrs)){if(k==='text')x.textContent=v;else if(k.startsWith('on'))x.addEventListener(k.slice(2),v);else if(k==='class')x.className=v;else if(v!==false&&v!=null)x.setAttribute(k,v===true?'':v);}for(const c of [].concat(children))x.append(c);return x;};
let store=new DraftStore(),part=0,patterns=[0,0,0,0,0],selected=null,cursor={start:0,pitch:60},tool=matchMedia('(pointer:coarse)').matches?'pan':'select',step=40,row=28,gesture=null,pending=null,parsedSummary=null,worker=null,workerId=0,soundGroup='',soundSection='',catalog=null,uiEpoch=0;
const STORAGE_KEY='dnbox-offline-draft-v2';
const draftStorage=new SafeDraftStorage({
 getItem:key=>localStorage.getItem(key),
 setItem:(key,raw)=>localStorage.setItem(key,raw)
},{parseDraft,validateDraft,locks:navigator.locks||null},STORAGE_KEY);
let draftPick=new SelectionEpoch(),fullPartClipboard=null;
let saveQueue=Promise.resolve(),saveToken=0,failedSaving=false;
const groupSelection=new Set();
let noteClipboard=null,workspaceView='editor';
const grid={root:0,scale:'chromatic',steps:1,snap:false,ghost:false};
let libraryView=null;
const projectOrigins=new WeakMap();
const vp=$('#viewport'),canvas=$('#canvas'),vel=$('#velocity'),vc=$('#velocity-canvas'),dialog=$('#dialog');
const pattern=()=>patterns[part],current=()=>getPart(store.doc,pattern(),part),context=()=>`${part}:${pattern()}:${part<2?store.doc.engines[part]:''}`;
function message(text,error=false){$('#message').textContent=text;$('#message').classList.toggle('error',error);}
function canModify(){
 if(draftStorage.status==='ready'&&!failedSaving)return true;
 message('Saved draft writes are locked. Recover a verified backup, or export unsent work and reload. No existing saved draft was overwritten.',true);
 return false;
}
// Raw project provenance follows document snapshots, including Undo/Redo.
// Unrelated draft replacement deliberately receives no borrowed origin.
function projectEdit(change){
 const origin=projectOrigins.get(store.doc),changed=change();
 if(changed&&origin)projectOrigins.set(store.doc,origin);
 return changed;
}
function act(fn,text='Local draft changed · unsent'){
 if(!canModify())return false;
 try{
  const changed=projectEdit(()=>store.edit(fn));
  if(changed){pending=null;persist();render();message(text+' · saving locally…');}
  return changed;
 }catch(e){message(e.message,true);return false;}
}
function persist(){
 // Capture every revision before enqueueing; never serialize a later mutable draft
 // against an earlier transaction. A failed save stops all subsequent writes.
 const snapshot=copy(store.doc),token=++saveToken;
 $('#draft-state').textContent='Local draft saving · device unavailable';
 const attempt=saveQueue.then(()=>{
  if(failedSaving)throw Error('A previous local save failed; export then reload to resume.');
  return draftStorage.save(snapshot);
 });
 saveQueue=attempt.then(
  ()=>{
   if(token===saveToken){
    $('#draft-state').textContent='Local draft saved · device unavailable';
    // An earlier pending save must not erase a newer MIDI/validation error.
    if($('#message').textContent.includes('saving locally'))message('Local draft saved in browser storage. Nothing sent to the device.');
   }
  },
  error=>{
   failedSaving=true;
   $('#draft-state').textContent='Local edits unsaved · export draft';
   message('Local save blocked: '+error.message+' Export unsent edits before reloading.',true);
  }
 );
 return saveQueue;
}
function button(label,fn,attrs={}){return E('button',{text:label,onclick:fn,...attrs});}
function number(label,value,min,max,change){const input=E('input',{type:'number',min,max,value,'aria-label':label,onchange:()=>change(Number(input.value))});return E('label',{},[E('span',{text:label}),input]);}
function modal(title,content){libraryView?.dispose();libraryView=null;$('#dialog-title').textContent=title;$('#dialog-body').replaceChildren(...[].concat(content));if(!dialog.open)dialog.showModal();}
function closeModal(){uiEpoch++;pending=null;libraryView?.dispose();libraryView=null;dialog.close();}
function download(name,data,type='application/json'){const url=URL.createObjectURL(new Blob([data],{type}));const a=E('a',{href:url,download:name});a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function changePart(t){cancelGesture();closeModal();part=t;if(t>=2)step=Math.max(46,step);selected=null;groupSelection.clear();soundGroup='';soundSection='';render();centerPitch();}
function render(){
 if(!$('#parts').children.length)$('#parts').append(...PARTS.map((n,t)=>button(n,()=>changePart(t),{'data-part':t})));for(const [t,x] of [...$('#parts').children].entries()){x.setAttribute('aria-pressed',t===part);let small=x.querySelector('small');if(!small){small=E('small');x.append(small);}small.textContent=t<2?`${store.doc.engines[t]} · P${String(patterns[t]+1).padStart(2,'0')}`:`P${String(patterns[t]+1).padStart(2,'0')}`;}
 $('#title').textContent=PARTS[part];$('#context-description').textContent=part<2?`Local ${store.doc.engines[part]} · engine binding is a draft, device unknown`:'Separate local pattern and sound context · device unknown';
 $('#pattern').value=pattern()+1;$('#length').value=current().length;$('#length').disabled=part===4;$('#length').max=part===4?16:64;$('#rate').value=current().rate;$('#rate').disabled=part===4;for(const id of ['direction','transpose']){$('#'+id+'-group').hidden=part>=2;$('#'+id).disabled=part>=2;if(part<2)$('#'+id).value=current()[id];}$('#undo').disabled=!store.history.length;$('#redo').disabled=!store.future.length;
 $('#import').disabled=part===4;$('#import').textContent=part===4?'BREAK import · unqualified':'Import MIDI';$('#note-list').hidden=part>1;$('#generator').textContent=part<2?'Generate / mutate':part===4?'BREAK generator settings':'Generator unavailable';
 $('#zoom').textContent=Math.round(step/40*100)+'%';vp.dataset.tool=tool;for(const b of $('#tools').children)b.setAttribute('aria-pressed',b.dataset.tool===tool);$('#lane-title').textContent=part<2?'VELOCITY · drag a NOTE onset bar vertically':part<4?'Hits and total accent · per-hit velocity is an unsupported auxiliary lane':'16 gates · Generator or pinned LoopA slice1–8';
 renderMusic();renderInspector();renderSound();
}
function renderMusic(){
 const visibleNotes=part<2?notesOf(current()):[],onsets=new Set(visibleNotes.map(n=>n.start));
 for(const start of groupSelection)if(!onsets.has(start))groupSelection.delete(start);
 $('#musical-grid').hidden=part>=2;$('#group-actions').disabled=part>=2;
 if(part>=2)$('#touch-keyboard').hidden=true;
 canvas.replaceChildren();vc.replaceChildren();if(part>=2){renderGrid();return;}
 const b=current(),width=48+step*128,height=32+128*row;canvas.style.width=width+'px';canvas.style.height=height+'px';vc.style.width=width+'px';
 const ruler=E('div',{class:'ruler'});for(let s=0;s<128;s+=4){const n=E('span',{text:`${Math.floor(s/16)+1}.${s%16/4+1}`});n.style.left=48+s*step+'px';ruler.append(n);}canvas.append(ruler);
 const surface=E('div',{class:'grid-surface'});Object.assign(surface.style,{width:128*step+'px',height:128*row+'px',backgroundSize:`${step}px ${row}px`});canvas.append(surface);
 for(let n=127;n>=0;n--){const key=E('div',{class:'pitch-key'+([1,3,6,8,10].includes(n%12)?' black':'')+(!scaleContains(n,grid.root,grid.scale)?' out-of-scale':''),text:pitchName(n)});key.style.height=row+'px';key.style.lineHeight=row+'px';canvas.append(key);}
 const inactive=E('div',{class:'inactive'});Object.assign(inactive.style,{left:48+b.length*step+'px',width:(64-b.length)*step+'px',height:128*row+'px'});canvas.append(inactive);const beyond=E('div',{class:'beyond'});Object.assign(beyond.style,{left:48+64*step+'px',width:64*step+'px',height:128*row+'px'});canvas.append(beyond);const boundary=E('span',{class:'bound-label',text:'64-step storage boundary · view only'});boundary.style.left=48+64*step+'px';canvas.append(boundary);
 renderGhost();
 for(const n of visibleNotes){
  const node=button(`${pitchName(n.pitch)} · ${n.velocity}`,()=>{}, {class:'note'+(selected===n.start||groupSelection.has(n.start)?' selected':''),'data-note':n.start,'aria-label':`${pitchName(n.pitch)}, step ${n.start+1}, ${n.slots} slots, velocity ${n.velocity}`,tabindex:'-1'});Object.assign(node.style,{left:48+n.start*step+2+'px',top:32+(127-n.pitch)*row+2+'px',width:Math.max(8,(n.end-n.start)*step-4)+'px',height:row-4+'px'});node.append(E('span',{class:'edge','data-resize':n.start}));canvas.append(node);
  const bar=button('',()=>{},{class:'velocity-bar'+(selected===n.start||groupSelection.has(n.start)?' selected':''),'data-velocity':n.start,'aria-label':`Velocity ${n.velocity}, ${pitchName(n.pitch)}`});Object.assign(bar.style,{left:48+n.start*step+5+'px',height:n.velocity/127*78+'px'});bar.append(E('span',{text:n.velocity}));vc.append(bar);
 }
 if(gesture?.mode==='draw'){const n=gesture.preview||gesture.note;const ghost=E('div',{class:'note selected','data-note':n.start,text:'New '+pitchName(n.pitch)});Object.assign(ghost.style,{left:48+n.start*step+2+'px',top:32+(127-n.pitch)*row+2+'px',width:n.slots*step-4+'px',height:row-4+'px'});canvas.append(ghost);}
 const cur=E('div',{class:'cursor'});Object.assign(cur.style,{left:48+cursor.start*step+'px',top:32+(127-cursor.pitch)*row+'px',width:step+'px',height:row+'px'});canvas.append(cur);vel.scrollLeft=vp.scrollLeft;
}
function renderGrid(){
 const k=current(),isBreak=part===4,voices=isBreak?[['Gate'],['Slice']]:[...KITS[part-2],['Total accent']];canvas.style.width=(108+step*(isBreak?16:128))+'px';canvas.style.height=(48*(voices.length+1))+'px';vc.style.width=canvas.style.width;
 const head=E('div',{class:'grid-head'});head.append(E('div',{class:'voice-label',text:isBreak?'BREAKS ·16':'Step / voice'}));for(let s=0;s<(isBreak?16:64);s++){const x=E('span',{text:s+1});Object.assign(x.style,{position:'absolute',left:108+s*step+'px',top:'12px',fontSize:'11px'});head.append(x);}canvas.append(head);
 voices.forEach(([name],v)=>{const line=E('div',{class:'drum-row'});line.append(button(name,()=>{soundGroup=`${PARTS[part]}/${name}`;renderSound();},{class:'voice-label'}));for(let s=0;s<(isBreak?16:64);s++){
  const on=isBreak?(k.gates>>>s)&1:bit(v===11?k.accent:k.hits[v],s),cell=button(isBreak&&v===1?(k.slices[s]?'A'+k.slices[s]:'Gen'):'',()=>{act(d=>{const q=getPart(d,pattern(),part);if(isBreak){if(v===0)q.gates^=1<<s;else q.slices[s]=(q.slices[s]+1)%9;}else setBit(v===11?q.accent:q.hits[v],s,!on);});},{class:'drum-cell'+(on?' hit':'')+(s>=k.length?' latent':''),'aria-label':`${name}, step ${s+1}, ${on?'on':'off'}`,'data-cell':`${v}:${s}`});Object.assign(cell.style,{left:108+s*step+'px',width:step-2+'px'});line.append(cell);
 }canvas.append(line);});
}
function renderInspector(){
 const host=$('#inspector');host.replaceChildren();if(part<2&&groupSelection.size){host.append(E('strong',{class:'group-count',text:groupSelection.size+' notes selected'}),button('Edit selected group',showGroupActions));return;}if(part>=2){host.append(E('span',{text:part===4?'Tap Gate to toggle; tap Slice to cycle Generator/A1–A8. Loop catalog and device actions unavailable.':'All 11 voices and total accent. Hidden steps survive length changes.',class:'inspector-label'}));return;}
 const n=notesOf(current()).find(n=>n.start===selected);if(!n){host.append(E('span',{text:`Cursor ${pitchName(cursor.pitch)} / step ${cursor.start+1}. Draw or Enter inserts. Select a note to edit length, velocity, accent and outgoing slide.`,class:'inspector-label'}));return;}
 host.append(E('strong',{text:`${pitchName(n.pitch)} · step ${n.start+1}`}));
 if(n.rawVelocity===0)host.append(E('span',{class:'muted',text:'Legacy velocity0 → effective100'}));
 const edit=patch=>{if(act(d=>putNote(d,pattern(),part,{...n,...patch,velocity:patch.velocity??n.rawVelocity},n.start))){selected=patch.start??n.start;cursor={start:selected,pitch:patch.pitch??n.pitch};renderMusic();renderInspector();}};host.append(number('Start step',n.start+1,1,65-n.slots,v=>edit({start:v-1})),number('Pitch',n.pitch,0,127,v=>edit({pitch:v})),number('Length',n.slots,1,64-n.start,v=>edit({slots:v,endpoint:0})),number('Velocity',n.velocity,1,127,v=>edit({velocity:v})),button('Accent '+(n.accent?'on':'off'),()=>edit({accent:!n.accent}),{'aria-pressed':n.accent}),button('Slide '+(n.slide?'on':'off'),()=>edit({slide:!n.slide}),{'aria-pressed':n.slide}),button('Delete',()=>{act(d=>removeNote(d,pattern(),part,n.start));selected=null;groupSelection.clear();}));
}
function centerPitch(){if(part<2)vp.scrollTop=32+(127-cursor.pitch)*row-vp.clientHeight/2;else vp.scrollTop=0;vp.scrollLeft=0;}
function point(e){const r=vp.getBoundingClientRect();return {start:Math.floor((e.clientX-r.left+vp.scrollLeft-48)/step),pitch:127-Math.floor((e.clientY-r.top+vp.scrollTop-32)/row)};}
function cancelGesture(clearPointers=true){gesture=null;velocityGesture=null;pinch=null;if(clearPointers)pointers.clear();renderMusic();renderInspector();}
const pointers=new Map();let pinch=null;
vp.addEventListener('pointerdown',e=>{
 pointers.set(e.pointerId,{x:e.clientX,y:e.clientY});if(pointers.size===2){cancelGesture(false);const [a,b]=[...pointers.values()];pinch={distance:Math.hypot(a.x-b.x,a.y-b.y),step};return;}if(part>=2||tool==='pan')return;
 const pos=point(e),on=e.target.closest('[data-note]');if(pos.start<0||pos.start>=64||pos.pitch<0||pos.pitch>127)return;vp.focus({preventScroll:true});cursor=pos;
 if(tool==='group'){
  if(on){const start=Number(on.dataset.note);if(groupSelection.has(start))groupSelection.delete(start);else groupSelection.add(start);selected=null;renderMusic();renderInspector();message(groupSelection.size+' notes selected. Open Note groups for atomic actions.');return;}
  gesture={id:e.pointerId,mode:'marquee',anchor:pos,preview:pos,revision:store.revision,context:context()};
  e.preventDefault();vp.setPointerCapture(e.pointerId);return;
 }
 if(on){groupSelection.clear();const start=Number(on.dataset.note),n=notesOf(current()).find(n=>n.start===start);if(e.pointerType==='touch'&&selected!==start){const near=notesOf(current()).filter(x=>Math.abs((127-x.pitch)*row-(127-pos.pitch)*row)<44&&pos.start>=x.start&&pos.start<x.start+x.slots);if(near.length>1){showNotes(near);return;}}selected=start;gesture={id:e.pointerId,mode:e.target.closest('[data-resize]')?'resize':'move',note:n,startX:e.clientX,startY:e.clientY,revision:store.revision,context:context(),dx:0,dy:0,left:vp.scrollLeft,top:vp.scrollTop,clientX:e.clientX,clientY:e.clientY};}
 else if(tool==='draw'){gesture={id:e.pointerId,mode:'draw',note:{...newNotePosition(pos),slots:1,velocity:100},startX:e.clientX,startY:e.clientY,revision:store.revision,context:context(),dx:0,dy:0,left:vp.scrollLeft,top:vp.scrollTop,clientX:e.clientX,clientY:e.clientY};}
 else{selected=null;groupSelection.clear();renderMusic();renderInspector();return;}
 e.preventDefault();vp.setPointerCapture(e.pointerId);renderMusic();renderInspector();const owner=gesture;requestAnimationFrame(()=>edgePan(owner));
});
vp.addEventListener('pointermove',e=>{
 if(pointers.has(e.pointerId))pointers.set(e.pointerId,{x:e.clientX,y:e.clientY});if(pinch&&pointers.size===2){const [a,b]=[...pointers.values()];zoomTo(pinch.step*Math.hypot(a.x-b.x,a.y-b.y)/Math.max(1,pinch.distance),(a.x+b.x)/2);return;}
 if(!gesture||gesture.id!==e.pointerId)return;
 if(gesture.mode==='marquee'){gesture.preview=point(e);drawMarquee(gesture);return;}
 gesture.clientX=e.clientX;gesture.clientY=e.clientY;gesture.dx=e.clientX-gesture.startX;gesture.dy=e.clientY-gesture.startY;previewGesture();
});
function previewGesture(){if(!gesture)return;const g=gesture,n=g.note,dx=Math.round((g.dx+vp.scrollLeft-g.left)/step),dy=Math.round((g.dy+vp.scrollTop-g.top)/row);let preview={...n};if(g.mode==='move'){preview.start=Math.max(0,Math.min(64-n.slots,n.start+dx));preview.pitch=Math.max(0,Math.min(127,n.pitch-dy));}else preview.slots=Math.max(1,Math.min(64-n.start,(g.mode==='draw'?1:n.slots)+dx));g.preview=preview;
 const node=canvas.querySelector(`[data-note="${n.start}"]`);if(node){node.style.left=48+preview.start*step+2+'px';node.style.top=32+(127-preview.pitch)*row+2+'px';node.style.width=preview.slots*step-4+'px';}
}
function edgePan(owner){
 const g=gesture;if(!g||g!==owner)return;if(Math.hypot(g.dx,g.dy)>=6){const r=vp.getBoundingClientRect();const x=g.clientX-r.left,y=g.clientY-r.top;let horizontal=x>=0&&x<22?-8:x>r.width-22&&x<=r.width?8:0;let vertical=g.mode==='move'?(y>=32&&y<54?-8:y>r.height-22&&y<=r.height?8:0):0;vp.scrollLeft=Math.max(0,Math.min(Math.max(0,48+64*step-vp.clientWidth),vp.scrollLeft+horizontal));vp.scrollTop=Math.max(0,Math.min(32+128*row-vp.clientHeight,vp.scrollTop+vertical));if(horizontal||vertical)previewGesture();}requestAnimationFrame(()=>edgePan(owner));
}
vp.addEventListener('pointerup',e=>{pointers.delete(e.pointerId);if(pointers.size<2)pinch=null;if(!gesture||gesture.id!==e.pointerId)return;const g=gesture;gesture=null;if(g.context!==context()||g.revision!==store.revision){renderMusic();message('Context changed; gesture cancelled.',true);return;}if(g.mode==='marquee'){const a=g.anchor,b=g.preview;const bounds={start:Math.max(0,Math.min(63,Math.min(a.start,b.start))),end:Math.max(1,Math.min(64,Math.max(a.start,b.start)+1)),low:Math.max(0,Math.min(127,Math.min(a.pitch,b.pitch))),high:Math.max(0,Math.min(127,Math.max(a.pitch,b.pitch)))};groupSelection.clear();for(const start of selectNotes(current(),bounds))groupSelection.add(start);selected=null;renderMusic();renderInspector();message(groupSelection.size+' notes selected.');return;}const n=g.preview||g.note;if(g.mode!=='draw'&&n.start===g.note.start&&n.pitch===g.note.pitch&&n.slots===g.note.slots){renderMusic();return;}if(g.mode!=='draw'&&Math.hypot(g.dx,g.dy)<6){renderMusic();return;}if(act(d=>putNote(d,pattern(),part,{...n,velocity:g.mode==='draw'?n.velocity:n.rawVelocity,endpoint:g.mode==='move'?(n.endpoint||0):0},g.mode==='draw'?null:g.note.start))){selected=n.start;cursor={start:n.start,pitch:n.pitch};renderMusic();renderInspector();}else renderMusic();});
vp.addEventListener('pointercancel',e=>{pointers.delete(e.pointerId);pinch=null;cancelGesture();});
vp.addEventListener('scroll',()=>{vel.scrollLeft=vp.scrollLeft;});
let velocityGesture=null;vel.addEventListener('pointerdown',e=>{const bar=e.target.closest('[data-velocity]');if(!bar||part>=2||tool==='group')return;selected=Number(bar.dataset.velocity);velocityGesture={id:e.pointerId,start:selected,revision:store.revision,context:context()};vel.setPointerCapture(e.pointerId);e.preventDefault();});
vel.addEventListener('pointermove',e=>{if(!velocityGesture)return;velocityGesture.value=Math.max(1,Math.min(127,Math.round((vel.getBoundingClientRect().bottom-10-e.clientY)/78*127)));const bar=vc.querySelector(`[data-velocity="${velocityGesture.start}"]`);if(bar){bar.style.height=velocityGesture.value/127*78+'px';bar.firstChild.textContent=velocityGesture.value;}});
vel.addEventListener('pointerup',()=>{const g=velocityGesture;velocityGesture=null;if(g&&g.context===context()&&g.revision===store.revision&&g.value){act(d=>{getPart(d,pattern(),part).steps[g.start].velocity=g.value;});}renderMusic();renderInspector();});vel.addEventListener('pointercancel',()=>{velocityGesture=null;renderMusic();});
function zoomTo(next,clientX=null){next=Math.max(part>=2?46:16,Math.min(96,next));const at=clientX===null?vp.clientWidth/2:clientX-vp.getBoundingClientRect().left;const anchor=(vp.scrollLeft+at-48)/step;step=next;renderMusic();vp.scrollLeft=48+anchor*step-at;$('#zoom').textContent=Math.round(step/40*100)+'%';}
vp.addEventListener('wheel',e=>{if(e.ctrlKey||e.metaKey){e.preventDefault();cancelGesture();zoomTo(step*(e.deltaY<0?1.12:.89),e.clientX);}},{passive:false});
function showNotes(notes=notesOf(current())){modal('Choose note / keyboard inspector',E('div',{class:'note-picker'},notes.map(n=>button(`${pitchName(n.pitch)} · step ${n.start+1} · length ${n.slots} · velocity ${n.velocity}`,()=>{groupSelection.clear();selected=n.start;cursor={start:n.start,pitch:n.pitch};closeModal();renderMusic();renderInspector();vp.scrollTop=32+(127-n.pitch)*row-vp.clientHeight/2;vp.scrollLeft=Math.max(0,n.start*step-40);vp.focus();}))));}
vp.addEventListener('keydown',e=>{
 if(part>=2)return;const n=notesOf(current()).find(n=>n.start===selected);if(e.key==='Escape'){cancelGesture();groupSelection.clear();selected=null;renderMusic();renderInspector();return;}if(tool==='group'){
  if(e.key==='Enter'){e.preventDefault();showGroupActions();return;}
  if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(e.key)){
   e.preventDefault();cursor.start=Math.max(0,Math.min(63,cursor.start+(e.key==='ArrowRight'?1:e.key==='ArrowLeft'?-1:0)));cursor.pitch=Math.max(0,Math.min(127,cursor.pitch+(e.key==='ArrowUp'?1:e.key==='ArrowDown'?-1:0)));selected=null;renderMusic();renderInspector();
  }else if(['Delete','Backspace',' '].includes(e.key))e.preventDefault();
  return;
 }if(!['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Enter','Delete','Backspace',' '].includes(e.key))return;e.preventDefault();if(e.key===' ')return;
 if((e.key==='Delete'||e.key==='Backspace')&&n){act(d=>removeNote(d,pattern(),part,n.start));selected=null;groupSelection.clear();renderInspector();return;}
 if(e.key==='Enter'){if(n)return;const at=newNotePosition(cursor);if(!act(d=>putNote(d,pattern(),part,{...at,slots:1,velocity:100})))return;cursor=at;selected=at.start;renderMusic();renderInspector();return;}
 if(n&&e.altKey&&['ArrowUp','ArrowDown'].includes(e.key)){act(d=>{getPart(d,pattern(),part).steps[n.start].velocity=Math.max(1,Math.min(127,n.velocity+(e.key==='ArrowUp'?1:-1)));});return;}
 if(n&&e.shiftKey&&['ArrowLeft','ArrowRight'].includes(e.key)){act(d=>putNote(d,pattern(),part,{...n,slots:n.slots+(e.key==='ArrowRight'?1:-1),velocity:n.rawVelocity,endpoint:0},n.start));return;}
 cursor.start=Math.max(0,Math.min(63,cursor.start+(e.key==='ArrowRight'?1:e.key==='ArrowLeft'?-1:0)));cursor.pitch=Math.max(0,Math.min(127,cursor.pitch+(e.key==='ArrowUp'?1:e.key==='ArrowDown'?-1:0)));selected=notesOf(current()).find(x=>x.start===cursor.start&&x.pitch===cursor.pitch)?.start??null;renderMusic();renderInspector();
});
$('#pattern').onchange=()=>{cancelGesture();closeModal();const n=Number($('#pattern').value);if(Number.isInteger(n)&&n>=1&&n<=32){patterns[part]=n-1;selected=null;groupSelection.clear();}render();};$('#length').onchange=()=>act(d=>{getPart(d,pattern(),part).length=Number($('#length').value);});$('#rate').onchange=()=>act(d=>{getPart(d,pattern(),part).rate=Number($('#rate').value);});
for(const id of ['direction','transpose'])$('#'+id).onchange=()=>{if(part>=2)return;act(d=>{getPart(d,pattern(),part)[id]=Number($('#'+id).value);});};
for(const b of $('#tools').children)b.onclick=()=>{cancelGesture();tool=b.dataset.tool;if(tool!=='group')groupSelection.clear();render();};$('#zoom-in').onclick=()=>zoomTo(step*1.25);$('#zoom-out').onclick=()=>zoomTo(step/1.25);$('#fit').onclick=()=>{zoomTo((vp.clientWidth-48)/current().length);vp.scrollLeft=0;};$('#note-list').onclick=()=>showNotes();$('#close-dialog').onclick=closeModal;dialog.addEventListener('cancel',event=>{event.preventDefault();closeModal();});
$('#undo').onclick=()=>{if(!canModify())return;cancelGesture();closeModal();if(store.undo()){persist();selected=null;groupSelection.clear();render();message('One local transaction undone · saving locally…');}};
$('#redo').onclick=()=>{if(!canModify())return;cancelGesture();closeModal();if(store.redo()){persist();selected=null;groupSelection.clear();render();message('One local transaction redone · saving locally…');}};
document.addEventListener('keydown',e=>{if(e.target.matches('input,textarea,[contenteditable]'))return;const key=e.key.toLowerCase();if((e.ctrlKey||e.metaKey)&&((key==='z'&&!e.shiftKey)||(key==='y'&&!e.shiftKey)||key==='z'&&e.shiftKey)){e.preventDefault();(key==='y'||e.shiftKey?$('#redo'):$('#undo')).click();}});
$('#export').onclick=()=>download('dnbox-local-draft.json',JSON.stringify(store.doc,null,2));
$('#open-draft').onchange=async e=>{
 const token=draftPick.next(),revision=store.revision,epoch=uiEpoch,key=context();
 try{
  const f=e.target.files[0];if(!f)return;
  if(f.size>1048576)throw Error('Draft limit:1MiB.');
  const bytes=await f.text();
  if(!draftPick.valid(token)||epoch!==uiEpoch||key!==context())return;
  const candidate=parseDraft(bytes);
  if(!draftPick.valid(token)||epoch!==uiEpoch||key!==context())return;
  modal('Replace complete local draft from '+f.name+'?',E('div',{},[
   E('p',{text:'This replaces all local parts and sound drafts, not a device project. One Undo restores this draft.'}),
   button('Replace local draft',()=>{
    if(!draftPick.valid(token)||store.revision!==revision||key!==context())return message('Draft selection or contents changed; open again.',true);
    if(!canModify())return;
    store.replace(candidate);persist();selected=null;groupSelection.clear();closeModal();render();
    message('Local draft replaced · saving locally. No device write.');
   })
  ]));
 }catch(err){if(draftPick.valid(token))message(err.message,true);}
 finally{e.target.value='';}
};
// Full part operations retain all locally stored cells, length, rate and per-part settings.
// Global sound drafts and engine assignments remain separate and never silently transfer.
$('#part-actions').onclick=()=>{
 cancelGesture();uiEpoch++;draftPick.next();
 const target={part,pattern:pattern(),key:context(),revision:store.revision};
 const same=()=>target.key===context()&&target.revision===store.revision;
 const confirm=(label,candidate,clip=null)=>{
  if(!same())return message('Part changed; reopen part actions.',true);
  const count=diffDraft(store.doc,candidate).length;
  modal(label+' · confirm complete part',E('div',{},[
   E('p',{text:`${PARTS[target.part]} / pattern ${target.pattern+1}: ${count} changed fields. Includes length, rate, all stored steps (including hidden tail) and pattern-local settings; excludes global sound and engine assignments. No device transfer.`}),
   button('Confirm '+label,()=>{
    if(!same())return message('Draft revision or part changed; reopen actions.',true);
    if(!canModify())return;
     const changed=projectEdit(()=>store.replace(candidate));
    if(clip&&changed)fullPartClipboard=clip;
    if(changed)persist();
    selected=null;groupSelection.clear();closeModal();render();
    message(changed?label+' applied locally · saving as one Undo transaction.':'No changes needed; local draft unchanged.');
   }),
   button('Cancel',closeModal)
  ]));
 };
 const actions=E('div',{class:'actions'},[
  button('Copy full part',()=>{
   if(!same())return message('Target changed; reopen part actions.',true);
   fullPartClipboard=takeFullPart(store.doc,target.pattern,target.part);
   closeModal();message('Copied '+PARTS[target.part]+' / pattern '+(target.pattern+1)+' with full stored tail and part settings. No edit/Undo frame.');
  }),
  button('Paste full part',()=>{
   try{if(!same())throw Error('Target changed; reopen actions.');
    confirm('Paste full part',replaceFullPart(store.doc,target.pattern,target.part,fullPartClipboard));
   }catch(e){message(e.message,true);}
  },{disabled:!fullPartClipboard||fullPartClipboard.kind!==partKind(target.part)}),
  button('Cut full part',()=>{
   try{if(!same())throw Error('Target changed; reopen actions.');
    confirm('Cut full part',clearFullPart(store.doc,target.pattern,target.part),takeFullPart(store.doc,target.pattern,target.part));
   }catch(e){message(e.message,true);}
  }),
  button('Clear full part',()=>{
   try{if(!same())throw Error('Target changed; reopen actions.');
    confirm('Clear full part',clearFullPart(store.doc,target.pattern,target.part));
   }catch(e){message(e.message,true);}
  })
 ]);
 modal('Part actions · '+PARTS[target.part],E('div',{},[
  E('p',{text:'Full-part copy/cut/paste/clear affects the complete saved local part, including hidden steps and its length/rate/settings. Same part type only. Does not include global sound or synth engine binding.'}),
  actions
 ]));
};
function soundKey(d){if(d.group==='Part mixer / each of5parts')return `mix/${part}/${d.parameter_index}`;if(d.group==='BREAKS'&&d.parameter_index>=14)return `mix/4/${d.parameter_index-13}`;return `${d.group==='Miami'||d.group.startsWith('Chicago')?'synth'+part+'/':''}${d.group}/${d.parameter_index}`;}
const chicagoNames={Cutoff:'VCF Cutoff',Reso:'VCF Peak',EnvMod:'VCF Env Amt',Wave:'OSC Waveform',Slide:'Glide',Volume:'Amp Level',AmpMod:'Amp Mode',Gnaw:'VCF Gnaw',Bite:'VCF Bite',FAttack:'Env Attack · filter',Decay:'Env Decay · filter',FSustain:'Env Sustain · filter',FRelease:'Env Release · filter',AAtt:'Env Attack · amp',ADec:'Env Decay · amp',ASus:'Env Sustain · amp',ARel:'Env Release · amp',ArpSw:'Arp Switch',ArpMd:'Arp Mode',ArpRg:'Arp Range',ArpSp:'Arp Speed',FxTyp:'Fx Type',FxEd1:'Fx Edit 1',FxEd2:'Fx Edit 2',PBend:'Pitch Bend',Damp:'Damper'};
function soundLabel(d){if(d.group==='Miami'&&d.parameter_index<18)return catalog.original.Miami[d.parameter_index].original_name;if(d.group.startsWith('Chicago'))return chicagoNames[d.name]||d.name;return d.name;}
function paramControl(d){
 const key=soundKey(d),isBreak=d.group==='BREAKS'&&d.parameter_index<12,value=isBreak?current().settings[d.parameter_index]:store.doc.sound[key];let disabled=(d.group==='Miami'&&d.parameter_index===16)||(d.group==='Chicago/Performance'&&d.name==='PBend');const label=soundLabel(d);const out=E('output',{text:value===undefined?'Unset · local only':String(value)});const title=E('label',{},[E('span',{text:label}),out]);const box=E('div',{class:'sound-row','data-param':key},[title]);
 const change=v=>act(doc=>{if(isBreak)getPart(doc,pattern(),4).settings[d.parameter_index]=v;else doc.sound[key]=v;},'Local sound/settings draft changed. No device or audio effect.');
 const wave=d.group==='Miami'&&[0,6,11].includes(d.parameter_index);
 if(wave){const input=E('select',{'aria-label':label,disabled});for(const [i,name] of ['Triangle','Square'].entries())input.append(E('option',{value:i,text:name,selected:value===i}));if(value===undefined)input.prepend(E('option',{value:'',text:'Unset',selected:true}));const missing=d.parameter_index===11?['DownSaw','UpSaw']:d.parameter_index===6?['Saw','Pulse']:['Saw'];for(const n of missing)input.append(E('option',{disabled:true,text:n+' · unavailable'}));input.onchange=()=>{if(input.value!=='')change(Number(input.value));};box.append(input,E('small',{text:'Current two-state domain; missing original modes stay unavailable.'}));}
 else{const input=E('input',{type:'range',min:d.minimum,max:d.maximum,step:1,value:value??d.minimum,'aria-label':label,disabled});input.oninput=()=>out.textContent=input.value+' · draft';input.onchange=()=>change(Number(input.value));box.append(input);}
 if(disabled)box.append(E('small',{text:'Physical-wheel caller; software ownership unqualified. No software edit.'}));return box;
}
function renderSound(){
 const host=$('#sound');host.replaceChildren();if(!catalog){host.append(E('p',{text:'Loading local control catalog…'}));return;}host.append(E('h2',{text:part<2?`${store.doc.engines[part]} · local base sound`:`${PARTS[part]} · local sound`}));
 host.append(E('p',{class:'muted',text:part<2?'Base sound applies across this synth’s patterns. No device values are known.':'Draft values only; actual sound remains unknown.'}));
 if(part<2){host.append(E('div',{class:'actions'},['Miami','Chicago'].map(engine=>button(engine,()=>{soundGroup='';act(d=>{d.engines[part]=engine;});},{'aria-pressed':store.doc.engines[part]===engine}))));}
 let groups=[...new Set(catalog.parameters.map(x=>x.group))].filter(g=>part<2?g.startsWith(store.doc.engines[part]):part<4?g.startsWith(PARTS[part]+'/'):g==='BREAKS');groups.push('Part mixer / each of5parts');const globals=['Shared FX bus','Shared master'];
 if(![...groups,...globals].includes(soundGroup))soundGroup=groups[0];host.append(E('div',{class:'group-tabs'},groups.map(g=>button(g.startsWith('Part mixer')?'Part mixer':g.replace(/^.*?\//,''),()=>{soundGroup=g;soundSection='';renderSound();},{'aria-pressed':g===soundGroup}))));host.append(E('details',{},[E('summary',{text:'Shared global scopes'}),E('p',{text:'These controls affect shared sound across parts. Open explicitly; no single-part isolation claim.'}),...globals.map(g=>button(g,()=>{soundGroup=g;soundSection='';renderSound();}))]));
 host.append(E('h3',{text:soundGroup}));if(soundGroup.startsWith('Chicago'))host.append(E('p',{class:'muted',text:'Original identities use human-facing labels. Amp/filter ADSRs stay independent; a unified original Env macro is unsupported. Literal grouping and acoustic parity unqualified.'}));
 if(soundGroup==='Chicago/Live Arp'||soundGroup==='Chicago/Performance')host.append(E('p',{class:'muted',text:'Live keyboard/performance settings; pattern notes do not use these controls. No local audio or live messages.'}));
 if(soundGroup==='BREAKS')host.append(E('div',{class:'notice',text:'Pattern generator fields0–11; Level/Pitch base sound; Rev/Dly part MIX. No generated waveform or qualified live output.'}));
 let fields=catalog.parameters.filter(x=>x.group===soundGroup);
 const sections=soundGroup==='Miami'?{'Filter / Crush':[9,10],'Carrier':[0,1,2,3,4],'XMod':[5,6,7,8],'Wobble':[11,12,13],'Output':[14,15,16,17],'Extensions':[18,19,20,21,22]}:soundGroup==='Chicago/core'?{'Tone':[0,1,2,5,10],'Bite / Gnaw':[17,18],'Filter envelope':[3,12,13,14],'Amp envelope':[16,19,20,21,22],'Output':[7],'Extensions':[4,6,8,9,11,15]}:null;
 if(sections){if(!sections[soundSection])soundSection=Object.keys(sections)[0];host.append(E('div',{class:'group-tabs sound-sections'},Object.keys(sections).map(name=>button(name,()=>{soundSection=name;renderSound();},{'aria-pressed':soundSection===name}))));fields=fields.filter(d=>sections[soundSection].includes(d.parameter_index));host.append(E('h3',{text:soundSection}));}
 for(const d of fields)host.append(paramControl(d));
 if(part===4&&soundGroup==='BREAKS'){host.append(E('p',{text:'LoopA/B catalog unavailable; sample references cannot be chosen or transferred.'}),button('Slice mode · '+(current().sliceMode?'note%8':'GM36–43'),()=>act(d=>{getPart(d,pattern(),4).sliceMode^=1;})),button('Generate output · unqualified',()=>{},{disabled:true}));}
 host.append(E('p',{class:'muted',text:'Runtime extensions are retained separately from original18/22 identity requirements. No firmware adapter, audition or device readback is implemented.'}));
}
$('#sound-toggle').onclick=()=>setWorkspaceView(workspaceView==='sound'?'editor':'sound');
function reportPlan(plan,captured){
 pending={plan,captured};const body=E('div');body.append(E('p',{text:`Pinned ${PARTS[captured.part]} / P${captured.pattern+1} · draft revision${plan.revision}. Other parts and sound remain unchanged.`}));
 for(const error of plan.errors)body.append(E('div',{class:'notice',text:error}));for(const notice of plan.notices||[])body.append(E('p',{text:notice}));
 if(plan.counts)body.append(E('p',{text:JSON.stringify(plan.counts)}));body.append(E('h3',{text:`Replacement: ${plan.changes.length} changed fields`}));
 const table=E('table',{class:'report-table'},[E('thead',{},E('tr',{},['Field','Before','After'].map(text=>E('th',{text}))))]);const tbody=E('tbody');for(const x of plan.changes.slice(0,200))tbody.append(E('tr',{},[E('td',{text:x.target+' / '+x.field}),E('td',{text:x.before}),E('td',{text:x.after})]));table.append(tbody);body.append(E('div',{class:'report-scroll'},table));if(plan.changes.length>200)body.append(E('p',{text:'First200 rows displayed; complete replacement/loss report is downloadable below.'}));
 body.append(E('h3',{text:`Losses / unsupported data (${plan.losses.length})`}));const loss=E('pre',{text:JSON.stringify(plan.losses.slice(0,100),null,2)});loss.style.whiteSpace='pre-wrap';body.append(loss);if(plan.losses.length>100)body.append(E('p',{text:'First100 loss entries displayed; complete report below.'}));
 const accept=E('input',{type:'checkbox','aria-label':'Accept reported replacements and losses'});body.append(E('label',{},[accept,E('span',{text:' I reviewed the replacement and all reported losses.'})]));const use=button('Use local preview',()=>{try{if(!pending||captured.key!==context())throw Error('Target changed. Preview again.');if(!accept.checked)throw Error('Accept the inspectable replacement/loss report first.');if(!canModify())return;projectEdit(()=>applyPlan(store,plan));persist();closeModal();selected=null;groupSelection.clear();render();message('Local preview applied as one Undo transaction · saving locally. No device write.');}catch(e){message(e.message,true);}},{disabled:plan.errors.length>0,'data-apply-plan':true});
 body.append(E('div',{class:'actions'},[use,button('Discard',()=>{closeModal();message('Preview discarded; draft unchanged.');}),button('Download full report',()=>download('dnbox-replacement-report.json',JSON.stringify({...plan,candidate:undefined},null,2)))]));modal('Review replacement / losses',body);
}
$('#generator').onclick=()=>{
 if(part>=2){if(part<4){modal('Generator unavailable',E('p',{text:'No dedicated909/808 generator was verified. Draw/import hits and edit full voice sound controls.'}));return;}modal('BREAK generator settings',E('div',{},[E('p',{text:'Stateful native BB generator output cannot be previewed offline. Edit source-backed settings in Sound; no audio or candidate is fabricated.'}),button('Open BREAK settings',()=>{closeModal();soundGroup='BREAKS';renderSound();$('#sound').hidden=false;}),button('Preview · unqualified',()=>{},{disabled:true}),button('Use · unavailable',()=>{},{disabled:true})]));return;}
 const captured={part,pattern:pattern(),key:context()},g=copy(current().generator),fields=E('div',{class:'form-grid'});for(const [k,label,min,max] of [['density','Density',0,100],['accent','Accent',0,100],['slide','Slide',0,100],['octaves','Octaves',1,3],['root','Root · C=0',0,11],['scale','Scale · Minor/Phrygian/HarmMinor/MinPent/Dorian/Major',0,5],['baseOctave','Base octave',0,4],['autoBars','Auto-mutate everyNbars ·0=off',0,16],['seed','Seed',0,4294967295]])fields.append(number(label,g[k],min,max,v=>g[k]=v));
 const body=E('div',{},[E('p',{text:`Pinned ${PARTS[part]} / P${pattern()+1} / length${current().length}of64. New resets all64, including hidden cells. Auto-mutate is not running.`}),fields,E('div',{class:'notice',text:'Local seeded generator. Device/audio/per-note-sound parity remains unqualified. Auto-bars is a stored setting, never a running timer.'}),E('div',{class:'actions'},[button('Preview New',()=>preview('new')),button('Preview Mutate',()=>preview('mutate')),button('Arm auto-mutate · unsupported',()=>{},{disabled:true})])]);
 function preview(mode){try{if(captured.key!==context())throw Error('Target changed. Open generator again.');reportPlan(generatePlan(store,captured.pattern,captured.part,mode,g),captured);}catch(e){message(e.message,true);}}
 modal('TB-3PO local generator',body);
};
const requests=new Map();function workerRequest(op,data){if(!worker)throw Error('Import worker unavailable.');return new Promise((resolve,reject)=>{const id=++workerId;requests.set(id,{resolve,reject});worker.postMessage({id,op,...data});});}
function resetWorker(){if(worker)worker.terminate();for(const x of requests.values())x.reject(Error('Import cancelled.'));requests.clear();worker=new Worker(new URL('./import-worker.js',import.meta.url),{type:'module'});worker.onmessage=({data})=>{const p=requests.get(data.id);if(!p)return;requests.delete(data.id);data.ok?p.resolve(data):p.reject(Error(data.error));};worker.onerror=()=>{for(const x of requests.values())x.reject(Error('MIDI worker failed; no draft change.'));requests.clear();};}
$('#import').onclick=()=>{if(part===4)return;$('#midi-file').click();};
$('#midi-file').onchange=async e=>{
 const f=e.target.files[0];e.target.value='';if(!f)return;
 const captured={part,pattern:pattern(),key:context(),epoch:++uiEpoch};
 try{
  if(part===4)throw Error('BREAK import is unqualified.');
  if(f.size>1048576)throw Error('MIDI limit:1MiB.');
  resetWorker();parsedSummary=null;message('Parsing bounded MIDI file locally…');
  const bytes=await f.arrayBuffer();
  if(captured.key!==context()||captured.epoch!==uiEpoch)return;
  const reply=await workerRequest('parse',{bytes});
  if(captured.key!==context()||captured.epoch!==uiEpoch)return;
  parsedSummary=reply.summary;
  showImportSettings(captured,f.name);
 }catch(err){if(captured.epoch===uiEpoch)message(err.message,true);}
};
function showImportSettings(captured,name){
 const settings={pattern:captured.pattern,part:captured.part,track:0,channel:parsedSummary.tracks[0]?.channels[0]??0,rate:current().rate,startBeat:1,beats:Math.max(1,Math.floor(current().length/[4,6,8,3][current().rate]))};
 const tracks=E('select',{'aria-label':'MIDI track'});parsedSummary.tracks.forEach((x,i)=>tracks.append(E('option',{value:i,text:x.name})));const channels=E('select',{'aria-label':'MIDI channel'});function channelsForTrack(){channels.replaceChildren(...Array.from({length:16},(_,i)=>E('option',{value:i,text:'Channel '+(i+1),selected:i===settings.channel})));}channelsForTrack();tracks.onchange=()=>{settings.track=Number(tracks.value);settings.channel=parsedSummary.tracks[settings.track]?.channels[0]??0;channelsForTrack();};channels.onchange=()=>settings.channel=Number(channels.value);
 const fields=E('div',{class:'form-grid'},[E('label',{text:'Track'},tracks),E('label',{text:'Channel'},channels),number('Start beat',1,1,1000000,v=>settings.startBeat=v),number('Beats',settings.beats,1,64,v=>settings.beats=v),number('Rate index ·0=1/16,1=1/16T,2=1/32,3=1/8T,4=1/8,5=1/4',settings.rate,0,5,v=>settings.rate=v)]);
 modal('Import MIDI into one local part',E('div',{},[E('p',{text:`${name} · SMF${parsedSummary.format} · PPQN${parsedSummary.ppqn} · ${PARTS[captured.part]} / P${captured.pattern+1}`}),fields,E('p',{text:'No automatic polyphonic reduction, wrapping or cross-pattern splitting. Tempo/controllers/program/SysEx are reported, never sent.'}),button('Build replacement preview',async()=>{try{if((captured.key!==context()||captured.epoch!==uiEpoch))throw Error('Target changed; import again.');const reply=await workerRequest('preview',{doc:store.doc,revision:store.revision,settings});if((captured.key!==context()||captured.epoch!==uiEpoch))throw Error('Target changed; preview cancelled.');reportPlan(reply.plan,captured);}catch(e){message(e.message,true);}})]));
}
$('#recover-draft').onclick=()=>{
 if(draftStorage.status!=='recovery')return;
 modal('Restore last validated local draft?',E('div',{},[
  E('p',{text:'The primary saved draft is damaged or missing. The separate last-good copy passed validation. Restoring replaces only the primary saved entry and keeps the validated backup. Export any unsent edits before restoring.'}),
  button('Restore last good copy',async()=>{
   try{
    const recovered=await draftStorage.restore();
    cancelGesture();store=new DraftStore(recovered);fullPartClipboard=null;selected=null;groupSelection.clear();
    closeModal();$('#recover-draft').hidden=true;$('#draft-state').textContent='Recovered local draft · device unavailable';
    render();centerPitch();message('The validated last-good draft was restored. No device data changed.');
   }catch(e){message('Recovery failed: '+e.message+'. The last-good copy remains untouched.',true);}
  }),
  button('Cancel',closeModal)
 ]));
};

function newNotePosition(pos){
 return grid.snap?{start:snapStep(pos.start,grid.steps),pitch:snapPitch(pos.pitch,grid.root,grid.scale)}:{...pos};
}
function renderGhost(){
 const label=$('#ghost-description');label.textContent='';
 if(!grid.ghost||part>=2)return;
 const other=1-part,p=patterns[other];
 label.textContent=`Read-only ${PARTS[other]} / P${p+1}`;
 for(const n of notesOf(getPart(store.doc,p,other))){
  const node=E('div',{class:'ghost-note','aria-hidden':'true'});
  Object.assign(node.style,{left:48+n.start*step+2+'px',top:32+(127-n.pitch)*row+2+'px',width:Math.max(4,(n.end-n.start)*step-4)+'px',height:row-4+'px'});
  canvas.append(node);
 }
}
function drawMarquee(g){
 canvas.querySelector('.selection-box')?.remove();
 const a=g.anchor,b=g.preview,box=E('div',{class:'selection-box','aria-hidden':'true'});
 Object.assign(box.style,{left:48+Math.min(a.start,b.start)*step+'px',top:32+(127-Math.max(a.pitch,b.pitch))*row+'px',width:(Math.abs(a.start-b.start)+1)*step+'px',height:(Math.abs(a.pitch-b.pitch)+1)*row+'px'});
 canvas.append(box);
}
function setWorkspaceView(view){
 cancelGesture();workspaceView=view;$('#sound').hidden=false;
 $('.workspace').dataset.view=view;
 $('#view-editor').setAttribute('aria-pressed',view==='editor');
 $('#view-sound').setAttribute('aria-pressed',view==='sound');
 if(view==='editor')vp.focus({preventScroll:true});else $('#view-sound').focus({preventScroll:true});
}
function renderKeyboard(){
 const octave=Number($('#keyboard-octave').value),host=$('#keyboard-keys');host.replaceChildren();
 if(!Number.isInteger(octave)||octave< -1||octave>9)return;
 for(let i=0;i<12;i++){
  const pitch=(octave+1)*12+i;if(pitch>127)break;
  host.append(button(pitchName(pitch),()=>{cursor.pitch=pitch;renderKeyboard();renderMusic();message('Local pitch '+pitchName(pitch)+' selected. Insert note adds it at step '+(cursor.start+1)+'.');},{'data-key-pitch':pitch,'aria-pressed':cursor.pitch===pitch,class:[1,3,6,8,10].includes(i)?'black':''}));
 }
}
function showGroupActions(){
 if(part>=2)return;
 cancelGesture();uiEpoch++;draftPick.next();
 const target={part,pattern:pattern(),key:context(),revision:store.revision};
 const same=()=>target.key===context()&&target.revision===store.revision;
 const count=E('p',{id:'group-count',class:'group-count','aria-live':'polite'});
 const list=E('div',{class:'note-checklist'});
 const update=()=>{count.textContent=groupSelection.size+' selected · complete notes, including TIE cells';};
 const checklist=()=>{
  list.replaceChildren();
  for(const n of notesOf(current())){
   const box=E('input',{type:'checkbox','aria-label':`Select ${pitchName(n.pitch)} at step ${n.start+1}`});box.checked=groupSelection.has(n.start);
   box.onchange=()=>{if(!same())return;box.checked?groupSelection.add(n.start):groupSelection.delete(n.start);selected=null;renderMusic();renderInspector();update();};
   list.append(E('label',{},[box,E('span',{text:`${pitchName(n.pitch)} · step ${n.start+1} · ${n.slots} slots${n.start>=current().length?' · hidden tail':''}`})]));
  }
  update();
 };
 const preview=(action,options={},clipboardAfter=null)=>{
  try{
   if(!same())throw Error('Draft or part changed; reopen note groups.');
   const plan=groupPlan(store,target.pattern,target.part,[...groupSelection],action,options);
   const changes=diffDraft(store.doc,plan.candidate).length;
   modal('Confirm local note-group change',[
    E('p',{text:`${changes} changed fields in ${PARTS[target.part]} / P${target.pattern+1}. Complete raw notes and hidden tails are preserved; one Undo restores this edit. No device write.`}),
    button('Apply local group change',()=>{
     try{
      if(!same())throw Error('Draft changed after preview. Reopen note groups.');
      if(!canModify())return;
      const changed=projectEdit(()=>applyPlan(store,plan));
      if(clipboardAfter&&changed)noteClipboard=clipboardAfter;
      if(changed)persist();
      groupSelection.clear();for(const start of plan.selection)groupSelection.add(start);
      selected=null;closeModal();render();message(changed?'Note group changed as one transaction · saving locally…':'No changes needed.');
     }catch(e){message(e.message,true);}
    },{'data-group-apply':true}),button('Cancel',closeModal)
   ]);
  }catch(e){message(e.message,true);}
 };
 const velocity=E('input',{id:'group-velocity',type:'number',min:1,max:127,value:100});
 const shift=E('input',{id:'group-step-shift',type:'number',min:-63,max:63,value:0});
 const transpose=E('input',{id:'group-pitch-shift',type:'number',min:-127,max:127,value:0});
 checklist();
 modal('Note groups · '+PARTS[target.part],[
  E('p',{text:'Drag a rectangle with Group select, or use these checkboxes. Actions include complete NOTE/TIE cells; collisions and out-of-range notes reject the whole edit.'}),count,
  E('div',{class:'actions'},[
   button('Select all notes',()=>{if(!same())return;groupSelection.clear();for(const n of notesOf(current()))groupSelection.add(n.start);selected=null;checklist();renderMusic();renderInspector();},{id:'select-all-notes'}),
   button('Select none',()=>{groupSelection.clear();checklist();renderMusic();renderInspector();})
  ]),list,
  E('div',{class:'actions'},[
   button('Copy notes',()=>{try{if(!same())throw Error('Part changed.');noteClipboard=takeNotes(store.doc,target.pattern,target.part,[...groupSelection]);closeModal();message('Copied complete local notes. Nothing sent to a device.');}catch(e){message(e.message,true);}},{id:'group-copy'}),
   button('Cut notes',()=>{try{preview('delete',{},takeNotes(store.doc,target.pattern,target.part,[...groupSelection]));}catch(e){message(e.message,true);}},{id:'group-cut'}),
   button('Paste at cursor',()=>preview('paste',{clipboard:noteClipboard,start:cursor.start}),{id:'group-paste',disabled:!noteClipboard}),
   button('Delete selected',()=>preview('delete'),{id:'group-delete'})
  ]),
  E('div',{class:'form-grid'},[
   E('label',{text:'Move by steps'},shift),E('label',{text:'Transpose semitones'},transpose),
   button('Preview group move',()=>preview('move',{steps:Number(shift.value),semitones:Number(transpose.value)}),{id:'group-move'}),
   E('label',{text:'Set onset velocity'},velocity),button('Preview velocity Set',()=>preview('velocity',{value:Number(velocity.value)}),{id:'group-set-velocity'})
  ])
 ]);
}
$('#group-actions').onclick=showGroupActions;
$('#view-editor').onclick=()=>setWorkspaceView('editor');$('#view-sound').onclick=()=>setWorkspaceView('sound');
for(const id of ['grid-root','grid-scale','grid-step','snap-toggle','ghost-toggle'])$('#'+id).onchange=()=>{
 cancelGesture();grid.root=Number($('#grid-root').value);grid.scale=$('#grid-scale').value;grid.steps=Number($('#grid-step').value);grid.snap=$('#snap-toggle').checked;grid.ghost=$('#ghost-toggle').checked;renderMusic();
 message('Grid preferences affect new-note insertion/drawing only. Existing notes and imports remain unchanged.');
};
$('#keyboard-toggle').onclick=()=>{cancelGesture();const box=$('#touch-keyboard');box.hidden=!box.hidden;$('#keyboard-toggle').setAttribute('aria-expanded',!box.hidden);renderKeyboard();};
$('#keyboard-octave').onchange=renderKeyboard;
$('#keyboard-insert').onclick=()=>{if(part>=2)return;const at=newNotePosition(cursor);if(act(d=>putNote(d,pattern(),part,{...at,slots:1,velocity:100}))){cursor=at;selected=at.start;groupSelection.clear();render();}};
vp.addEventListener('lostpointercapture',()=>{if(gesture)cancelGesture();});
vel.addEventListener('lostpointercapture',()=>{if(velocityGesture)cancelGesture();});
window.addEventListener('blur',()=>cancelGesture());
document.addEventListener('visibilitychange',()=>{if(document.hidden)cancelGesture();});

const libraryController=createLibraryController({
 getDraft:()=>store.doc,getRevision:()=>store.revision,
 replaceDraft:candidate=>{
  if(!canModify())throw Error('Local saving is locked. Export your work before reloading.');
  cancelGesture();const changed=store.replace(candidate);if(changed)persist();selected=null;groupSelection.clear();render();return changed;
 }
});
$('#library').onclick=()=>{cancelGesture();uiEpoch++;draftPick.next();modal('Community library',[]);libraryView=mountLibrary({host:$('#dialog-body'),controller:libraryController,notify:message,onCommitted:closeModal});};

$('#project-file').onclick=()=>{
 cancelGesture();const key=context(),revision=store.revision;uiEpoch++;
 const model=E('select',{'aria-label':'Project model'},['SMK-37 Pro','SMK-37 Elite'].map(name=>E('option',{value:name,text:name})));
 const input=E('input',{type:'file',accept:'.bin,.x0x,application/octet-stream','aria-label':'Revision-13 project file'});
 input.onchange=async()=>{
  const token=++uiEpoch,chosen=model.value,f=input.files[0];if(!f)return;
  try{
   if(f.size!==36452)throw Error('Select an exact 36452-byte raw revision-13 project binary. Portable .x0xproject bundles are not supported here.');
   const bytes=new Uint8Array(await f.arrayBuffer());
   const {openProject}=await import('./project-adapter.js');
   if(token!==uiEpoch||context()!==key||store.revision!==revision)return;
   const project=openProject(bytes,{model:chosen});
   modal('Preview revision-13 project file',[
    E('p',{text:'Replace the complete local musical draft from this file? Unrepresented project fields remain in the original file for lossless export. No device communication. An offline draft does not expose every project field.'}),
    button('Open musical draft from project',()=>{
     try{if(token!==uiEpoch||context()!==key||store.revision!==revision)throw Error('Draft changed; reopen the project file.');if(!canModify())return;const changed=store.replace(project.draft);projectOrigins.set(store.doc,project);if(changed)persist();selected=null;groupSelection.clear();closeModal();render();message('Project musical draft opened locally. Keep the original file; no device write.');}catch(e){message(e.message,true);}
    }),button('Cancel',closeModal)
   ]);
  }catch(e){if(token===uiEpoch)message(e.message,true);}
 };
 const exportButton=button('Export edited project binary',()=>{
  try{const origin=projectOrigins.get(store.doc);if(!origin)throw Error('Open a supported project file first.');const bytes=origin.exportDraft(store.doc);download('dnbox-edited-project.bin',bytes,'application/octet-stream');message('Exported project file locally. No samples, device storage or firmware changed.');}catch(e){message(e.message,true);}
 },{disabled:!projectOrigins.has(store.doc)});
 modal('Project file · local only',[
  E('p',{text:'Choose the source model explicitly. This accepts a raw revision-13 project binary, not a portable .x0xproject bundle, firmware package or sample archive. Keep its original backup and sample data. Live read/write remains unavailable until the firmware provides owner-bound atomic editor transactions.'}),
  E('label',{text:'Source model'},model),input,exportButton
 ]);
};


const loadedDraft=draftStorage.load();
if(loadedDraft.draft)store=new DraftStore(loadedDraft.draft);
if(loadedDraft.status==='recovery'){
 $('#recover-draft').hidden=false;
 $('#draft-state').textContent='Validated backup found · restore required';
}else if(loadedDraft.status==='locked'){
 $('#draft-state').textContent='Saved draft unreadable · writes blocked';
}
if(loadedDraft.warning)message(loadedDraft.warning,true);
window.addEventListener('storage',e=>{
 if(draftStorage.changed(e.key,e.newValue))message('Another editor tab changed draft storage. This tab will not overwrite the newer saved copy. Export your work before reloading.',true);
});
render();centerPitch();fetch('./controls.json').then(r=>{if(!r.ok)throw Error('Control catalog unavailable.');return r.json();}).then(d=>{catalog=d;renderSound();}).catch(e=>message(e.message,true));
// Read-only inspection for frozen-source browser validation; no test mutation hook.
Object.defineProperty(window,'dnboxEditor',{value:Object.freeze({snapshot:()=>copy(store.doc),revision:()=>store.revision,context:()=>({part,pattern:pattern(),selected,tool}),notes:()=>part<2?notesOf(current()):[],pending:()=>pending?{revision:pending.plan.revision,errors:[...pending.plan.errors]}:null})});
