// SPDX-License-Identifier: GPL-3.0-only
import {copy,gate,getPart,integer,validateDraft} from './model.js';
import {notesOf} from './editing.js';

const CLIPBOARD_SCHEMA='dnbox-note-group/1';
const fail=message=>{throw Error(message);};
const exactKeys=(value,want)=>value&&Object.getPrototypeOf(value)===Object.prototype&&
 Object.keys(value).length===want.length&&want.every(key=>Object.hasOwn(value,key));

function target(doc,pattern,part) {
 if(!integer(pattern,0,31))fail('Invalid group target pattern.');
 if(!integer(part,0,1))fail('Note groups require a melodic part.');
 validateDraft(doc);
 return getPart(doc,pattern,part);
}

function validCell(cell) {
 return exactKeys(cell,['note','flags','velocity'])&&integer(cell.note,0,127)&&
  integer(cell.flags,0,255)&&gate(cell)<3&&integer(cell.velocity,0,127)&&
  !(gate(cell)===0&&(cell.flags>>4));
}

function selectionOf(part,starts) {
 if(!Array.isArray(starts)||!starts.length||starts.length>64||
    Array.from(starts).some(start=>!integer(start,0,63))||new Set(starts).size!==starts.length)
  fail('Choose a nonempty selection of distinct NOTE onsets.');
 const notes=new Map(notesOf(part).map(note=>[note.start,note]));
 return starts.slice().sort((a,b)=>a-b).map(start=>{
  const note=notes.get(start);
  if(!note)fail('The selection contains a missing NOTE onset or an orphan TIE.');
  return note;
 });
}

// Time bounds are half-open; pitches are inclusive. A rectangle can intersect
// a sustained note even when its onset lies outside the rectangle.
export function selectNotes(part,{start,end,low,high}) {
 if(!Array.isArray(part?.steps)||part.steps.length!==64||
    Array.from(part.steps).some(cell=>!validCell(cell)))fail('Invalid melodic note cells.');
 if(![start,end,low,high].every(Number.isFinite)||start<0||end>64||start>end||
    low<0||high>127||low>high)fail('Invalid note selection rectangle.');
 if(start===end)return [];
 return notesOf(part).filter(note=>note.start<end&&note.end>start&&
  note.pitch>=low&&note.pitch<=high).map(note=>note.start);
}

export function takeNotes(doc,pattern,part,starts) {
 const source=target(doc,pattern,part),notes=selectionOf(source,starts),origin=notes[0].start;
 return {
  schema:CLIPBOARD_SCHEMA,kind:'melodic',sourcePattern:pattern,sourcePart:part,
  notes:notes.map(note=>({start:note.start-origin,cells:copy(source.steps.slice(note.start,note.start+note.slots))}))
 };
}

function validateClipboard(clipboard) {
 if(!exactKeys(clipboard,['schema','kind','sourcePattern','sourcePart','notes'])||
    clipboard.schema!==CLIPBOARD_SCHEMA||clipboard.kind!=='melodic'||
    !integer(clipboard.sourcePattern,0,31)||!integer(clipboard.sourcePart,0,1)||
    !Array.isArray(clipboard.notes)||!clipboard.notes.length||clipboard.notes.length>64)
  fail('Invalid melodic note clipboard.');
 let end=0;
 for(let index=0;index<clipboard.notes.length;index++) {
  const note=clipboard.notes[index];
  if(!exactKeys(note,['start','cells'])||!integer(note.start,0,63)||
     (index===0?note.start!==0:note.start<end)||!Array.isArray(note.cells)||
     !integer(note.cells.length,1,64-note.start))fail('Invalid or overlapping clipboard note bounds.');
  for(let cellIndex=0;cellIndex<note.cells.length;cellIndex++) {
   const cell=note.cells[cellIndex];
   if(!validCell(cell)||gate(cell)!==(cellIndex===0?1:2))fail('Invalid clipboard NOTE/TIE cells.');
  }
  end=note.start+note.cells.length;
 }
 return clipboard;
}

function clearNotes(part,notes) {
 for(const note of notes)for(let step=note.start;step<note.start+note.slots;step++)
  part.steps[step]={...part.steps[step],flags:0};
}

function placeNotes(part,clipboard,start,pitchOffset) {
 if(!integer(start,0,63)||!integer(pitchOffset,-127,127))fail('Group destination is outside stored bounds.');
 const placements=clipboard.notes.map(note=>({
  start:start+note.start,
  cells:note.cells.map(cell=>({...cell,note:cell.note+pitchOffset}))
 }));
 for(const note of placements) {
  if(note.start+note.cells.length>64||note.cells.some(cell=>!integer(cell.note,0,127)))
   fail('The complete group exceeds stored pitch or step bounds.');
  for(let index=0;index<note.cells.length;index++)if(gate(part.steps[note.start+index])!==0)
   fail('This group would overlap an unselected note or TIE.');
  const next=note.start+note.cells.length;
  if(next<64&&gate(part.steps[next])===2)fail('This group would absorb an unselected TIE.');
 }
 // Validate the entire placement before copying any member. All raw cells,
 // including nonuniform TIE metadata and legacy velocity zero, travel together.
 for(const note of placements)for(let index=0;index<note.cells.length;index++)
  part.steps[note.start+index]=note.cells[index];
 return placements.map(note=>note.start);
}

function optionsFor(options,names) {
 if(!options||Object.getPrototypeOf(options)!==Object.prototype||
    Object.keys(options).some(key=>!names.includes(key)))fail('Invalid group operation options.');
}

// Build an immutable preview. applyPlan() owns revision checking and creates
// the single Undo transaction; neither preview nor clipboard creation edits RAM.
export function groupPlan(store,pattern,part,starts,action,options={}) {
 const source=target(store.doc,pattern,part);
 if(!Number.isSafeInteger(store.revision)||store.revision<0)fail('Invalid draft revision.');
 const candidate=copy(store.doc),destination=getPart(candidate,pattern,part);
 let selection;
 if(action==='paste') {
  optionsFor(options,['clipboard','start','pitchOffset']);
  const clipboard=validateClipboard(options.clipboard);
  selection=placeNotes(destination,clipboard,options.start,options.pitchOffset??0);
 } else {
  const notes=selectionOf(source,starts);
  selection=notes.map(note=>note.start);
  if(action==='delete') {
   optionsFor(options,[]);clearNotes(destination,notes);selection=[];
  } else if(action==='velocity') {
   optionsFor(options,['value']);
   if(!integer(options.value,1,127))fail('Group velocity must be an integer from 1 to 127.');
   for(const note of notes)destination.steps[note.start].velocity=options.value;
  } else if(action==='move') {
   optionsFor(options,['steps','semitones']);
   if(!integer(options.steps,-63,63)||!integer(options.semitones,-127,127))
    fail('Group movement requires integer offsets within stored bounds.');
   const clipboard=takeNotes(store.doc,pattern,part,starts);
   clearNotes(destination,notes);
   selection=placeNotes(destination,clipboard,notes[0].start+options.steps,options.semitones);
  } else fail('Unsupported group operation.');
 }
 validateDraft(candidate);
 return {revision:store.revision,pattern,part,candidate,selection};
}
