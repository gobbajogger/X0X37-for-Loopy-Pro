// SPDX-License-Identifier: GPL-3.0-only
// Entire pattern-part operations. No global sound or firmware project mutation.
import {copy,createDraft,getPart,validateDraft,integer} from './model.js';
export function partKind(part){
  if(!integer(part,0,4))throw Error('Invalid part index.');
  return part<2?'synth':part<4?'drum':'breaks';
}
function check(doc,pattern,part){
  if(!integer(pattern,0,31))throw Error('Invalid pattern index.');
  partKind(part);validateDraft(doc);
}
export function takeFullPart(doc,pattern,part){
  check(doc,pattern,part);
  return {kind:partKind(part),sourcePattern:pattern,sourcePart:part,data:copy(getPart(doc,pattern,part))};
}
export function replaceFullPart(doc,pattern,part,snapshot){
  check(doc,pattern,part);
  if(!snapshot||snapshot.kind!==partKind(part)||!snapshot.data)throw Error('Clipboard type does not match destination part; no changes made.');
  const candidate=copy(doc),pat=candidate.patterns[pattern];
  if(part<2)pat.bass[part]=copy(snapshot.data);
  else if(part<4)pat.drum[part-2]=copy(snapshot.data);
  else pat.breaks=copy(snapshot.data);
  validateDraft(candidate);
  return candidate;
}
export function clearFullPart(doc,pattern,part){
  return replaceFullPart(doc,pattern,part,takeFullPart(createDraft(),0,part));
}
