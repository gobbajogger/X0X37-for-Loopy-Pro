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
let S,trace=[],touchTimer=null;const bounds=[];
function reset(fixture='drum'){if(touchTimer){clearTimeout(touchTimer);touchTimer=null;}
 if(fixture==='drone')throw new RangeError('Active Drone fixture withdrawn; saved-unavailable/fallback contract remains with firmware owner.');
 const kit=()=>({track:0,steps:[[1,5,9,13],[],[],[],[],[],[],[],[],[],[]],length:16,rate:0,playhead:4,mix:Array.from({length:11},()=>[61,64,0,0])});
 S={touched:null,view:'SEQ',part:0,page:0,pageMemory:{},banks:{3:'CHICAGO',4:'MIAMI'},controls:{3:[...CHI_Q],4:[...MIA_Q]},kits:[kit(),kit()],mixsel:0,mixPart:Array.from({length:5},()=>[100,64,0,0]),stepPage:0,melodicLength:16,playing:true,wait:false,rec:false,held:-1,heldInput:-1,chance:95,velocity:100,muted:[],pattern:1,queued:0,power:3,tempo:125,swingRaw:0,pumpRaw:21,metroView:0,filter:64,lightsEnabled:true,dirty:true,overlay:null,list:0,save:null,values:{},locks:{},motion:{},variants:{toms:0,rim:0,clap:0},heldPatch:false,songBars:0,songSel:0,usedPatterns:[1],now:0,press:null,framePhase:0,note:'Controlled source-value fixture; no audio, recording or storage',cueEvidence:'Qualitative source-law diagrams; no measured live cursor/native factory parity'};
 Object.defineProperties(S,{swing:{enumerable:true,get(){return 50+Math.floor(this.swingRaw/4);},set(v){this.swingRaw=clamp(Math.round((v-50)*4),0,100);}},pump:{enumerable:true,get(){return 24*this.pumpRaw/127;},set(v){this.pumpRaw=clamp(Math.round(v*127/24),0,127);}},engine:{enumerable:true,get(){return this.part<3?['909','808','BREAK'][this.part]:this.banks[this.part];}},sourcePart:{enumerable:true,get(){return SOURCE_PART[this.part];}},track:{enumerable:true,get(){return this.kits[selectedKit()].track;},set(v){this.kits[selectedKit()].track=clamp(v,0,10);}},steps:{enumerable:true,get(){return this.kits[selectedKit()].steps;},set(v){this.kits[selectedKit()].steps=v;}},length:{enumerable:true,get(){return this.part<2||mixKit()>=0?this.kits[selectedKit()].length:this.melodicLength;},set(v){if(this.part<2||mixKit()>=0)this.kits[selectedKit()].length=v;else this.melodicLength=v;}},rate:{enumerable:true,get(){return this.kits[selectedKit()].rate;},set(v){this.kits[selectedKit()].rate=v;}},playhead:{enumerable:true,get(){return this.kits[selectedKit()].playhead;},set(v){this.kits[selectedKit()].playhead=v;}}});
 if(fixture==='808variant'){S.part=1;S.track=2;S.variants={toms:1,rim:1,clap:1};}
 if(['chicago','miami','juno','lock','melodic','chicago-envelope','juno-envelope'].includes(fixture)){S.part=['miami','juno','juno-envelope'].includes(fixture)?4:3;S.view='PART';S.playing=false;if(fixture.startsWith('juno')){S.banks[4]='JUNO';S.controls[4]=[...JUN_Q];}S.page=fixture==='lock'?1:fixture.endsWith('envelope')?(fixture.startsWith('juno')?2:3):0;}
 if(['home','wait','savefail'].includes(fixture)){S.view='HOME';S.playing=fixture==='home';S.wait=fixture==='wait';S.save=fixture==='savefail'?'SAVE FAILED':null;}
 if(fixture==='fresh'){S.playing=false;S.kits[0].steps=Array.from({length:11},()=>[]);S.usedPatterns=[];}
 if(['drum-para','snare-para'].includes(fixture)){S.view='PART';S.playing=false;if(fixture==='snare-para')S.track=1;}
 if(fixture==='song'){S.view='SONG';S.playing=false;}
 if(fixture==='break'){S.part=2;S.view='PART';S.playing=false;}
 if(fixture==='fx'){S.view='FX';S.page=1;}
 if(['mix','mix909','mix808'].includes(fixture)){S.view='MIX';S.page=fixture==='mix909'?1:fixture==='mix808'?2:0;S.playing=false;}
 if(fixture==='load'){S.view='HOME';S.playing=false;S.overlay='ASK';}
 for(let kit=0;kit<2;kit++)S.kits[kit].parameters=D.typedDrums[kit].records.map(r=>Object.fromEntries(r.parameters.map(p=>[p.local_id,p.fresh_project_default_raw])));
 S.kits[0].mix=[61,58,52,52,52,103,122,89,60,47,42].map(v=>[v,64,0,0]);S.kits[1].mix=Array.from({length:11},()=>[64,64,0,0]);S.kits[1].steps[0]=[];remember();trace=[];sync();render();
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
function header(){
 slab(0,0,240,20);
 const label=S.view==='PART'?(S.part<2?S.engine:laneName(S.part,true)):S.view==='SEQ'?(S.part<3?S.engine:laneName(S.part,true)):S.view==='MIX'&&mixKit()>=0?(mixKit()?'808 MIX':'909 MIX'):S.view;
 text(label,5,1,14,S.part>2&&S.view==='PART'?C.green:partInk[mixKit()>=0?mixKit():S.part],true);
 if(S.view==='PART'){const groups=Math.ceil(pageCount()/4);for(let i=0;i<groups;i++)rect(86+i*6,7,3,3,i===Math.floor(S.page/4)?C.amber:C.edge);}
 patternPicture(100,2);text(S.pattern,123,3,11,C.white,false,'right');
 if(S.dirty&&S.view!=='PART'&&g.measureText(label).width<48)text('UNSAVED',96,5,9,C.amber,true,'right');
 if(S.queued){text('›'+S.queued,149,3,11,C.amber,true,'right');}
 text(S.wait?'WAIT':S.tempo,179,3,11,S.wait?C.amber:C.gray,S.wait,'right');
 if(S.playing&&!S.wait)triangle(184,4,C.white);else if(S.wait){line(184,5,184,13,C.amber);line(189,5,189,13,C.amber);}else rect(184,5,7,7,C.dim);
 if(S.rec)rect(195,6,5,5,'#f05e56');if(S.dirty)rect(204,3,3,3,S.save?'#ee745c':C.amber);if(S.save)text('!',201,8,10,'#ee745c',true);
 power();
}

function cards(title,names,values,footer){
 slab(3,158,234,11);text(title,8,158,11,S.view==='MIX'&&mixKit()>=0?partInk[mixKit()]:C.amber);if(S.view==='MIX')for(let dot=0;dot<5;dot++)rect(181+dot*6,162,3,3,dot===S.page?C.ink:C.edge);
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
 cards('PERFORM',['TEMPO','SWING','FILTER','PUMP'],[S.tempo+'bpm',S.swing+'%',S.filter<60?'LP '+Math.floor((60-S.filter)*100/60)+'%':S.filter>68?'HP '+Math.floor((S.filter-68)*100/59)+'%':'OFF',shortDecimal(S.pump,true)+'dB'],'WHITE: PATTERN   TWO: CHAIN   BLACK: MUTE');
}
function pages(){
 if(S.part<2){const map=D.trackMaps.find(x=>x.engine===(S.part?'808':'909')&&x.voice_index===S.track);return map.pages.map((r,i)=>({...r,title:r.title==='KIT'?'KIT':identityName(S.track)+(i?' 2':''),cells:r.cells.map(c=>({...c,editable:!!c.label}))}));}
 return D.engines[S.engine].pages;
}
function fixtureValue(label,p,k){const c=control(p,k);return c?controlText(c):'—';}

function matrix(){if(S.engine==='JUNO'){junoMatrix();return;}const pp=pages(),first=Math.floor(S.page/4)*4;for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page,ink=active?C.ink:C.gray;slab(3,y,234,49,active);if(S.part<2){text(p+1,7,y,10,ink,true);drumGlyph(row.title==='KIT'?15:identityIndex(S.track),16,y+1,ink);if(row.title==='KIT')text('KIT',43,y,10,ink,true);}else text((p+1)+' '+row.title,8,y,10,ink,true);if(active&&!(row.title==='ENVELOPE'&&S.touched?.page===p&&S.touched?.part===S.part))text('K'+row.cells.map((c,k)=>c.editable?k+5:null).filter(x=>x).join(' '),232,y,10,C.ink,true,'right');if(row.title==='ENVELOPE'&&['CHICAGO','JUNO'].includes(S.engine)){envelopeRow(p,y,ink);continue;}for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const c=control(p,k),x=4+k*59;if(k)line(x,y+14,x,y+45,C.line);text(cell.label,x+29,y+13,10,ink,false,'center');if(row.title==='SOUND'){text(controlText(c),x+29,y+30,11,ink,false,'center');continue;}if(S.engine==='CHICAGO'&&c.id===17){semanticGlyph(c,x+16,y+25,26,10,ink);text(controlText(c),x+29,y+36,10,ink,false,'center');}else{semanticGlyph(c,x+5,y+27,26,18,ink);text(controlText(c),x+54,y+31,11,ink,false,'right');}}}slab(0,227,240,13);text(S.held>=0?'STEP '+S.held+' / '+S.chance+'%  K2: CHANCE':'< >: ROW   PARA: NEXT   SCALE: ALL',7,228,10,C.gray);text((first+1)+'-'+Math.min(first+4,pp.length),235,228,10,C.amber,false,'right');}

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
function mix(){const kit=mixKit();if(kit>=0){const k=S.kits[kit];for(let i=0;i<11;i++){const x=4+21*i,ink=partInk[kit];slab(x,20,19,134,i===k.track);if(i===k.track)rect(x+1,21,17,12,ink);g.save();g.translate(x-1,23);g.scale(.85,.85);drumGlyph(identityIndex(i,kit),0,0,i===k.track?C.bg:C.gray);g.restore();rect(x+6,35,7,98,C.side);rect(x+7,36,5,96,C.line);const h=Math.floor(k.mix[i][0]*96/127),dark=kit?'#452019':i===k.track?'#5a3c10':'#392810';rect(x+7,132-h,5,h,dark);for(let sy=0;sy<96;sy+=8)rect(x+7,126-sy,5,6,sy<h?dark:C.line);line(x+3,144,x+15,144,C.line);rect(x+3+Math.floor(k.mix[i][1]*10/127),141,3,7,i===k.track?C.white:C.gray);}cards((kit?'808':'909')+' MIX: '+identityName(k.track,kit),['LEVEL','PAN','REV','DLY'],mixDrumControls(kit,k.track).map(controlText),'BLACK: TRACK  PATCH+BLK: MUTE');return;}
 const master=S.page>2,wave=S.page===4;if(!master){for(let i=0;i<5;i++){const x=4+i*38;slab(x,20,30,68,i===S.mixsel);rect(x+12,24,6,64,C.line);rect(x+6,85-S.mixPart[i][0]*64/127,18,3,C.white);text(laneName(i).replace('CHICAGO','CHI').replace('MIAMI','MIA').replace('BREAK','BRK'),x+15,89,10,partInk[i],false,'center');}}else{line(4,86,200,86,C.line);line(4,23,4,86,C.line);text('—',102,46,14,C.dim,false,'center');}slab(205,21,32,66);rect(208,25,7,61,C.line);text('GR',217,24,10,C.gray);slab(3,103,114,34);slab(123,103,114,34);text('COMP',8,106,10,C.gray);text('— dB',8,120,13,C.gray);text('PUMP',128,106,10,C.gray);text(shortDecimal(S.pump,true)+'dB',128,120,13,C.amber);text('AUDIO LOAD —%',8,145,10,C.gray);cards(['PARTS','909 MIX','808 MIX','MASTER','WAVE'][S.page],master?['DRIVE','COMP','PUMP','FILTER']:['LEVEL','PAN','REV','DLY'],master?['0%','OFF',shortDecimal(S.pump,true)+'dB','OFF']:mixValues(S.mixPart[S.mixsel]),master?'AGAIN: NEXT PAGE  SCALE: ALL AS A LIST':'BLACK 1-5: PART  PATCH+BLK: MUTE');}

function breaks(){
 // Equivalent actual BUILD_ONLY baseline: no available loop outline. Keep the native eight-slice surface and16-step strip; never substitute a made-up waveform.
 slab(3,22,234,65);line(8,48,232,48,C.line);for(let i=0;i<8;i++){if(i)line(8+i*28,26,8+i*28,70,C.line);text(i+1,22+i*28,74,10,C.gray,false,'center');}
 slab(3,91,234,20);for(let i=0;i<16;i++){const x=8+i*14;rect(x,95,12,12,C.side);line(x,95,x+11,95,C.edge);}
 slab(3,115,114,16);slab(123,115,114,16);text('A  NONE',60,117,11,C.amber,false,'center');text('B  NONE',180,117,11,C.amber,false,'center');
 text('A KEYS 1–8 / 8',8,134,11,C.gray);text('EDIT A 8',144,134,11,C.gray);text('C/D COUNT   C#/D# PAGE',8,147,10,C.dim);
 if(S.view==='PART')cards('LOOPS',['LOOP A','LOOP B','B CHANCE','PITCH'],['NONE','INVALID','0%','0st'],'STEP+1–8: PIN  9 REV  10 JNG  11 STUT');
 else{const ns=[['RATE',null,null,null],['CMPLX','ANCHOR','ROLL','FILL'],['RTG2X','RTG3X','RTG4X','RTG8X']][S.page];cards(['SEQUENCE','GROOVE','RETRIG'][S.page],ns,ns.map(x=>x?'0%':null),'9 REV  10 JUNGLE  11 STUTTER');}
}

function melodic(){
 for(let i=0;i<16;i++){const x=28+i*13;line(x,24,x,134,C.line);if([0,4,8,12].includes(i))rect(x,43+(i%3)*16,11,5,C.green);}text('ACCENT',4,140,10,C.gray);text('SLIDE',94,140,10,C.gray);
 const ns=S.held>=0?['NOTE','GATE','ACCENT','SLIDE']:[['LENGTH','RATE','DIRECTION','TRANSP'],['PAT SOUND',null,null,null],['FREE PLAY',null,null,null]][S.page];cards(S.held>=0?'STEP '+S.held:['SEQUENCE','PRESET','KEYS'][S.page],ns,S.held>=0?['C4','NOTE','OFF','OFF']:S.page===0?[S.length+'st',D.rates[S.rate],'FWD','0']:['OFF',null,null,null],S.held>=0?'K2: CHANCE   K3: VELOCITY':'PRESS: ON  TAP: OFF  HOLD+KNOBS: EDIT');
}
function deepRows(){
 if(S.view==='PART'&&S.part!==2){const rows=pages().flatMap((p,pi)=>p.cells.map((c,k)=>({...c,page:pi,k})).filter(c=>c.label));
 if(S.part<2){const rec=drumRecord(S.part,S.track);for(const id of rec.more_local_ids||rec.parameters.filter(p=>p.access.more).map(p=>p.local_id)){const meta=rec.parameters.find(p=>p.local_id===id);rows.push({label:meta.source_caption,editable:true,page:-1,k:meta.local_id,parameter_id:meta.local_id});}}return rows;}
 return [{label:'LENGTH',editable:true},{label:'RATE',editable:true}];
}

function editList(d){const c=deepRows()[S.list];if(!c?.editable)return 'Read-only/unmapped';return S.view==='PART'&&S.part!==2?editControl(c.page,c.k,d):'This scalar is outside the bounded editor';}

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

function event(ev,d=1){let outcome='';if(S.overlay==='ASK'&&!['PATCH','SCALE','SEQ PLAY'].includes(ev)){outcome='Question input mask; unchanged';}else if(ev==='K2')nav(d);else if(ev==='SCALE'){if(S.overlay==='ASK'){outcome='Confirmation input; storage not executed';S.overlay=null;}else if(!S.overlay){S.overlay='LIST';S.list=0;}}else if(ev==='PATCH'){if(S.overlay){S.overlay=null;outcome='Caller restored';}else setView(S.view==='HOME'?'SONG':'HOME');}else if(['PARA','SEQ','FX','NOTE REPEAT'].includes(ev)){const v={PARA:'PART',SEQ:'SEQ',FX:'FX','NOTE REPEAT':'MIX'}[ev];if(S.view===v){S.page=(S.page+1)%pageCount();remember();}else if(!setView(v))outcome='RELEASE STEP FIRST';}else if(ev==='SEQ PLAY'){if(S.overlay==='ASK')S.overlay=null;S.playing=!S.playing;outcome='Normal transport command; no audio';}else if(ev==='SEQ REC'){S.rec=!S.rec;outcome='REC arm fixture only; CP2/CP3 pending';}else if(ev==='K3'){if(S.overlay==='LIST')outcome=editList(d);else if(S.view==='SEQ'&&S.part>2&&S.held>=0)S.velocity=clamp(S.velocity+d,1,127);else{remember();const next=clamp(S.part+Math.sign(d),0,4);if(next!==S.part){S.held=-1;S.press=null;}S.part=next;S.page=S.pageMemory[pageKey()]??0;if(S.view==='MIX'){S.mixsel=S.part;S.page=S.part<2?S.part+1:0;}normalizePage();}}else if(ev==='K4'){S.queued=clamp((S.queued||S.pattern)+Math.sign(d),1,32);outcome='Pending target fixture, no automatic adoption';}else if(/^K[5-8]$/.test(ev)){const k=+ev[1]-5;if(S.view==='HOME'){if(k===0)S.tempo+=d;if(k===1)S.swingRaw=clamp(S.swingRaw+d,0,100);if(k===2)S.filter=clamp(S.filter+d,0,127);if(k===3)S.pumpRaw=clamp(S.pumpRaw+d,0,127);S.dirty=true;}else if(S.view==='PART'&&S.part!==2)outcome=editControl(S.page,k,d);else if(S.view==='MIX'&&mixKit()>=0){const kit=mixKit(),vals=kit>=0?S.kits[kit].mix[S.kits[kit].track]:S.mixPart[S.mixsel];const meta=drumRecord(kit,S.kits[kit].track).parameters.find(p=>p.access.mix_knob==='K'+(k+5));setDrumRaw(kit,S.kits[kit].track,meta.local_id,clamp(vals[k]+d,0,127));S.dirty=true;}else if(S.view==='SEQ'&&S.part<2){if(k===0)S.length=clamp(S.length+d,1,64);if(k===1)S.rate=clamp(S.rate+d,0,D.rates.length-1);if(k<2)S.dirty=true;else outcome='Unmapped control';}else outcome='Layout-only scalar; no accepted edit';}else if(ev==='OCTAVE+'||ev==='OCTAVE-'){if(S.view==='SEQ'&&S.part<2)S.stepPage=clamp(S.stepPage+(ev.endsWith('+')?1:-1),0,3);}else if(ev==='BT')outcome='SAVE request only; result lifecycle not modeled';else if(ev==='GLOBE')outcome='Settings component reserved to incumbent';S.note=outcome||'Source route model';trace.push({event:ev+(d<0?' −':''),result:S.note,state:{view:S.view,page:S.page,part:S.part,sourcePart:S.sourcePart}});remember();sync();render();}

function black(n){S.press=null;S.held=-1;const kit=mixKit();if(S.view==='HOME'&&n<5)S.muted.includes(n)?S.muted.splice(S.muted.indexOf(n),1):S.muted.push(n);else if(S.view==='MIX'&&S.page===0){if(n<5)S.mixsel=n;}else if(kit>=0)S.kits[kit].track=clamp(n,0,10);else if(S.part<2){S.track=clamp(n,0,10);S.page=0;remember();}S.note='Source target selected; no generated audition';trace.push({event:blacks[n],result:S.note});sync();render();}

function padDown(n){
 const step=S.stepPage*16+n+1,kit=mixKit();S.heldInput=step;
 if(['HOME','SONG'].includes(S.view)){S.queued=n+1;S.note='Pattern target fixture';return;}
 if(kit>=0||S.part<2&&['SEQ','PART'].includes(S.view)){
  const ki=kit>=0?kit:S.part,k=S.kits[ki],a=k.steps[k.track],created=!a.includes(step);if(created){a.push(step);S.dirty=true;}
  S.press={step,kit:ki,voice:k.track,pattern:S.pattern,time:S.now,created,used:false};if(['SEQ','PART'].includes(S.view))S.held=step;
 }else if(S.view==='SEQ'&&S.part>2)S.held=step;
 S.note='Input down; source owner retained';
}
function pad(n,held=false){padDown(n);if(!held)release(80);trace.push({event:(held?'Hold ':'Tap ')+'pad'+n,result:S.note});render();}

function release(elapsed=800){
 S.now+=elapsed;
 const p=S.press;if(p){const k=S.kits[p.kit],valid=p.pattern===S.pattern&&k.track===p.voice&&(mixKit()===p.kit||S.part===p.kit&&['SEQ','PART'].includes(S.view));
  if(valid&&!p.created&&!p.used&&S.now-p.time<300){const a=k.steps[p.voice],at=a.indexOf(p.step);if(at>=0){a.splice(at,1);S.dirty=true;}}
 }
 S.press=null;S.held=-1;S.heldInput=-1;S.note='Original input released; base scope';trace.push({event:'Release '+elapsed+'ms',result:S.note});render();
}

function updateLights(){const owner=mixKit()>=0?mixKit():S.part,rgb=[[127,74,20],[127,36,32],[106,60,127],[32,103,127],[65,121,45]][owner],dim=rgb.map(x=>Math.floor(x/4));const pads=[];for(let i=0;i<16;i++){let v=0;if(S.lightsEnabled&&['HOME','SONG'].includes(S.view)){v=S.usedPatterns.includes(i+1)?1:0;if(S.view==='HOME'&&i+1===S.pattern||S.view==='SONG'&&S.songSel<S.songBars&&i+1===S.pattern)v=2;if(S.view==='HOME'&&i+1===S.queued&&(S.framePhase&8))v=2;}else if(S.lightsEnabled&&(mixKit()>=0||S.part<2&&['SEQ','PART'].includes(S.view))){const k=S.kits[selectedKit()],step=S.stepPage*16+i+1;let on=step<=k.length&&k.steps[k.track].includes(step);if(S.playing&&S.stepPage*16+i===k.playhead)on=!on;v=on?2:0;}if((S.heldInput>=0?S.heldInput:S.held)===S.stepPage*16+i+1)v=2;pads.push(v);}
 document.querySelectorAll('[data-pad]').forEach((b,i)=>{const v=pads[i],color=v===2?rgb:v===1?dim:[0,0,0];b.style.background=`rgb(${color.join(',')})`;b.style.borderColor=v?C.gray:C.line;b.setAttribute('aria-label',`Physical pad${i}, ${v===2?'bright':v===1?'dim':'off'}; kit/part owner${owner}`)});const on={'PATCH':['HOME','SONG'].includes(S.view),'SEQ':S.view==='SEQ','PARA':S.view==='PART','FX':S.view==='FX','NOTE REPEAT':S.view==='MIX','SCALE':!!S.overlay,'SEQ REC':S.rec,'SEQ PLAY':S.wait?!!(S.framePhase&32):S.playing&&S.playhead%4===0,'OCTAVE+':S.stepPage>0};document.querySelectorAll('#panelButtons [data-ev]').forEach(b=>{const lit=!!on[b.dataset.ev];b.setAttribute('data-lit',String(lit));b.setAttribute('aria-label',b.dataset.ev+'; source LED '+(lit?'on':'off'));});if($('ledState'))$('ledState').textContent=`Model source layer: owner${owner}; pads=${pads.join(' ')}; no physical observation.`;return {pads,buttons:on,owner,rgb,evidence:'Source layer model; not physical test'};}

function sync(){if($('trackButtons'))[...$('trackButtons').children].forEach((b,i)=>{const partScope=S.view==='HOME'||S.view==='MIX'&&S.page===0;b.setAttribute('aria-pressed',String(partScope?i===S.mixsel:i===S.track));b.setAttribute('aria-label',blacks[i]+': '+(partScope?(i<5?laneName(i):'no part binding'):identityName(i,selectedKit())));});}

function mixKit(){return S.view==='MIX'&&[1,2].includes(S.page)?S.page-1:-1;}

function selectedKit(){const m=mixKit();return m>=0?m:S.part<2?S.part:0;}

function laneName(p,header=false){if(p<3)return ['909','808','BREAK'][p];const bank=S.banks[p];return bank+(header||bank===S.banks[p===3?4:3]?(p===3?' B':' A'):'');}

function pageKey(view=S.view,part=S.part){return ['PART','SEQ','GEN'].includes(view)?view+':'+SOURCE_PART[part]:view;}

function remember(){S.pageMemory[pageKey()]=S.page;}

function normalizePage(){if(S.page>=pageCount()||S.page<0)S.page=0;remember();}

function setView(v){if(v==='SEQ'&&S.view==='PART'&&S.held>=0)return false;const keep=S.view==='SEQ'&&v==='PART';remember();if(!keep){S.held=-1;S.press=null;}S.view=v;S.overlay=null;S.page=S.pageMemory[pageKey()]??0;normalizePage();return true;}

function controlKey(id,voice=S.track){return S.part<2?'T_'+S.engine+':'+voice+':'+id:S.sourcePart+':'+S.engine+':'+id;}

function effectiveQ(id,voice=S.track){
 const key=controlKey(id,voice),base=S.part<2?drumRaw(S.part,voice,id):(S.values[key]??S.controls[S.part]?.[id]??64),lock=S.pattern+':'+S.held+':'+key;
 if(S.held>=0)return lock in S.locks?S.locks[lock]:base;
 if(key in S.motion)return S.motion[key];return base;
}

function control(p,k){
 let cell=p>=0?pages()[p]?.cells[k]:S.part<2?(()=>{const m=drumRecord(S.part,S.track).parameters.find(p=>p.local_id===k);return m?{label:m.source_caption,parameter_id:k,editable:true}:null;})():null;
 if(!cell?.label)return null;const id=cell.parameter_id,voice=cell.voice??S.track,key=controlKey(id,voice),meta=S.part<2?drumRecord(S.part,voice).parameters.find(p=>p.local_id===id):null;
 return {...cell,id,voice,meta,domain:S.part<2?'drum':'synth',q:typeof id==='number'?effectiveQ(id,voice):null,scope:S.held>=0?(S.pattern+':'+S.held+':'+key in S.locks?'step':'base'):key in S.motion?'motion':'base',key};
}

function enumCount(engine,id){return engine==='CHICAGO'?({11:2,13:2,14:3,15:3,16:10}[id]||0):engine==='MIAMI'?({0:3,3:25,6:4,11:4,13:8}[id]||0):0;}

function controlText(c){if(c.domain==='drum')return drumText(c);if(c.q===null)return c.label==='ENGINE'?S.engine:c.label==='PRESET'?(S.engine==='JUNO'?'JUNO POLY':'01'):'ACTIVE';const q=c.q,id=c.id;
 if(S.engine==='CHICAGO'){if(id===4){const n=Math.round(centeredQ(q)*200)-100;return(n>0?'+':'')+n+'%';}if(id===11||id===13)return q>=64?(id===11?'GATE':'ON'):(id===11?'ENV':'OFF');if(id===14)return ['DOWN','UP','UP/DN'][Math.round(q/127*2)];if(id===15)return ['1 OCT','2 OCT','FULL'][Math.round(q/127*2)];if(id===16)return ['1/2','1/4D','1/2T','1/4','1/8D','1/4T','1/8','1/8T','1/16','1/32'][Math.round(q/127*9)];if(id===17)return ['OFF','CHO/FLG','PHASER','RING MOD','COMPRESSOR','DECIMATOR','EQ','ENSEM','SH DLY','BPM DL','HALL','GT REV'][clamp(q,0,11)];}
 if(S.engine==='MIAMI'){if(id===0||id===6)return ['TRI','SAW','SQR','PUL'][Math.round(q/127*((id===0?3:4)-1))];if(id===11)return ['TRI','DOWN','UP','SQR'][Math.round(q/127*3)];if(id===3){const n=Math.round(q/127*24)-12;return(n>0?'+':'')+n+'st';}if(id===13)return ['1/2','1/4','1/8','1/16','1/3','1/6','1/12','1/24'][Math.round(q/127*7)];if(id===7){const n=Math.round((.25+.5*centeredQ(q))*2000)-1000;return(n>0?'+':n<0?'-':'')+(Math.abs(n)/100).toFixed(2);}}
 return Math.floor((q*100+63)/127)+'%';}

function editControl(p,k,d){
 const c=control(p,k);if(!c?.editable||c.q===null)return 'Reserved/unmapped; no edit';
 const count=c.domain==='drum'?(c.meta.enum_names?.length||0):enumCount(S.engine,c.id),max=c.domain==='drum'?c.meta.maximum_raw:S.engine==='CHICAGO'&&c.id===17?11:127;
 const next=c.domain==='drum'?clamp(c.q+(count?Math.sign(d):d),c.meta.minimum_raw,max):count?Math.round(clamp(Math.round(c.q/127*(count-1))+Math.sign(d),0,count-1)*127/(count-1)):clamp(c.q+d,0,max);
 if(S.held>=0){S.locks[S.pattern+':'+S.held+':'+c.key]=next;if(S.press)S.press.used=true;}else if(c.domain==='drum')setDrumRaw(S.part,c.voice,c.id,next);else S.values[c.key]=next;
 S.touched={page:p,knob:k,part:S.part,voice:c.voice};if(touchTimer)clearTimeout(touchTimer);touchTimer=setTimeout(()=>{S.touched=null;render();},900);S.dirty=true;return (S.held>=0?'Step '+S.held:'Base')+' '+c.label+' '+controlText({...c,q:next});
}

function paramCurve(f,x,y,w,h,c){let prev=null;for(let i=0;i<w;i++){const v=clamp(f(i/(w-1)),-1,1),ny=y+h/2-v*h*.43;if(prev!==null)line(x+i-1,prev,x+i,ny,c);prev=ny;}line(x,y+h/2,x+w,y+h/2,C.line);}

function semanticGlyph(c,x,y,w,h,ink){if(S.engine==='CHICAGO'&&c.id===0){const ys=D.chicagoWave.wave_y_by_q[c.q],xx=Math.round(x),yy=Math.round(y);line(xx,yy+9,xx+28,yy+9,C.line);for(let i=1;i<29;i++)line(xx+i-1,yy+ys[i-1],xx+i,yy+ys[i],ink);return;}if(S.engine==='JUNO'){junoSharedGlyph(c,x,y,w,h,ink);return;}if(c.domain==='drum'){drumParameterGlyph(c,x,y,w,h,ink);return;}const q=(c.q??64)/127,id=c.id,chi=S.engine==='CHICAGO',jun=S.engine==='JUNO',mia=S.engine==='MIAMI';const eq=n=>effectiveQ(n)/127;
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


window.proposal={reset,event,black,pad,padDown,release,render,pages,pageCount,drumGlyph,control,controlText,semanticGlyph,lights:updateLights,configure(patch){const page=patch.page;const p={...patch};if(p.part!==undefined)S.part=p.part;if('engine' in p){if(S.part>2)S.banks[S.part]=p.engine;delete p.engine;}delete p.sourcePart;Object.assign(S,p);if(page!==undefined)remember();normalizePage();sync();render();},get state(){return structuredClone(S)},get trace(){return structuredClone(trace)}};

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
function junoSharedGlyph(c,x,y,w,h,ink){
 const q=c.q/127,id=c.id,eq=id=>effectiveQ(id)/127;
 if(id===0||id===12){const width=.1+.8*eq(12),mix=eq(0);let py=null;line(x,y+h/2,x+w-1,y+h/2,C.line);for(let i=0;i<29;i++){const phase=i/28,saw=2*phase-1,pulse=phase<width?1:-1,v=id===12?pulse:(1-mix)*saw+mix*pulse,xx=x+i*(w-1)/28,yy=y+(1-v)*h*.5;if(py!==null)line(x+(i-1)*(w-1)/28,py,xx,yy,ink);py=yy;}return;}
 if(id===2||id===3){sharedFilter(x,y,w,h,1,effectiveQ(2),effectiveQ(3),ink);return;}
 if(id===9){sharedFilter(x,y,w,h,q===0?0:3,c.q,0,ink);return;}
 if(id===4){sharedFilter(x,y,w,h,1,effectiveQ(2),effectiveQ(3),C.dim);const px=x+Math.round(q*(w-3));line(x+1,y+2,px,y+2,ink);line(px-3,y,px,y+2,ink);line(px,y+2,px-3,y+4,ink);return;}
 if(id===1||id===15){line(x,y+h,x+w,y+h,C.line);for(let i=0;i<6;i++)rect(x+i*w/6,y+h-3-i*2,w/6-1,2,i<Math.round(q*6)?ink:C.line);return;}
 if(id===14){const amp=q*(h/4);for(let row=0;row<2;row++){let py=y+h*(row?.7:.3);for(let i=1;i<w;i++){const yy=y+h*(row?.7:.3)+Math.sin(i/w*Math.PI*2+(row?Math.PI:0))*amp;line(x+i-1,py,x+i,yy,ink);py=yy;}}return;}
 if([10,11,13].includes(id)){const cycles=id===11?2:1+3*q*q,amp=id===11?(.1+.9*q):1;paramCurve(t=>{const phase=t*cycles%1;return amp*(id===13?Math.sin(phase*Math.PI*2):1-4*Math.abs(phase-.5));},x,y,w,h,ink);return;}
 line(x,y+h/2,x+w,y+h/2,C.line);rect(x+q*(w-3),y+h/2-2,3,5,ink);
}

function patternPicture(x,y){
 // Preserve the current five-lane musical thumbnail grammar; fixture data only.
 rect(x,y,11,16,C.line);rect(x+1,y+1,9,14,C.bg);
 for(let part=0;part<5;part++)for(let bin=0;bin<8;bin++){
  let on=false;if(part<2)on=S.kits[part].steps.some(a=>a.some(st=>Math.floor((st-1)/2)===bin));
  if(on)rect(x+1+bin,y+1+part*3,1,2,partInk[part]);
 }
}

function junoMatrix(){
 // Current Juno large readouts and neutral bars are retained. New shared-family
 // thumbnails use only the unused row-heading gutter, never the value area.
 const pp=pages(),first=Math.floor(S.page/4)*4;
 for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page,ink=active?C.ink:C.gray;slab(3,y,234,49,active);text((p+1)+' '+row.title,8,y,10,ink,true);if(active)text('K5 6 7 8',232,y,10,C.ink,true,'right');
  if(p<4)junoGroupCue(p,105,y+1,66,9,ink);
  for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const x=4+k*59,c=control(p,k);if(k)line(x,y+14,x,y+45,C.line);text(cell.label,x+29,y+12,11,ink,false,'center');text(controlText(c),x+29,y+26,row.title==='SOUND'?12:18,ink,true,'center');line(x+8,y+45,x+50,y+45,C.line);if(c.q!==null)rect(x+8,y+45,1+Math.floor(41*c.q/127),1,ink);}
 }
 slab(0,227,240,13);text(S.held>=0?'STEP '+S.held+' / '+S.chance+'%  K2: CHANCE':'< >: ROW   PARA: NEXT   SCALE: ALL',7,228,10,C.gray);text((first+1)+'-'+Math.min(first+4,pp.length),235,228,10,C.amber,false,'right');
}
function junoGroupCue(page,x,y,w,h,ink){
 if(page===0){junoSharedGlyph(control(0,0),x,y,w,h,ink);return;}
 if(page===1){sharedFilter(x,y,w,h,1,effectiveQ(2),effectiveQ(3),ink);return;}
 if(page===3){junoSharedGlyph(control(3,0),x,y,w/2-2,h,ink);junoSharedGlyph(control(3,3),x+w/2+2,y,w/2-2,h,ink);return;}
 // Established ADSR family as a header cue; all four large A/D/S/R values remain.
 const v=[5,6,7,8].map(id=>effectiveQ(id)/127),a=.002+3*v[0]**2,d=.01+3*v[1]**2,r=.01+4*v[3]**2,ss=v[2],rr=ss>1e-5?Math.log(1e-5/ss)/(48000*Math.log(1-1/Math.max(1,r*48000))):0,total=a+d+rr,hold=12,span=w-hold;
 const wa=2+(span-6)*a/(total||1),wd=2+(span-6)*d/(total||1),wr=span-wa-wd,xx=[x,x+wa,x+wa+wd,x+wa+wd+hold,x+w],bot=y+h,top=y;
 line(xx[0],bot,xx[1],top,ink);line(xx[1],top,xx[2],bot-ss*h,ink);line(xx[2],bot-ss*h,xx[3],bot-ss*h,ink);let py=bot-ss*h;for(let i=1;i<=Math.max(1,Math.ceil(wr));i++){const t=rr*i/Math.max(1,Math.ceil(wr)),ny=bot-ss*h*(1-1/Math.max(1,r*48000))**Math.round(t*48000);line(xx[3]+(i-1)*wr/Math.max(1,Math.ceil(wr)),py,xx[3]+i*wr/Math.max(1,Math.ceil(wr)),ny,ink);py=ny;}
}

function performSourceGlyph(name,x,y,w,h,ink){
 if(!['TEMPO','SWING','FILTER','PUMP'].includes(name))return false;
 const xx=v=>x+(v+19)*w/38,yy=v=>y+(v-28)*h/20,L=(a,b,c,d,col=ink)=>line(xx(a),yy(b),xx(c),yy(d),col),B=(a,b,ww,hh,col=ink)=>rect(xx(a),yy(b),Math.max(1,ww*w/38),Math.max(1,hh*h/20),col);
 if(name==='TEMPO'){L(-10,47,-4,28);L(-4,28,4,28);L(4,28,10,47);L(-10,47,10,47);const phase=S.metroView?S.metroView-1:0,arm=S.metroView?(phase<16?phase-8:24-phase):0;L(0,43,arm,30,S.metroView?C.white:C.gray);B(arm*3/4-2,32,5,3);if(S.metroView&&phase<4)B(-2,45,5,2);}
 if(name==='SWING'){const middle=Math.trunc(S.swingRaw*7/127);B(-18,43,37,1,C.line);B(-18,30,3,14);B(middle,34,3,10);B(16,30,3,14);}
 if(name==='FILTER'){const q=S.filter,mode=q<60?1:q>68?3:0,cut=q<60?8+Math.trunc(q*119/59):q>68?Math.trunc((q-69)*120/58):127;sharedFilter(x,y,w,h,mode,cut,0,ink);}
 if(name==='PUMP'){const depth=Math.trunc(S.pumpRaw*14/127);L(-18,31,-9,31);L(-9,31,-6,31+depth);L(-6,31+depth,15,31);B(15,31,4,1);B(-18,48,37,1,C.line);}return true;
}
