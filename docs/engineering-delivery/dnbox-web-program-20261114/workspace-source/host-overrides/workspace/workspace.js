'use strict';
const tools={
 editor:{src:'/tools/offline-sequencer/',title:'Offline pattern editor',text:'Write locally in the corrected editor. Save a JSON draft and reopen it to keep editing. Project export has a narrower format contract; the guide explains the difference from portable project backups.'},
 presets:{src:'/tools/presets/',title:'Temporary DNPR preset librarian',text:'Choose the source-bank profile that matches your file, inspect its warnings, then explicitly Keep it for this temporary session. Eligible user downloads preserve the original bytes and retained validation profile; protected records stay inspectable. No sample audio or keyboard transfer.'},
 performance:{src:'/performance/',title:'DnBOX performance companion',text:'Try the clearly simulated demo first: it makes no audio. A physical session requires your explicit MIDI permission, input/output selection, opening and arming. Choose a known matching firmware profile. Physical MIDI is untested.'}
};
const section=document.querySelector('#tools');
document.querySelector('.skip').addEventListener('click',event=>{
 event.preventDefault();
 section.scrollIntoView({block:'start'});
 section.focus({preventScroll:true});
});
const linkedDocuments=new WeakSet();
function connectToolLinks(frame){
 if(frame.dataset.tool==='performance')return;
 let doc;
 try{doc=frame.contentDocument;if(!doc||doc.location.origin!==location.origin)return;}catch{return;}
 if(linkedDocuments.has(doc))return;
 linkedDocuments.add(doc);
 // Bubble after component handlers; a guarded/default-prevented link stays with its component.
 doc.defaultView.addEventListener('click',event=>{
  if(event.defaultPrevented||event.button!==0||event.metaKey||event.ctrlKey||event.shiftKey||event.altKey)return;
  const link=event.target?.closest?.('a[href]');
  if(!link||link.hasAttribute('download')||(link.target&&link.target!=='_self'))return;
  const destination=new URL(link.href,doc.baseURI);
  if(destination.origin!==location.origin||destination.search||destination.hash)return;
  const path=destination.pathname.replace(/\/index\.html$/,'/');
  const key=path==='/tools/presets/'||path==='/librarian/'?'presets':path==='/tools/offline-sequencer/'||path==='/offline-sequencer/'?'editor':null;
  if(!key||key===frame.dataset.tool)return;
  event.preventDefault();
  // Keep both documents alive so pending saves, previews and temporary presets remain theirs.
  location.hash=key;
 });
}
function showTool(){
 const key=Object.hasOwn(tools,location.hash.slice(1))?location.hash.slice(1):'editor';
 const item=tools[key];
 // Explicitly blur the outgoing frame; the companion owns its release/disarm policy.
 const old=section.querySelector('iframe:not([hidden])');
 if(old&&old.dataset.tool!==key){old.contentWindow?.blur();document.querySelector(`[data-tool="${key}"]`)?.focus({preventScroll:true});}
 let frame=section.querySelector(`iframe[data-tool="${key}"]`);
 if(!frame){frame=document.createElement('iframe');frame.dataset.tool=key;frame.title=item.title;frame.addEventListener('load',()=>connectToolLinks(frame));frame.src=item.src;section.append(frame);}
 for(const f of section.querySelectorAll('iframe'))f.hidden=f!==frame;
 for(const a of document.querySelectorAll('nav [data-tool]')){if(a.dataset.tool===key)a.setAttribute('aria-current','page');else a.removeAttribute('aria-current');}
 document.querySelector('#context').textContent=item.text;
 document.title=`${item.title} — DnBOX local workspace`;
}
addEventListener('hashchange',showTool);showTool();
