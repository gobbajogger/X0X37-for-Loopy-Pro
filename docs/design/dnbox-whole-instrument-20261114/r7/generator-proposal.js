/* SPDX-License-Identifier: GPL-3.0-only */
/* R7 working design: native-root roll, native held values, select-only GEN focus.
 * Public generator mathematics is modeled separately. No firmware/audio/storage executes. */
const r7GenBase={reset,render,header,event,noteDown,noteUp,pageCount,melodic,prepareJourney,setView,applyBoundary};
const GEN_NAMES=[['DENS','ACCENT','SLIDE','OCTS'],['ROOT','SCALE','OCTAVE','MUTATE']];
const GEN_KEYS=[['density','accent','slide','range'],['root','scale','base','mutate']];
const GEN_MAX=[[100,100,100,3],[11,5,4,16]];
const GEN_SCALE_NAMES=['MIN','PHRY','HARM','MPEN','DOR','MAJ'];
function genInfo(b=bassData(),part=S.sourcePart,pattern=S.currentPatterns[part]){
 return b.gen??={density:70,accent:40,slide:25,range:2,root:9,scale:0,base:part===2?1:2,mutate:0,seed:(part===2?0x3b0:0x5a1)+pattern*(part===2?977:613)};
}
function genSeed(b=bassData()){return (genInfo(b).seed&65535).toString(16).toUpperCase().padStart(4,'0');}
function genGridOwner(){return !S.clipOn&&S.view==='GEN'&&S.part>2&&!S.rec&&!S.freePlay[S.sourcePart-2]&&!S.heldPatch&&!S.heldSave&&!S.overlay;}
function genWhiteDown(n){
 const b=bassData(),at=S.stepPage*16+n;if(at>=64)return;S.held=at+1;S.chance=chanceValue();S.velocity=b.notes[at].velocity;
 S.press={input:n,step:at+1,sourcePart:S.sourcePart,pattern:S.pattern,created:false,used:true,genFocus:true,before:structuredClone(b)};
 S.note='Proposed GEN focus: select only; tap and release do not write or delete';
}
function genCommit(before,after,part=S.sourcePart,pattern=S.pattern-1){if(JSON.stringify(before)===JSON.stringify(after))return false;S.genUndo??=[];S.genRedo=[];S.genUndo.push({part,pattern,before,after});return true;}
function genFinishFocus(p){
 if(p.pattern!==S.pattern||p.sourcePart!==S.sourcePart)return;const after=structuredClone(bassData());
 if(genCommit(p.before,after,p.sourcePart,p.pattern-1))S.note='Bounded generator/correction data history; native project byte-pool capacity is not simulated';
}
function genApply(newLine){const b=bassData(),before=structuredClone(b);S.now+=16;S.genEntropy=((S.genEntropy??0x5a1)+1)>>>0;if(newLine){b.gen.seed=genNewSeed(S.genEntropy);genGenerateData(b);}else genMutateData(b,(b.gen.seed^Math.trunc(S.now))>>>0);for(const n of b.notes){delete n.chord;if(newLine||n.gate!=='NOTE')delete n.capturedSoundPresent;}S.genStale[S.sourcePart-2]=false;genCommit(before,structuredClone(b));S.dirty=true;showNotice(newLine?'NEW LINE '+genSeed(b):'MUTATED');S.note='Source generator math; simulated entropy/time, not native wall clock or audio. History capacity and external note-sound storage remain bounded interfaces';}
function genHistory(redo){
 const focus=S.press?.genFocus?S.press:null;if(focus)genFinishFocus(focus);
 const from=redo?(S.genRedo??=[]):(S.genUndo??=[]),to=redo?(S.genUndo??=[]):(S.genRedo??=[]),entry=from.pop();S.usedButtons.push('PATCH');
 if(!entry){S.note='No modeled GEN correction entry; native project history outside this fixture is not simulated';return;}
 S.patternData[entry.pattern].bass[entry.part-2]=structuredClone(redo?entry.after:entry.before);if(focus&&focus.pattern-1===entry.pattern&&focus.sourcePart===entry.part)focus.before=structuredClone(S.patternData[entry.pattern].bass[entry.part-2]);to.push(entry);S.dirty=true;showNotice((redo?'REDO':'UNDO')+' P'+(entry.pattern+1)+' NOTES');
 S.note='One modeled GEN correction history entry; transport/REC unchanged; native history byte pool is not simulated';
}
function genKnownFixture(){
 const b=bassData();b.notes=Array.from({length:64},()=>({gate:'REST',note:45,velocity:100,accent:false,slide:false,end:0}));
 for(const [i,gate,note] of [[0,'NOTE',53],[1,'TIE',45],[2,'NOTE',55],[3,'REST',45],[4,'NOTE',57]])b.notes[i]={...b.notes[i],gate,note};b.length=16;genInfo(b);S.view='GEN';S.page=0;S.playing=false;S.rec=false;S.dirty=true;S.genUndo=[];S.genRedo=[];S.genStale=[false,false];
}
reset=function(fixture='drum'){
 r7GenBase.reset(fixture==='gen-known'?'chicago':fixture);S.physicalOwners={};S.whiteRoleOrigins={};S.whiteRoleTimes={};S.genUndo=[];S.genRedo=[];S.genStale=[false,false];
 for(let p=0;p<32;p++)for(let b=0;b<2;b++){const data=S.patternData[p].bass[b];genInfo(data,b+2,p);for(const n of data.notes)if(n.gate==='REST'){n.note=b===0?33:45;n.rawVelocity=0;}}
 if(fixture==='gen-known')genKnownFixture();remember();render();
};
pageCount=function(){return S.view==='GEN'&&!S.clipOn?2:r7GenBase.pageCount();};
header=function(){
 if(S.view!=='GEN'||S.clipOn)return r7GenBase.header();slab(0,0,240,20);text(laneName(S.part),5,1,14,partInk[S.part],true);text('TB-3PO',96,4,10,C.amber,false,'right');patternPicture(100,2);text(S.pattern,123,3,11,C.value,false,'right');
 if(S.queued)text('›'+S.queued,149,3,11,S.playing&&!S.wait?(S.framePhase&16?C.value:C.gray):C.amber,true,'right');text(S.wait?'WAIT':S.tempo,179,S.wait?3:0,S.wait?11:16,S.wait?C.amber:C.gray,S.wait,'right');if(S.wait){line(184,5,184,13,C.amber);line(189,5,189,13,C.amber);}else triangle(184,4,S.playing?C.value:C.dim);if(S.rec)rect(195,6,5,5,'#f05e56');saveMark();power();
};
function nativeRoll(){
 const b=bassData(),gp=genInfo(b),pitches=[];for(let i=0;i<b.length;i++){const n=resolvedNote(b,i);if(n!==null)pitches.push(n);}
 let lo=(pitches.length?Math.min(...pitches):12*(gp.base+1)+gp.root)-2,hi=(pitches.length?Math.max(...pitches):12*(gp.base+1)+gp.root)+2;if(hi-lo<14)hi=lo+14;
 const ny=n=>20+92-Math.trunc((n-lo)*88/(hi-lo)),col=partInk[S.part];
 for(let n=lo;n<=hi;n++)if((n-gp.root)%12===0){const y=ny(n);line(27,y+2,240,y+2,C.line);text(noteName(n),1,y-4,12,C.dim,true);}
 for(let i=0;i<16;i++){const at=S.stepPage*16+i,x=27+i*13+(i>>2),cell=b.notes[at];if(at>=b.length){rect(x+5,116,2,2,C.line);continue;}
  if(S.playing&&at===S.playhead)rect(x,20,12,98,sourceDim(col,2));const pitch=resolvedNote(b,at);
  if(cell.gate!=='REST'&&pitch!==null){const y=ny(pitch),w=cell.end?Math.trunc((cell.end*12+14)/15):12,ink=S.held===at+1?C.value:cell.accent?col:sourceDim(col,9);stepCell(x,y,w,5,ink,true);
   if(cell.gate==='TIE'){if(i)rect(27+(i-1)*13+((i-1)>>2)+6,y,x-(27+(i-1)*13+((i-1)>>2)+6)+Math.min(w,6),5,ink);else{rect(x-4,y+1,4,3,ink);rect(x-7,y+2,3,1,ink);}}
   if(i===15&&b.notes[at+1<b.length?at+1:0].gate==='TIE'){rect(x+12,y+1,2,3,ink);rect(x+14,y+2,2,1,ink);}if(cell.slide&&i<15)line(x+11,y+2,x+14,ny(b.notes[at+1<b.length?at+1:0].note)+2,C.value);
  }
  stepCell(x+1,121,10,4,cell.gate!=='REST'&&cell.accent?C.value:C.line,cell.gate!=='REST'&&cell.accent);stepCell(x+1,133,10,4,cell.gate!=='REST'&&cell.slide?col:C.line,cell.gate!=='REST'&&cell.slide);
 }
 text('AC',3,118,12,C.gray,true);text('SL',3,130,12,C.gray,true);
 if(S.held>=0||S.rec){const at=S.held>=0?S.held-1:b.write,c=b.notes[at],pitch=(resolvedNote(b,at)??c.note)+b.transpose-24;nativeReadout((S.held>=0?'STEP ':'WRITE ')+(at+1)+' '+noteName(pitch)+(c.gate==='TIE'?' TIE':'')+(c.accent?' ACC':'')+(c.slide?' SLIDE':'')+' V'+c.velocity);}
 else{const root=noteName(gp.root+12).replace(/\d+$/,'')+' '+GEN_SCALE_NAMES[gp.scale];text(root,4,140,15,C.gray);const end=bounds.at(-1).x+bounds.at(-1).width;text('SEED '+genSeed(b),end+18,140,15,C.gray);}
}
function nativeReadout(t){g.font='700 15px Barlow';g.textBaseline='top';g.textAlign='left';g.fillStyle=C.value;g.fillText(t,4,140);bounds.push({text:t,x:4,y:140,width:g.measureText(t).width,height:15});}
function heldStepCards(){
 const b=bassData(),n=b.notes[S.held-1],vals=[noteName((resolvedNote(b,S.held-1)??n.note)+b.transpose-24),n.gate,n.accent?'ON':'OFF',n.slide?'ON':'OFF'];slab(3,158,234,11);text(laneName(S.part)+' STEP '+S.held,8,158,11,C.amber);
 if(S.view==='GEN'){text(noteName(genInfo(b).root+12).replace(/\d+$/,'')+' '+GEN_SCALE_NAMES[genInfo(b).scale]+' '+genSeed(b),220,158,10,C.gray,false,'right');for(let j=0;j<2;j++)rect(229+j*6,162,3,3,j===S.page?C.amber:C.line);}
 for(let i=0;i<4;i++){slab(i*60+3,171,54,55);text(['NOTE','GATE','ACCENT','SLIDE'][i],i*60+30,171,12,C.gray,true,'center');text(vals[i],i*60+30,193,23,C.value,true,'center');}
 slab(0,229,240,11);text('K2: CHANCE   K3: VELOCITY   RELEASE: BACK',4,229,10,C.gray);
}
function genNeutralGlyph(x,q){for(let j=0;j<7;j++){const h=3+j*2;rect(x+j*6,205-h,3,h,q&&j*127/6<=q?partInk[S.part]:C.line);}rect(x,207,37,1,C.line);rect(x+Math.trunc(q*36/127),206,1,3,partInk[S.part]);}
function genNativeValue(t,x,y){g.font='700 15px Barlow';g.textBaseline='top';g.textAlign='center';g.fillStyle=C.amber;g.fillText(String(t),x,y);const width=g.measureText(String(t)).width;bounds.push({text:String(t),x:x-width/2,y,width,height:15});}
function genCards(){
 if(S.held>=0&&S.press?.genFocus){heldStepCards();return;}const gp=genInfo(),page=S.page;slab(3,158,234,11);text('P'+S.pattern+' '+(page?'SCALE':'GENERATE'),8,158,11,C.amber);for(let j=0;j<2;j++)rect(229+j*6,162,3,3,j===page?C.amber:C.line);
 for(let i=0;i<4;i++){const key=GEN_KEYS[page][i],v=gp[key];slab(i*60+3,171,54,55);text(GEN_NAMES[page][i],i*60+30,171,11,C.gray,false,'center');
  if(key==='range'||key==='root'||key==='scale'){const count=key==='range'?3:key==='root'?12:6,pw=count>6?3:6,width=count*(pw+2)-2,left=i*60+30-width/2;for(let j=0;j<count;j++)rect(left+j*(pw+2),191,pw,9,j===(key==='range'?v-1:v)?partInk[S.part]:C.line);}
  else genNeutralGlyph(i*60+12,Math.trunc(v*127/GEN_MAX[page][i]));
  const value=key==='root'?noteName(v+12).replace(/\d+$/,''):key==='scale'?GEN_SCALE_NAMES[v]:key==='base'?v+1:key==='mutate'?v?v+'bar':'OFF':key==='range'?v:v+'%';genNativeValue(value,i*60+30,210);
 }
 slab(0,229,240,11);text('OCT+: NEW'+(S.genStale?.[S.sourcePart-2]?'*':'')+'  OCT−: MUTATE'+(genGridOwner()?'   HOLD: EDIT':S.rec||S.freePlay[S.sourcePart-2]?'   KEYS: SOUND':''),4,229,10,C.amber);
}
function genEdit(k,d){
 const b=bassData();if(S.held>=0&&S.press?.genFocus){const cell=b.notes[S.held-1];if(k===0){cell.note=clamp((resolvedNote(b,S.held-1)??cell.note)+d,12,108);cell.gate='NOTE';}if(k===1)cell.gate=['REST','NOTE','TIE'][clamp(['REST','NOTE','TIE'].indexOf(cell.gate)+Math.sign(d),0,2)];if(k===2)cell.accent=d>0;if(k===3)cell.slide=d>0;S.dirty=true;S.note='GEN cell changes only after an explicit knob turn; generator controls return on release';return;}
 const before=structuredClone(b),key=GEN_KEYS[S.page][k],gp=genInfo();gp[key]=clamp(gp[key]+d,key==='range'?1:0,GEN_MAX[S.page][k]);if(key!=='mutate')S.genStale[S.sourcePart-2]=true;genCommit(before,structuredClone(b));S.dirty=true;S.note='Current generator configuration edit; notes unchanged, no parameter-motion write';
}
event=function(ev,d=1){
 if(!S.clipOn&&!S.overlay&&S.heldPatch&&['SEQ REC','SEQ PLAY'].includes(ev)){genHistory(ev==='SEQ PLAY');render();return;}
 if(ev==='ARP'&&!S.clipOn&&S.overlay!=='ASK'){if(S.part<3){S.note='Current generator requires a bass slot';render();return;}if(S.view==='GEN'&&!S.overlay){S.page=(S.page+1)%2;remember();}else setView('GEN');render();return;}
 if(S.view==='GEN'&&!S.clipOn&&S.overlay!=='ASK'&&ev==='SCALE'){if(!S.overlay){if(S.press?.genFocus)genFinishFocus(S.press);S.press=null;S.held=-1;S.overlay='LIST';S.list=0;}render();return;}
 if(S.view==='GEN'&&!S.clipOn&&S.overlay==='LIST'&&/^K[5-8]$/.test(ev)){genEdit(+ev[1]-5,d);render();return;}
 if(S.view==='GEN'&&!S.clipOn&&!S.overlay){
  if(/^K[5-8]$/.test(ev)){genEdit(+ev[1]-5,d);render();return;}
  if(ev==='K3'&&S.held<0&&clamp(S.part+Math.sign(d),0,4)<3){remember();S.part=clamp(S.part+Math.sign(d),0,4);S.liveNotes={};S.liveEpoch++;setView('PART');render();return;}
  if(ev==='K3'&&S.held>=0&&S.press?.genFocus){const c=bassData().notes[S.held-1];c.velocity=clamp(c.velocity+d,1,127);c.rawVelocity=c.velocity;S.dirty=true;render();return;}
  if((ev==='OCTAVE+'||ev==='OCTAVE-')&&S.held<0&&!S.heldSave){genApply(ev==='OCTAVE+');render();return;}
 }
 const wasRecTransition=ev==='SEQ REC'&&!S.clipOn&&S.overlay!=='ASK',heldBefore=S.view==='SEQ'&&S.part>2&&S.held>=0;const result=r7GenBase.event(ev,d);if(ev==='K3'&&heldBefore){const n=bassData().notes[S.held-1];if(n)n.rawVelocity=n.velocity;}if(wasRecTransition){S.held=-1;S.press=null;render();}return result;
};
noteDown=function(physical,velocity=100,origin='key'){
 if(S.view!=='GEN')return r7GenBase.noteDown(physical,velocity,origin);
 if(!S.rec&&!S.freePlay[S.sourcePart-2]){S.note='GEN default keys are select-only in this proposed route';return;}
 if(physical<5||physical>31){S.note='GEN legacy keyboard uses central27; no outer-key source route';return;}
 const origins=S.keyOrigins[physical]??{},wasHeld=Object.keys(origins).length>0;origins[origin]=true;S.keyOrigins[physical]=origins;if(wasHeld)return;
 S.liveNotes[physical]={part:S.sourcePart,note:36+physical+12*S.octave,velocity,epoch:S.liveEpoch};S.note='Current GEN audition route; it is not local SEQ note capture';render();
};
function drawGenerator(){nativeRoll();genCards();}
render=function(){return r7GenBase.render();};
melodic=function(){nativeRoll();if(S.held>=0){heldStepCards();return;}const b=bassData(),names=[['LENGTH','RATE','DIRECTION','TRANSP'],['PAT SOUND',null,null,null],['FREE PLAY',null,null,null]][S.page],vals=S.page===0?[b.length+'st',D.rates[b.rate],['FWD','REV','PING','RND'][b.direction],(b.transpose-24)+'st']:S.page===2?[S.freePlay[S.sourcePart-2]?'ON':'OFF',null,null,null]:['OFF',null,null,null];cards(['P'+S.pattern+' SEQUENCE','PRESET','KEYS'][S.page],names,vals,S.rec?'CHORD+OCTAVE: REST  NOTE REPEAT+OCTAVE: TIE':'PRESS: ON  TAP: OFF  HOLD+KNOBS: EDIT');};
const r7GenInspector={pages,deepRows,fixtureValue,editList,control,controlText};
function genRows(){return GEN_NAMES.flatMap((names,p)=>names.map((label,k)=>({label,page:p,k,editable:true,parameter_id:'GEN:'+GEN_KEYS[p][k],kind:'R_GEN'})));}
function genValueText(page,k){const key=GEN_KEYS[page][k],v=genInfo()[key];return key==='root'?noteName(v+12).replace(/\d+$/,''):key==='scale'?GEN_SCALE_NAMES[v]:key==='base'?String(v+1):key==='mutate'?(v?v+'bar':'OFF'):key==='range'?String(v):v+'%';}
pages=function(){return S.view==='GEN'?[0,1].map(p=>({title:p?'SCALE':'GENERATE',cells:genRows().filter(c=>c.page===p)})):r7GenInspector.pages();};
deepRows=function(){return S.view==='GEN'?genRows():r7GenInspector.deepRows();};
fixtureValue=function(label,p,k){return S.view==='GEN'?genValueText(p,k):r7GenInspector.fixtureValue(label,p,k);};
editList=function(d){if(S.view!=='GEN')return r7GenInspector.editList(d);const row=genRows()[S.list],page=S.page;if(!row)return 'Unmapped';S.page=row.page;genEdit(row.k,d);S.page=page;return 'Current GEN scalar ref edited; original lower page retained';};
control=function(p,k){if(S.view!=='GEN')return r7GenInspector.control(p,k);const row=genRows().find(c=>c.page===p&&c.k===k);if(!row)return null;return {...row,domain:'generator',id:row.parameter_id,q:genInfo()[GEN_KEYS[p][k]],scope:'pattern',owner:{part:S.sourcePart,pattern:S.currentPatterns[S.sourcePart]},key:row.parameter_id};};
controlText=function(c){return c?.domain==='generator'?genValueText(c.page,c.k):r7GenInspector.controlText(c);};
const r7GenLights=updateLights;updateLights=function(){if(S.view!=='GEN'||S.clipOn)return r7GenLights();const result=r7GenLights();result.buttons.ARP=true;result.buttons.SEQ=false;for(const name of ['ARP','SEQ']){const el=document.querySelector('#panelButtons [data-ev=\"'+name+'\"]');if(el){el.setAttribute('data-lit',String(result.buttons[name]));el.setAttribute('aria-label',name+'; source LED '+(result.buttons[name]?'on':'off'));}}return result;};
setView=function(v){if(S.press?.genFocus)genFinishFocus(S.press);return r7GenBase.setView(v);};
applyBoundary=function(reason){const p=S.press;if(p?.genFocus&&S.pendingPatterns[p.sourcePart]>=0&&S.pendingPatterns[p.sourcePart]!==p.pattern-1)genFinishFocus(p);return r7GenBase.applyBoundary(reason);};
prepareJourney=function(id){if(id==='gen'){reset('gen-known');return;}return r7GenBase.prepareJourney(id);};
Object.assign(window.proposal,{reset,render,event,noteDown,noteUp,pageCount,genKnownFixture,pages,control,controlText,prepareJourney,boundary:applyBoundary,lights:updateLights});
