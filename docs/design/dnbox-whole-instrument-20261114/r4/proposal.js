/* SPDX-License-Identifier: GPL-3.0-only */
/* Qualitative UI projections from pinned3c41 ui.c/ui_original_controls.h and juno6.c; not audio execution. */
// Source-derived proposal state. part is display position; sourcePart preserves firmware lane IDs.
const SOURCE_PART=[0,1,4,3,2], clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const CHI_Q=[0,18,32,95,99,105,0,34,51,72,22,0,102,0,0,0,0,0,0,0];
const MIA_Q=[0,100,0,64,0,100,0,64,0,76,0,0,64,0,100,0];
const JUN_Q=[42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100];

/* Design model only. Existing physical routes are retained; no firmware or audio. */
const C={bg:'#050a0d',face:'#101a1f',edge:'#26383d',side:'#070e11',sel:'#253b2d',selEdge:'#3e5c40',ink:'#e0ebd6',white:'#dee7df',gray:'#9caaa8',dim:'#809294',line:'#28383d',green:'#8fbf79',amber:'#c49b45'};
const parts=['909','808','BREAK','CHICAGO B','CHICAGO A'];
const partInk=['#ff9529','#ff4841','#d579ff','#41ceff','#83f25a'];
const drumNames=['KICK','SNARE','LOW TOM','MID TOM','HIGH TOM','RIMSHOT','HANDCLAP','CLOSED HAT','OPEN HAT','CRASH','RIDE'];
const blacks=['F#3','G#3','A#3','C#4','D#4','F#4','G#4','A#4','C#5','D#5','F#5'];
const $=id=>document.getElementById(id),cv=$('lcd'),g=cv.getContext('2d');
let S,trace=[];const bounds=[];
function reset(fixture='drum'){
 if(fixture==='drone')throw new RangeError('Active Drone fixture withdrawn; saved-unavailable/fallback contract remains with firmware owner.');
 const kit=()=>({track:0,steps:[[1,5,9,13],[],[],[],[],[],[],[],[],[],[]],length:16,rate:0,playhead:4,mix:Array.from({length:11},()=>[61,64,0,0])});
 S={view:'SEQ',part:0,page:0,pageMemory:{},banks:{3:'CHICAGO',4:'MIAMI'},controls:{3:[...CHI_Q],4:[...MIA_Q]},kits:[kit(),kit()],mixsel:0,mixPart:Array.from({length:5},()=>[100,64,0,0]),stepPage:0,melodicLength:16,playing:true,wait:false,rec:false,held:-1,heldInput:-1,chance:95,velocity:100,muted:[],pattern:1,queued:0,power:3,tempo:125,swing:50,pump:4,filter:64,lightsEnabled:true,dirty:true,overlay:null,list:0,save:null,values:{},locks:{},motion:{},variants:{toms:0,rim:0,clap:0},heldPatch:false,songBars:0,framePhase:0,note:'Controlled source-value fixture; no audio, recording or storage',cueEvidence:'Qualitative source-law diagrams; no measured live cursor/native factory parity'};
 Object.defineProperties(S,{engine:{enumerable:true,get(){return this.banks[this.part]||'CHICAGO';}},sourcePart:{enumerable:true,get(){return SOURCE_PART[this.part];}},track:{enumerable:true,get(){return this.kits[selectedKit()].track;},set(v){this.kits[selectedKit()].track=clamp(v,0,10);}},steps:{enumerable:true,get(){return this.kits[selectedKit()].steps;},set(v){this.kits[selectedKit()].steps=v;}},length:{enumerable:true,get(){return this.part<2||mixKit()>=0?this.kits[selectedKit()].length:this.melodicLength;},set(v){if(this.part<2||mixKit()>=0)this.kits[selectedKit()].length=v;else this.melodicLength=v;}},rate:{enumerable:true,get(){return this.kits[selectedKit()].rate;},set(v){this.kits[selectedKit()].rate=v;}},playhead:{enumerable:true,get(){return this.kits[selectedKit()].playhead;},set(v){this.kits[selectedKit()].playhead=v;}}});
 if(fixture==='808variant'){S.part=1;S.track=2;S.variants={toms:1,rim:1,clap:1};}
 if(['chicago','miami','juno','lock','melodic','chicago-envelope','juno-envelope'].includes(fixture)){S.part=['miami','juno','juno-envelope'].includes(fixture)?4:3;S.view='PART';S.playing=false;if(fixture.startsWith('juno')){S.banks[4]='JUNO';S.controls[4]=[...JUN_Q];}S.page=['chicago','lock'].includes(fixture)?1:fixture.endsWith('envelope')?(fixture.startsWith('juno')?2:3):0;}
 if(['home','wait','savefail'].includes(fixture)){S.view='HOME';S.playing=fixture==='home';S.wait=fixture==='wait';S.save=fixture==='savefail'?'SAVE FAILED':null;}
 if(fixture==='song'){S.view='SONG';S.playing=false;}
 if(fixture==='break'){S.part=2;S.view='PART';}
 if(fixture==='fx'){S.view='FX';S.page=1;}
 if(['mix','mix909','mix808'].includes(fixture)){S.view='MIX';S.page=fixture==='mix909'?1:fixture==='mix808'?2:0;S.playing=false;}
 if(fixture==='load'){S.view='HOME';S.playing=false;S.overlay='ASK';}
 S.kits[0].mix=[61,58,52,52,52,103,122,89,60,47,42].map(v=>[v,64,0,0]);S.kits[1].mix=Array.from({length:11},()=>[64,64,0,0]);S.kits[1].steps[0]=[];remember();trace=[];sync();render();
}

function rect(x,y,w,h,c){g.fillStyle=c;g.fillRect(x,y,w,h)}
function line(x,y,x2,y2,c,w=1){g.strokeStyle=c;g.lineWidth=w;g.beginPath();g.moveTo(x+.5,y+.5);g.lineTo(x2+.5,y2+.5);g.stroke()}
function text(t,x,y,size=10,c=C.gray,bold=false,align='left'){
 if(t==null)return;g.font=`${bold?'600':'500'} ${size}px Barlow`;g.textBaseline='top';g.textAlign=align;g.fillStyle=c;g.fillText(String(t),x,y);
 const width=g.measureText(String(t)).width;bounds.push({text:String(t),x:align==='center'?x-width/2:align==='right'?x-width:x,y,width,height:size});
}
function slab(x,y,w,h,active=false){rect(x,y,w,h,active?C.sel:C.face);line(x,y,x+w-1,y,active?C.selEdge:C.edge);line(x,y,x,y+h-1,C.edge);line(x+w-1,y,x+w-1,y+h-1,C.side);line(x,y+h-1,x+w-1,y+h-1,C.side)}
function stepCell(x,y,w,h,ink,raised=false){g.fillStyle=ink;g.beginPath();g.roundRect(x,y,w,h,2);g.fill();line(x+2,y+1,x+w-3,y+1,raised?'#e5ba74':C.edge);line(x+2,y+h-2,x+w-3,y+h-2,C.side);}
function triangle(x,y,c){g.fillStyle=c;g.beginPath();g.moveTo(x,y);g.lineTo(x+7,y+5);g.lineTo(x,y+10);g.fill()}
function ellipse(x,y,rx,ry,c){g.strokeStyle=c;g.lineWidth=1;g.beginPath();g.ellipse(x,y,rx,ry,0,0,Math.PI*2);g.stroke()}
function power(x,y){const c=C.gray;if(S.power===4){line(x+1,y+1,x+1,y+5,c);line(x+5,y+1,x+5,y+5,c);rect(x,y+4,7,4,c);line(x+3,y+8,x+3,y+10,c);return;}rect(x,y,13,7,C.edge);rect(x+13,y+2,2,3,C.edge);rect(x+1,y+1,11,5,C.bg);if(S.power<0){text('?',x+6,y-2,9,c,true,'center');return;}for(let i=0;i<S.power;i++)rect(x+2+i*3,y+2,2,3,c);}
// Physical percussion pictograms, drawn at the current 24px label-column size.
// These are new proposal assets; recognition is not presumed from source tests.
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
 const k=String(kind||'').toUpperCase();
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
function header(){
 slab(0,0,240,20);const label=S.view==='PART'?(S.part<2?(S.part?'808':'909'):laneName(S.part,true)):S.view==='SEQ'?(S.part<2?(S.part?'808':'909'):S.part===2?'BREAK':laneName(S.part,true))+' SEQ':S.view==='MIX'&&mixKit()>=0?'MIX: '+(mixKit()?'808':'909'):S.view;
 text(label,5,1,15,S.part>2&&S.view==='PART'?C.green:partInk[mixKit()>=0?mixKit():S.part],true);
 if(S.save)text('!',120,2,14,'#ee745c',true);
 if(S.dirty){rect(236,2,3,3,S.save?'#ee745c':C.amber);if(!(S.view==='PART'||S.view==='SEQ'&&S.part>2)){rect(70,3,56,12,partInk[mixKit()>=0?mixKit():S.part]);text('UNSAVED',98,3,10,C.bg,true,'center');}}
 if(S.view==='SONG'){text('BAR1',147,3,11,C.amber,true,'center');}else{rect(132,4,10,12,C.edge);for(let r=0;r<5;r++)line(134,5+r*2,140,5+r*2,r===0?partInk[0]:C.dim);text(S.pattern,151,3,12,C.white,true,'center');}if(S.queued){text('>',161,3,12,C.amber,true);text(S.queued,173,3,12,C.amber,true);}
 if(S.view==='PART'&&S.part!==2){triangle(179,4,S.playing?C.white:C.dim);const n=Math.ceil(pageCount()/4);for(let i=0;i<n;i++){rect(231-(n-i-1)*10,7,4,4,i===Math.floor(S.page/4)?C.amber:C.edge);}if(S.rec)rect(190,5,5,5,'#f05e56');return;}
 if(S.wait){text('EXT',187,3,11,C.amber,true,'right');text('WAIT',229,3,12,C.amber,true,'right');}else {text(S.tempo,213,2,14,C.amber,true,'right');triangle(218,4,S.playing?C.white:C.dim);}if(S.rec)rect(232,4,5,5,'#f05e56');
}
function cards(title,names,values,footer){
 slab(3,158,234,11);text(title,8,158,11,S.view==='MIX'&&mixKit()>=0?partInk[mixKit()]:C.amber);if(S.view==='MIX')for(let dot=0;dot<5;dot++)rect(181+dot*6,162,3,3,dot===S.page?C.ink:C.edge);power(218,160);
 for(let i=0;i<4;i++){slab(i*60+3,171,54,55);if(!names[i])continue;text(names[i],i*60+30,171,11,C.gray,false,'center');graph(names[i],i*60+10,188,39,17,partInk[mixKit()>=0?mixKit():S.part],values[i]);text(values[i],i*60+30,210,16,C.amber,true,'center');}
 slab(0,229,240,11);text(footer,4,229,10,C.gray);
}
function drumSeq(){
 const col=partInk[S.part];
 for(let r=0;r<12;r++){const y=21+r*11;if(r===S.track)slab(0,y-1,24,11,true);drumGlyph(r===11?15:identityIndex(r),1,y,r===S.track?C.ink:C.gray);
 for(let i=0;i<16;i++){const step=S.stepPage*16+i+1,x=27+i*13+(i>>2),on=r<11&&S.steps[r].includes(step),muted=S.muted.includes(r);if(step>S.length){rect(x+5,y+3,2,2,C.line);continue;}stepCell(x,y,12,9,on?(muted?C.dim:r===S.track?col:'#5f552d'):C.line,on);if(S.held===step&&r===S.track){g.strokeStyle=C.amber;g.strokeRect(x+.5,y+.5,11,8);}if(S.playing&&step-1===S.playhead&&on)rect(x+2,y+2,8,4,C.white);}}
 cards('P'+S.pattern+' SEQUENCE',['LENGTH','RATE',null,null],[S.length+'st',D.rates[S.rate],null,null],'BLACK: TRACK  WHITE: STEP  PATCH+BLK: MUTE');
 // Formerly unmapped K7/K8 display cells become one selected-instrument
 // readout, with no editable card, value or knob caption.
 rect(123,171,114,55,C.bg);g.save();g.translate(144,175);g.scale(3,3);drumGlyph(identityIndex(S.track),0,0,partInk[S.part]);g.restore();text(identityName(S.track),180,212,12,C.ink,true,'center');
}
function home(){
 slab(3,21,234,33);for(let i=0;i<16;i++){const x=7+i*14+(i>=8?4:0);rect(x,23,12,22,C.side);line(x,23,x+11,23,C.edge);if(i===0)for(let a=0;a<8;a++)if(a%2===0)rect(x+2+a,25,1,2,partInk[0]);if(i+1===S.pattern)text(i+1,x+6,45,10,C.gray,false,'center');if(i+1===S.queued){g.strokeStyle=C.amber;g.strokeRect(x-.5,21.5,13,25);}}
 for(let r=0;r<5;r++){const y=55+r*20;slab(3,y,234,18);text(laneName(r),7,y+2,11,partInk[r]);for(let i=0;i<16;i++){const x=59+i*11;rect(x,y+3,9,8,C.side);if(r===0&&[0,4,8,12].includes(i))rect(x,y+3,3,8,partInk[r]);}if(S.muted.includes(r))line(6,y+15,48,y+15,C.gray);}
 cards('PERFORM',['TEMPO','SWING','FILTER','PUMP'],[S.tempo+'bpm',S.swing+'%',S.filter<60?'LP '+Math.floor((60-S.filter)*100/60)+'%':S.filter>68?'HP '+Math.floor((S.filter-68)*100/59)+'%':'OFF',S.pump.toFixed(1)+'dB'],'WHITE: PATTERN   TWO: CHAIN   BLACK: MUTE');
}
function pages(){
 if(S.part<2){const map=D.trackMaps.find(x=>x.engine===(S.part?'808':'909')&&x.voice_index===S.track);return map.pages.map((r,i)=>({...r,title:r.title==='KIT'?'KIT':identityName(S.track)+(i?' 2':''),cells:r.cells.map(c=>({...c,editable:!!c.label}))}));}
 return D.engines[S.engine].pages;
}
function fixtureValue(label,p,k){const c=control(p,k);return c?controlText(c):'—';}

function matrix(){const pp=pages(),first=Math.floor(S.page/4)*4;for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page,ink=active?C.ink:C.gray;slab(3,y,234,49,active);text((p+1)+' '+row.title,8,y,10,ink,true);if(active)text('K'+row.cells.map((c,k)=>c.editable?k+5:null).filter(x=>x).join(' '),232,y,10,C.ink,true,'right');if(row.title==='ENVELOPE'&&S.engine==='CHICAGO'){envelopeRow(p,y,ink);continue;}for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const c=control(p,k),x=4+k*59;if(k)line(x,y+14,x,y+45,C.line);text(cell.label,x+29,y+13,10,ink,false,'center');if(row.title==='SOUND'){text(controlText(c),x+29,y+30,11,ink,false,'center');continue;}if(S.engine==='CHICAGO'&&c.id===17){semanticGlyph(c,x+16,y+25,26,10,ink);text(controlText(c),x+29,y+36,10,ink,false,'center');}else{semanticGlyph(c,x+5,y+27,26,18,ink);text(controlText(c),x+54,y+31,11,ink,false,'right');}}}slab(0,227,240,13);text(S.held>=0?'STEP '+S.held+' / '+S.chance+'%  K2: CHANCE':'< >: ROW   PARA: NEXT   SCALE: ALL',7,228,10,C.gray);text((first+1)+'-'+Math.min(first+4,pp.length),235,228,10,C.amber,false,'right');}

function song(){
 const names=[0,1,2,3,4].map(p=>laneName(p).replace('CHICAGO','CHI').replace('MIAMI','MIA').replace('BREAK','BRK'));text('BAR',4,20,11,C.gray);names.forEach((n,i)=>text(n,58+i*36,20,11,partInk[i],false,'center'));
 for(let r=0;r<Math.min(11,S.songBars+1);r++){const y=31+r*11;slab(0,y,238,11,r===0);text(r+1,4,y,11,r===0?C.white:C.gray);if(r<S.songBars)for(let i=0;i<5;i++)text('P'+(r+1),58+i*36,y,11,partInk[i],false,'center');else text(r?'END':'EMPTY: WHITE KEYS ADD BARS',50,y,10,C.dim);}
 cards('BAR ('+(S.page+1)+'/2)',S.page===0?['BAR','909','808','BREAK']:['BAR',laneName(3).replace('CHICAGO','CHI'),laneName(4).replace('MIAMI','MIA'),'REPEAT'],S.page===0?['1','P1','P1','P1']:['1','P1','P1','1'],'WHITE: THE BAR\'S PATTERN   BLACK: MUTE');
}
function fx(){
 text('REVERB',98,22,10,C.gray,false,'center');text('DELAY',192,22,10,C.gray,false,'center');for(let i=0;i<5;i++){slab(3,34+i*15,234,13);text(laneName(i),7,35+i*15,10,partInk[i]);for(let j=0;j<2;j++){const x=60+j*94;line(x,40+i*15,x+76,40+i*15,C.line);rect(x+16+i*7,37+i*15,3,7,partInk[i]);}}
 text('DELAY 1/8  PING-PONG',9,116,14,C.white);text('FEEDBACK  37%     RV DECAY  50%',9,142,11,C.gray);
 const names=S.page===0?['DECAY','TONE',null,null]:['TIME','FEEDBACK','TONE','TAPE'];cards(S.page?'DELAY':'REVERB',names,S.page?['1/8','37%','50%','OFF']:['50%','50%',null,null],'FX: NEXT PAGE   SCALE: ALL AS A LIST');
}
function mix(){const kit=mixKit();if(kit>=0){const k=S.kits[kit];for(let i=0;i<11;i++){const x=4+21*i,ink=partInk[kit];slab(x,20,19,134,i===k.track);if(i===k.track)rect(x+1,21,17,12,ink);g.save();g.translate(x-1,23);g.scale(.85,.85);drumGlyph(identityIndex(i,kit),0,0,i===k.track?C.bg:C.gray);g.restore();rect(x+6,35,7,98,C.side);rect(x+7,36,5,96,C.line);const h=Math.floor(k.mix[i][0]*96/127),dark=kit?'#452019':i===k.track?'#5a3c10':'#392810';rect(x+7,132-h,5,h,dark);for(let sy=0;sy<96;sy+=8)rect(x+7,126-sy,5,6,sy<h?dark:C.line);line(x+3,144,x+15,144,C.line);rect(x+3+Math.floor(k.mix[i][1]*10/127),141,3,7,i===k.track?C.white:C.gray);}cards((kit?'808':'909')+' MIX: '+identityName(k.track,kit),['LEVEL','PAN','REV','DLY'],mixValues(k.mix[k.track]),'BLACK: TRACK  PATCH+BLK: MUTE');return;}
 const master=S.page>2,wave=S.page===4;if(!master){for(let i=0;i<5;i++){const x=4+i*38;slab(x,20,30,68,i===S.mixsel);rect(x+12,24,6,64,C.line);rect(x+6,85-S.mixPart[i][0]*64/127,18,3,C.white);text(laneName(i).replace('CHICAGO','CHI').replace('MIAMI','MIA').replace('BREAK','BRK'),x+15,89,10,partInk[i],false,'center');}}else{line(4,86,200,86,C.line);line(4,23,4,86,C.line);text('—',102,46,14,C.dim,false,'center');}slab(205,21,32,66);rect(208,25,7,61,C.line);text('GR',217,24,10,C.gray);slab(3,103,114,34);slab(123,103,114,34);text('COMP',8,106,10,C.gray);text('— dB',8,120,13,C.gray);text('PUMP',128,106,10,C.gray);text(S.pump.toFixed(1)+'dB',128,120,13,C.amber);text('AUDIO LOAD —%',8,145,10,C.gray);cards(['PARTS','909 MIX','808 MIX','MASTER','WAVE'][S.page],master?['DRIVE','COMP','PUMP','FILTER']:['LEVEL','PAN','REV','DLY'],master?['0%','OFF',S.pump.toFixed(1)+'dB','OFF']:mixValues(S.mixPart[S.mixsel]),master?'AGAIN: NEXT PAGE  SCALE: ALL AS A LIST':'BLACK 1-5: PART  PATCH+BLK: MUTE');}

function breaks(){
 for(let i=0;i<16;i++){const x=7+i*14;slab(x,25,12,12);if(i%2===0)rect(x+1,26,10,10,partInk[2]);}
 text('LOOP A',7,49,11,C.gray);for(let x=8;x<232;x++)line(x,81-Math.sin(x*.13)*Math.sin(x*.73)*20,x+1,81-Math.sin((x+1)*.13)*Math.sin((x+1)*.73)*20,partInk[2]);for(let i=0;i<8;i++){line(8+i*28,56,8+i*28,108,C.line);text(i+1,17+i*28,112,10,C.gray);}
 text('A: 8 SLICES    B: 8 SLICES',7,137,11,C.gray);
 if(S.view==='PART')cards('LOOPS',['LOOP A','LOOP B','B CHANCE','PITCH'],['1','2','0%','0'],'STEP + 1-8: PIN A SLICE');else{const ns=[['RATE',null,null,null],['CMPLX','ANCHOR','ROLL','FILL'],['RTG2X','RTG3X','RTG4X','RTG8X']][S.page];cards(['SEQUENCE','GROOVE','RETRIG'][S.page],ns,ns.map(x=>x?'50%':null),'9: REV  10: JUNGLE  11: STUTTER');}
}
function melodic(){
 for(let i=0;i<16;i++){const x=28+i*13;line(x,24,x,134,C.line);if([0,4,8,12].includes(i))rect(x,43+(i%3)*16,11,5,C.green);}text('ACCENT',4,140,10,C.gray);text('SLIDE',94,140,10,C.gray);
 const ns=S.held>=0?['NOTE','GATE','ACCENT','SLIDE']:[['LENGTH','RATE','DIRECTION','TRANSP'],['PAT SOUND',null,null,null],['FREE PLAY',null,null,null]][S.page];cards(S.held>=0?'STEP '+S.held:['SEQUENCE','PRESET','KEYS'][S.page],ns,S.held>=0?['C4','NOTE','OFF','OFF']:S.page===0?[S.length+'st',D.rates[S.rate],'FWD','0']:['OFF',null,null,null],S.held>=0?'K2: CHANCE   K3: VELOCITY':'PRESS: ON  TAP: OFF  HOLD+KNOBS: EDIT');
}
function deepRows(){if(S.view==='PART'&&S.part!==2){const rows=pages().flatMap((p,pi)=>p.cells.map((c,k)=>({...c,page:pi,k})).filter(c=>c.label));if(S.part<2){const map=D.trackMaps.find(x=>x.engine===(S.part?'808':'909')&&x.voice_index===S.track);for(const label of map.more_list_only)rows.push({label,editable:true,page:-1,k:label});}return rows;}return [{label:'LENGTH',editable:true},{label:'RATE',editable:true}];}
function editList(d){const c=deepRows()[S.list];if(!c?.editable)return 'Read-only/unmapped';return S.view==='PART'&&S.part>2?editControl(c.page,c.k,d):'This scalar is outside the bounded editor';}

function render(){
 bounds.length=0;rect(0,0,240,240,C.bg);header();
 if(S.view==='HOME')home();else if(S.view==='PART'&&S.part!==2)matrix();else if(S.part===2&&['PART','SEQ'].includes(S.view))breaks();else if(S.view==='SEQ')S.part<2?drumSeq():melodic();else if(S.view==='SONG')song();else if(S.view==='FX')fx();else if(S.view==='MIX')mix();
 if(S.overlay==='LIST'){rect(0,20,240,138,C.bg);const rows=deepRows(),top=Math.max(0,S.list-3);for(let i=0;i<6;i++){const r=rows[top+i];if(!r)break;slab(3,23+i*22,230,21,top+i===S.list);text(r.label,9,26+i*22,13,top+i===S.list?C.ink:C.gray);text(r.page!==undefined?fixtureValue(r.label,r.page,r.k):r.editable?'50%':'READ ONLY',226,26+i*22,13,C.gray,false,'right');}}
 if(S.overlay==='ASK'){rect(0,20,240,138,C.bg);text('LOAD SAVED PROJECT?',120,47,17,C.white,true,'center');text('DISCARD UNSAVED EDITS',120,76,13,C.gray,false,'center');text('SCALE: YES   PATCH: NO',120,105,13,C.green,true,'center');text(S.playing?'PLAY: CANCEL + STOP':'PLAY: CANCEL + START',120,134,11,C.amber,true,'center');}
 cv.setAttribute('aria-label',`${S.view}; ${laneName(S.part,true)}; ${S.wait?'external WAIT':S.playing?'RUN':'STOP'}; row ${S.page+1}; ${S.part<2?identityName(S.track):''}. Proposal only.`);
 if($('state'))$('state').textContent=`${S.note} · ${S.view} · ${S.wait?'WAIT':S.playing?'RUN':'STOP'} · page ${S.page+1}/${pageCount()} · ${S.held>=0?'held step '+S.held:'base edit scope'}`;
 if($('log'))$('log').textContent=trace.slice(-12).map(x=>x.event+' → '+x.result).join('\n');
 updateLights();
 return {state:structuredClone(S),bounds:structuredClone(bounds),pages:pageCount()};
}
function pageCount(){return S.view==='PART'?S.part===2?1:pages().length:S.view==='SEQ'?S.part<2?1:3:S.view==='SONG'?2:S.view==='FX'?2:S.view==='MIX'?5:1;}
function nav(d){if(S.overlay==='LIST'){S.list=clamp(S.list+d,0,deepRows().length-1);return;}if(S.held>=0){S.chance=clamp(S.chance+d,0,100);return;}const n=pageCount();S.page=['PART','FX','MIX'].includes(S.view)?(S.page+d%n+n)%n:clamp(S.page+d,0,n-1);remember();}

function event(ev,d=1){let outcome='';if(S.overlay==='ASK'&&!['PATCH','SCALE','SEQ PLAY'].includes(ev)){outcome='Question input mask; unchanged';}else if(ev==='K2')nav(d);else if(ev==='SCALE'){if(S.overlay==='ASK'){outcome='Confirmation input; storage not executed';S.overlay=null;}else if(!S.overlay){S.overlay='LIST';S.list=0;}}else if(ev==='PATCH'){if(S.overlay){S.overlay=null;outcome='Caller restored';}else setView(S.view==='HOME'?'SONG':'HOME');}else if(['PARA','SEQ','FX','NOTE REPEAT'].includes(ev)){const v={PARA:'PART',SEQ:'SEQ',FX:'FX','NOTE REPEAT':'MIX'}[ev];if(S.view===v){S.page=(S.page+1)%pageCount();remember();}else if(!setView(v))outcome='RELEASE STEP FIRST';}else if(ev==='SEQ PLAY'){if(S.overlay==='ASK')S.overlay=null;S.playing=!S.playing;outcome='Normal transport command; no audio';}else if(ev==='SEQ REC'){S.rec=!S.rec;outcome='REC arm fixture only; CP2/CP3 pending';}else if(ev==='K3'){if(S.overlay==='LIST')outcome=editList(d);else if(S.view==='SEQ'&&S.part>2&&S.held>=0)S.velocity=clamp(S.velocity+d,1,127);else{remember();const next=clamp(S.part+Math.sign(d),0,4);if(next!==S.part)S.held=-1;S.part=next;S.page=S.pageMemory[pageKey()]??0;if(S.view==='MIX'){S.mixsel=S.part;S.page=S.part<2?S.part+1:0;}normalizePage();}}else if(ev==='K4'){S.queued=clamp((S.queued||S.pattern)+Math.sign(d),1,32);outcome='Pending target fixture, no automatic adoption';}else if(/^K[5-8]$/.test(ev)){const k=+ev[1]-5;if(S.view==='HOME'){if(k===0)S.tempo+=d;if(k===1)S.swing=clamp(S.swing+d,0,100);if(k===2)S.filter=clamp(S.filter+d,0,127);if(k===3)S.pump+=d/10;S.dirty=true;}else if(S.view==='PART'&&S.part>2)outcome=editControl(S.page,k,d);else if(S.view==='MIX'&&mixKit()>=0){const kit=mixKit(),vals=kit>=0?S.kits[kit].mix[S.kits[kit].track]:S.mixPart[S.mixsel];vals[k]=clamp(vals[k]+d,0,127);S.dirty=true;}else if(S.view==='SEQ'&&S.part<2){if(k===0)S.length=clamp(S.length+d,1,64);if(k===1)S.rate=clamp(S.rate+d,0,D.rates.length-1);if(k<2)S.dirty=true;else outcome='Unmapped control';}else outcome='Layout-only scalar; no accepted edit';}else if(ev==='OCTAVE+'||ev==='OCTAVE-'){if(S.view==='SEQ'&&S.part<2)S.stepPage=clamp(S.stepPage+(ev.endsWith('+')?1:-1),0,3);}else if(ev==='BT')outcome='SAVE request only; result lifecycle not modeled';else if(ev==='GLOBE')outcome='Settings component reserved to incumbent';S.note=outcome||'Source route model';trace.push({event:ev+(d<0?' −':''),result:S.note,state:{view:S.view,page:S.page,part:S.part,sourcePart:S.sourcePart}});remember();sync();render();}

function black(n){const kit=mixKit();if(S.view==='HOME'&&n<5)S.muted.includes(n)?S.muted.splice(S.muted.indexOf(n),1):S.muted.push(n);else if(S.view==='MIX'&&S.page===0){if(n<5)S.mixsel=n;}else if(kit>=0)S.kits[kit].track=clamp(n,0,10);else if(S.part<2){S.track=clamp(n,0,10);S.page=0;remember();}S.note='Source target selected; no generated audition';trace.push({event:blacks[n],result:S.note});sync();render();}

function pad(n,held=false){const step=S.stepPage*16+n+1,kit=mixKit();if(S.view==='HOME'||S.view==='SONG'){S.queued=n+1;S.note='Pattern target fixture';}else if(kit>=0||S.view==='SEQ'&&S.part<2){if(held&&S.view==='SEQ'){S.held=step;S.heldInput=step;}else{const k=S.kits[kit>=0?kit:S.part],a=k.steps[k.track],at=a.indexOf(step);at<0?a.push(step):a.splice(at,1);if(held)S.heldInput=step;S.dirty=true;}}else if(held&&S.view==='SEQ'&&S.part>2){S.held=step;S.heldInput=step;}else S.note='Live musical input; no audio in model';trace.push({event:(held?'Hold ':'')+'pad'+n,result:S.note});render();}

function release(){S.held=-1;S.heldInput=-1;S.note='Original input released; base scope';trace.push({event:'Release',result:S.note});render();}

function updateLights(){const owner=mixKit()>=0?mixKit():S.part,rgb=[[127,74,20],[127,36,32],[106,60,127],[32,103,127],[65,121,45]][owner],dim=rgb.map(x=>Math.floor(x/4));const pads=[];for(let i=0;i<16;i++){let v=0;if(S.lightsEnabled&&['HOME','SONG'].includes(S.view)){v=i+1===S.pattern?2:i===0?1:0;if(i+1===S.queued&&(S.framePhase&8))v=2;}else if(S.lightsEnabled&&(mixKit()>=0||S.part<2&&['SEQ','PART'].includes(S.view))){const k=S.kits[selectedKit()],step=S.stepPage*16+i+1;let on=step<=k.length&&k.steps[k.track].includes(step);if(S.playing&&S.stepPage*16+i===k.playhead)on=!on;v=on?2:0;}if((S.heldInput>=0?S.heldInput:S.held)===S.stepPage*16+i+1)v=2;pads.push(v);}
 document.querySelectorAll('[data-pad]').forEach((b,i)=>{const v=pads[i],color=v===2?rgb:v===1?dim:[0,0,0];b.style.background=`rgb(${color.join(',')})`;b.style.borderColor=v?C.gray:C.line;b.setAttribute('aria-label',`Physical pad${i}, ${v===2?'bright':v===1?'dim':'off'}; kit/part owner${owner}`)});const on={'PATCH':['HOME','SONG'].includes(S.view),'SEQ':S.view==='SEQ','PARA':S.view==='PART','FX':S.view==='FX','NOTE REPEAT':S.view==='MIX','SCALE':!!S.overlay,'SEQ REC':S.rec,'SEQ PLAY':S.wait?!!(S.framePhase&32):S.playing&&S.playhead%4===0,'OCTAVE+':S.stepPage>0};document.querySelectorAll('#panelButtons [data-ev]').forEach(b=>{const lit=!!on[b.dataset.ev];b.setAttribute('data-lit',String(lit));b.setAttribute('aria-label',b.dataset.ev+'; source LED '+(lit?'on':'off'));});if($('ledState'))$('ledState').textContent=`Model source layer: owner${owner}; pads=${pads.join(' ')}; no physical observation.`;return {pads,buttons:on,owner,rgb,evidence:'Source layer model; not physical test'};}

function sync(){if($('trackButtons'))[...$('trackButtons').children].forEach((b,i)=>{const partScope=S.view==='HOME'||S.view==='MIX'&&S.page===0;b.setAttribute('aria-pressed',String(partScope?i===S.mixsel:i===S.track));b.setAttribute('aria-label',blacks[i]+': '+(partScope?(i<5?laneName(i):'no part binding'):identityName(i,selectedKit())));});}

function mixKit(){return S.view==='MIX'&&[1,2].includes(S.page)?S.page-1:-1;}

function selectedKit(){const m=mixKit();return m>=0?m:S.part<2?S.part:0;}

function laneName(p,header=false){if(p<3)return ['909','808','BREAK'][p];const bank=S.banks[p];return bank+(header||bank===S.banks[p===3?4:3]?(p===3?' B':' A'):'');}

function pageKey(view=S.view,part=S.part){return ['PART','SEQ','GEN'].includes(view)?view+':'+SOURCE_PART[part]:view;}

function remember(){S.pageMemory[pageKey()]=S.page;}

function normalizePage(){if(S.page>=pageCount()||S.page<0)S.page=0;remember();}

function setView(v){if(v==='SEQ'&&S.view==='PART'&&S.held>=0)return false;const keep=S.view==='SEQ'&&v==='PART';remember();if(!keep)S.held=-1;S.view=v;S.overlay=null;S.page=S.pageMemory[pageKey()]??0;normalizePage();return true;}

function controlKey(id){return S.sourcePart+':'+S.engine+':'+id;}

function effectiveQ(id){const key=controlKey(id),lock=S.held+':'+key;if(S.held>=0&&lock in S.locks)return S.locks[lock];if(key in S.motion)return S.motion[key];return S.values[key]??S.controls[S.part]?.[id]??64;}

function control(p,k){const cell=pages()[p]?.cells[k];if(!cell?.label)return null;const id=cell.parameter_id;const q=typeof id==='number'?effectiveQ(id):null;return {...cell,id,q,scope:S.held>=0&&S.held+':'+controlKey(id) in S.locks?'step':controlKey(id) in S.motion?'motion':'base',key:controlKey(id)};}

function enumCount(engine,id){return engine==='CHICAGO'?({11:2,13:2,14:3,15:3,16:10}[id]||0):engine==='MIAMI'?({0:3,3:25,6:4,11:4,13:8}[id]||0):0;}

function controlText(c){if(c.q===null)return c.label==='ENGINE'?S.engine:c.label==='PRESET'?(S.engine==='JUNO'?'JUNO POLY':'01'):'ACTIVE';const q=c.q,id=c.id;
 if(S.engine==='CHICAGO'){if(id===4){const n=Math.round(centeredQ(q)*200)-100;return(n>0?'+':'')+n+'%';}if(id===11||id===13)return q>=64?(id===11?'GATE':'ON'):(id===11?'ENV':'OFF');if(id===14)return ['DOWN','UP','UP/DN'][Math.round(q/127*2)];if(id===15)return ['1 OCT','2 OCT','FULL'][Math.round(q/127*2)];if(id===16)return ['1/2','1/4D','1/2T','1/4','1/8D','1/4T','1/8','1/8T','1/16','1/32'][Math.round(q/127*9)];if(id===17)return ['OFF','CHO/FLG','PHASER','RING MOD','COMPRESSOR','DECIMATOR','EQ','ENSEM','SH DLY','BPM DL','HALL','GT REV'][clamp(q,0,11)];}
 if(S.engine==='MIAMI'){if(id===0||id===6)return ['TRI','SAW','SQR','PUL'][Math.round(q/127*((id===0?3:4)-1))];if(id===11)return ['TRI','DOWN','UP','SQR'][Math.round(q/127*3)];if(id===3){const n=Math.round(q/127*24)-12;return(n>0?'+':'')+n+'st';}if(id===13)return ['1/2','1/4','1/8','1/16','1/3','1/6','1/12','1/24'][Math.round(q/127*7)];if(id===7){const n=Math.round((.25+.5*centeredQ(q))*2000)-1000;return(n>0?'+':n<0?'-':'')+(Math.abs(n)/100).toFixed(2);}}
 return Math.floor((q*100+63)/127)+'%';}

function editControl(p,k,d){const c=control(p,k);if(!c?.editable||c.q===null)return 'Reserved/unmapped; no edit';const count=enumCount(S.engine,c.id),max=S.engine==='CHICAGO'&&c.id===17?11:127;const next=count?Math.round(clamp(Math.round(c.q/127*(count-1))+Math.sign(d),0,count-1)*127/(count-1)):clamp(c.q+d,0,max);if(S.held>=0)S.locks[S.held+':'+c.key]=next;else S.values[c.key]=next;S.dirty=true;return (S.held>=0?'Step '+S.held:'Base')+' '+c.label+' '+controlText({...c,q:next});}

function paramCurve(f,x,y,w,h,c){let prev=null;for(let i=0;i<w;i++){const v=clamp(f(i/(w-1)),-1,1),ny=y+h/2-v*h*.43;if(prev!==null)line(x+i-1,prev,x+i,ny,c);prev=ny;}line(x,y+h/2,x+w,y+h/2,C.line);}

function semanticGlyph(c,x,y,w,h,ink){const q=(c.q??64)/127,id=c.id,chi=S.engine==='CHICAGO',jun=S.engine==='JUNO',mia=S.engine==='MIAMI';const eq=n=>effectiveQ(n)/127;
 if((chi&&id===0)||(jun&&id===0)){const width=jun ? .1+.8*eq(12) : .5;paramCurve(t=>{const phase=(t*2)%1,saw=phase*2-1,pulse=phase<width?1:-1;return (1-q)*saw+q*pulse;},x,y,w,h,ink);return;}
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

function envelopeRow(p,y,ink){const cc=[0,1,2,3].map(k=>control(p,k)),v=cc.map(c=>c.q/127);for(let k=0;k<4;k++)text('K'+(k+5)+' '+['A','D','S','R'][k]+' '+controlText(cc[k]),31+k*59,y+13,10,ink,false,'center');const a=13*v[0]**4+.001,d=25*v[1]**4+.001,r=25*v[3]**4+.002,s=v[2],dur=[a*1.466337,d*4.6051702,s>.04?r*Math.log(s/.04):0],sum=dur.reduce((a,b)=>a+b,0),widths=dur.map(z=>3+Math.round(163*z/(sum||1)));widths[1]+=172-widths.reduce((a,b)=>a+b,0);const xx=[13,13+widths[0],13+widths[0]+widths[1],13+widths[0]+widths[1]+42,227],top=y+26,bot=y+46;for(let stage=0;stage<3;stage++){const from=stage===2?3:stage,to=from+1;let px=xx[from],py=stage===0?bot:stage===1?top:bot-s*(bot-top);for(let i=1;i<=Math.max(1,xx[to]-xx[from]);i++){const t=dur[stage]*i/Math.max(1,xx[to]-xx[from]),level=stage===0?Math.min(1,1.3-1.3*Math.exp(-t/a)):stage===1?s+(1-s)*Math.exp(-t/d):s*Math.exp(-t/r),nx=xx[from]+i,ny=bot-level*(bot-top);line(px,py,nx,ny,ink);px=nx;py=ny;}}line(xx[2],bot-s*(bot-top),xx[3],bot-s*(bot-top),ink);}

function mixValues(v){return [v[0]===0?'OFF':(20*Math.log10(v[0]/127*(mixKit()===0?1.35:mixKit()===1?2:1))).toFixed(1)+'dB',v[1]===64?'C':v[1]<64?'L'+(64-v[1]):'R'+(v[1]-64),Math.round(v[2]*100/127)+'%',Math.round(v[3]*100/127)+'%'];}


window.proposal={reset,event,black,pad,release,render,pages,pageCount,drumGlyph,control,controlText,semanticGlyph,lights:updateLights,configure(patch){const page=patch.page;const p={...patch};if(p.part!==undefined)S.part=p.part;if('engine' in p){if(S.part>2)S.banks[S.part]=p.engine;delete p.engine;}delete p.sourcePart;Object.assign(S,p);if(page!==undefined)remember();normalizePage();sync();render();},get state(){return structuredClone(S)},get trace(){return structuredClone(trace)}};

function centeredQ(q){return q<=64?q/128:.5+(q-64)/126;}

// Existing qualitative effect families from ui.c orig_fx_glyph, in the same physical cell.
function effectGlyph(t,x,y,w,h,c){g.save();g.translate(x,y);g.scale(w/26,h/18);const r=25,b=18,m=9;if(t===0){line(0,m,r,m,c);line(10,2,15,b-2,c);}else if(t===1||t===7){const n=t===7?3:2;for(let row=0;row<n;row++){const yy=n===3?3+row*5:5+row*7;let py=yy;for(let k=1;k<=4;k++){const ny=yy+(k%2?2:-2);line((k-1)*6,py,k*6,ny,c);py=ny;}}}else if(t===2){line(0,5,3,5,c);for(let i=0;i<2;i++){const a=3+i*10;line(a,5,a+4,b-2,c);line(a+4,b-2,a+8,5,c);line(a+8,5,i?r:a+10,5,c);}}else if(t===3){for(let i=0;i<6;i++){const a=i*4,hh=i<3?2+i*2:12-i*2;line(a,m,a+2,m+(i%2?hh:-hh),c);line(a+2,m+(i%2?hh:-hh),a+4,m,c);}}else if(t===4){line(0,b,11,7,c);line(11,7,r,3,c);}else if(t===5){for(let i=0;i<4;i++){const a=i*6,yy=15-i*4;line(a,yy,a+6,yy,c);if(i<3)line(a+6,yy,a+6,yy-4,c);}}else if(t===6){line(0,m,6,m,c);line(6,m,12,2,c);line(12,2,18,m,c);line(18,m,r,m,c);}else if(t===8||t===9){const start=t===9?12:0;if(t===9){line(0,3,8,3,c);line(8,3,8,11,c);line(8,11,0,11,c);line(0,11,0,3,c);line(4,5,4,7,c);line(4,7,6,7,c);}for(let i=0;i<3;i++)rect(start+i*(t===9?6:9),b-12+i*4,t===9?1:2,12-i*4,c);line(0,b,r,b,c);}else if(t===10){line(0,b,0,2,c);line(0,2,r,2,c);line(r,2,r,b,c);for(let i=0;i<5;i++)rect(3+i*4,5+i,1,10-i*2,c);}else if(t===11){for(let i=0;i<4;i++)rect(i*5,3+i*2,2,12-i*2,c);rect(20,1,2,17,c);line(22,b,r,b,c);}g.restore();}
