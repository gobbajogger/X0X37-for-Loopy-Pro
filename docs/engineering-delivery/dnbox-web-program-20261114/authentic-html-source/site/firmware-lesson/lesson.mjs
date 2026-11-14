// SPDX-License-Identifier: GPL-3.0-only
// Website-owned adapter. PR899 runtime, firmware, LCD and source controls are unchanged.
import {nativeFrame,HTMLScreen,qualify909,qualify909Family,observedMessage,readBD,equalRect} from './screen.mjs';
const $=s=>document.querySelector(s),frame=$('#native-runtime'),display=new HTMLScreen($('#lcd'));
const state={booted:false,qualified:false,canSelectBD:false,saveMessage:null,bd:[],rgba:null,frames:0,renderTimes:[],pending:null,events:[],errors:[]};
let doc=null,references=[],rowLanes=[],saveMessages=[],off=null,on=null,held=null,lastFrame=null,timer=null,readyTimer=null,attachedDoc=null,startupWatch=null,activation=0,bootAttempted=false;
const momentaries=[];
const decode=s=>Uint8ClampedArray.from(atob(s),c=>c.charCodeAt(0));
const say=text=>$('#outcome').textContent=text;
const input=detail=>{
 if(!state.booted||!doc)return false;
 state.events.push({at:performance.now(),...detail});
 doc.dispatchEvent(new frame.contentWindow.CustomEvent('dnbox:input',{detail}));return true;
};
function release(owner){
 if(typeof owner==='string'&&held?.owner!==owner)return;
 if(held){input({kind:held.kind,id:held.id,down:false});held=null;}
 momentaries.forEach(b=>b.setAttribute('aria-pressed','false'));
}
function pending(label,goal=null){state.pending={label,goal,before:state.rgba?.slice(),at:performance.now()};say(label+' · sent; waiting for the actual screen.');}
function press(button,kind,id,label){
 button.id=`physical-${kind}-${id}`;
 button.disabled=true;button.setAttribute('aria-pressed','false');momentaries.push(button);
 const down=(e,owner)=>{
  if(!state.booted||held)return;e?.preventDefault();
  held={kind,id,owner};pending(label);input({kind,id,down:true});button.setAttribute('aria-pressed','true');
 };
 button.addEventListener('pointerdown',e=>{const owner=`${kind}:${id}:pointer:${e.pointerId}`;down(e,owner);if(held?.owner===owner){try{button.setPointerCapture(e.pointerId);}catch{}}});
 for(const type of ['pointerup','pointercancel','lostpointercapture'])button.addEventListener(type,e=>release(`${kind}:${id}:pointer:${e.pointerId}`));
 button.addEventListener('keydown',e=>{if([' ','Enter'].includes(e.key)&&!e.repeat)down(e,`${kind}:${id}:key:${e.key}`);});
 button.addEventListener('keyup',e=>{if([' ','Enter'].includes(e.key)){e.preventDefault();release(`${kind}:${id}:key:${e.key}`);}});
 button.addEventListener('blur',()=>{release(`${kind}:${id}:key: `);release(`${kind}:${id}:key:Enter`);});
 // Click with detail0 supplies native AT/keyboard activation without an invented hold.
 button.addEventListener('click',e=>{if(e.detail===0&&!held&&state.booted){const owner=`${kind}:${id}:AT:${++activation}`;down(null,owner);setTimeout(()=>release(owner),180);}});
}
async function tap(kind,id,label,goal=null){
 if(!state.booted||held)return;
 const owner=`tap:${kind}:${id}:${++activation}`;held={kind,id,owner};pending(label,goal);input({kind,id,down:true});
 await new Promise(resolve=>setTimeout(resolve,180));release(owner);
}
const defaultRoles=['Master','Select','Algorithm','Presets','Parameter 1','Parameter 2','Parameter 3','Parameter 4'];
for(let id=0;id<8;id++){
 const group=document.createElement('div');group.className='encoder';
 const label=document.createElement('b');label.textContent='K'+(id+1);
 const role=document.createElement('small');role.textContent=defaultRoles[id];group.append(label,role);
 for(const clicks of [-1,1]){const b=document.createElement('button');b.id=`encoder-${id}-${clicks<0?'left':'right'}`;b.textContent=clicks<0?'−':'+';b.disabled=true;b.dataset.runtimeControl='true';b.setAttribute('aria-label',`K${id+1} ${defaultRoles[id]}, one click ${clicks<0?'left':'right'}`);b.addEventListener('click',()=>{if(held)return;pending(`K${id+1}, one ${clicks<0?'left':'right'} click`);input({kind:'encoder',id,clicks});});group.append(b);}
 $('#encoders').append(group);
}
['ARP','NOTE REPEAT','SCALE','CHORD','GLOBE','BT / SAVE','PATCH','PARA','FX','SEQ','SEQ PLAY','SEQ REC'].forEach((name,i)=>{
 const b=document.createElement('button');b.textContent=name;b.dataset.runtimeControl='true';press(b,'button',43+i,name);$('#buttons').append(b);
});
for(let id=0;id<16;id++){const b=document.createElement('button');b.textContent=String(id+1);b.dataset.runtimeControl='true';b.setAttribute('aria-label',`Physical pad ${id+1}`);press(b,'pad',id,'Pad '+(id+1));$('#pads').append(b);}
for(const step of [1,5,9,13]){
 const b=document.createElement('button');b.textContent='Step '+step;b.dataset.step=String(step);b.disabled=true;
 b.id='place-step-'+step;b.addEventListener('click',()=>{
  const value=state.bd[step-1];
  if(!state.qualified||value===null||value===undefined){say('The actual 909 BD state is unconfirmed. Use the original panel to find 909 SEQ, Pattern 1, REC off.');return;}
  if(value){say('BD step '+step+' is already on. It stays on; no toggle was sent.');return;}
  tap('pad',step-1,'Place BD step '+step,{step});
 });$('#instrument .beat-actions').append(b);
}
$('#select-bd').addEventListener('click',()=>{if(state.canSelectBD)tap('key',6,'F♯3 selects BD',{selectBD:true});});
$('#play').addEventListener('click',()=>tap('button',53,'SEQ PLAY'));
$('#save').addEventListener('click',()=>tap('button',48,'Short BT / SAVE request',{save:true}));
$('#backup').addEventListener('click',()=>{if(!state.booted)return;doc?.querySelector('#export-save').click();$('#backup-status').textContent='Browser-backup download requested. Check the downloaded .fm1save; this is separate from a physical keyboard backup.';});
function enable(){
 document.querySelectorAll('[data-runtime-control]').forEach(b=>b.disabled=!state.booted);
 $('#play').disabled=!state.booted;$('#save').disabled=!state.booted;$('#backup').disabled=!state.booted;
 $('#select-bd').disabled=!state.canSelectBD;
 document.querySelectorAll('.beat-actions button').forEach(b=>{const v=state.bd[Number(b.dataset.step)-1];b.disabled=!state.qualified||v===null||v===undefined;b.dataset.on=String(v===true);b.setAttribute('aria-label',`BD step ${b.dataset.step}: ${v===true?'on; keep hit':v===false?'off; place hit':'unconfirmed'}`);});
}
function scale(){
 const space=$('.lcd-space'),r=space.getBoundingClientRect(),dpr=devicePixelRatio||1,lcd=$('#lcd');
 lcd.style.transform=`scale(${space.clientWidth/240})`;
 // Align the native origin to a physical pixel; text above may have fractional line heights.
 lcd.style.left=(Math.round(r.left*dpr)/dpr-r.left)+'px';
 lcd.style.top=(Math.round(r.top*dpr)/dpr-r.top)+'px';
}
new ResizeObserver(scale).observe($('.lcd-space'));scale();
function observe(){
 // The original runtime uses its default panel size until the board boots.
 // That pre-boot canvas is not an eligible LCD readback, not a display failure.
 if(!doc||document.hidden||!state.booted)return;
 try{
  const rgba=nativeFrame(doc.querySelector('#panel'));
  state.rgba=rgba;state.frames++;
  const qualified=state.booted&&references.length>0&&qualify909(rgba,references);
  const changed=display.render(rgba,qualified);state.renderTimes.push(display.renderMs);
  if(state.renderTimes.length>500)state.renderTimes.shift();
  state.qualified=qualified;state.bd=qualified?readBD(rgba,off,on):[];
  state.canSelectBD=state.booted&&qualify909Family(rgba,references,rowLanes);
  state.saveMessage=observedMessage(rgba,saveMessages);
  const text=state.saveMessage?`Firmware reports: ${state.saveMessage}. ${state.saveMessage==='SAVED'?'Firmware save confirmed; browser backup is separate.':'Save is waiting, not confirmed.'}`:qualified?`909 SEQ · BD · Pattern 1 · steps 1–16 · ${state.bd.map((v,i)=>v?i+1:null).filter(Boolean).join(', ')||'no confirmed hits'} on.`:state.canSelectBD?'909 SEQ, Pattern 1, first page: another drum row is selected. Use F♯3 to return to BD.':'Live firmware screen. This layout or status message is unclassified; the requested result is unconfirmed.';
  if($('#screen-state').textContent!==text)$('#screen-state').textContent=text;
  $('#lcd').dataset.layout=qualified?'909-seq':'unclassified';
  $('#lcd [data-region="header"]')?.setAttribute('aria-label',state.saveMessage?'Actual firmware status: '+state.saveMessage:'Native header; layout unclassified');
  if(qualified)for(let i=0;i<16;i++)$('#lcd').querySelector(`[data-region="cell-0-${i}"]`)?.setAttribute('aria-label',`BD step ${i+1}: ${state.bd[i]===true?'on':state.bd[i]===false?'off':'unconfirmed'}`);
  if(state.pending){
   const goal=state.pending.goal;
   const confirmed=goal?.selectBD?qualified:goal?.save?state.saveMessage&&state.pending.before&&!equalRect(rgba,state.pending.before,0,0,240,20):goal?.step?qualified&&state.bd[goal.step-1]===true:state.pending.before&&!equalRect(rgba,state.pending.before,0,0,240,240);
   if(confirmed){
    const marker=$('#response-marker');marker.classList.remove('observed');void marker.offsetWidth;marker.classList.add('observed');
    say(goal?.selectBD?'BD selection confirmed by the returned 909 display.':goal?.save?`Firmware reports: ${state.saveMessage}. ${state.saveMessage==='SAVED'?'Firmware save confirmed.':'Save is waiting, not confirmed.'}`:goal?.step?`BD step ${goal.step} is on in the returned firmware display.`:state.pending.label+' · native screen updates observed. Verify the requested result on the display; no causal or save confirmation is inferred.');
    state.pending=null;
   }else if(performance.now()-state.pending.at>2500){say(state.pending.label+' · no screen change confirmed. A press alone does not prove an outcome.');state.pending=null;}
  }
  lastFrame=rgba;enable();
 }catch(error){state.errors.push(String(error));state.qualified=false;state.canSelectBD=false;state.bd=[];release();state.booted=false;enable();$('#runtime-state').textContent='Display unavailable';$('#screen-state').textContent=error.message+'. No inferred display is shown.';$('#lcd').replaceChildren();clearInterval(timer);}
}
function runtimeUnavailable(error){
 release();state.booted=false;state.qualified=false;state.canSelectBD=false;state.saveMessage=null;state.pending=null;state.bd=[];
 clearInterval(timer);clearInterval(readyTimer);clearTimeout(startupWatch);enable();
 state.errors.push(String(error));$('#boot').hidden=false;$('#boot').disabled=true;
 $('#runtime-state').textContent='Runtime unavailable';$('#screen-state').textContent='No live firmware display is available. Lesson actions are disabled.';
 $('#runtime-error-text').textContent='The local runtime could not be loaded or did not match this lesson. Reload after it is available, or use the matching playing guide. Your physical keyboard has not been changed.';
 $('#runtime-error').hidden=false;$('#lcd').replaceChildren();
}
function attach(){
 try{
 doc=frame.contentDocument;
 // about:blank is the initial in-flight document, not a loaded runtime failure.
 if(doc?.URL==='about:blank')return;
 if(!doc||doc.documentElement.dataset.dnbox!=='true'||doc.documentElement.dataset.device!=='smk37'||!doc.querySelector('canvas#panel')||!['#start','#status','#save-status','#export-save'].every(selector=>doc.querySelector(selector)))throw Error('Required pinned runtime document is unavailable');
 if(attachedDoc===doc)return;attachedDoc=doc;clearInterval(timer);clearInterval(readyTimer);clearTimeout(startupWatch);
 $('#runtime-error').hidden=true;
 // Presentation-only adapter. The original JS, WASM, CPU, firmware and controls are not patched.
 const style=doc.createElement('style');style.textContent='header.site,.intro,.metrics,.metric-footnote,.panel-help,.library,.guide-panel,.play-controls,footer,.deck-head{display:none!important}.player{display:block!important;padding:0!important}.wrap,main{max-width:none!important}.deck{padding:0!important}.stage{margin:0!important}body{padding:0!important}.side{display:block!important}.side>section{display:none!important}.side>section:has(#save-status){display:block!important}';doc.head.append(style);
 doc.querySelector('#start').addEventListener('click',()=>{bootAttempted=true;});
 doc.addEventListener('dnbox:booting',()=>{release();bootAttempted=true;state.booted=false;state.qualified=false;state.canSelectBD=false;enable();$('#boot').disabled=true;$('#runtime-state').textContent='Booting real firmware…';});
 doc.addEventListener('dnbox:booted',e=>{state.booted=e.detail?.board===1;$('#runtime-state').textContent=state.booted?'Pro · 0.0.25.1 running':'Unsupported board';enable();});
 doc.addEventListener('dnbox:stopped',()=>{release();state.booted=false;state.qualified=false;state.canSelectBD=false;enable();$('#runtime-state').textContent='Emulator stopped';});
 readyTimer=setInterval(()=>{
  const nativeStart=doc.querySelector('#start'),button=$('#boot');
  if(doc.documentElement.dataset.ready==='error'){runtimeUnavailable(Error('Runtime prerequisites unavailable'));return;}
  button.hidden=state.booted||nativeStart.hidden;
  button.disabled=doc.documentElement.dataset.ready!=='true'||nativeStart.disabled;
  button.textContent=bootAttempted?'Retry sound & firmware':'Enable sound & start firmware';
  if(!state.booted&&doc.documentElement.dataset.ready==='true')$('#runtime-state').textContent=doc.querySelector('#status').textContent;
  const browserStatus=doc.querySelector('#save-status')?.textContent||'';
  if($('#browser-save-status').textContent!==browserStatus)$('#browser-save-status').textContent=browserStatus;
 },100);
 timer=setInterval(observe,100);
 }catch(error){runtimeUnavailable(error);}
}
$('#boot').addEventListener('click',()=>{release();bootAttempted=true;$('#boot').disabled=true;doc?.querySelector('#start').click();});
frame.addEventListener('load',attach);
frame.addEventListener('error',()=>runtimeUnavailable(Error('Local runtime frame failed to load')));
$('#runtime-reload').addEventListener('click',()=>location.reload());
startupWatch=setTimeout(()=>{if(!attachedDoc)runtimeUnavailable(Error('Local runtime did not finish loading'));},20000);
// A fast404 can finish before this module imports. Validate it too, rather than
// requiring the very canvas whose absence contract validation must report.
if(frame.contentDocument?.readyState==='complete'&&frame.contentDocument.URL!=='about:blank')attach();
function stage(mode='initial'){release();const name=location.hash.slice(1)||'place';
 const copy={place:['Select the kick. Place four steps.','909 SEQ, BD selected, first page. Each Place button reads the actual cell first and leaves an existing hit intact.'],hear:['Hear the space between.','Press SEQ PLAY once. Watch the actual firmware playhead; press again to stop. A moving picture is not an acoustic-quality guarantee.'],keep:['Request the save. Read the reply.','Stop transport, turn REC off and release held controls. Short-press BT / SAVE. Read the actual SAVED, waiting or failed message; the guide does not invent a confirmation.']};
 const [title,text]=copy[name]||copy.place;$('#lesson-copy').replaceChildren();const h=document.createElement('h3'),p=document.createElement('p');h.id='lesson-heading';h.tabIndex=-1;h.textContent=title;p.textContent=text;$('#lesson-copy').append(h,p);
 document.querySelectorAll('.steps a').forEach(a=>{if(a.hash==='#'+name)a.setAttribute('aria-current','step');else a.removeAttribute('aria-current');});
 if(mode==='navigate')h.focus({preventScroll:true});
}
history.scrollRestoration='manual';let phaseURL=location.href,phaseRestoring=false,scrollPending=false;
function rememberPhase(){
 if(phaseRestoring||phaseURL!==location.href)return;
 const active=document.activeElement,focus=active?.id||null;
 const details=[...document.querySelectorAll('details')].map((d,index)=>({id:d.id,index,open:d.open}));
 history.replaceState({...history.state,dnboxLesson:{y:scrollY,focus,details}},'');
}
function showPhase(mode){
 phaseRestoring=true;stage(mode);phaseURL=location.href;
 if(mode==='history'){
  const saved=history.state?.dnboxLesson;
  const details=document.querySelectorAll('details');
  saved?.details?.forEach(item=>{const d=item.id?document.getElementById(item.id):details[item.index];if(d?.matches('details'))d.open=item.open;});
  requestAnimationFrame(()=>{const target=saved?.focus&&document.getElementById(saved.focus);if(target)target.focus({preventScroll:true});else document.activeElement?.blur();scrollTo(0,saved?.y||0);phaseRestoring=false;});
 }else{phaseRestoring=false;if(mode==='navigate')rememberPhase();}
}
document.querySelectorAll('.steps a').forEach(a=>{a.id='phase-'+a.hash.slice(1);a.addEventListener('click',e=>{
 if(e.button||e.metaKey||e.ctrlKey||e.altKey||e.shiftKey||a.href===location.href)return;
 e.preventDefault();rememberPhase();history.pushState({...history.state,dnboxLesson:{y:scrollY,focus:null}},'',a.hash);showPhase('navigate');
});});
if(!history.state?.dnboxLesson)history.replaceState({...history.state,dnboxLesson:{y:scrollY,focus:null}},'');
window.addEventListener('popstate',()=>showPhase('history'));
window.addEventListener('hashchange',()=>{if(phaseURL!==location.href){history.replaceState({...history.state,dnboxLesson:{y:0,focus:null}},'');showPhase('navigate');}});
window.addEventListener('scroll',()=>{if(scrollPending)return;scrollPending=true;requestAnimationFrame(()=>{scrollPending=false;rememberPhase();});},{passive:true});
document.addEventListener('focusin',rememberPhase);
$('.skip').addEventListener('click',e=>{e.preventDefault();$('#instrument').focus({preventScroll:true});$('#instrument').scrollIntoView({block:'start'});rememberPhase();});
document.addEventListener('toggle',e=>{if(e.target.matches('details'))rememberPhase();},true);
window.addEventListener('pagehide',rememberPhase);
window.addEventListener('pageshow',e=>{if(e.persisted)showPhase('history');});
showPhase(performance.getEntriesByType('navigation')[0]?.type==='back_forward'?'history':'initial');
for(const type of ['blur','pagehide'])window.addEventListener(type,release);
document.addEventListener('visibilitychange',()=>{if(document.hidden)release();});
try{const r=await fetch('references.json');if(!r.ok)throw Error('Reference HTTP '+r.status);const data=await r.json();references=data.signatures.map(decode);rowLanes=data.rowLanes.map(decode);saveMessages=data.saveMessages.map(m=>({text:m.text,header:decode(m.header)}));off=decode(data.off);on=decode(data.on);}catch(error){state.errors.push(String(error));$('#coverage').textContent='Semantic reference unavailable. Native HTML pixels only; no step-placement checks.';}
window.firmwareLesson={state,get frame(){return state.rgba;},get held(){return held;},release,
 // Inspection/test snapshot pauses this website observer, never the machine or framebuffer.
 snapshot(){clearInterval(timer);return state.rgba?Array.from(state.rgba):null;},
 resume(){clearInterval(timer);timer=setInterval(observe,100);}
};
