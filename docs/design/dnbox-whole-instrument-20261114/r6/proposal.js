/* SPDX-License-Identifier: GPL-3.0-only */
/* Qualitative UI projections from pinned3c41 ui.c/ui_original_controls.h and juno6.c; not audio execution. */
// Source-derived proposal state. part is display position; sourcePart preserves firmware lane IDs.
const BLACK_LOGICAL=[1,3,5,8,10,13,15,17,20,22,25];
const WHITE_LOGICAL=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26];
const SOURCE_PART=[0,1,4,3,2], clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const CHI_Q=[0,18,32,95,99,105,0,34,51,72,22,0,102,0,0,0,0,0,0,0];
const MIA_Q=[0,100,0,64,0,100,0,64,0,76,0,0,64,0,100,0];
const JUN_Q=[42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100];

/* Design model only. Existing physical routes are retained; no firmware or audio. */
const C={bg:'#050a0d',face:'#101a1f',edge:'#26383d',side:'#070e11',sel:'#253b2d',selEdge:'#3e5c40',ink:'#e0ebd6',value:'#dee6de',white:'#dee7df',gray:'#9caaa8',dim:'#809294',line:'#28383d',green:'#8fbf79',amber:'#c49b45'};
const partInk=['#ff9529','#ff4841','#d579ff','#41ceff','#83f25a'];
const drumNames=['KICK','SNARE','LOW TOM','MID TOM','HIGH TOM','RIMSHOT','HANDCLAP','CLOSED HAT','OPEN HAT','CRASH','RIDE'];
const blacks=['F#3','G#3','A#3','C#4','D#4','F#4','G#4','A#4','C#5','D#5','F#5'];
const $=id=>document.getElementById(id),cv=$('lcd'),g=cv.getContext('2d');
const clipImages={};for(const [k,src] of Object.entries(D.nativeClips||{})){const im=new Image();im.src=src;clipImages[k]=im;}
let S,trace=[],touchTimer=null,noticeTimer=null;const bounds=[];
function reset(fixture='drum'){if(noticeTimer){clearTimeout(noticeTimer);noticeTimer=null;}if(touchTimer){clearTimeout(touchTimer);touchTimer=null;}
 if(fixture==='drone')throw new RangeError('Active Drone fixture withdrawn; saved-unavailable/fallback contract remains with firmware owner.');
 const kit=()=>({track:0,steps:[[1,5,9,13],[],[],[],[],[],[],[],[],[],[]],length:16,rate:0,playhead:4,mix:Array.from({length:11},()=>[61,64,0,0])});
 S={touched:null,view:'SEQ',part:0,page:0,pageMemory:{},banks:{3:'CHICAGO',4:'MIAMI'},controls:{3:[...CHI_Q],4:[...MIA_Q]},kits:[kit(),kit()],mixsel:0,mixPart:Array.from({length:5},()=>[100,64,0,0]),stepPage:0,melodicLength:16,playing:true,wait:false,rec:false,held:-1,heldInput:-1,chance:100,velocity:100,muted:[],pattern:1,queued:0,power:3,tempo:125,swingRaw:0,pumpRaw:21,metroView:0,filter:64,lightsEnabled:true,dirty:true,overlay:null,list:0,save:null,values:{},locks:{},motion:{},variants:{toms:0,rim:0,clap:0},heldPatch:false,songData:[],songSel:0,songPos:0,songOn:false,currentPatterns:[0,0,0,0,0],patternBank:0,usedPatterns:[1],now:0,press:null,framePhase:0,note:'Controlled source-value fixture; no audio, recording or storage',cueEvidence:'Qualitative source-law diagrams; no measured live cursor/native factory parity'};
 Object.defineProperties(S,{songBars:{enumerable:true,get(){return this.songData.length;},set(v){v=clamp(v,0,192);while(this.songData.length<v)songFill(this.songData.length);this.songData.length=v;this.songSel=clamp(this.songSel,0,Math.min(v,191));}},swing:{enumerable:true,get(){return 50+Math.floor(this.swingRaw/4);},set(v){this.swingRaw=clamp(Math.round((v-50)*4),0,100);}},pump:{enumerable:true,get(){return 24*this.pumpRaw/127;},set(v){this.pumpRaw=clamp(Math.round(v*127/24),0,127);}},engine:{enumerable:true,get(){return this.part<3?['909','808','BREAK'][this.part]:this.banks[this.part];}},sourcePart:{enumerable:true,get(){return SOURCE_PART[this.part];}},track:{enumerable:true,get(){return this.kits[selectedKit()].track;},set(v){this.kits[selectedKit()].track=clamp(v,0,10);}},steps:{enumerable:true,get(){return this.kits[selectedKit()].steps;},set(v){this.kits[selectedKit()].steps=v;}},length:{enumerable:true,get(){return this.part===2&&mixKit()<0?16:this.part<2||mixKit()>=0?this.kits[selectedKit()].length:this.melodicLength;},set(v){if(this.part<2||mixKit()>=0)this.kits[selectedKit()].length=v;else this.melodicLength=v;}},rate:{enumerable:true,get(){return this.part>2&&mixKit()<0?bassData().rate:this.kits[selectedKit()].rate;},set(v){if(this.part>2&&mixKit()<0)bassData().rate=v;else this.kits[selectedKit()].rate=v;}},playhead:{enumerable:true,get(){return this.playheads?this.playheads[this.sourcePart]:this.kits[selectedKit()].playhead;},set(v){if(this.playheads)this.playheads[this.sourcePart]=v;else this.kits[selectedKit()].playhead=v;}}});
 if(fixture==='808variant'){S.part=1;S.track=2;S.variants={toms:1,rim:1,clap:1};}
 if(['chicago','miami','juno','lock','melodic','chicago-envelope','juno-envelope'].includes(fixture)){S.part=['miami','juno','juno-envelope'].includes(fixture)?4:3;S.view='PART';S.playing=false;if(fixture.startsWith('juno')){S.banks[4]='JUNO';S.controls[4]=[...JUN_Q];}S.page=fixture==='lock'?1:fixture.endsWith('envelope')?(fixture.startsWith('juno')?2:3):0;}
 if(['home','wait','savefail'].includes(fixture)){S.view='HOME';S.playing=['home','wait'].includes(fixture);S.wait=fixture==='wait';S.save=fixture==='savefail'?'SAVE FAILED':null;}
 if(fixture==='fresh'){S.playing=false;S.dirty=false;S.kits[0].steps=Array.from({length:11},()=>[]);S.usedPatterns=[];}
 if(['drum-para','snare-para'].includes(fixture)){S.view='PART';S.playing=false;if(fixture==='snare-para')S.track=1;}
 if(fixture==='song'){S.view='SONG';S.playing=false;S.songOn=true;}
 if(fixture==='break'){S.part=2;S.view='PART';S.playing=false;}
 if(fixture==='fx'){S.view='FX';S.page=1;}
 if(['mix','mix909','mix808'].includes(fixture)){S.view='MIX';S.page=fixture==='mix909'?1:fixture==='mix808'?2:0;S.playing=false;}
 if(fixture==='load'){S.view='HOME';S.playing=false;S.overlay='ASK';}
 for(let kit=0;kit<2;kit++)S.kits[kit].parameters=D.typedDrums[kit].records.map(r=>Object.fromEntries(r.parameters.map(p=>[p.local_id,p.fresh_project_default_raw])));
 S.kits[0].mix=[61,58,52,52,52,103,122,89,60,47,42].map(v=>[v,64,0,0]);S.kits[1].mix=Array.from({length:11},()=>[64,64,0,0]);S.kits[1].steps[0]=[];initMusicStore();remember();trace=[];sync();render();
}

function newBass(){return {length:16,rate:0,direction:0,transpose:24,write:0,notes:Array.from({length:64},()=>({gate:'REST',note:48,velocity:100,accent:false,slide:false,end:0}))};}
function initMusicStore(){
 const seed=S.kits.map(k=>({steps:structuredClone(k.steps),accent:[],length:k.length,rate:k.rate}));
 S.patternData=Array.from({length:32},(_,i)=>({drums:[0,1].map(k=>i===0?structuredClone(seed[k]):{steps:Array.from({length:11},()=>[]),accent:[],length:16,rate:0}),bass:[newBass(),newBass()],brk:{steps:[],slice:Array(16).fill(0),length:16}}));
 S.playheads=[4,4,4,4,4];S.pendingPatterns=Array(5).fill(-1);S.partMutes=0;S.pendingPartMutes=0;S.trackMutes=[0,0];S.pendingTrackMutes=[0,0];S.queuedMutes=false;S.chain=null;S.chainFirst=-1;S.recordOwner=null;S.octave=0;S.freePlay=[false,false];S.liveNotes={};S.keyOrigins={};S.padKeyOwners={};S.physicalPadsHeld=[];S.lastPadInput=null;S.liveEpoch=1;S.buttons=[];S.usedButtons=[];S.panicLatched=false;S.heldSave=false;S.motionLanes={};S.motionCapacity=160;S.lastMotionKey=null;S.notice=null;S.copyRequest=null;S.askType=null;S.clockSource=S.wait?'EXTERNAL':'INTERNAL';S.clockSignal=!S.wait;S.clipOn=false;S.clipPage=null;S.clipRequests=[];S.localReturn=null;S.breakPage=0;S.breakLive=null;
 for(let k=0;k<2;k++)for(const prop of ['steps','accent','length','rate','playhead'])Object.defineProperty(S.kits[k],prop,{enumerable:true,configurable:true,get(){return prop==='playhead'?S.playheads[k]:S.patternData[S.currentPatterns[k]].drums[k][prop];},set(v){if(prop==='playhead')S.playheads[k]=v;else S.patternData[S.currentPatterns[k]].drums[k][prop]=v;}});
 for(const prop of ['pattern','queued','melodicLength','usedPatterns'])delete S[prop];
 Object.defineProperties(S,{
  pattern:{enumerable:true,configurable:true,get(){return this.currentPatterns[['PART','SEQ','GEN'].includes(this.view)?this.sourcePart:0]+1;},set(v){this.currentPatterns[['PART','SEQ','GEN'].includes(this.view)?this.sourcePart:0]=clamp(v-1,0,31);}},
  queued:{enumerable:true,configurable:true,get(){const p=this.pendingPatterns[['PART','SEQ','GEN'].includes(this.view)?this.sourcePart:0];return p<0?0:p+1;},set(v){const p=v?clamp(v-1,0,31):-1;if(!['PART','SEQ','GEN'].includes(this.view)||this.heldPatch)this.pendingPatterns.fill(p);else this.pendingPatterns[this.sourcePart]=p;}},
  melodicLength:{enumerable:true,configurable:true,get(){return bassData().length;},set(v){bassData().length=v;}},
  usedPatterns:{enumerable:true,configurable:true,get(){return this.patternData.map((p,i)=>[p,i]).filter(([p])=>p.drums.some(d=>d.steps.some(a=>a.length))||p.bass.some(b=>b.notes.some(n=>n.gate!=='REST'))||p.brk.steps.length).map(([,i])=>i+1);}}
 });
}
function bassData(part=([2,3].includes(S.sourcePart)?S.sourcePart:3)){return S.patternData[S.currentPatterns[part]].bass[part-2];}
function partData(part,pattern=S.currentPatterns[part]){const p=S.patternData[pattern];return part<2?p.drums[part]:part===4?p.brk:p.bass[part-2];}
function bassStrengths(b){const out=Array(b.length).fill(0);let held=0;if(b.notes[0].gate==='TIE'){let j=b.length-1;while(j>=0&&b.notes[j].gate==='TIE')j--;if(j>=0&&b.notes[j].gate==='NOTE')held=b.notes[j].velocity;}for(let i=0;i<b.length;i++){const n=b.notes[i];if(n.gate==='NOTE')held=n.velocity;else if(n.gate!=='TIE')held=0;out[i]=held;}return out;}
function partStrengths(part,pattern=S.currentPatterns[part]){const d=partData(part,pattern);if(part<2)return Array.from({length:d.length},(_,i)=>d.steps.some(a=>a.includes(i+1))?88:0);if(part===4)return Array.from({length:16},(_,i)=>d.steps.includes(i+1)?127:0);return bassStrengths(d);}
function patternStrengths(part,pattern){const d=partData(part,pattern),out=Array(8).fill(0);partStrengths(part,pattern).forEach((v,i)=>{const bin=Math.min(7,Math.floor(i*8/d.length));out[bin]=Math.max(out[bin],v);});return out;}
function sourceDim(ink,n){return blendInk('#000000',ink,n);}
function patternPartUsed(part,pattern){const d=partData(part,pattern);return part<2?d.steps.some(a=>a.length):part===4?d.steps.length>0:d.notes.some(n=>n.gate!=='REST');}
function cuePattern(pattern,all=false){if(all)S.pendingPatterns.fill(pattern);else S.pendingPatterns[S.sourcePart]=pattern;}
function applyBoundary(reason='909 wrap'){
 // Explicit simulated source boundary, never a browser wall-clock audio transport.
 if(S.songOn&&!S.rec&&S.songBars&&reason!=='Stop'){
  if(reason==='Start'){S.songPos=S.songSel<S.songBars?S.songSel:0;S.songRep=(S.songData[S.songPos].mute>>5)+1;}
  else if((S.songRep??1)>1)S.songRep--;else{S.songPos=(S.songPos+1)%S.songBars;S.songRep=(S.songData[S.songPos].mute>>5)+1;}
  const bar=S.songData[S.songPos];S.pendingPatterns=[...bar.pat];S.partMutes=bar.mute&31;S.trackMutes=[0,0];
 }
 for(let p=0;p<5;p++)if(S.pendingPatterns[p]>=0){const changed=S.currentPatterns[p]!==S.pendingPatterns[p];S.currentPatterns[p]=S.pendingPatterns[p];S.pendingPatterns[p]=-1;if(changed&&p===S.sourcePart&&[2,3].includes(p)){S.held=-1;S.press=null;}}
 S.partMutes^=S.pendingPartMutes;S.pendingPartMutes=0;for(let k=0;k<2;k++){S.trackMutes[k]^=S.pendingTrackMutes[k];S.pendingTrackMutes[k]=0;}
 S.note='Simulated '+reason+'; cue and mute masks applied independently';render();
}
function requestPartMute(displayPart){const bit=1<<SOURCE_PART[displayPart];if(S.queuedMutes&&S.playing)S.pendingPartMutes^=bit;else S.partMutes^=bit;}

function eligibleHeld(){return S.held>=0&&!(S.rec&&S.part>2)&&(!S.press||S.press.pattern===S.pattern&&(S.press.sourcePart===undefined||S.press.sourcePart===S.sourcePart));}
const PROBABILITY=[...Array.from({length:20},(_,i)=>100-i*5),...Array.from({length:7},(_,i)=>Array.from({length:i+2},(_,j)=>(j+1)+':'+(i+2))).flat()];
function chanceKey(){return [S.currentPatterns[S.sourcePart],S.sourcePart,S.part<2?S.track:0,S.held].join(':');}
function chanceValue(){return laneFor('PROB:'+S.sourcePart+':'+(S.part<2?S.track:0))?.values[S.held-1]??100;}
function probabilityTurn(d){const key='PROB:'+S.sourcePart+':'+(S.part<2?S.track:0),current=chanceValue(),at=PROBABILITY.indexOf(current),next=PROBABILITY[clamp(at+(d>0?-1:1),0,PROBABILITY.length-1)];if(S.press)S.press.used=true;const l=laneFor(key,next!==100);if(next!==100&&!l){showNotice('MOTION FULL');return;}if(l){l.values[S.held-1]=next===100?null:next;if(!l.values.some(v=>v!==null))delete S.motionLanes[l.pattern+':'+l.part+':'+l.key];}S.chance=next;S.dirty=true;}

function copyTarget(dest){S.usedButtons.push('BT');if(S.currentPatterns.every(p=>p===dest)){S.note='All five parts already use destination; no copy';return;}S.copyRequest={source:[...S.currentPatterns],dest};if(S.usedPatterns.includes(dest+1)){S.askType='COPY';S.overlay='ASK';S.note='Copy source IDs frozen at target press; queued patterns excluded';}else performCopy();}
function performCopy(){const q=S.copyRequest;if(!q)return;const results=[];for(let part=0;part<5;part++){
 const src=q.source[part];if(src===q.dest){results.push({part,result:'same'});continue;}
 const lanes=Object.values(S.motionLanes),from=lanes.filter(l=>l.part===part&&l.pattern===src),old=lanes.filter(l=>l.part===part&&l.pattern===q.dest),available=S.motionCapacity-lanes.length+old.length;
 if(from.length>available){results.push({part,result:'unchanged: MOTION FULL'});continue;}
 const a=S.patternData[src],b=S.patternData[q.dest];if(part<2)b.drums[part]=structuredClone(a.drums[part]);else if(part===4)b.brk=structuredClone(a.brk);else b.bass[part-2]=structuredClone(a.bass[part-2]);if(part===0)b.swing=a.swing??0;
 for(const l of old)delete S.motionLanes[l.pattern+':'+l.part+':'+l.key];for(const l of from){const clone=structuredClone(l);clone.pattern=q.dest;clone.applied=null;clone.recReq=clone.holdReq=false;clone.holdLeft=0;clone.lastRec=null;S.motionLanes[clone.pattern+':'+part+':'+clone.key]=clone;}results.push({part,result:'copied'});
 }S.lastCopy={...q,results};S.copyRequest=null;S.askType=null;S.overlay=null;S.dirty=true;showNotice(results.some(r=>r.result.startsWith('unchanged'))?'MOTION FULL: NOT ALL PASTED':'PARTS PASTED TO P'+(q.dest+1));S.note='Copy completed in model; current patterns unchanged';}
function copyQuestion(){
 rect(0,20,240,138,C.bg);text('PASTE OVER P'+(S.copyRequest.dest+1)+'?',120,25,16,C.white,true,'center');text('IT HAS NOTES',120,43,14,C.gray,false,'center');
 for(let pos=0;pos<5;pos++){const part=SOURCE_PART[pos],y=59+pos*13,src=S.copyRequest.source[part],dst=S.copyRequest.dest;slab(3,y,234,12);text(laneName(pos),8,y+1,11,partInk[pos]);text('P'+(src+1),105,y+1,11,C.white,true,'right');line(111,y+7,124,y+7,C.gray);line(121,y+4,124,y+7,C.gray);line(121,y+10,124,y+7,C.gray);text('P'+(dst+1),130,y+1,11,C.amber,true);
  const before=patternStrengths(part,dst),after=patternStrengths(part,src);for(let bin=0;bin<8;bin++){if(before[bin])rect(166+bin*3,y+3,2,3,C.dim);if(after[bin])rect(166+bin*3,y+7,2,3,partInk[pos]);}const n=Object.values(S.motionLanes).filter(l=>l.pattern===src&&l.part===part).length;rect(212,y+5,2,2,C.dim);text(n+'',229,y+1,11,C.gray,false,'right');
 }if(!S.clipOn){g.fillStyle='#ff4d41';g.beginPath();g.roundRect(22,127,92,24,4);g.fill();}g.strokeStyle=C.gray;g.beginPath();g.roundRect(126.5,127.5,91,23,4);g.stroke();if(!S.clipOn)text('SCALE YES',68,131,14,C.bg,true,'center');text('PATCH NO',172,131,14,C.gray,true,'center');rect(0,229,240,11,C.bg);text(S.clipOn?'PLAY: HOST · QUESTION STAYS':S.playing?'PLAY: CANCEL + STOP':'PLAY: CANCEL + START',4,229,10,C.amber);
}

function showNotice(t,ms=1300){S.notice=t;if(noticeTimer)clearTimeout(noticeTimer);noticeTimer=setTimeout(()=>{S.notice=null;noticeTimer=null;render();},ms);}
function divisionOwner(){return !S.clipOn&&!S.rec&&!S.heldPatch&&!S.heldSave&&S.overlay!=='ASK'&&['SEQ','PART'].includes(S.view)&&S.part>2&&S.engine==='MIAMI'&&S.freePlay[S.sourcePart-2];}
function requestTrackMute(kit,voice){const bit=1<<voice;if(S.queuedMutes&&S.playing)S.pendingTrackMutes[kit]^=bit;else S.trackMutes[kit]^=bit;}

function noteName(n){return ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][((n%12)+12)%12]+(Math.floor(n/12)-1);}
function resolvedNote(b,i){let n=i;for(let k=0;k<b.length;k++){const c=b.notes[n];if(c.gate==='NOTE')return c.note;if(c.gate!=='TIE')return null;n=(n+b.length-1)%b.length;}return null;}
function recordStopped(gate,n=48){if(S.playing){S.note='Running tie/rest capture unmodeled; no mutation';return;}const b=bassData(),at=b.write;b.notes[at]={...b.notes[at],gate,note:gate==='NOTE'?n:b.notes[at].note,velocity:100,end:0,accent:false,slide:false};b.write=(at+1)%b.length;S.dirty=true;S.note='Current stopped entry: visited cell replaced; V100; not a separate take';}
function noteDown(physical,velocity=100,origin='key'){
 const origins=S.keyOrigins[physical]??{},wasHeld=Object.keys(origins).length>0;origins[origin]=true;S.keyOrigins[physical]=origins;if(wasHeld)return 'Physical alias already held; no new onset';
 if(S.clipOn)return 'Clip Keys OFF fixture; no local synth note';if(S.part<3||!['PART','SEQ'].includes(S.view))return 'No admitted synth key route in this fixture';
 if(S.rec){if(physical<5||physical>31)return 'Current REC uses central27; outer key not recorded';if(S.view==='SEQ'&&!S.playing)recordStopped('NOTE',36+physical+12*S.octave);else S.note=S.view==='PART'?'PART audition only; no new record onset':bassData().direction>1?'REC NEEDS FWD/REV; audition only':'Running capture unmodeled; audition request only';}
 else if(S.view!=='PART'&&!S.freePlay[S.sourcePart-2])return 'Grid step mode; live keyboard route inactive';
 S.liveNotes[physical]={part:S.sourcePart,note:36+physical+12*S.octave,velocity,epoch:S.liveEpoch};if(!(S.rec&&(S.playing||S.view==='PART')))S.note='Admitted original physical key; no audio in model';render();return S.note;
}
function noteUp(physical,origin='key'){const origins=S.keyOrigins[physical];if(origins){delete origins[origin];if(Object.keys(origins).length){render();return;}delete S.keyOrigins[physical];}delete S.liveNotes[physical];render();}
function baseQ(id,voice=S.track){const key=controlKey(id,voice);return S.part<2?drumRaw(S.part,voice,id):(S.values[key]??S.controls[S.part]?.[id]??64);}
function motionOwner(key){if(key.startsWith('PROB:'))return Number(key.split(':')[1]);if(key.startsWith('T_909:'))return 0;if(key.startsWith('T_808:'))return 1;const v=Number(key.split(':')[0]);return Number.isInteger(v)?v:S.sourcePart;}
function laneFor(key,create=false){const owner=motionOwner(key),id=S.currentPatterns[owner]+':'+owner+':'+key;let lane=S.motionLanes[id];if(!lane&&create){if(Object.keys(S.motionLanes).length>=S.motionCapacity)return null;lane=S.motionLanes[id]={pattern:S.currentPatterns[owner],part:owner,key,values:Array(64).fill(null),applied:null,recReq:false,holdReq:false,holdLeft:0,lastRec:null,quiet:0};}return lane;}
function motionTouch(c){if(!S.playing)return;const lane=laneFor(c.key,S.rec);if(!lane){if(S.rec)showNotice('MOTION FULL: BASE EDIT KEPT');return;}if(S.rec)lane.recReq=true;else lane.holdReq=true;S.lastMotionKey=c.key;}
function motionStep(part,step){
 if(!S.playing||S.wait)return;
 const pass=partData(part).length;
 for(const l of Object.values(S.motionLanes)){if(l.part!==part)continue;if(l.pattern!==S.currentPatterns[part]){l.applied=null;l.recReq=l.holdReq=false;l.holdLeft=0;l.lastRec=null;continue;}
  if(l.holdReq){l.holdReq=false;l.holdLeft=pass;}
  if(l.recReq){l.recReq=false;const q=baseByKey(l.key);if(l.lastRec!==null){const d=(step-l.lastRec+pass)%pass;if(d>=2&&d<=4&&l.values[l.lastRec]!==null)for(let j=1;j<d;j++)l.values[(l.lastRec+j)%pass]=Math.trunc(l.values[l.lastRec]+(q-l.values[l.lastRec])*j/d);}l.values[step]=q;l.lastRec=step;l.quiet=0;l.holdLeft=0;l.applied=null;continue;}
  if(l.lastRec!==null&&++l.quiet>4)l.lastRec=null;
  if(l.lastRec!==null||l.holdLeft){if(l.holdLeft)l.holdLeft--;l.applied=null;continue;}l.applied=l.values[step];
 }
}
function baseByKey(key){if(key.startsWith('PROB:'))return 100;const a=key.split(':');if(a[0].startsWith('T_'))return drumRaw(a[0]==='T_808'?1:0,+a[1],+a[2]);return S.values[key]??S.controls[SOURCE_PART.indexOf(+a[0])]?.[+a[2]]??64;}
function releaseMotion(){for(const l of Object.values(S.motionLanes)){l.applied=null;l.recReq=l.holdReq=false;l.holdLeft=0;l.lastRec=null;l.quiet=0;}}
function simulatedStep(part=S.sourcePart,step){if(!S.playing||S.wait){S.note='No simulated step in STOP/WAIT';render();return;}const d=partData(part),next=step===undefined?(S.playheads[part]+1)%d.length:clamp(step,0,d.length-1);S.playheads[part]=next;motionStep(part,next);S.note='Explicit model step event; no audio clock or measured timing';render();}
function scopeDot(c,x,y){const lane=laneFor(c.key),lock=eligibleHeld()?lane?.values[S.held-1]:null,exists=lane&&lane.values.some(v=>v!==null);if(eligibleHeld())rect(x,y,2,2,lock!==null&&lock!==undefined?C.amber:C.line);else if(exists)rect(x,y,2,2,lane.applied!==null?C.white:C.dim);}
function rowScopeCue(page,x,y){
 if(page!==S.page)return;
 if(eligibleHeld()){text('P'+S.pattern,x,y,9,C.amber);const n=Math.floor((S.held-1)*7/Math.max(1,S.length-1));for(let i=0;i<8;i++)rect(x+17+i*3,y+2,2,4,i===n?C.amber:C.line);text(S.held,x+64,y,9,C.amber,true,'right');return;}
 const row=pages()[page],k=row.cells.findIndex(c=>typeof c.parameter_id==='number'&&controlKey(c.parameter_id,c.voice??S.track)===S.lastMotionKey);if(k<0)return;const l=laneFor(S.lastMotionKey);if(!l||!(l.holdLeft||l.holdReq||l.recReq||l.lastRec!==null))return;
 const ink=l.holdLeft||l.holdReq?C.amber:C.white;line(x+2,y+2,x+8,y+2,ink);line(x+8,y+2,x+8,y+7,ink);line(x+8,y+7,x+2,y+7,ink);line(x+2,y+7,x+2,y+4,ink);line(x+2,y+4,x,y+6,ink);text(l.holdLeft||l.holdReq?(l.holdReq?S.length:l.holdLeft):'K'+(k+5),x+15,y,9,ink,true);
 if(l.recReq){g.strokeStyle=ink;g.strokeRect(x+47.5,y+.5,5,5);}else if(l.lastRec!==null)rect(x+48,y+1,5,5,ink);
}
function arrowInput(d){if(S.heldPatch||S.overlay==='ASK')S.note='Source arrow masked; caller unchanged';else if(S.clipOn||S.buttons.includes('FX'))S.note='Connected/held-FX arrow owner unmodeled; no fallback edit';else if(S.overlay==='LIST')S.note=editList(d);else{nav(d);S.note='Source single-edge SELECT/page/probability route; repeat timing unmodeled';}trace.push({event:d<0?'<':'>',result:S.note});render();}
function buttonDown(ev){if(S.buttons.includes(ev))return;S.buttons.push(ev);if(ev==='<'||ev==='>'){S.usedButtons.push(ev);arrowInput(ev==='<'?-1:1);return;}if(ev==='BT')S.heldSave=true;if(ev==='PATCH')S.heldPatch=true;if(S.panicLatched&&['FX','SEQ'].includes(ev))S.usedButtons.push(ev);
 if(!S.clipOn&&['FX','SEQ'].every(b=>S.buttons.includes(b))&&!S.panicLatched){S.panicLatched=true;S.usedButtons.push('FX','SEQ');S.liveNotes={};S.liveEpoch++;S.recordOwner=null;showNotice('NOTES OFF · TRANSPORT '+(S.playing?'RUNS':'STOPPED'));S.note='Keyboard FX+SEQ note retirement; transport/data preserved';render();return;}
 if(['SEQ PLAY','SEQ REC'].includes(ev)){S.usedButtons.push(ev);event(ev);}else render();
}
function buttonUp(ev){if(!S.buttons.includes(ev))return;S.buttons=S.buttons.filter(b=>b!==ev);const used=S.usedButtons.includes(ev);S.usedButtons=S.usedButtons.filter(b=>b!==ev);if(ev==='BT')S.heldSave=false;if(ev==='PATCH')S.heldPatch=false;if(!S.buttons.includes('FX')&&!S.buttons.includes('SEQ'))S.panicLatched=false;if(!used)event(ev);else render();}

function prepareJourney(id){
 reset(id==='64'?'fresh':['motion','record','panic'].includes(id)?'chicago':'home');
 if(id==='64'){S.length=64;S.steps[0]=[1,64];S.dirty=true;}
 if(id==='motion'){S.page=1;S.playing=true;}
 if(id==='record'){S.view='SEQ';S.page=0;S.playing=false;}
 if(id==='cue'){S.view='SEQ';S.part=0;S.length=64;S.playing=true;S.queuedMutes=true;}
 if(id==='panic')S.playing=true;
 if(id==='copy'){S.playing=false;S.currentPatterns=[0,1,2,3,4];S.patternData[5].drums[0].steps[0]=[2];}
 S.note='Declared guided design fixture; no audio or firmware execution';remember();sync();render();
}

function rect(x,y,w,h,c){g.fillStyle=c;g.fillRect(x,y,w,h)}
function line(x,y,x2,y2,c,w=1){g.strokeStyle=c;g.lineWidth=w;g.beginPath();g.moveTo(x+.5,y+.5);g.lineTo(x2+.5,y2+.5);g.stroke()}
function text(t,x,y,size=10,c=C.gray,bold=false,align='left'){
 if(t==null)return;g.font=`${bold?'600':'500'} ${size}px Barlow`;g.textBaseline='top';g.textAlign=align;g.fillStyle=c;g.fillText(String(t),x,y);
 const width=g.measureText(String(t)).width;bounds.push({text:String(t),x:align==='center'?x-width/2:align==='right'?x-width:x,y,width,height:size});
}
function blendInk(a,b,n){const aa=a.match(/[a-f0-9]{2}/gi).map(v=>parseInt(v,16)),bb=b.match(/[a-f0-9]{2}/gi).map(v=>parseInt(v,16));return '#'+aa.map((v,i)=>Math.floor((v*(16-n)+bb[i]*n)/16).toString(16).padStart(2,'0')).join('');}
function slab(x,y,w,h,active=false){
 // Existing surface_panel chamfers/highlights, inside the unchanged bounds.
 let r=h>=18?4:h>=9?2:1;if(w<2*r+3||h<2*r+3)r=1;
 const face=active?C.sel:C.face,edge=active?C.selEdge:C.edge,side=active?'#172a1f':C.side;
 for(let row=0;row<h;row++){const inset=row<r?r-row:row>=h-r?row-(h-r)+1:0;let shade=face;if(active&&h>=18&&row>0&&row<5)shade=blendInk(face,'#ffffff',5-row);if(active&&h>=18&&row>=h-4)shade=blendInk(face,'#000000',(row-(h-4)+1)*2);rect(x+inset,y+row,w-2*inset,1,shade);}
 rect(x+r,y,w-2*r,1,edge);line(x,y+r,x+r,y,edge);rect(x,y+r,1,h-2*r,active?edge:C.edge);line(x+w-r-1,y,x+w-1,y+r,edge);rect(x+w-1,y+r,1,h-2*r,side);line(x+w-1,y+h-r-1,x+w-r-1,y+h-1,side);rect(x+r,y+h-1,w-2*r,1,C.side);line(x,y+h-r-1,x+r,y+h-1,side);if(w>24&&h>=18)rect(x+r+2,y+1,Math.floor(w/4),1,active?edge:C.edge);if(active&&h>=18)rect(x+r+1,y+h-2,w-2*r-2,1,side);
}

function stepCell(x,y,w,h,ink,raised=false){g.fillStyle=ink;g.beginPath();g.roundRect(x,y,w,h,2);g.fill();line(x+2,y+1,x+w-3,y+1,raised?'#e5ba74':C.edge);line(x+2,y+h-2,x+w-3,y+h-2,C.side);}
function triangle(x,y,c){g.fillStyle=c;g.beginPath();g.moveTo(x,y);g.lineTo(x+7,y+5);g.lineTo(x,y+10);g.fill()}
function ellipse(x,y,rx,ry,c){g.strokeStyle=c;g.lineWidth=1;g.beginPath();g.ellipse(x,y,rx,ry,0,0,Math.PI*2);g.stroke()}
function power(x=211,y=4){
 const c=C.gray;rect(x,y,24,11,c);rect(x+24,y+3,2,5,C.gray);rect(x+1,y+1,22,9,C.bg);
 if(S.powerFixturePercent!==undefined){text(S.powerFixturePercent+'%',x+12,y+1,9,C.ink,true,'center');return;}
 if(S.power===4){line(x+8,y+2,x+8,y+5,c);line(x+14,y+2,x+14,y+5,c);rect(x+7,y+4,9,3,c);line(x+11,y+7,x+11,y+9,c);return;}
 if(S.power<0){text('?',x+12,y,10,c,true,'center');return;}
 for(let i=0;i<clamp(S.power,0,3);i++)rect(x+3+i*7,y+3,5,5,c);
}

function drumGlyph(n,x,y,c){
 if(n===0){ellipse(x+10,y+5,5,4,c);line(x+6,y+8,x+4,y+10,c);line(x+14,y+8,x+16,y+10,c);line(x+14,y+5,x+19,y+2,c);rect(x+17,y,3,2,c);}
 else if(n===1){ellipse(x+10,y+2,7,2,c);line(x+3,y+2,x+3,y+7,c);line(x+17,y+2,x+17,y+7,c);line(x+3,y+7,x+17,y+7,c);for(let a=5;a<16;a+=3)line(x+a,y+5,x+a-1,y+9,c);}
 else if(n>=2&&n<=4){const size=7-(n-2);ellipse(x+10,y+2,size,2,c);line(x+10-size,y+2,x+10-size,y+8,c);line(x+10+size,y+2,x+10+size,y+8,c);line(x+10-size,y+8,x+10+size,y+8,c);for(let i=0;i<n-1;i++)rect(x+8+i*2,y+4,1,2,c);}
 else if(n===5){ellipse(x+10,y+5,7,3,c);line(x+3,y,x+16,y+9,c);line(x+6,y+9,x+18,y,c);}
 else if(n===6){g.strokeStyle=c;g.beginPath();g.moveTo(x+3,y+8);g.lineTo(x+8,y+4);g.lineTo(x+9,y);g.lineTo(x+11,y+4);g.lineTo(x+17,y+7);g.lineTo(x+14,y+10);g.lineTo(x+10,y+7);g.lineTo(x+7,y+10);g.closePath();g.stroke();line(x+14,y,x+13,y+3,c);line(x+18,y+2,x+15,y+4,c);}
 else if(n===7||n===8){ellipse(x+10,y+3,8,1.5,c);ellipse(x+10,y+(n===8?7:5),8,1.5,c);line(x+10,y,x+10,y+10,c);}
 else if(n===9){line(x+2,y+7,x+18,y+2,c);line(x+3,y+8,x+18,y+3,c);line(x+10,y+5,x+10,y+10,c);line(x+2,y+1,x+4,y+3,c);line(x+18,y+7,x+20,y+9,c);}
 else if(n===10){ellipse(x+10,y+4,9,2.5,c);ellipse(x+10,y+3,2,1.5,c);line(x+10,y,x+10,y+10,c);}
 else if(n>=20&&n<=22){const rank=n-20,wide=6-rank;ellipse(x+10,y+1.5,wide,1.5,c);line(x+10-wide,y+2,x+8,y+10,c);line(x+10+wide,y+2,x+12,y+10,c);line(x+8,y+10,x+12,y+10,c);line(x+1,y+9-rank*3,x+4,y+9-rank*3,c);}
 else if(n===23){line(x+7,y+1,x+13,y+1,c);line(x+7,y+1,x+4,y+9,c);line(x+13,y+1,x+16,y+9,c);ellipse(x+10,y+9,6,1,c);}
 else if(n===13){line(x+3,y+2,x+17,y+9,c,2);line(x+5,y+10,x+15,y+1,c,2);}
 else if(n===14){ellipse(x+7,y+3,4,3,c);line(x+10,y+6,x+15,y+10,c,2);ellipse(x+15,y+2,3,2,c);line(x+17,y+4,x+20,y+8,c,2);}
 else {line(x+3,y+5,x+10,y+1,c);line(x+10,y+1,x+17,y+5,c);line(x+3,y+9,x+10,y+5,c);line(x+10,y+5,x+17,y+9,c);}
}
function identityIndex(n,kit=S.part){if(kit===1){if(n>=2&&n<=4&&S.variants.toms)return 20+n-2;if(n===5&&S.variants.rim)return 13;if(n===6&&S.variants.clap)return 14;if(n===7)return 23;if(n===8)return 9;if(n===9)return 8;if(n===10)return 7;}return n;}
function identityName(n,kit=S.part){if(kit===1){if(n>=2&&n<=4&&S.variants.toms)return ['LOW CONGA','MID CONGA','HIGH CONGA'][n-2];if(n===5&&S.variants.rim)return 'CLAVE';if(n===6&&S.variants.clap)return 'MARACAS';if(n===7)return 'COWBELL';if(n===8)return 'CYMBAL';if(n===9)return 'OPEN HAT';if(n===10)return 'CLOSED HAT';}return drumNames[n];}
function graph(kind,x,y,w,h,c,val=50){
 const k=String(kind||'').toUpperCase();if(performSourceGlyph(k,x,y,w,h,c))return;
 if(/LENGTH/.test(k)){for(let i=0;i<8;i++)rect(x+i*5,y+(i>3?8:0),3,4,i<S.length/8?c:C.line);}
 else if(/RATE/.test(k)){for(let i=0;i<4;i++)line(x+i*w/3,y+2,x+i*w/3,y+h-2,i===0||i===3?c:C.line);line(x,y+h,x+w,y+h,C.line);}
 else if(/SWING/.test(k)){for(let i=0;i<4;i++)rect(x+i*10+(i%2?3:0),y+2,3,13,c);}
 else if(/PUMP|ENVELOPE|ATTACK|DECAY|RELEASE/.test(k)){line(x,y+2,x+w*.2,y+h,c);line(x+w*.2,y+h,x+w*.75,y+h*.5,c);line(x+w*.75,y+h*.5,x+w,y+h*.5,c);}
 else if(k==='FILTER'&&val==='OFF'){line(x,y+h/2,x+w,y+h/2,C.dim);}
 else if(/CUTOFF|FILTER|PEAK|RES|TONE|HPF/.test(k)){line(x,y+h*.3,x+w*.35,y+h*.3,c);line(x+w*.35,y+h*.3,x+w*.5,y,c);line(x+w*.5,y,x+w*.72,y+h,c);line(x+w*.72,y+h,x+w,y+h,c);}
 else if(/WAVE|SUB|WOBBLE|SHAPE|FLUTTER/.test(k)){for(let i=0;i<w;i++)if(i)line(x+i-1,y+h/2-Math.sin((i-1)*.3)*h*.4,x+i,y+h/2-Math.sin(i*.3)*h*.4,c);}
 else if(/DIRECTION/.test(k)){line(x,y+h/2,x+w,y+h/2,c);line(x+w-5,y+h/2-4,x+w,y+h/2,c);line(x+w-5,y+h/2+4,x+w,y+h/2,c);}
 else if(/TEMPO/.test(k)){line(x+2,y+h,x+w/2,y,c);line(x+w/2,y,x+w-2,y+h,c);line(x+2,y+h,x+w-2,y+h,c);line(x+w/2,y+h-3,x+w*.7,y+3,c);}
 else {line(x,y+h/2,x+w,y+h/2,C.line);rect(x+w*Math.max(0,Math.min(100,Number(val)||50))/100-1,y+h/2-3,3,7,c);}
}
function modelSaveState(){return S.storageUnsupported===2?'FAILED':S.saveQueued||S.save==='SAVE QUEUED'?'QUEUED':S.save==='SAVE FAILED'?'FAILED':S.dirty?'UNSAVED':'SAVED';}
function saveMark(){const state=modelSaveState();if(state!=='SAVED'){const ink=state==='FAILED'?'#ee745c':C.amber;rect(204,3,3,3,ink);if(state==='FAILED'||state==='QUEUED')text(state==='FAILED'?'!':'Q',204,8,9,ink,true);}}
function header(){
 if(S.clipOn){slab(0,0,240,20);text('LP',5,1,14,C.amber,true);if(modelSaveState()!=='SAVED')text(modelSaveState()==='UNSAVED'?'UNSAVED':modelSaveState()==='QUEUED'?'QUEUED':'FAILED',96,5,9,C.amber,true,'right');text('INT',149,2,14,C.gray,false,'right');text(S.tempo,179,0,16,C.gray,false,'right');triangle(184,4,C.dim);if(S.rec)rect(195,6,5,5,'#f05e56');saveMark();power();return;}
 slab(0,0,240,20);
 const label=S.view==='PART'?(S.part<2?S.engine:laneName(S.part,true)):S.view==='SEQ'?((S.part<3?S.engine:laneName(S.part,true))+' SEQ'):S.view==='MIX'&&mixKit()>=0?(mixKit()?'808 MIX':'909 MIX'):S.view;
 text(label,5,1,14,S.part>2&&S.view==='PART'?C.green:partInk[mixKit()>=0?mixKit():S.part],true);const labelEnd=bounds.at(-1).x+bounds.at(-1).width;g.font='600 9px Barlow';const saveCaption=modelSaveState()==='UNSAVED'?'UNSAVED':modelSaveState()==='QUEUED'?'QUEUED':'FAILED',saveWidth=g.measureText(saveCaption).width;
 if(S.view==='PART'){const groups=Math.ceil(pageCount()/4);for(let i=0;i<groups;i++)rect(86+i*6,7,3,3,i===Math.floor(S.page/4)?C.amber:C.edge);}
 patternPicture(100,2);text(S.songOn?(S.playing?S.songPos+1:S.songSel+1):S.pattern,123,3,11,C.white,false,'right');
 if(modelSaveState()!=='SAVED'&&S.view!=='PART'&&labelEnd+3<=96-saveWidth)text(saveCaption,96,5,9,C.amber,true,'right');
 if(S.queued&&!S.songOn){text('›'+S.queued,149,3,11,S.playing&&!S.wait?(S.framePhase&16?C.white:C.gray):C.amber,true,'right');}
 text(S.wait?'WAIT':S.tempo,179,S.wait?3:0,S.wait?11:16,S.wait?C.amber:C.gray,S.wait,'right');
 if(S.playing&&!S.wait)triangle(184,4,C.white);else if(S.wait){line(184,5,184,13,C.amber);line(189,5,189,13,C.amber);}else triangle(184,4,C.dim);
 if(S.rec)rect(195,6,5,5,'#f05e56');saveMark();
 power();
}

function cards(title,names,values,footer){
 slab(3,158,234,11);text(title,8,158,11,S.view==='MIX'&&mixKit()>=0?partInk[mixKit()]:C.amber);if(S.view==='MIX')for(let dot=0;dot<5;dot++)rect(181+dot*6,162,3,3,dot===S.page?C.ink:C.edge);
 for(let i=0;i<4;i++){slab(i*60+3,171,54,55);if(!names[i])continue;text(names[i],i*60+30,171,11,C.gray,false,'center');if(S.view==='SONG')songCardGlyph(i,i*60+12,188,partInk[S.part]);else graph(names[i],i*60+10,188,39,17,partInk[mixKit()>=0?mixKit():S.part],values[i]);text(values[i],i*60+30,210,16,C.amber,true,'center');}
 slab(0,229,240,11);text(footer,4,229,10,C.gray);
}
function drumSeq(){
 const col=partInk[S.part];
 for(let r=0;r<12;r++){const y=21+r*11;if(r===S.track)slab(0,y-1,24,11,true);drumGlyph(r===11?15:identityIndex(r),1,y,r===S.track?C.ink:C.gray);
 for(let i=0;i<16;i++){const step=S.stepPage*16+i+1,x=27+i*13+(i>>2),on=(r<11?S.steps[r]:S.kits[S.part].accent).includes(step),muted=!!(S.trackMutes[S.part]&(1<<r));if(step>S.length){rect(x+5,y+3,2,2,C.line);continue;}stepCell(x,y,12,9,on?(muted?C.dim:r===S.track?col:'#5f552d'):C.line,on);if(S.held===step&&r===S.track){g.strokeStyle=C.amber;g.strokeRect(x+.5,y+.5,11,8);}if(S.playing&&step-1===S.playhead&&on)rect(x+2,y+2,8,4,C.white);}}
 cards('P'+S.pattern+' SEQUENCE',['LENGTH','RATE',null,null],[S.length+'st',D.rates[S.rate],null,null],'BLACK: TRACK  WHITE: STEP  PATCH+BLK: MUTE');
 // Formerly unmapped K7/K8 display cells become one selected-instrument
 // readout, with no editable card, value or knob caption.
 rect(123,171,114,55,C.bg);text((S.stepPage*16+1)+'–'+(S.stepPage*16+16)+' / '+S.length,180,173,10,C.gray,false,'center');if(eligibleHeld())text(String(S.chance)+(typeof S.chance==='number'?'%':''),180,188,18,C.amber,true,'center');else{g.save();g.translate(157,188);g.scale(2,2);drumGlyph(identityIndex(S.track),0,0,partInk[S.part]);g.restore();}text(identityName(S.track),180,212,12,C.ink,true,'center');
}
function home(){
 slab(3,21,234,33);for(let i=0;i<16;i++){const pid=S.patternBank*16+i,x=7+i*14+(i>=8?4:0);rect(x,23,12,22,C.side);line(x,23,x+11,23,C.edge);
  for(let pos=0;pos<5;pos++){const part=SOURCE_PART[pos],ink=partInk[pos],used=patternPartUsed(part,pid),current=S.currentPatterns[part]===pid,y=24+pos*4;if(used){const strength=patternStrengths(part,pid);for(let bin=0;bin<8;bin++)if(strength[bin]){let t=4+Math.floor((strength[bin]*12+63)/127);if(!current)t=Math.floor((t*9+8)/16);rect(x+2+bin,y,1,3,sourceDim(ink,t));}if(current)rect(x,y,1,3,ink);}
   // Proposal: the source header's pending-arrow family is repeated per part.
   // It uses the two unused right-edge pixels; saved content/current tick stay intact.
   if(S.pendingPatterns[part]===pid&&S.pendingPatterns[part]!==S.currentPatterns[part]){rect(x+10,y,1,1,ink);rect(x+11,y+1,1,1,ink);rect(x+10,y+2,1,1,ink);}
  }
  if(pid===S.currentPatterns[S.sourcePart])text(pid+1,x+6,45,10,C.gray,false,'center');
  if(pid===S.pendingPatterns[0]&&pid!==S.currentPatterns[0]&&(!S.playing||S.wait||S.framePhase&16)){g.strokeStyle=S.playing&&!S.wait?C.white:C.amber;g.strokeRect(x-.5,21.5,13,25);}
  if(S.chain&&pid>=S.chain[0]&&pid<=S.chain[1])rect(x,52,12,1,C.amber);
 }
 for(let pos=0;pos<5;pos++){const part=SOURCE_PART[pos],y=55+pos*20,d=partData(part),mute=!!(S.partMutes&(1<<part)),ink=mute?C.dim:partInk[pos],base=S.playing&&part!==4?S.playheads[part]&~15:0,strength=partStrengths(part);slab(3,y,234,18);rect(4,y+3,2,11,ink);let name=laneName(pos);g.font='500 11px Barlow';if(g.measureText(name).width>47)name=name.replace('CHICAGO','CHI').replace('MIAMI','MIA');text(name,9,y+2,11,ink);
  for(let i=0;i<16&&base+i<d.length;i++){const x=58+i*11+(i>>2),on=!!strength[base+i];rect(x,y+5,9,8,C.side);line(x,y+5,x+8,y+5,on?ink:C.edge);if(on)rect(x+1,y+6,7,6,mute?C.dim:sourceDim(ink,11));line(x,y+12,x+8,y+12,on?sourceDim(ink,6):C.edge);if(S.playing&&(S.playheads[part]&15)===i)rect(x+1,y+14,7,2,C.white);}
  // Proposal: pending mute gets a persistent dashed mark; actual mute keeps source dimming.
  if(S.pendingPartMutes&(1<<part))for(let x=10;x<47;x+=6)rect(x,y+16,3,1,C.amber);
 }
 cards('PERFORM',['TEMPO','SWING','FILTER','PUMP'],[S.tempo+'bpm',S.swing+'%',S.filter<60?'LP '+Math.floor((60-S.filter)*100/60)+'%':S.filter>68?'HP '+Math.floor((S.filter-68)*100/59)+'%':'OFF',shortDecimal(S.pump,true)+'dB'],'WHITE: PATTERN   TWO: CHAIN   BLACK: MUTE');
}

function pages(){
 if(S.part<2){const map=D.trackMaps.find(x=>x.engine===(S.part?'808':'909')&&x.voice_index===S.track);return map.pages.map((r,i)=>({...r,title:r.title==='KIT'?'KIT':identityName(S.track)+(i?' 2':''),cells:r.cells.map(c=>({...c,editable:!!c.label}))}));}
 return D.engines[S.engine].pages;
}
function fixtureValue(label,p,k){const c=control(p,k);return c?controlText(c):'—';}

function matrix(){if(S.engine==='JUNO'){junoMatrix();return;}const pp=pages(),first=Math.floor(S.page/4)*4;for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page,ink=active?C.ink:C.gray,glyphInk=active?C.ink:C.value;slab(3,y,234,49,active);if(S.part<2){text(p+1,7,y,10,ink,true);drumGlyph(row.title==='KIT'?15:identityIndex(S.track),16,y+1,ink);if(row.title==='KIT')text('KIT',43,y,10,ink,true);}else text((p+1)+' '+row.title,8,y,10,ink,true);rowScopeCue(p,105,y+1);if(active&&!(row.title==='ENVELOPE'&&S.touched?.page===p&&S.touched?.part===S.part))text('K'+row.cells.map((c,k)=>c.editable?k+5:null).filter(x=>x).join(' '),232,y,10,C.ink,true,'right');if(row.title==='ENVELOPE'&&['CHICAGO','JUNO'].includes(S.engine)){envelopeRow(p,y,glyphInk);continue;}for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const c=control(p,k),x=4+k*59,vInk=valueInk(c,active);if(k)line(x,y+14,x,y+45,C.line);scopeDot(c,x+3,y+15);text(cell.label.toUpperCase(),x+29,y+13,10,ink,false,'center');if(row.title==='SOUND'){text(controlText(c),x+29,y+30,11,vInk,false,'center');continue;}if(S.engine==='CHICAGO'&&c.id===17){semanticGlyph(c,x+16,y+25,26,10,vInk);text(controlText(c),x+29,y+36,10,vInk,false,'center');}else{semanticGlyph(c,x+5,y+27,26,18,vInk);text(controlText(c),x+54,y+31,11,vInk,false,'right');}}}matrixFooter(first+1,Math.min(first+4,pp.length));}

// Existing manual Song authoring. Canonical five-part IDs are never screen-column IDs.
function songFill(k){while(S.songData.length<=k&&S.songData.length<192)S.songData.push(S.songData.length?structuredClone(S.songData[S.songData.length-1]):{pat:[...S.currentPatterns],mute:0});}
function songPattern(part){const k=S.songSel<S.songBars?S.songSel:S.songBars-1;return k>=0?S.songData[k].pat[part]:S.currentPatterns[part];}
function songRepeat(){return S.songSel<S.songBars?S.songData[S.songSel].mute>>5:0;}
function songEdit(k,d){
 if(k===0)S.songSel=clamp(S.songSel+d,0,Math.min(S.songBars,191));
 else if(S.page===1&&k===3){const v=clamp(songRepeat()+d,0,7);songFill(S.songSel);S.songData[S.songSel].mute=(S.songData[S.songSel].mute&31)|(v<<5);}
 else{const part=(S.page===0?[null,0,1,4]:[null,3,2])[k],v=clamp(songPattern(part)+d,0,31);songFill(S.songSel);S.songData[S.songSel].pat[part]=v;}
 S.dirty=true;return 'Song BAR '+(S.songSel+1)+(S.songSel===S.songBars?' / END':' / stored');
}
function songCardGlyph(k,x,y,ink){
 if(S.page===1&&k===3){for(let i=0;i<8;i++)rect(x-1+i*5,y+4,3,9,i<=songRepeat()?ink:C.line);return;}
 const count=k===0?Math.min(S.songBars+1,192):32,val=k===0?S.songSel:songPattern((S.page===0?[null,0,1,4]:[null,3,2])[k]),selected=count<=8?val:Math.floor(val*7/(count-1));
 for(let j=0;j<Math.min(count,8);j++){g.fillStyle=j===selected?ink:C.line;g.beginPath();g.roundRect(x+j%4*10,y+Math.floor(j/4)*10,7,7,1);g.fill();}
}
function song(){
 const names=[0,1,2,3,4].map(p=>laneName(p).replace('CHICAGO','CHI').replace('MIAMI','MIA').replace('BREAK','BRK'));text('BAR',4,20,11,C.gray);names.forEach((n,i)=>text(n,58+i*36,20,11,partInk[i],false,'center'));
 const top=Math.max(0,S.songSel-5);
 for(let r=0;r<11;r++){const k=top+r;if(k>S.songBars||k>=192)break;const y=31+r*11,bar=S.songData[k],selected=k===S.songSel;slab(0,y,238,11,selected);text(k+1,4,y,11,selected?C.white:C.gray);if(S.songOn&&S.playing&&S.songPos===k)triangle(26,y+1,S.rec?'#f05e56':C.white);
  if(bar){for(let i=0;i<5;i++){const part=SOURCE_PART[i],mute=bar.mute&(1<<part);text(mute?'—':'P'+(bar.pat[part]+1),58+i*36,y,11,mute?C.dim:partInk[i],false,'center');}if(bar.mute>>5)text('x'+((bar.mute>>5)+1),238,y,10,C.white,false,'right');}else text(k?'END':'EMPTY: WHITE KEYS ADD BARS',50,y,10,C.dim);
 }
 const ids=S.page===0?[0,1,4]:[3,2],values=[S.songSel+1,...ids.map(p=>'P'+(songPattern(p)+1))];if(S.page===1)values.push('x'+(songRepeat()+1));
 cards('BAR ('+(S.page+1)+'/2)',S.page===0?['BAR','909','808','BREAK']:['BAR',laneName(3).replace('CHICAGO','CHI'),laneName(4).replace('MIAMI','MIA'),'REPEAT'],values,'WHITE: THE BAR\'S PATTERN   BLACK: MUTE');
}

function fx(){
 text('REVERB',98,22,10,C.gray,false,'center');text('DELAY',192,22,10,C.gray,false,'center');for(let i=0;i<5;i++){slab(3,34+i*15,234,13);text(laneName(i),7,35+i*15,10,partInk[i]);for(let j=0;j<2;j++){const x=60+j*94;line(x,40+i*15,x+76,40+i*15,C.line);rect(x+16+i*7,37+i*15,3,7,partInk[i]);}}
 text('DELAY 1/8  PING-PONG',9,116,14,C.white);text('FEEDBACK  37%     RV DECAY  50%',9,142,11,C.gray);
 const names=S.page===0?['DECAY','TONE',null,null]:['TIME','FEEDBACK','TONE','TAPE'];cards(S.page?'DELAY':'REVERB',names,S.page?['1/8','37%','50%','OFF']:['50%','50%',null,null],'FX: NEXT PAGE   SCALE: ALL AS A LIST');
}
function mix(){const kit=mixKit();if(kit>=0){const k=S.kits[kit];for(let i=0;i<11;i++){const x=4+21*i,ink=partInk[kit];slab(x,20,19,134,i===k.track);if(i===k.track)rect(x+1,21,17,12,ink);g.save();g.translate(x-1,23);g.scale(.85,.85);drumGlyph(identityIndex(i,kit),0,0,i===k.track?C.bg:C.gray);g.restore();rect(x+6,35,7,98,C.side);rect(x+7,36,5,96,C.line);const h=Math.floor(k.mix[i][0]*96/127),dark=kit?'#452019':i===k.track?'#5a3c10':'#392810';rect(x+7,132-h,5,h,dark);for(let sy=0;sy<96;sy+=8)rect(x+7,126-sy,5,6,sy<h?dark:C.line);line(x+3,144,x+15,144,C.line);rect(x+3+Math.floor(k.mix[i][1]*10/127),141,3,7,i===k.track?C.white:C.gray);}cards((kit?'808':'909')+' MIX: '+identityName(k.track,kit),['LEVEL','PAN','REV','DLY'],mixDrumControls(kit,k.track).map(controlText),'BLACK: TRACK  PATCH+BLK: MUTE');return;}
 const master=S.page>2,wave=S.page===4;if(!master){for(let i=0;i<5;i++){const x=4+i*38;slab(x,20,30,68,i===S.mixsel);rect(x+12,24,6,64,C.line);rect(x+6,85-S.mixPart[i][0]*64/127,18,3,C.white);text(laneName(i).replace('CHICAGO','CHI').replace('MIAMI','MIA').replace('BREAK','BRK'),x+15,89,10,partInk[i],false,'center');}}else{line(4,86,200,86,C.line);line(4,23,4,86,C.line);text('—',102,46,14,C.dim,false,'center');}slab(205,21,32,66);rect(208,25,7,61,C.line);text('GR',217,24,10,C.gray);slab(3,103,114,34);slab(123,103,114,34);text('COMP',8,106,10,C.gray);text('— dB',8,120,13,C.gray);text('PUMP',128,106,10,C.gray);text(shortDecimal(S.pump,true)+'dB',128,120,13,C.amber);text('AUDIO LOAD —%',8,145,10,C.gray);cards(['PARTS','909 MIX','808 MIX','MASTER','WAVE'][S.page],master?['DRIVE','COMP','PUMP','FILTER']:['LEVEL','PAN','REV','DLY'],master?['0%','OFF',shortDecimal(S.pump,true)+'dB','OFF']:mixValues(S.mixPart[S.mixsel]),master?'AGAIN: NEXT PAGE  SCALE: ALL AS A LIST':'BLACK 1-5: PART  PATCH+BLK: MUTE');}

function breaks(){
 // Equivalent actual BUILD_ONLY baseline: no available loop outline. Keep the native eight-slice surface and16-step strip; never substitute a made-up waveform.
 slab(3,22,234,65);line(8,48,232,48,C.line);for(let i=0;i<8;i++){if(i)line(8+i*28,26,8+i*28,70,C.line);text(i+1,22+i*28,74,10,C.gray,false,'center');}
 slab(3,91,234,20);for(let i=0;i<16;i++){const x=8+i*14;const on=partData(4).steps.includes(i+1);rect(x,95,12,12,on?partInk[2]:C.side);line(x,95,x+11,95,on?partInk[2]:C.edge);const pin=partData(4).slice[i];if(pin)text(pin,x+6,95,10,on?C.bg:C.gray,false,'center');}
 slab(3,115,114,16);slab(123,115,114,16);text('A  NONE',60,117,11,C.amber,false,'center');text('B  NONE',180,117,11,C.amber,false,'center');
 text('A KEYS 1–8 / 8',8,134,11,C.gray);text('EDIT A 8',144,134,11,C.gray);text('C/D COUNT   C#/D# PAGE',8,147,10,C.dim);
 if(S.view==='PART')cards('LOOPS',['LOOP A','LOOP B','B CHANCE','PITCH'],['NONE','INVALID','0%','0st'],'STEP+1–8: PIN  9 REV  10 JNG  11 STUT');
 else{const ns=[['RATE',null,null,null],['CMPLX','ANCHOR','ROLL','FILL'],['RTG2X','RTG3X','RTG4X','RTG8X']][S.page];cards(['SEQUENCE','GROOVE','RETRIG'][S.page],ns,ns.map(x=>x?'0%':null),'9 REV  10 JUNGLE  11 STUTTER');}
}

function melodic(){
 const b=bassData(),all=b.notes.slice(0,b.length).filter(n=>n.gate==='NOTE').map(n=>n.note),lo=(all.length?Math.min(...all):48)-2,hi=Math.max(lo+14,(all.length?Math.max(...all):48)+2),ny=n=>35+(hi-n)*72/(hi-lo),col=partInk[S.part];
 for(let n=lo;n<=hi;n++)if(n%12===0){const y=ny(n);line(27,y+2,239,y+2,C.line);text(noteName(n),1,y-4,10,C.dim);}
 const strength=bassStrengths(b);for(let i=0;i<16;i++){const step=S.stepPage*16+i,n=b.notes[step],x=27+i*13+(i>>2);if(step>=b.length){rect(x+5,124,2,2,C.line);continue;}if(S.playing&&step===S.playhead)rect(x,24,12,101,sourceDim(col,2));const pitch=resolvedNote(b,step);if(strength[step]&&pitch!==null){const y=ny(pitch),w=n.end?Math.ceil(n.end*12/15):12;stepCell(x,y,w,5,S.held===step+1?C.white:n.accent?col:sourceDim(col,9));if(n.gate==='TIE'){if(i)rect(x-6,y,6,5,col);else line(x-6,y+2,x,y+2,col);}if(n.slide&&i<15)line(x+11,y+2,x+14,ny(b.notes[(step+1)%b.length].note)+2,C.white);}rect(x+1,121,10,4,n.accent?C.white:C.line);rect(x+1,133,10,4,n.slide?col:C.line);}
 text('AC',3,120,10,C.gray);text('SL',3,132,10,C.gray);
 const at=S.held>=0?S.held-1:b.write,n=b.notes[at];if(S.held>=0||S.rec)text((S.held>=0?'STEP ':'WRITE ')+(at+1)+' '+noteName((resolvedNote(b,at)??n.note)+b.transpose-24)+' V'+n.velocity+(S.held>=0&&S.chance!==100?'  '+S.chance+(typeof S.chance==='number'?'%':''):''),4,143,13,C.white,true);else text('C CHROMATIC   SEED 0000',4,143,11,C.gray);
 const names=S.held>=0?['NOTE','GATE','ACCENT','SLIDE']:[['LENGTH','RATE','DIRECTION','TRANSP'],['PAT SOUND',null,null,null],['FREE PLAY',null,null,null]][S.page];
 const vals=S.held>=0?[noteName(n.note),n.gate,n.accent?'ON':'OFF',n.slide?'ON':'OFF']:S.page===0?[b.length+'st',D.rates[b.rate],['FWD','REV','PING','RND'][b.direction],(b.transpose-24)+'st']:S.page===2?[S.freePlay[S.sourcePart-2]?'ON':'OFF',null,null,null]:['OFF',null,null,null];
 cards(S.held>=0?laneName(S.part)+' STEP '+S.held:['P'+S.pattern+' SEQUENCE','PRESET','KEYS'][S.page],names,vals,S.held>=0?'K2: CHANCE   K3: VELOCITY':S.rec?'CHORD+OCTAVE: REST   NOTE REPEAT+OCTAVE: TIE':'PRESS: ON  TAP: OFF  HOLD+KNOBS: EDIT');if(S.rec&&S.held<0&&S.page===0)text(S.playing?'NOTES + MOTION':'REWRITE 1–'+b.length,232,158,10,C.amber,true,'right');
}

function toggleClip(){if(!S.clipOn){S.localReturn={view:S.view,page:S.page};S.view='CLIP';S.clipOn=true;S.clipPage=null;S.liveNotes={};S.liveEpoch++;}else{S.clipOn=false;S.view=S.localReturn?.view??'SEQ';S.page=S.localReturn?.page??0;S.clipPage=null;S.clipRequests.push({type:'owned clip-note retirement request'});}S.note='Source KNOB BANK mode route; no host connected or playback confirmed';sync();render();}
function clipNativeBody(){const im=clipImages[S.clipPage==='SCALE'?'scale':'wait'];if(im?.complete&&im.naturalWidth)g.drawImage(im,0,20,240,220,0,20,240,220);else{text('WAITING FOR',120,54,14,C.white,true,'center');text('LOOPY PRO',120,76,14,C.white,true,'center');} }
function clipEvent(ev,d){if(S.overlay==='ASK'&&ev==='PATCH'){S.overlay=null;S.copyRequest=null;S.askType=null;S.note='Clip question cancelled; no storage/copy';render();return;}if(S.overlay==='ASK'&&!['SEQ PLAY','SEQ REC'].includes(ev)){S.note='Clip physical SCALE/normal controls masked; question unchanged';render();return;}if(ev==='SCALE')S.clipPage=S.clipPage==='SCALE'?null:'SCALE';else if(['SEQ PLAY','SEQ REC'].includes(ev)){S.clipRequests.push({type:ev+' host request'});}else if(ev==='NOTE REPEAT'){S.clipRequests.push({type:'Clip Keys owned all-off + CC123 request'});showNotice('PANIC');}S.note=['SCALE','SEQ PLAY','SEQ REC','NOTE REPEAT'].includes(ev)?'Bounded Clip route; host response not asserted':'Clip scalar/other page interaction unmodeled; retained native pixels only';trace.push({event:ev,result:S.note});render();}

function deepRows(){
 if(S.view==='PART'&&S.part!==2){const rows=pages().flatMap((p,pi)=>p.cells.map((c,k)=>({...c,page:pi,k})).filter(c=>c.label));
 if(S.part<2){const rec=drumRecord(S.part,S.track);for(const id of rec.more_local_ids||rec.parameters.filter(p=>p.access.more).map(p=>p.local_id)){const meta=rec.parameters.find(p=>p.local_id===id);rows.push({label:meta.source_caption,editable:true,page:-1,k:meta.local_id,parameter_id:meta.local_id});}}return rows;}
 return [{label:'LENGTH',editable:true},{label:'RATE',editable:true}];
}

function editList(d){const c=deepRows()[S.list];if(!c?.editable)return 'Read-only/unmapped';return S.view==='PART'&&S.part!==2?editControl(c.page,c.k,d):'This scalar is outside the bounded editor';}

function render(){
 bounds.length=0;rect(0,0,240,240,C.bg);header();
 if(S.clipOn)clipNativeBody();else if(S.view==='HOME')home();else if(S.view==='PART'&&S.part!==2)matrix();else if(S.part===2&&['PART','SEQ'].includes(S.view))breaks();else if(S.view==='SEQ')S.part<2?drumSeq():melodic();else if(S.view==='SONG')song();else if(S.view==='FX')fx();else if(S.view==='MIX')mix();
 if(S.overlay==='LIST'){rect(0,20,240,138,C.bg);const rows=deepRows(),top=Math.max(0,S.list-3);for(let i=0;i<6;i++){const r=rows[top+i];if(!r)break;slab(3,23+i*22,230,21,top+i===S.list);text(r.label,9,26+i*22,13,top+i===S.list?C.ink:C.gray);text(r.page!==undefined?fixtureValue(r.label,r.page,r.k):r.editable?'50%':'READ ONLY',226,26+i*22,13,C.gray,false,'right');}}
 if(S.overlay==='ASK'&&S.askType==='COPY')copyQuestion();else if(S.overlay==='ASK'){rect(0,20,240,138,C.bg);text('LOAD SAVED PROJECT?',120,47,17,C.white,true,'center');text('DISCARD UNSAVED EDITS',120,76,13,C.gray,false,'center');text(S.clipOn?'PATCH: NO':'SCALE: YES   PATCH: NO',120,105,13,C.green,true,'center');text(S.clipOn?'PLAY: HOST · QUESTION STAYS':S.playing?'PLAY: CANCEL + STOP':'PLAY: CANCEL + START',120,134,11,C.amber,true,'center');}
 if(S.notice&&S.overlay!=='ASK'){const matrix=S.view==='PART'&&S.part!==2;rect(0,matrix?227:229,240,matrix?13:11,C.bg);text(S.notice,4,matrix?228:229,10,C.amber);}
 if(S.held>=0&&!eligibleHeld()&&S.view==='PART'){rect(0,227,240,13,C.bg);text('P'+S.pattern+' BASE · OLD PAD HELD',7,228,10,C.amber);}
 cv.setAttribute('aria-label',`${S.view}; ${laneName(S.part,true)}; ${S.wait?'external WAIT':S.playing?'RUN':'STOP'}; row ${S.page+1}; ${S.part<2?identityName(S.track):''}. Proposal only.`);
 if($('state'))$('state').textContent=`${S.note} · ${S.view} · ${S.wait?'WAIT':S.playing?'RUN':'STOP'} · page ${S.page+1}/${pageCount()} · ${S.held>=0?'held step '+S.held:'base edit scope'}`;
 if($('log'))$('log').textContent=trace.slice(-12).map(x=>x.event+' → '+x.result).join('\n');
 updateLights();
 return {state:structuredClone(S),bounds:structuredClone(bounds),pages:pageCount()};
}
function pageCount(){if(S.clipOn)return 1;return S.view==='PART'?S.part===2?1:pages().length:S.view==='SEQ'?S.part<2?1:3:S.view==='SONG'?2:S.view==='FX'?2:S.view==='MIX'?5:1;}
function nav(d){if(S.overlay==='LIST'){S.list=clamp(S.list+d,0,deepRows().length-1);return;}if(eligibleHeld()){probabilityTurn(d);return;}const n=pageCount();S.page=['PART','FX','MIX'].includes(S.view)?(S.page+d%n+n)%n:clamp(S.page+d,0,n-1);remember();}

function event(ev,d=1){if(ev==='KNOB BANK'){toggleClip();return;}if(S.clipOn){clipEvent(ev,d);return;}let outcome='';if(S.overlay==='ASK'&&!['PATCH','SCALE','SEQ PLAY'].includes(ev)){outcome='Question input mask; unchanged';}else if(ev==='K2')nav(d);else if(ev==='SCALE'){if(S.overlay==='ASK'){if(S.askType==='COPY')performCopy();else{outcome='Confirmation input; storage not executed';S.overlay=null;}}else if(!S.overlay){S.overlay='LIST';S.list=0;}}else if(ev==='PATCH'){if(S.overlay){S.overlay=null;S.copyRequest=null;S.askType=null;outcome='Caller restored; no copy';}else setView(S.view==='HOME'?'SONG':'HOME');}else if(['PARA','SEQ','FX','NOTE REPEAT'].includes(ev)){const v={PARA:'PART',SEQ:'SEQ',FX:'FX','NOTE REPEAT':'MIX'}[ev];if(S.view===v){S.page=(S.page+1)%pageCount();remember();}else if(!setView(v))outcome='RELEASE STEP FIRST';}else if(ev==='SEQ PLAY'){if(S.overlay==='ASK'){S.overlay=null;S.copyRequest=null;S.askType=null;}if(!S.playing){S.songOn=S.view==='SONG';if(S.songOn)S.songPos=S.songSel<S.songBars?S.songSel:0;}applyBoundary(S.playing?'Stop':'Start');S.playing=!S.playing;S.wait=S.playing&&S.clockSource==='EXTERNAL'&&!S.clockSignal;if(!S.playing)releaseMotion();outcome='Normal transport command; simulated boundary, no audio';}else if(ev==='SEQ REC'){S.rec=!S.rec;S.liveNotes={};S.liveEpoch++;if(S.rec&&S.part>2)bassData().write=0;outcome='Current broad REC policy; no selected CP2 capture strategy';}else if(ev==='K3'){if(S.overlay==='LIST')outcome=editList(d);else if(S.view==='SEQ'&&S.part>2&&S.held>=0){const n=bassData().notes[S.held-1];n.velocity=clamp(n.velocity+d,1,127);S.velocity=n.velocity;if(S.press)S.press.used=true;S.dirty=true;}else{remember();const next=clamp(S.part+Math.sign(d),0,4);if(next!==S.part){S.held=-1;S.press=null;}S.part=next;S.page=S.pageMemory[pageKey()]??0;S.liveNotes={};S.liveEpoch++;if(S.view==='MIX'){S.mixsel=S.part;S.page=S.part<2?S.part+1:0;}normalizePage();}}else if(ev==='K4'){if(S.view==='PART'&&S.part>2&&pages()[S.page]?.title==='SOUND')outcome='Preset admission unmodeled; reserved producer interface';else{S.queued=clamp((S.queued||S.pattern)+Math.sign(d),1,32);S.chain=null;S.patternBank=Math.floor((S.queued-1)/16);outcome='Pending current-part/all-part target; explicit simulated boundary required';}}else if(/^K[5-8]$/.test(ev)){const k=+ev[1]-5;if(S.view==='HOME'){if(k===0)S.tempo+=d;if(k===1)S.swingRaw=clamp(S.swingRaw+d,0,100);if(k===2)S.filter=clamp(S.filter+d,0,127);if(k===3)S.pumpRaw=clamp(S.pumpRaw+d,0,127);S.dirty=true;}else if(S.view==='SONG')outcome=songEdit(k,d);else if(S.view==='PART'&&S.part!==2)outcome=editControl(S.page,k,d);else if(S.view==='MIX'&&mixKit()>=0){const kit=mixKit(),vals=kit>=0?S.kits[kit].mix[S.kits[kit].track]:S.mixPart[S.mixsel];const meta=drumRecord(kit,S.kits[kit].track).parameters.find(p=>p.access.mix_knob==='K'+(k+5));setDrumRaw(kit,S.kits[kit].track,meta.local_id,clamp(vals[k]+d,0,127));motionTouch({key:'T_'+(kit?'808':'909')+':'+S.kits[kit].track+':'+meta.local_id});S.dirty=true;}else if(S.part===2&&['SEQ','PART'].includes(S.view)){outcome='BREAK scalar edits unmodeled; unavailable-loop fixture unchanged';}else if(S.view==='SEQ'&&S.part>2){const b=bassData(),n=S.held>=0?b.notes[S.held-1]:null;if(n){if(k===0){n.note=clamp((resolvedNote(b,S.held-1)??n.note)+d,12,108);n.gate='NOTE';}if(k===1){n.gate=['REST','NOTE','TIE'][clamp(['REST','NOTE','TIE'].indexOf(n.gate)+Math.sign(d),0,2)];n.end=0;}if(k===2)n.accent=d>0;if(k===3)n.slide=d>0;if(S.press)S.press.used=true;}else if(S.page===0){if(k===0)b.length=clamp(b.length+d,1,64);if(k===1)b.rate=clamp(b.rate+Math.sign(d),0,D.rates.length-1);if(k===2)b.direction=clamp(b.direction+Math.sign(d),0,3);if(k===3)b.transpose=clamp(b.transpose+d,0,48);}else if(S.page===2&&k===0)S.freePlay[S.sourcePart-2]=d>0;else outcome='Unmodeled preset route; incumbent constraints preserved';S.dirty=true;}else if(S.view==='SEQ'&&S.part<2){if(k===0)S.length=clamp(S.length+d,1,64);if(k===1)S.rate=clamp(S.rate+d,0,D.rates.length-1);if(k<2)S.dirty=true;else outcome='Unmapped control';}else outcome='Layout-only scalar; no accepted edit';}else if(ev==='OCTAVE+'||ev==='OCTAVE-'){if(S.heldSave){S.patternBank=ev.endsWith('+')?1:0;S.usedButtons.push('BT');}else if(S.part>2&&S.held>=0&&!S.rec){const b=bassData(),n=b.notes[S.held-1];n.note=clamp((resolvedNote(b,S.held-1)??n.note)+(ev.endsWith('+')?12:-12),12,108);n.gate='NOTE';if(S.press)S.press.used=true;S.dirty=true;}else if(S.part>2&&(S.view==='PART'||S.rec||S.freePlay[S.sourcePart-2])){if(S.buttons.includes('NOTE REPEAT')&&S.view==='SEQ'){recordStopped('TIE');S.usedButtons.push('NOTE REPEAT');}else if(S.buttons.includes('CHORD')&&S.view==='SEQ'){recordStopped('REST');S.usedButtons.push('CHORD');}else S.octave=clamp(S.octave+(ev.endsWith('+')?1:-1),-2,3);}else if(['HOME','SONG'].includes(S.view))S.patternBank=clamp(S.patternBank+(ev.endsWith('+')?1:-1),0,1);else if(['SEQ','PART'].includes(S.view))S.stepPage=clamp(S.stepPage+(ev.endsWith('+')?1:-1),0,3);}else if(ev==='BT')outcome='SAVE request only; result lifecycle not modeled';else if(ev==='GLOBE'){S.liveNotes={};S.liveEpoch++;outcome='Current Global entry retires keys; held continuity remains unresolved';window.__openReservedSettings?.();}S.note=outcome||'Source route model';trace.push({event:ev+(d<0?' −':''),result:S.note,state:{view:S.view,page:S.page,part:S.part,sourcePart:S.sourcePart}});remember();sync();render();}

function blackDown(n){
 if(S.clipOn){S.note='Clip Keys route; no local target write';render();return;}
 if(S.part>2&&['SEQ','PART'].includes(S.view)&&(S.view==='PART'||S.rec||S.freePlay[S.sourcePart-2])){noteDown(5+BLACK_LOGICAL[n]);return;}if(S.part!==2||!['SEQ','PART'].includes(S.view)){black(n);return;}
 if(n<8&&S.held>=0){const b=partData(4),slot=S.breakPage*8+n+1;if(slot>8){S.note='Unused slice in current empty-loop fixture';render();return;}b.slice[S.held-1]=b.slice[S.held-1]===slot?0:slot;if(S.press)S.press.used=true;S.dirty=true;S.note='Stored step slice pin; no available loop/audio proof';}
 else{S.breakLive={input:n,pattern:S.currentPatterns[4],page:S.breakPage,slice:n<8?S.breakPage*8+n+1:null,effect:n===8?'REV':n===9?'JUNGLE':n===10?'STUTTER':null};S.note='Original BREAK live-override request; empty/unavailable loop remains silent';}render();
}
function blackUp(n){if((5+BLACK_LOGICAL[n]) in S.liveNotes)noteUp(5+BLACK_LOGICAL[n]);if(S.breakLive?.input===n){S.breakLive=null;S.note='Original BREAK override released; current generator/step policy resumes';render();}}
function black(n){
 const kit=mixKit();if(S.clipOn){S.note='Clip Keys route; no local target write';render();return;}
 if(S.part===2&&['SEQ','PART'].includes(S.view)){blackDown(n);blackUp(n);return;}
 if(S.heldPatch&&(kit>=0||S.part<2&&['PART','SEQ'].includes(S.view))){requestTrackMute(kit>=0?kit:S.part,n);S.usedButtons.push('PATCH');S.note='Track mute request; selection and original held owner retained';render();return;}
 if(S.heldPatch&&S.view==='MIX'&&S.page===0&&n<5){requestPartMute(n);S.usedButtons.push('PATCH');render();return;}
 S.press=null;S.held=-1;
 if(S.view==='SONG'&&!S.heldPatch&&n<5){songFill(S.songSel);S.songData[S.songSel].mute^=1<<SOURCE_PART[n];S.dirty=true;}else if(S.view==='HOME'&&n<5)requestPartMute(n);else if(S.view==='MIX'&&S.page===0){if(n<5)S.mixsel=n;}else if(kit>=0)S.kits[kit].track=clamp(n,0,10);else if(S.part<2){S.track=clamp(n,0,10);S.page=0;remember();}S.note='Source target selected; no generated audition';trace.push({event:blacks[n],result:S.note});sync();render();
}

function padDown(n){
 if(!Number.isInteger(n)||n<0||n>=16||S.physicalPadsHeld.includes(n))return;S.lastPadInput=n;S.physicalPadsHeld.push(n);
 const kit=mixKit(),step=S.part===2&&kit<0?n+1:S.stepPage*16+n+1;S.heldInput=step;if(S.clipOn){S.note='No accepted host grid/cell: no local pattern write';return;}if(S.overlay==='ASK'){S.note='Question masks pad input';return;}
 if(S.part<2&&S.view==='PART'&&!S.heldSave){S.note='Selected drum audition; no hidden step written';return;}
 if(divisionOwner()){S.heldInput=-1;if(n<8)S.values[controlKey(13)]=Math.floor((n*127+3)/7);else if(n<12)S.values[controlKey(11)]=Math.floor(((n-8)*127+1)/3);else{S.note='Unused source pad13–16; no action';return;}S.dirty=true;S.note='Miami pad changes only source rate/shape target';return;}
 if(S.heldSave){copyTarget(S.patternBank*16+n);return;}
 if(['SEQ','PART'].includes(S.view)&&S.part>2&&(S.view==='PART'||S.rec||S.freePlay[S.sourcePart-2])){const physical=5+WHITE_LOGICAL[n];S.padKeyOwners[n]=physical;S.lastPadInput=n;noteDown(physical,100,'pad:'+n);return;}
 if(S.view==='SONG'&&!S.heldPatch){songFill(S.songSel);S.songData[S.songSel].pat=Array(5).fill(S.patternBank*16+n);S.songSel=Math.min(191,S.songSel+1);S.dirty=true;S.note='Write all five part patterns; advance Song cursor';return;}
 if(S.view==='HOME'||S.heldPatch){const target=S.patternBank*16+n;if(S.chainFirst>=0&&S.chainFirst!==target)S.chain=[Math.min(S.chainFirst,target),Math.max(S.chainFirst,target)];else{S.chainFirst=target;S.chain=null;cuePattern(target,true);}S.note='All-part cue; Song unchanged';return;}
 if(kit>=0||S.part<2&&['SEQ','PART'].includes(S.view)){
  const ki=kit>=0?kit:S.part,k=S.kits[ki];if(S.buttons.includes('CHORD')){const at=k.accent.indexOf(step);if(at<0)k.accent.push(step);else k.accent.splice(at,1);S.usedButtons.push('CHORD');S.dirty=true;S.note='Source shared drum accent toggled; selected voice hits unchanged';return;}const a=k.steps[k.track],created=!a.includes(step);if(created){a.push(step);S.dirty=true;}
  S.press={step,input:n,kit:ki,voice:k.track,pattern:S.currentPatterns[ki]+1,time:S.now,created,used:false};if(['SEQ','PART'].includes(S.view))S.held=step;S.chance=chanceValue();
 }else if(S.part===2&&['SEQ','PART'].includes(S.view)){const b=partData(4),created=!b.steps.includes(step);if(created)b.steps.push(step);S.held=step;S.press={step,input:n,sourceBreak:true,pattern:S.currentPatterns[4]+1,time:S.now,created,used:false};S.dirty=true;}else if(S.view==='SEQ'&&S.part>2){const b=bassData(),cell=b.notes[step-1],created=cell.gate==='REST';if(created)cell.gate='NOTE';S.held=step;S.chance=chanceValue();S.velocity=cell.velocity;S.press={step,input:n,sourcePart:S.sourcePart,pattern:S.pattern,time:S.now,created,used:false};}
 S.note='Input down; source owner retained';
}
function pad(n,held=false){padDown(n);if(!held)release(80,n);trace.push({event:(held?'Hold ':'Tap ')+'pad'+n,result:S.note});render();}

function release(elapsed=800,input=null){
 const padInput=input??S.lastPadInput??S.press?.input??null;
 if(padInput!==null){S.physicalPadsHeld=S.physicalPadsHeld.filter(n=>n!==padInput);if(S.padKeyOwners[padInput]!==undefined){noteUp(S.padKeyOwners[padInput],'pad:'+padInput);delete S.padKeyOwners[padInput];}}
 S.now+=Number.isFinite(elapsed)?elapsed:800;
 const p=S.press,owns=!p||padInput===null||p.input===padInput;
 if(p&&owns){
  if(p.sourceBreak){if(S.part===2&&p.pattern===S.currentPatterns[4]+1&&!p.created&&!p.used&&elapsed<300){const a=partData(4).steps;a.splice(a.indexOf(p.step),1);}}
  else if(p.sourcePart!==undefined){if(p.sourcePart===S.sourcePart&&p.pattern===S.pattern&&!p.created&&!p.used&&elapsed<300)bassData().notes[p.step-1].gate='REST';}
  else{const k=S.kits[p.kit],valid=p.pattern===S.currentPatterns[p.kit]+1&&k.track===p.voice&&(mixKit()===p.kit||S.part===p.kit&&['SEQ','PART'].includes(S.view));if(valid&&p.step===S.stepPage*16+p.input+1&&!p.created&&!p.used&&elapsed<300){const a=k.steps[p.voice],at=a.indexOf(p.step);if(at>=0){a.splice(at,1);S.dirty=true;}}}
 }
 if(owns){S.press=null;S.held=-1;S.heldInput=-1;}
 if(padInput!==null&&S.chainFirst===S.patternBank*16+padInput)S.chainFirst=-1;
 if(S.lastPadInput===padInput)S.lastPadInput=S.physicalPadsHeld.at(-1)??null;
 S.note=owns?'Original input released; current base/motion scope':'Older input released; newer edit owner retained';trace.push({event:'Release '+elapsed+'ms pad'+padInput,result:S.note});render();
}

function cancelPad(n){S.physicalPadsHeld=S.physicalPadsHeld.filter(i=>i!==n);if(S.padKeyOwners[n]!==undefined){noteUp(S.padKeyOwners[n],'pad:'+n);delete S.padKeyOwners[n];}if(S.press?.input===n){S.press=null;S.held=-1;S.heldInput=-1;}if(S.lastPadInput===n)S.lastPadInput=S.physicalPadsHeld.at(-1)??null;S.note='Prototype input cancelled without short-tap deletion';render();}

function updateLights(){const owner=mixKit()>=0?mixKit():S.part,rgb=[[127,74,20],[127,36,32],[106,60,127],[32,103,127],[65,121,45]][owner],dim=rgb.map(x=>Math.floor(x/4));const pads=[];for(let i=0;i<16;i++){let v=0;if(S.clipOn){v=0;}else if(divisionOwner()){if(S.lightsEnabled&&i<12){const rate=Math.floor((baseQ(13)*7+63)/127),shape=Math.round(baseQ(11)*3/127);v=i===rate||i===8+shape?2:1;}}else if(S.lightsEnabled&&['HOME','SONG'].includes(S.view)){v=S.usedPatterns.includes(S.patternBank*16+i+1)?1:0;if(S.view==='HOME'&&S.patternBank*16+i+1===S.pattern||S.view==='SONG'&&S.songSel<S.songBars&&S.patternBank*16+i===S.songData[S.songSel].pat[0])v=2;if(S.view==='HOME'&&S.patternBank*16+i+1===S.queued&&(S.framePhase&8))v=2;}else if(S.lightsEnabled&&(mixKit()>=0||S.part<2&&['SEQ','PART'].includes(S.view))){const k=S.kits[selectedKit()],step=S.stepPage*16+i+1;let on=step<=k.length&&(S.buttons.includes('CHORD')?k.accent:k.steps[k.track]).includes(step);if(S.playing&&S.stepPage*16+i===k.playhead)on=!on;v=on?2:0;}if(!S.clipOn&&!divisionOwner()&&S.lightsEnabled&&S.part>2&&['SEQ','PART'].includes(S.view)){if(S.view==='SEQ'&&!S.rec&&!S.freePlay[S.sourcePart-2]){const step=S.stepPage*16+i,n=bassData().notes[step],inLoop=step<S.length;let on=inLoop&&n.gate==='NOTE';if(S.playing&&step===S.playhead)on=!on;v=on?2:inLoop&&n.gate==='TIE'?1:0;}else v=(5+WHITE_LOGICAL[i]) in S.liveNotes?2:[4,11].includes(i)?1:0;}if(!S.clipOn&&S.part===2&&['SEQ','PART'].includes(S.view)&&S.lightsEnabled){let on=partData(4).steps.includes(i+1);if(S.playing&&i===S.playhead)on=!on;v=on?2:0;}
 if(!S.clipOn&&!divisionOwner()&&(S.physicalPadsHeld.includes(i)||Object.keys(S.keyOrigins[5+WHITE_LOGICAL[i]]??{}).length))v=2;pads.push(v);}
 document.querySelectorAll('[data-pad]').forEach((b,i)=>{const v=pads[i],color=v===2?rgb:v===1?dim:[0,0,0];b.style.background=`rgb(${color.join(',')})`;b.style.borderColor=v?C.gray:C.line;b.setAttribute('aria-label',`Physical pad${i}, ${v===2?'bright':v===1?'dim':'off'}; kit/part owner${owner}`)});const ss=modelSaveState(),on={'BT':ss==='QUEUED'||(ss==='FAILED'?!!(S.framePhase&2):ss==='UNSAVED'&&!!(S.framePhase&8)),'PATCH':['HOME','SONG'].includes(S.view)||S.songOn&&!!(S.framePhase&8),'SEQ':S.view==='SEQ','PARA':S.view==='PART','FX':S.view==='FX','NOTE REPEAT':S.view==='MIX','SCALE':!!S.overlay,'SEQ REC':S.rec,'SEQ PLAY':S.wait?!!(S.framePhase&32):S.playing&&S.playheads[0]%4===0,'OCTAVE+':['HOME','SONG'].includes(S.view)?S.patternBank>0:S.stepPage>0};document.querySelectorAll('#panelButtons [data-ev]').forEach(b=>{const lit=!!on[b.dataset.ev];b.setAttribute('data-lit',String(lit));b.setAttribute('aria-label',b.dataset.ev+(['ARP','FADER BANK'].includes(b.dataset.ev)||S.clipOn?'; button feedback unmodeled':' ; source LED '+(lit?'on':'off')));});if($('ledState'))$('ledState').textContent=`Model source layer: owner${owner}; pads=${pads.join(' ')}; no physical observation.`;return {pads,buttons:on,owner,rgb,evidence:'Source layer model; not physical test'};}

function sync(){if($('trackButtons'))[...$('trackButtons').children].forEach((b,i)=>{const partScope=S.view==='HOME'||S.view==='MIX'&&S.page===0;b.setAttribute('aria-pressed',String(partScope?i===S.mixsel:i===S.track));b.setAttribute('aria-label',blacks[i]+': '+(partScope?(i<5?laneName(i):'no part binding'):identityName(i,selectedKit())));});}

function mixKit(){return S.view==='MIX'&&[1,2].includes(S.page)?S.page-1:-1;}

function selectedKit(){const m=mixKit();return m>=0?m:S.part<2?S.part:0;}

function laneName(p,header=false){if(p<3)return ['909','808','BREAK'][p];const bank=S.banks[p];return bank+(header||bank===S.banks[p===3?4:3]?(p===3?' B':' A'):'');}

function pageKey(view=S.view,part=S.part){return ['PART','SEQ','GEN'].includes(view)?view+':'+SOURCE_PART[part]:view;}

function remember(){S.pageMemory[pageKey()]=S.page;}

function normalizePage(){if(S.page>=pageCount()||S.page<0)S.page=0;remember();}

function setView(v){if(v==='SEQ'&&S.view==='PART'&&S.held>=0)return false;const keep=S.view==='SEQ'&&v==='PART';remember();if(!keep){S.held=-1;S.press=null;}S.view=v;S.overlay=null;if(!S.playing)S.songOn=v==='SONG';if(v!=='PART'&&!(v==='SEQ'&&S.part>2&&S.freePlay[S.sourcePart-2])){S.liveNotes={};S.liveEpoch++;}S.page=S.pageMemory[pageKey()]??0;normalizePage();return true;}

function controlKey(id,voice=S.track){return S.part<2?'T_'+S.engine+':'+voice+':'+id:S.sourcePart+':'+S.engine+':'+id;}

function effectiveQ(id,voice=S.track){
 const key=controlKey(id,voice),base=baseQ(id,voice),lane=laneFor(key),legacy=S.pattern+':'+S.held+':'+key;
 if(eligibleHeld())return lane?.values[S.held-1]??S.locks[legacy]??base;
 if(lane&&lane.applied!==null)return lane.applied;return S.motion[key]??base;
}

function control(p,k){
 let cell=p>=0?pages()[p]?.cells[k]:S.part<2?(()=>{const m=drumRecord(S.part,S.track).parameters.find(p=>p.local_id===k);return m?{label:m.source_caption,parameter_id:k,editable:true}:null;})():null;
 if(!cell?.label)return null;const id=cell.parameter_id,voice=cell.voice??S.track,key=controlKey(id,voice),meta=S.part<2?drumRecord(S.part,voice).parameters.find(p=>p.local_id===id):null;
 return {...cell,id,voice,meta,domain:S.part<2?'drum':'synth',q:typeof id==='number'?effectiveQ(id,voice):null,scope:eligibleHeld()?(laneFor(key)?.values[S.held-1]!=null||S.pattern+':'+S.held+':'+key in S.locks?'step':'base'):laneFor(key)?.applied!=null||key in S.motion?'motion':'base',key};
}

function enumCount(engine,id){return engine==='CHICAGO'?({11:2,13:2,14:3,15:3,16:10}[id]||0):engine==='MIAMI'?({0:3,3:25,6:4,11:4,13:8}[id]||0):0;}

function controlText(c){if(c.domain==='drum')return drumText(c);if(c.q===null)return c.label==='ENGINE'?S.engine:c.label==='PRESET'?(S.engine==='JUNO'?'JUNO POLY':'01'):'ACTIVE';const q=c.q,id=c.id;
 if(S.engine==='CHICAGO'){if(id===4){const n=Math.round(centeredQ(q)*200)-100;return(n>0?'+':'')+n+'%';}if(id===11||id===13)return q>=64?(id===11?'GATE':'ON'):(id===11?'ENV':'OFF');if(id===14)return ['DOWN','UP','UP/DN'][Math.round(q/127*2)];if(id===15)return ['1 OCT','2 OCT','FULL'][Math.round(q/127*2)];if(id===16)return ['1/2','1/4D','1/2T','1/4','1/8D','1/4T','1/8','1/8T','1/16','1/32'][Math.round(q/127*9)];if(id===17)return ['OFF','CHO/FLG','PHASER','RING MOD','COMPRESSOR','DECIMATOR','EQ','ENSEM','SH DLY','BPM DL','HALL','GT REV'][clamp(q,0,11)];}
 if(S.engine==='MIAMI'){const v=D.miamiUI.edited_q_lookup.controls.find(v=>v.parameter_id===id);if(v)return v.number_by_q[q]+v.unit_by_q[q];if(id===0||id===6)return ['TRI','SAW','SQR','PUL'][Math.round(q/127*((id===0?3:4)-1))];if(id===11)return ['TRI','DOWN','UP','SQR'][Math.round(q/127*3)];if(id===3){const n=Math.round(q/127*24)-12;return(n>0?'+':'')+n+'st';}if(id===13)return ['1/2','1/4','1/8','1/16','1/3','1/6','1/12','1/24'][Math.round(q/127*7)];if(id===7){const n=Math.round((.25+.5*centeredQ(q))*2000)-1000;return(n>0?'+':n<0?'-':'')+(Math.abs(n)/100).toFixed(2);}}
 return Math.floor((q*100+63)/127)+'%';}

function editControl(p,k,d){
 const c=control(p,k);if(!c?.editable||c.q===null)return 'Reserved/unmapped; no edit';
 if(S.heldSave){const lane=laneFor(c.key);if(eligibleHeld()){if(lane){lane.values[S.held-1]=null;if(!lane.values.some(v=>v!==null))delete S.motionLanes[lane.pattern+':'+lane.part+':'+lane.key];}delete S.locks[S.pattern+':'+S.held+':'+c.key];if(S.press)S.press.used=true;}else{if(lane)delete S.motionLanes[lane.pattern+':'+lane.part+':'+lane.key];for(const key of Object.keys(S.locks))if(key.startsWith(S.pattern+':')&&key.endsWith(':'+c.key))delete S.locks[key];}S.usedButtons.push('BT');S.dirty=true;return eligibleHeld()?'Only held step value cleared':'Whole parameter lane cleared';}
 const current=eligibleHeld()?c.q:baseQ(c.id,c.voice),count=c.domain==='drum'?(c.meta.enum_names?.length||0):enumCount(S.engine,c.id),max=c.domain==='drum'?c.meta.maximum_raw:S.engine==='CHICAGO'&&c.id===17?11:127;
 let next=c.domain==='drum'?clamp(current+(count?Math.sign(d):d),c.meta.minimum_raw,max):count?Math.round(clamp(Math.round(current/127*(count-1))+Math.sign(d),0,count-1)*127/(count-1)):clamp(current+d,0,max);
 if(S.engine==='MIAMI'&&count){const v=D.miamiUI.edited_q_lookup.controls.find(v=>v.parameter_id===c.id),names=[...new Set(v.number_by_q)],at=names.indexOf(v.number_by_q[current]);next=Math.round(clamp(at+Math.sign(d),0,names.length-1)*127/(names.length-1));}
 if(eligibleHeld()){const lane=laneFor(c.key,true);if(!lane)return 'MOTION FULL: lock not written';lane.values[S.held-1]=next;if(S.press)S.press.used=true;}else if(c.domain==='drum')setDrumRaw(S.part,c.voice,c.id,next);else S.values[c.key]=next;if(!eligibleHeld())motionTouch(c);
 S.touched={page:p,knob:k,part:S.part,voice:c.voice};if(touchTimer)clearTimeout(touchTimer);touchTimer=setTimeout(()=>{S.touched=null;render();},900);S.dirty=true;return (S.held>=0?'Step '+S.held:'Base')+' '+c.label+' '+controlText({...c,q:next});
}

function paramCurve(f,x,y,w,h,c){let prev=null;for(let i=0;i<w;i++){const v=clamp(f(i/(w-1)),-1,1),ny=y+h/2-v*h*.43;if(prev!==null)line(x+i-1,prev,x+i,ny,c);prev=ny;}line(x,y+h/2,x+w,y+h/2,C.line);}

function semanticGlyph(c,x,y,w,h,ink){if(S.engine==='MIAMI'){const slot=D.miamiUI.edited_q_lookup.wave_slots.find(s=>s.parameter_id===c.id);if(slot){const xx=Math.round(x),yy=Math.round(y),ys=slot.y_by_q[c.q],zero=slot.zero_by_q[c.q];line(xx,yy+zero,xx+28,yy+zero,C.line);for(let i=1;i<29;i++)line(xx+i-1,yy+ys[i-1],xx+i,yy+ys[i],ink);return;}}if(S.engine==='CHICAGO'&&c.id===0){const ys=D.chicagoWave.wave_y_by_q[c.q],xx=Math.round(x),yy=Math.round(y);line(xx,yy+9,xx+28,yy+9,C.line);for(let i=1;i<29;i++)line(xx+i-1,yy+ys[i-1],xx+i,yy+ys[i],ink);return;}if(S.engine==='JUNO'){line(x,y+h/2,x+w,y+h/2,C.line);rect(x+Math.round(c.q/127*(w-3)),y+h/2-2,3,5,ink);return;}if(c.domain==='drum'){drumParameterGlyph(c,x,y,w,h,ink);return;}const q=(c.q??64)/127,id=c.id,chi=S.engine==='CHICAGO',jun=S.engine==='JUNO',mia=S.engine==='MIAMI';const eq=n=>effectiveQ(n)/127;
 if(jun&&id===0){const width=jun ? .1+.8*eq(12) : .5;paramCurve(t=>{const phase=(t*2)%1,saw=phase*2-1,pulse=phase<width?1:-1;return (1-q)*saw+q*pulse;},x,y,w,h,ink);return;}
 if((chi&&id===1)||(mia&&id===4)){const width=1+Math.round(q*(w-4)),l=x+(w-width)/2;line(x,y+h,l,y+h,ink);line(l,y+h,l+width,y+2,ink);line(l+width,y+2,x+w,y+2,ink);return;}
 if(chi&&(id===5||id===6)){paramCurve(t=>{const a=Math.sin(t*Math.PI*3);if(id===5){const gain=.3+6*q;return Math.tanh(gain*a)/Math.tanh(gain);}let v=a*(1+3*q);while(v>1||v< -1)v=v>1?2-v:-2-v;return v;},x,y,w,h,ink);return;}
 if((chi&&[2,3,4].includes(id))||(jun&&[2,3,4,9].includes(id))||(mia&&id===9)){let cut=jun?eq(2)**2:eq(chi?2:9),res=chi?eq(3):jun?eq(3):0;if(id===4){const depth=chi?2*centeredQ(c.q)-1:jun?7000/12000*q:q;const target=clamp(cut+depth,0,1);line(x+target*w,y+h-3,x+target*w,y+h,C.gray);}const pivot=x+3+cut*(w-9),peak=y+4-res*4;if(jun&&id===9){if(c.q===0)line(x,y+h/2,x+w,y+h/2,ink);else{line(x,y+h,x+q*q*(w-7),y+h,ink);line(x+q*q*(w-7),y+h,x+q*q*(w-7)+6,y+3,ink);line(x+q*q*(w-7)+6,y+3,x+w,y+3,ink);}return;}line(x,y+4,pivot-2,y+4,ink);line(pivot-2,y+4,pivot,peak,ink);line(pivot,peak,Math.min(x+w,pivot+6),y+h,ink);line(Math.min(x+w,pivot+6),y+h,x+w,y+h,ink);return;}
 if(jun&&id===12){const duty=.1+.8*q;paramCurve(t=>t<duty?1:-1,x,y,w,h,ink);return;}
 if(jun&&id===1){paramCurve(t=>((t%1)<.5?1:-1)*q*.8,x,y,w,h,ink);return;}
 if(jun&&[5,6,7,8].includes(id)){const sustain=eq(7);if(id===5){const end=x+3+q*q*(w-4);line(x,y+h,end,y,ink);line(end,y,x+w,y,ink);}if(id===6){line(x,y,x+3+q*q*(w-4),y+h*(1-sustain),ink);line(x+3+q*q*(w-4),y+h*(1-sustain),x+w,y+h*(1-sustain),ink);}if(id===7){line(x,y+h*(1-q),x+w,y+h*(1-q),ink);line(x+3,y+h,x+3,y+h*(1-q),ink);}if(id===8)paramCurve(t=>2*Math.exp(-2*t/(.01+4*q*q))-1,x,y,w,h,ink);return;}
 if(jun&&id===14){const amp=.5*q;paramCurve(t=>amp*Math.sin(4*Math.PI*t),x,y,w,8,ink);paramCurve(t=>-amp*Math.sin(4*Math.PI*t),x,y+10,w,8,ink);return;}
 if(jun&&[10,11,13].includes(id)){const cycles=id===10?1+4*q*q:id===13?1+3*q:2,amp=id===11?.1+.9*q:id===14?.1+.8*q:1;paramCurve(t=>amp*(id===10||id===11?1-4*Math.abs((t*cycles%1)-.5):Math.sin(2*Math.PI*t*cycles)),x,y,w,h,ink);if(id===14){line(x,y+h,x+w,y+h,C.gray);line(x,y,x+w,y,C.gray);}return;}
 if(mia&&[0,6,11].includes(id)){const n=enumCount(S.engine,id),v=Math.round(q*(n-1));paramCurve(t=>{t=(t*2)%1;if(id===11)return v===0?1-4*Math.abs(t-.5):v===1?1-2*t:v===2?2*t-1:t<.5?1:-1;return v===0?1-4*Math.abs(t-.5):v===1?2*t-1:v===2?(t<.5?1:-1):(t<.1?1:-1/9);},x,y,w,h,ink);return;}
 if(chi&&id===17){effectGlyph(c.q,x,y,w,h,ink);return;}
 if(chi&&id===14){const dir=Math.round(q*2),yy=y+h/2;line(x,y+h,x+w,y,C.line);if(dir!==0){line(x,y+h,x+w,y,ink);line(x+w-5,y,x+w,y,ink);line(x+w,y,x+w,y+5,ink);}if(dir!==1){line(x,y,x+w,y+h,ink);line(x+w-5,y+h,x+w,y+h,ink);line(x+w,y+h-5,x+w,y+h,ink);}return;}
 if(chi&&id===15){const n=1+Math.round(q*2);for(let a=0;a<3;a++){const xx=x+a*w/3;line(xx,y+h,xx+w/3-2,y+h,ink);if(a<n){rect(xx+2,y+h-4-a*4,3,3,ink);line(xx+5,y+h-3-a*4,xx+5,y+1,ink);}}return;}
 if((chi&&id===16)||(mia&&id===13)){const n=chi?Math.round(q*9):Math.round(q*7);for(let a=0;a<Math.min(8,n+1);a++){const xx=x+a*w/Math.min(8,n+1);line(xx,y+h,xx,y+3,ink);}return;}
 if(chi&&id===13){if(q<.5){line(x,y+h/2,x+w,y+h/2,ink);line(x+w/2-4,y+2,x+w/2+4,y+h-2,ink);}else for(let a=0;a<3;a++){rect(x+a*8,y+h-3-a*5,3,3,ink);line(x+a*8+3,y+h-a*5,x+a*8+3,y+1,ink);}return;}
 if(mia&&[2,8,10,12,15].includes(id)){if(id===15){line(x,y+h/2,x+w,y+h/2,C.line);line(x+w/2,y,x+w/2,y+h,C.line);line(x+w/2-q*w/2,y+3,x+w/2+q*w/2,y+h-3,ink);}else if(id===2)paramCurve(t=>q*(t%1<.5?1:-1),x,y,w,h,ink);else if(id===8)paramCurve(t=>Math.sin(3*Math.PI*t+q*1.6*Math.sin(3*Math.PI*t)),x,y,w,h,ink);else if(id===10)paramCurve(t=>{const steps=12-Math.round(q*10);return Math.round(Math.sin(3*Math.PI*t)*steps)/steps;},x,y,w,h,ink);else paramCurve(t=>(.1+.9*q)*(1-4*Math.abs((t*2%1)-.5)),x,y,w,h,ink);return;}
 if((chi&&id===12)||(jun&&id===15)||(mia&&[1,5,14].includes(id))){for(let a=0;a<6;a++)rect(x+a*w/6,y+h-a*2,w/6-1,a*2+1,a<Math.round(q*6)?ink:C.line);return;}
 if(chi&&(id===11||id===13)){if(q>=.5){line(x,y+h,x,y+2,ink);line(x,y+2,x+w,y+2,ink);line(x+w,y+2,x+w,y+h,ink);}else{line(x,y+h,x+w*.2,y+2,ink);line(x+w*.2,y+2,x+w*.5,y+h*.6,ink);line(x+w*.5,y+h*.6,x+w,y+h,ink);}return;}
 // Other explicitly bounded scalar/selector cues retain their parameter label and value.
 graph(c.label,x,y,w,h,ink,c.q===null?0:Math.floor((c.q*100+63)/127));
}

function envelopeRow(p,y,ink){
 const cc=[0,1,2,3].map(k=>control(p,k)),v=cc.map(c=>c.q/127),jun=S.engine==='JUNO',edited=S.touched?.page===p&&S.touched?.part===S.part?S.touched.knob:-1;
 if(edited>=0)text(cc[edited].label+' '+controlText(cc[edited]),232,y,10,C.ink,true,'right');
 for(let k=0;k<4;k++)text('K'+(k+5)+' '+['A','D','S','R'][k]+' '+controlText(cc[k]),31+k*59,y+8,10,k===edited?C.ink:C.gray,false,'center');
 // The Chicago full-row layout/labels are existing firmware. This fallback is identified as a q-only schematic until native compilation is bound.
 const a=jun?.002+3*v[0]**2:13*v[0]**4+.001,d=jun?.01+3*v[1]**2:25*v[1]**4+.001,r=jun?.01+4*v[3]**2:25*v[3]**4+.002,sus=v[2];
 const dur=jun?[a,d,sus>1e-5?Math.log(1e-5/sus)/(48000*Math.log(1-1/Math.max(1,r*48000))):0]:[a*1.466337,d*4.6051702,sus>.04?r*Math.log(sus/.04):0];
 const sum=dur.reduce((a,b)=>a+b,0),ww=dur.map(z=>3+Math.round(163*z/(sum||1)));ww[1]+=172-ww.reduce((a,b)=>a+b,0);if(ww[1]<3){ww[2]-=3-ww[1];ww[1]=3;}
 const xx=[13,13+ww[0],13+ww[0]+ww[1],13+ww[0]+ww[1]+42,227],top=y+22,bot=y+44,ys=bot-Math.round(sus*(bot-top));
 line(13,bot,227,bot,C.side);line(xx[3],top,xx[3],ys-2,C.dim);
 for(let stage=0;stage<3;stage++){const from=stage===2?3:stage,to=from+1,n=Math.max(1,xx[to]-xx[from]);let px=xx[from],py=stage===0?bot:stage===1?top:ys;
  for(let i=1;i<=n;i++){const t=dur[stage]*i/n,u=i/n,level=jun?(stage===0?u:stage===1?1-(1-sus)*u:sus*(1-1/Math.max(1,r*48000))**Math.round(t*48000)):(stage===0?Math.min(1,1.3-1.3*Math.exp(-t/a)):stage===1?sus+(1-sus)*Math.exp(-t/d):sus*Math.exp(-t/r));const ny=stage===2&&i===n?bot:bot-Math.round(level*(bot-top));line(px,py,xx[from]+i,ny,ink);px=xx[from]+i;py=ny;}
 }line(xx[2],ys,xx[3],ys,ink);if(edited>=0)rect(xx[edited],bot+2,Math.max(1,xx[edited+1]-xx[edited]+1),2,C.ink);
}

function mixValues(v){return [v[0]===0?'OFF':(20*Math.log10(v[0]/127*(mixKit()===0?1.35:mixKit()===1?2:1))).toFixed(1)+'dB',v[1]===64?'C':v[1]<64?'L'+(64-v[1]):'R'+(v[1]-64),Math.round(v[2]*100/127)+'%',Math.round(v[3]*100/127)+'%'];}


window.proposal={reset,event,black,pad,padDown,release,render,pages,pageCount,drumGlyph,control,controlText,semanticGlyph,prepareJourney,cancelPad,blackDown,blackUp,noteDown,noteUp,buttonDown,buttonUp,step:simulatedStep,boundary:applyBoundary,lights:updateLights,configure(patch){const page=patch.page;const p={...patch};if(p.part!==undefined)S.part=p.part;if('engine' in p){if(p.engine==='DRONE')throw new RangeError('Active Drone unavailable; archived identity/fallback remains with firmware owner.');if(S.part>2)S.banks[S.part]=p.engine;delete p.engine;}delete p.sourcePart;Object.assign(S,p);if(page!==undefined)remember();normalizePage();sync();render();},get state(){return structuredClone(S)},get trace(){return structuredClone(trace)}};

function centeredQ(q){return q<=64?q/128:.5+(q-64)/126;}

// Existing qualitative effect families from ui.c orig_fx_glyph, in the same physical cell.
function effectGlyph(t,x,y,w,h,c){g.save();g.translate(x,y);g.scale(w/26,h/18);const r=25,b=18,m=9;if(t===0){line(0,m,r,m,c);line(10,2,15,b-2,c);}else if(t===1||t===7){const n=t===7?3:2;for(let row=0;row<n;row++){const yy=n===3?3+row*5:5+row*7;let py=yy;for(let k=1;k<=4;k++){const ny=yy+(k%2?2:-2);line((k-1)*6,py,k*6,ny,c);py=ny;}}}else if(t===2){line(0,5,3,5,c);for(let i=0;i<2;i++){const a=3+i*10;line(a,5,a+4,b-2,c);line(a+4,b-2,a+8,5,c);line(a+8,5,i?r:a+10,5,c);}}else if(t===3){for(let i=0;i<6;i++){const a=i*4,hh=i<3?2+i*2:12-i*2;line(a,m,a+2,m+(i%2?hh:-hh),c);line(a+2,m+(i%2?hh:-hh),a+4,m,c);}}else if(t===4){line(0,b,11,7,c);line(11,7,r,3,c);}else if(t===5){for(let i=0;i<4;i++){const a=i*6,yy=15-i*4;line(a,yy,a+6,yy,c);if(i<3)line(a+6,yy,a+6,yy-4,c);}}else if(t===6){line(0,m,6,m,c);line(6,m,12,2,c);line(12,2,18,m,c);line(18,m,r,m,c);}else if(t===8||t===9){const start=t===9?12:0;if(t===9){line(0,3,8,3,c);line(8,3,8,11,c);line(8,11,0,11,c);line(0,11,0,3,c);line(4,5,4,7,c);line(4,7,6,7,c);}for(let i=0;i<3;i++)rect(start+i*(t===9?6:9),b-12+i*4,t===9?1:2,12-i*4,c);line(0,b,r,b,c);}else if(t===10){line(0,b,0,2,c);line(0,2,r,2,c);line(r,2,r,b,c);for(let i=0;i<5;i++)rect(3+i*4,5+i,1,10-i*2,c);}else if(t===11){for(let i=0;i<4;i++)rect(i*5,3+i*2,2,12-i*2,c);rect(20,1,2,17,c);line(22,b,r,b,c);}g.restore();}

// Typed drum storage/formatting. Source local IDs never pass through synth formatting.
function drumRecord(kit,voice){return D.typedDrums[kit].records.find(r=>r.voice_id===voice);}
function drumRaw(kit,voice,id){const p=drumRecord(kit,voice).parameters.find(p=>p.local_id===id),a=p.access.mix_knob;return a&&voice<11?S.kits[kit].mix[voice][+a[1]-5]:S.kits[kit].parameters[voice][id];}
function setDrumRaw(kit,voice,id,q){const p=drumRecord(kit,voice).parameters.find(p=>p.local_id===id),a=p.access.mix_knob;if(a&&voice<11)S.kits[kit].mix[voice][+a[1]-5]=q;S.kits[kit].parameters[voice][id]=q;}
function mixDrumControls(kit,voice){const r=drumRecord(kit,voice);return [5,6,7,8].map(k=>{const meta=r.parameters.find(p=>p.access.mix_knob==='K'+k);return {domain:'drum',kit,voice,meta,id:meta.local_id,q:drumRaw(kit,voice,meta.local_id)};});}
function drumEngineering(c){const p=c.meta,curve=p.dsp_curve,kit=c.kit??S.part,q=c.q;let z=curve;
 if(curve.by_sound){const rec=drumRecord(kit,c.voice),variant=kit===1&&c.voice>=2&&c.voice<=4?S.variants.toms:kit===1&&c.voice===5?S.variants.rim:kit===1&&c.voice===6?S.variants.clap:0;z=curve.by_sound[rec.sound_variants?.[0]]||Object.values(curve.by_sound)[0];}
 const t=Math.fround(q/127);if(z.kind==='public_generated_binary32_exp'||z.kind==='positive_exponential'){const lo=Math.fround(z.minimum),hi=Math.fround(z.maximum),ratio=Math.fround(hi/lo);return Math.fround(lo*Math.fround(ratio**t));}
 if(z.kind==='raw_enum_passthrough')return q;if(z.minimum!==undefined&&z.maximum!==undefined)return Math.fround(Math.fround(z.minimum)+Math.fround(Math.fround(z.maximum-z.minimum)*t));return q/127;
}
function shortDecimal(x,sign=false){const t=Math.trunc(x*10+(x<0?-.5:.5)),a=Math.abs(t),prefix=t<0?'-':sign&&t>0?'+':'';return prefix+(a<100?Math.floor(a/10)+'.'+a%10:Math.floor((a+5)/10));}
function drumText(c){const p=c.meta,q=c.q,r=p.source_readout.rule,x=drumEngineering(c);
 if(r==='raw_integer')return String(q);if(r==='enum_name')return p.enum_names[q]??'INVALID';if(r==='pan_L64_C_R63')return q===64?'C':q<64?'L'+(64-q):'R'+(q-64);
 if(r==='gain_to_dB_or_OFF')return x<=0?'OFF':shortDecimal(20*Math.log10(x),true)+'dB';
 if(r==='engineering_value_times_100_rounded')return Math.floor(x*100+.5)+'%';if(r==='raw_over_127_times_100_rounded')return Math.floor(q*100/127+.5)+'%';if(r==='808_snare_tone_mid64_piecewise_percent')return Math.floor(100*centeredQ(q)+.5)+'%';
 if(r==='milliseconds_auto_ms_or_s')return x<999.5?Math.floor(x+.5)+'ms':shortDecimal(x/1000)+'s';if(r==='seconds_auto_ms_or_s')return x<.9995?Math.floor(x*1000+.5)+'ms':shortDecimal(x)+'s';if(r==='hertz_auto_Hz_or_kHz')return x<999.5?Math.floor(x+.5)+'Hz':shortDecimal(x/1000)+'kHz';if(r==='ratio_to_signed_semitones')return shortDecimal(12*Math.log2(x),true)+'st';if(r==='signed_semitones')return shortDecimal(x,true)+'st';throw new Error('Unsupported typed readout '+r);
}
function drumParameterGlyph(c,x,y,w,h,ink){
 const role=c.meta.drum_parameter_glyph,q=c.q/c.meta.maximum_raw,W=26,H=18,bot=y+H,top=y+1,right=x+25;
 const base=()=>line(x,bot,right,bot,C.line),rnd=i=>{let z=Math.imul(i,2654435761)>>>0;z^=z>>>13;return (z>>>0)%100;};
 const si=id=>{const meta=drumRecord(S.part,c.voice).parameters.find(p=>p.local_id===id);return drumEngineering({...c,meta,q:effectiveQ(id,c.voice)});};
 const tail=(ms,noisy=false)=>{const span=clamp(2+Math.round(23*Math.sqrt(clamp(ms,0,4000)/4000)),2,25);base();let last=-1;for(let i=0;i<=span;i++){let hh=Math.round(2**(-9.966*i/span)*16);if(noisy)hh=Math.trunc(hh*(40+rnd(i*7))/140);hh=Math.max(hh,1);if(noisy)rect(x+i,bot-hh,1,hh,ink);else{rect(x+i,bot-hh,1,1,ink);if(last>hh+1)rect(x+i-1,bot-last,1,last-hh,ink);}last=hh;}};
 if(role==='DG_DECAY'){const r=c.meta.source_readout.rule,v=drumEngineering(c);if(r==='milliseconds_auto_ms_or_s')tail(v);else if(r==='seconds_auto_ms_or_s')tail(v*1000);else {const span=2+Math.round(q*23);base();let py=bot-16;for(let i=0;i<=span;i++){const yy=bot-Math.max(1,Math.round(2**(-9.966*i/span)*16));if(i)line(x+i-1,py,x+i,yy,ink);py=yy;}}return;}
 if(role==='DG_NTAIL'){tail(24+.7*drumEngineering(c),true);return;}
 if(role==='DG_BDSWEEP'){const tau=clamp(si(0),6,32),m=clamp(si(4),0,1),pm=si(5),b=49*(1+m*(pm-1)),df=4.6*tau*(1+1.2*m);base();let py=0;for(let i=0;i<26;i++){const t=i*100/25-16,hz=b+(t>0?df*Math.exp(-t/tau):df),yy=bot-1-Math.round(clamp(Math.log2(hz/40)/3.32,0,1)*16);if(i)line(x+i-1,py,x+i,yy,ink);py=yy;}return;}
 if(role==='DG_TUNE'){base();const cyclic=1+2*q,amp=4;line(x,top,x+8,top+3,ink);line(x+8,top+3,right,top+4,ink);let py=y+12;for(let i=0;i<26;i++){const yy=y+12-Math.trunc(amp*Math.sin(6.2831853*i*cyclic/25)+.5);if(i)line(x+i-1,py,x+i,yy,ink);py=yy;}rect(x+Math.round(q*25),bot-1,1,2,ink);return;}
 if(role==='DG_ATTACK'){base();const hh=2+Math.round(q*15);rect(x+2,bot-hh,2,hh,ink);for(let i=5;i<26;i++){const bh=Math.round(7*2**(-(i-5)/6));if(bh>0)rect(x+i,bot-bh,1,1,ink);}return;}
 if(role==='DG_LPF'){base();const hz=300*2**(q*4.3219),knee=x+Math.round(Math.log2(hz/100)/Math.log2(100)*25),level=top+3,x2=Math.min(right,knee+Math.trunc((bot-level)/2));line(x,level,knee,level,ink);line(knee,level,x2,level+(x2-knee)*2,ink);if(x2<right)line(x2,bot,right,bot,ink);return;}
 if(role==='DG_SHELLS'){base();const t=centeredQ(c.q)*1.5708;[Math.cos(t),Math.sin(t)].forEach((v,i)=>{const xx=x+7+i*10,hh=Math.round(v*15);line(xx-4,bot,xx,bot-hh,ink);line(xx,bot-hh,xx+4,bot,ink);});return;}
 if(role==='DG_SNAPPY'){base();for(let i=0;i<26;i+=2){const hh=Math.round(q*2**(-i/9)*16*(35+rnd(i*3))/135);rect(x+i,bot-Math.max(1,hh),1,Math.max(1,hh),hh?ink:C.line);}return;}
 if(role==='DG_SOUND'){const n=c.meta.maximum_raw+1,ww=Math.trunc(24/n);for(let i=0;i<n;i++){const xx=x+1+i*ww;if(i===c.q)rect(xx,y+4,ww-3,9,ink);else{line(xx,y+4,xx+ww-3,y+4,C.line);line(xx,y+4,xx,y+13,C.line);line(xx,y+13,xx+ww-3,y+13,C.line);line(xx+ww-3,y+4,xx+ww-3,y+13,C.line);}}return;}
 if(role==='DG_LEVEL'){base();for(let i=0;i<6;i++)rect(x+i*4,bot-3-i*2,3,2,i<Math.round(q*6)?ink:C.line);return;}
 if(role==='DG_ACCENT'){base();const heights=[Math.max(1,Math.round(16*(1-.7*q)*.6)),10];for(let i=0;i<2;i++){const xx=x+3+i*13;line(xx-3,bot,xx,bot-heights[i],ink);line(xx,bot-heights[i],xx+4,bot,ink);}line(x+7,top,x+9,top+2,ink);line(x+9,top+2,x+7,top+4,ink);line(x+19,top,x+21,top+2,ink);line(x+21,top+2,x+19,top+4,ink);return;}
 // The source has no dedicated glyph for this scalar. Keep its neutral value bar.
 line(x,y+h/2,x+w,y+h/2,C.line);rect(x+Math.round(q*(w-3)),y+h/2-2,3,5,ink);
}

/* Proposal extensions explicitly reuse current UI families; no live audio cursor. */
function sharedFilter(x,y,w,h,mode,cut,res,ink){
 const l=x,r=x+w-1,p=l+Math.round((7+cut*24/127)*(w-1)/38),pass=y+Math.round(h*8/18),floor=y+h-1,peak=pass-Math.round(res/127*h*7/18),sx=v=>v*(w-1)/38;
 line(l,floor,r,floor,C.line);if(mode===0){line(l,y+h/2,r,y+h/2,C.dim);return;}
 if(mode===1){line(l,pass,p-sx(4),pass,ink);line(p-sx(4),pass,p,peak,ink);line(p,peak,p+sx(6),floor,ink);line(p+sx(6),floor,r,floor,ink);}
 else{line(l,floor,p-sx(6),floor,ink);line(p-sx(6),floor,p,peak,ink);line(p,peak,p+sx(4),pass,ink);line(p+sx(4),pass,r,pass,ink);}
}

function patternPicture(x,y){
 // Native9x9 pattern identity; Home's five-part musical thumbnails remain in Home.
 const rows=S.songOn?[0x1e0,0x120,0x1e0,0x040,0x07c,0x004,0x00f,0x009,0x00f]:[0x1b6,0x1b6,0,0x1b6,0x1b6,0,0x1b6,0x1b6,0];for(let r=0;r<9;r++)for(let c=0;c<9;c++)if(rows[r]&(1<<(8-c)))rect(x+c,y+3+r,1,1,C.white);
}

function matrixFooter(first,last){
 slab(0,227,240,13);if(eligibleHeld()){text('STEP '+S.held+' / '+S.chance+(typeof S.chance==='number'?'%':'')+'  K2: CHANCE',7,228,10,C.gray);}else{
  text('< >',7,228,10,C.gray);for(let r=0;r<3;r++)rect(31,229+r*3,10,r===1?2:1,C.gray);text('PARA',50,228,10,C.gray);g.strokeStyle=C.gray;g.strokeRect(76.5,229.5,7,8);line(86,233,92,233,C.gray);line(89,230,92,233,C.gray);line(89,236,92,233,C.gray);text('SCALE: ALL',103,228,10,C.gray);
  if(divisionOwner())text('PADS '+controlText({domain:'synth',q:baseQ(13),id:13}),211,228,10,C.amber,false,'right');
 }text(first+'-'+last,235,228,10,C.amber,false,'right');
}

function junoMatrix(){
 // Current Juno large readouts and neutral bars are retained. New shared-family
 // heading thumbnails are withdrawn; scoped feedback uses only the selected-row gutter.
 const pp=pages(),first=Math.floor(S.page/4)*4;
 for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page,ink=active?C.ink:C.gray;slab(3,y,234,49,active);text((p+1)+' '+row.title,8,y,10,ink,true);if(active)text('K5 6 7 8',232,y,10,C.ink,true,'right');
  rowScopeCue(p,105,y+1);
  for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const x=4+k*59,c=control(p,k);if(k)line(x,y+14,x,y+45,C.line);scopeDot(c,x+3,y+15);text(cell.label.toUpperCase(),x+29,y+12,11,ink,false,'center');text(controlText(c),x+29,y+26,row.title==='SOUND'?12:18,active?C.ink:C.value,true,'center');line(x+8,y+45,x+50,y+45,C.line);if(c.q!==null)rect(x+8,y+45,1+Math.floor(41*c.q/127),1,active?C.ink:C.value);}
 }
 matrixFooter(first+1,Math.min(first+4,pp.length));
}

function performSourceGlyph(name,x,y,w,h,ink){
 if(!['TEMPO','SWING','FILTER','PUMP'].includes(name))return false;
 const xx=v=>x+(v+19)*w/38,yy=v=>y+(v-28)*h/20,L=(a,b,c,d,col=ink)=>line(xx(a),yy(b),xx(c),yy(d),col),B=(a,b,ww,hh,col=ink)=>rect(xx(a),yy(b),Math.max(1,ww*w/38),Math.max(1,hh*h/20),col);
 if(name==='TEMPO'){L(-10,47,-4,28);L(-4,28,4,28);L(4,28,10,47);L(-10,47,10,47);const phase=S.metroView?S.metroView-1:0,arm=S.metroView?(phase<16?phase-8:24-phase):0;L(0,43,arm,30,S.metroView?C.white:C.gray);B(arm*3/4-2,32,5,3);if(S.metroView&&phase<4)B(-2,45,5,2);}
 if(name==='SWING'){const middle=Math.trunc(S.swingRaw*7/127);B(-18,43,37,1,C.line);B(-18,30,3,14);B(middle,34,3,10);B(16,30,3,14);}
 if(name==='FILTER'){const q=S.filter,mode=q<60?1:q>68?3:0,cut=q<60?8+Math.trunc(q*119/59):q>68?Math.trunc((q-69)*120/58):127;sharedFilter(x,y,w,h,mode,cut,0,ink);}
 if(name==='PUMP'){const depth=Math.trunc(S.pumpRaw*14/127);L(-18,31,-9,31);L(-9,31,-6,31+depth);L(-6,31+depth,15,31);B(15,31,4,1);B(-18,48,37,1,C.line);}return true;
}

function valueInk(c,active){if(S.engine==='CHICAGO'&&[14,15,16].includes(c.id)&&effectiveQ(13)<64)return C.dim;return active?C.ink:C.value;}
