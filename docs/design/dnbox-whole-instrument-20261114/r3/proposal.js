/* Design model only. Existing physical routes are retained; no firmware or audio. */
const C={bg:'#050a0d',face:'#101a1f',edge:'#26383d',side:'#070e11',sel:'#253b2d',selEdge:'#3e5c40',ink:'#e0ebd6',white:'#dee7df',gray:'#9caaa8',dim:'#809294',line:'#28383d',green:'#8fbf79',amber:'#c49b45'};
const parts=['909','808','BREAK','CHICAGO B','CHICAGO A'];
const partInk=['#ff9529','#ff4841','#d579ff','#41ceff','#83f25a'];
const drumNames=['KICK','SNARE','LOW TOM','MID TOM','HIGH TOM','RIMSHOT','HANDCLAP','CLOSED HAT','OPEN HAT','CRASH','RIDE'];
const blacks=['F#3','G#3','A#3','C#4','D#4','F#4','G#4','A#4','C#5','D#5','F#5'];
const $=id=>document.getElementById(id),cv=$('lcd'),g=cv.getContext('2d');
let S,trace=[];const bounds=[];
function reset(fixture='drum'){
 S={view:'SEQ',engine:'CHICAGO',part:0,page:0,soundPage:0,seqPage:0,mixPage:0,fxPage:0,songPage:0,songBars:0,track:0,stepPage:0,length:16,rate:0,playing:true,wait:false,rec:false,held:-1,chance:95,velocity:100,muted:[],pattern:1,queued:0,power:3,tempo:125,swing:50,pump:4,filter:64,lightsEnabled:true,dirty:false,overlay:null,list:0,save:null,values:{},locks:{},variants:{toms:0,rim:0,clap:0},heldPatch:false,steps:[[1,5,9,13],[],[],[],[],[],[],[],[],[],[]],note:'Illustrative fixture; no audio or storage'};
 if(fixture==='808variant'){S.part=1;S.track=2;S.variants={toms:1,rim:1,clap:1};}
 if(['chicago','miami','juno','drone','lock','melodic'].includes(fixture)){S.part=4;S.engine=fixture==='miami'?'MIAMI':fixture==='juno'?'JUNO':fixture==='drone'?'DRONE':'CHICAGO';S.view='PART';S.playing=false;S.page=fixture==='chicago'||fixture==='lock'?1:0;S.soundPage=S.page;}
 if(fixture==='home'||fixture==='wait'||fixture==='savefail'){S.view='HOME';S.playing=false;S.wait=fixture==='wait';S.save=fixture==='savefail'?'SAVE FAILED':null;S.dirty=fixture==='savefail';}
 if(fixture==='song'){S.view='SONG';S.playing=false;}
 if(fixture==='break'){S.part=2;S.view='PART';}
 if(fixture==='fx'){S.view='FX';S.page=1;}
 if(fixture==='mix'){S.view='MIX';S.page=0;}
 if(fixture==='load'){S.view='HOME';S.playing=false;S.overlay='ASK';}
 S.framePhase=0;S.playhead=4;
 trace=[];sync();render();
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
 else if(n===12){ellipse(x+10,y+1.5,5,1.5,c);line(x+5,y+2,x+7,y+10,c);line(x+15,y+2,x+13,y+10,c);line(x+7,y+10,x+13,y+10,c);}
 else if(n===13){line(x+3,y+2,x+17,y+9,c,2);line(x+5,y+10,x+15,y+1,c,2);}
 else if(n===14){ellipse(x+7,y+3,4,3,c);line(x+10,y+6,x+15,y+10,c,2);ellipse(x+15,y+2,3,2,c);line(x+17,y+4,x+20,y+8,c,2);}
 else {line(x+3,y+5,x+10,y+1,c);line(x+10,y+1,x+17,y+5,c);line(x+3,y+9,x+10,y+5,c);line(x+10,y+5,x+17,y+9,c);}
}
function identityIndex(n){return S.part===1&&n>=2&&n<=4&&S.variants.toms?12:S.part===1&&n===5&&S.variants.rim?13:S.part===1&&n===6&&S.variants.clap?14:n;}
function identityName(n){return S.part===1&&n>=2&&n<=4&&S.variants.toms?['LOW CONGA','MID CONGA','HIGH CONGA'][n-2]:S.part===1&&n===5&&S.variants.rim?'CLAVE':S.part===1&&n===6&&S.variants.clap?'MARACAS':drumNames[n];}
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
 slab(0,0,240,20);const label=S.view==='PART'?(S.part<2?(S.part?'808':'909'):S.engine+' A'):S.view==='SEQ'?(S.part<2?(S.part?'808':'909'):S.part===2?'BREAK':S.engine+' A')+' SEQ':S.view;
 text(label,5,1,15,S.part>2&&S.view==='PART'?C.green:partInk[S.part],true);
 if(S.save)text('!',120,2,14,'#ee745c',true);
 if(S.dirty)rect(236,2,3,3,S.save?'#ee745c':C.amber);
 if(S.view==='SONG'){text('BAR1',147,3,11,C.amber,true,'center');}else{rect(132,4,10,12,C.edge);for(let r=0;r<5;r++)line(134,5+r*2,140,5+r*2,r===0?partInk[0]:C.dim);text(S.pattern,151,3,12,C.white,true,'center');}if(S.queued){text('>',161,3,12,C.amber,true);text(S.queued,173,3,12,C.amber,true);}
 if(S.view==='PART'&&S.part!==2){triangle(179,4,S.playing?C.white:C.dim);const n=Math.ceil(pageCount()/4);for(let i=0;i<n;i++){rect(231-(n-i-1)*10,7,4,4,i===Math.floor(S.page/4)?C.amber:C.edge);}if(S.rec)rect(190,5,5,5,'#f05e56');return;}
 if(S.wait){text('EXT',187,3,11,C.amber,true,'right');text('WAIT',229,3,12,C.amber,true,'right');}else {text(S.tempo,213,2,14,C.amber,true,'right');triangle(218,4,S.playing?C.white:C.dim);}if(S.rec)rect(232,4,5,5,'#f05e56');
}
function cards(title,names,values,footer){
 slab(3,158,234,11);text(title,8,158,11,C.amber);power(218,160);
 for(let i=0;i<4;i++){slab(i*60+3,171,54,55);if(!names[i])continue;text(names[i],i*60+30,171,11,C.gray,false,'center');graph(names[i],i*60+10,188,39,17,partInk[S.part],values[i]);text(values[i],i*60+30,210,16,C.amber,true,'center');}
 slab(0,229,240,11);text(footer,4,229,10,C.gray);
}
function drumSeq(){
 const col=partInk[S.part];
 for(let r=0;r<12;r++){const y=21+r*11;if(r===S.track)slab(0,y-1,24,11,true);drumGlyph(r===11?15:identityIndex(r),1,y,r===S.track?C.ink:C.gray);
 for(let i=0;i<16;i++){const step=S.stepPage*16+i+1,x=27+i*13+(i>>2),on=r<11&&S.steps[r].includes(step),muted=S.muted.includes(r);if(step>S.length){rect(x+5,y+3,2,2,C.line);continue;}stepCell(x,y,12,9,on?(muted?C.dim:r===S.track?col:'#5f552d'):C.line,on);if(S.held===step&&r===S.track){g.strokeStyle=C.amber;g.strokeRect(x+.5,y+.5,11,8);}if(S.playing&&i===4&&on)rect(x+2,y+2,8,4,C.white);}}
 cards('P'+S.pattern+' SEQUENCE',['LENGTH','RATE',null,null],[S.length+'st',D.rates[S.rate],null,null],'BLACK: TRACK  WHITE: STEP  PATCH+BLK: MUTE');
 // Formerly unmapped K7/K8 display cells become one selected-instrument
 // readout, with no editable card, value or knob caption.
 rect(123,171,114,55,C.bg);g.save();g.translate(144,175);g.scale(3,3);drumGlyph(identityIndex(S.track),0,0,partInk[S.part]);g.restore();text(identityName(S.track),180,212,12,C.ink,true,'center');
}
function home(){
 slab(3,21,234,33);for(let i=0;i<16;i++){const x=7+i*14+(i>=8?4:0);rect(x,23,12,22,C.side);line(x,23,x+11,23,C.edge);if(i===0)for(let a=0;a<8;a++)if(a%2===0)rect(x+2+a,25,1,2,partInk[0]);if(i+1===S.pattern)text(i+1,x+6,45,10,C.gray,false,'center');if(i+1===S.queued){g.strokeStyle=C.amber;g.strokeRect(x-.5,21.5,13,25);}}
 for(let r=0;r<5;r++){const y=55+r*20;slab(3,y,234,18);text(parts[r],7,y+2,11,partInk[r]);for(let i=0;i<16;i++){const x=59+i*11;rect(x,y+3,9,8,C.side);if(r===0&&[0,4,8,12].includes(i))rect(x,y+3,3,8,partInk[r]);}if(S.muted.includes(r))line(6,y+15,48,y+15,C.gray);}
 cards('PERFORM',['TEMPO','SWING','FILTER','PUMP'],[S.tempo+'bpm',S.swing+'%',S.filter<60?'LP '+Math.floor((60-S.filter)*100/60)+'%':S.filter>68?'HP '+Math.floor((S.filter-68)*100/59)+'%':'OFF',S.pump.toFixed(1)+'dB'],'WHITE: PATTERN   TWO: CHAIN   BLACK: MUTE');
}
function pages(){
 if(S.part<2){const map=D.trackMaps.find(x=>x.engine===(S.part?'808':'909')&&x.voice_index===S.track);return map.pages.map((r,i)=>({...r,title:r.title==='KIT'?'KIT':identityName(S.track)+(i?' 2':''),cells:r.cells.map(c=>({...c,editable:!!c.label}))}));}
 return D.engines[S.engine].pages;
}
function fixtureValue(label,p,k){const key=S.engine+':'+p+':'+k;if(S.held>=0&&S.held+':'+key in S.locks)return S.locks[S.held+':'+key]+'%';if(key in S.values)return S.values[key]+'%';if(['JUNO','DRONE'].includes(S.engine)&&!['ENGINE','PRESET','STATUS'].includes(label))return'50%';if(S.engine==='MIAMI'){const m={WAVE:'TRI',SHAPE:'TRI',RATE:'1/16',PITCH:'0st',TUNE:'+0.00'};if(label in m)return m[label];}const defaults={WAVE:'0%',GLIDE:'14%',CUTOFF:'25%',PEAK:'75%',ENV:'+56%',BITE:'83%',GNAW:'0%',ATTACK:'27%',DECAY:'40%',SUSTAIN:'57%',RELEASE:'17%',ENGINE:S.engine,PRESET:'01',STATUS:'ACTIVE',WIDTH:'50%',LEVEL:'80%',ON:'ON',MODE:'UP',RANGE:'2 OCT',SPEED:'1/16',TYPE:'OFF','EDIT 1':'0%','EDIT 2':'0%','ENV/GATE':'ENV'};return defaults[label]||'50%';}
function matrix(){
 const pp=pages(),first=Math.floor(S.page/4)*4;
 for(let r=0;r<4;r++){const p=first+r;if(!pp[p])continue;const row=pp[p],y=24+r*50,active=p===S.page;slab(3,y,234,49,active);text((r+1)+' '+row.title,8,y,10,active?C.ink:C.gray,true);
 if(active){const mapped=row.cells.map((c,k)=>c.editable?k+5:null).filter(x=>x);text('KNOBS '+mapped.join(' '),232,y,10,C.ink,true,'right');}
 if(row.title==='ENVELOPE'&&['CHICAGO'].includes(S.engine)&&S.part>2){for(let k=0;k<4;k++)text(['K5 A27%','K6 D40%','K7 S57%','K8 R17%'][k],8+k*59,y+13,10,active?C.ink:C.gray);line(10,y+42,18,y+26,C.gray);line(18,y+26,56,y+36,C.gray);line(56,y+36,190,y+36,C.gray);line(190,y+36,226,y+44,C.gray);continue;}
 for(let k=0;k<4;k++){const cell=row.cells[k];if(!cell?.label)continue;const x=4+k*59,c=active?C.ink:C.gray;if(k)line(x,y+14,x,y+45,C.line);text(cell.label,x+29,y+13,10,c,false,'center');if(row.title==='SOUND'&&k===2){text('ACTIVE',180,y+30,12,c,true,'center');break;}if(['JUNO','DRONE'].includes(S.engine)&&row.title!=='SOUND'){text(fixtureValue(cell.label,p,k),x+29,y+28,14,c,true,'center');line(x+8,y+45,x+50,y+45,C.line);line(x+8,y+45,x+29,y+45,c);}else{graph(cell.label,x+7,y+27,23,15,c);text(fixtureValue(cell.label,p,k),x+53,y+31,11,c,false,'right');}}}
 slab(0,227,240,13);text(S.held>=0?'STEP '+S.held+' / '+S.chance+'%  K2: CHANCE':'< >: ROW   PARA: NEXT   SCALE: ALL',7,228,10,C.gray);text((first+1)+'-'+Math.min(first+4,pp.length),235,228,10,C.amber,false,'right');
}
function song(){
 const names=['909','808','BRK','CH-B','CH-A'];text('BAR',4,20,11,C.gray);names.forEach((n,i)=>text(n,58+i*36,20,11,partInk[i],false,'center'));
 for(let r=0;r<Math.min(11,S.songBars+1);r++){const y=31+r*11;slab(0,y,238,11,r===0);text(r+1,4,y,11,r===0?C.white:C.gray);if(r<S.songBars)for(let i=0;i<5;i++)text('P'+(r+1),58+i*36,y,11,partInk[i],false,'center');else text(r?'END':'EMPTY: WHITE KEYS ADD BARS',50,y,10,C.dim);}
 cards('BAR ('+(S.page+1)+'/2)',S.page===0?['BAR','909','808','BREAK']:['BAR','CH-B','CH-A','REPEAT'],S.page===0?['1','P1','P1','P1']:['1','P1','P1','1'],'WHITE: THE BAR\'S PATTERN   BLACK: MUTE');
}
function fx(){
 text('REVERB',98,22,10,C.gray,false,'center');text('DELAY',192,22,10,C.gray,false,'center');for(let i=0;i<5;i++){slab(3,34+i*15,234,13);text(parts[i],7,35+i*15,10,partInk[i]);for(let j=0;j<2;j++){const x=60+j*94;line(x,40+i*15,x+76,40+i*15,C.line);rect(x+16+i*7,37+i*15,3,7,partInk[i]);}}
 text('DELAY 1/8  PING-PONG',9,116,14,C.white);text('FEEDBACK  37%     RV DECAY  50%',9,142,11,C.gray);
 const names=S.page===0?['DECAY','TONE',null,null]:['TIME','FEEDBACK','TONE','TAPE'];cards(S.page?'DELAY':'REVERB',names,S.page?['1/8','37%','50%','OFF']:['50%','50%',null,null],'FX: NEXT PAGE   SCALE: ALL AS A LIST');
}
function mix(){
 for(let i=0;i<5;i++){const x=4+i*47;slab(x,24,43,129,i===S.part);text(parts[i].replace('CHICAGO ','CH-'),x+21,27,10,partInk[i],false,'center');rect(x+16,46,9,90,C.side);rect(x+17,66+i*8,7,69-i*8,partInk[i]);text('80%',x+21,137,11,C.gray,false,'center');}
 const master=S.page>2;cards(['PARTS','909 MIX','808 MIX','MASTER','WAVE'][S.page],master?['DRIVE','COMP','PUMP','FILTER']:['LEVEL','PAN','REV','DLY'],master?['0%','OFF','4.0dB','OFF']:['80%','C','20%','12%'],'BLACK 1-5: PART   PATCH+BLK: MUTE');
}
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
function editList(d){const r=deepRows()[S.list];if(!r?.editable)return'Unmapped/read-only: no edit';if(['ENGINE','PRESET','STATUS'].includes(r.label))return'SOUND transaction reserved to incumbent CP1';if(['WAVE','TYPE','ON','MODE','RANGE','SPEED','ENV/GATE'].includes(r.label))return'Existing enum route retained; fixed inspection fixture';if(S.view==='PART'&&S.part!==2){const key=S.engine+':'+r.page+':'+r.k;const shown=parseFloat(fixtureValue(r.label,r.page,r.k))||0;S.values[key]=(S.values[key]??shown)+d;S.dirty=true;return'Selected scalar changed in deep list; active page unchanged';}return'Source list route retained; this scalar preview is not modeled';}
function render(){
 bounds.length=0;rect(0,0,240,240,C.bg);header();
 if(S.view==='HOME')home();else if(S.view==='PART'&&S.part!==2)matrix();else if(S.part===2&&['PART','SEQ'].includes(S.view))breaks();else if(S.view==='SEQ')S.part<2?drumSeq():melodic();else if(S.view==='SONG')song();else if(S.view==='FX')fx();else if(S.view==='MIX')mix();
 if(S.overlay==='LIST'){rect(0,20,240,138,C.bg);const rows=deepRows(),top=Math.max(0,S.list-3);for(let i=0;i<6;i++){const r=rows[top+i];if(!r)break;slab(3,23+i*22,230,21,top+i===S.list);text(r.label,9,26+i*22,13,top+i===S.list?C.ink:C.gray);text(r.page!==undefined?fixtureValue(r.label,r.page,r.k):r.editable?'50%':'READ ONLY',226,26+i*22,13,C.gray,false,'right');}}
 if(S.overlay==='ASK'){rect(0,20,240,138,C.bg);text('LOAD SAVED PROJECT?',120,47,17,C.white,true,'center');text('DISCARD UNSAVED EDITS',120,76,13,C.gray,false,'center');text('SCALE: YES   PATCH: NO',120,105,13,C.green,true,'center');text(S.playing?'PLAY: CANCEL + STOP':'PLAY: CANCEL + START',120,134,11,C.amber,true,'center');}
 cv.setAttribute('aria-label',`${S.view}; ${S.part<2?parts[S.part]:S.engine}; ${S.wait?'external WAIT':S.playing?'RUN':'STOP'}; row ${S.page+1}; ${S.part<2?drumNames[S.track]:''}. Proposal only.`);
 if($('state'))$('state').textContent=`${S.note} · ${S.view} · ${S.wait?'WAIT':S.playing?'RUN':'STOP'} · page ${S.page+1}/${pageCount()} · ${S.held>=0?'held step '+S.held:'base edit scope'}`;
 if($('log'))$('log').textContent=trace.slice(-12).map(x=>x.event+' → '+x.result).join('\n');
 updateLights();
 return {state:structuredClone(S),bounds:structuredClone(bounds),pages:pageCount()};
}
function pageCount(){return S.view==='PART'?S.part===2?1:pages().length:S.view==='SEQ'?S.part<2?1:3:S.view==='SONG'?2:S.view==='FX'?2:S.view==='MIX'?5:1;}
function nav(d){if(S.overlay==='LIST'){S.list=Math.max(0,Math.min(deepRows().length-1,S.list+d));return;}if(S.held>=0){S.chance=Math.max(0,Math.min(100,S.chance+d));return;}const n=pageCount();S.page=['PART','FX','MIX'].includes(S.view)?(S.page+d+n)%n:Math.max(0,Math.min(n-1,S.page+d));if(S.view==='PART')S.soundPage=S.page;}
function event(ev,d=1){let outcome='';
 if(ev==='K2')nav(d);
 else if(ev==='SCALE'){if(S.overlay==='ASK'){outcome='Confirmed input only; storage is outside this model';S.overlay=null;}else if(!S.overlay){S.overlay='LIST';S.list=0;}}
 else if(ev==='PATCH'){if(S.overlay){S.overlay=null;outcome='Caller restored';}else {S.view=S.view==='HOME'?'SONG':'HOME';S.page=0;}}
 else if(ev==='PARA'){if(S.view==='PART'){S.page=(S.page+1)%pageCount();S.soundPage=S.page;}else{S.view='PART';S.page=S.soundPage;}}
 else if(ev==='SEQ'){if(S.view==='PART'&&S.held>=0){outcome='RELEASE STEP FIRST';}else if(S.view==='SEQ')S.page=(S.page+1)%pageCount();else{S.view='SEQ';S.page=0;}}
 else if(ev==='FX'||ev==='NOTE REPEAT'){const v=ev==='FX'?'FX':'MIX';if(S.view===v)S.page=(S.page+1)%pageCount();else{S.view=v;S.page=0;}}
 else if(ev==='SEQ PLAY'){if(S.overlay==='ASK'){S.overlay=null;outcome='Question cancelled; ';}S.playing=!S.playing;outcome+='transport command, no generated audio';}
 else if(ev==='SEQ REC'){S.rec=!S.rec;outcome='Arm fixture only; CP2/CP3 capture strategy is not selected';}
 else if(ev==='K3'){if(S.overlay==='LIST'){outcome=editList(d);}else if(S.held>=0)S.velocity=Math.max(0,Math.min(127,S.velocity+d));else{S.part=(S.part+d+5)%5;S.page=0;S.soundPage=0;}}
 else if(ev==='K4'){S.queued=Math.max(1,Math.min(32,(S.queued||S.pattern)+d));outcome='Pending target fixture; adoption requires musical boundary';}
 else if(/^K[5-8]$/.test(ev)){const k=Number(ev[1])-5;let changed=false;if(S.view==='HOME'){if(k===0)S.tempo+=d;if(k===1)S.swing=Math.max(0,Math.min(100,S.swing+d));if(k===2)S.filter=Math.max(0,Math.min(127,S.filter+d));if(k===3)S.pump+=d/10;changed=true;}else if(S.view==='PART'&&S.part!==2){const cell=pages()[S.page]?.cells[k];if(!cell?.label||!cell.editable){outcome='Unmapped/read-only: no edit';}else if(S.page===pages().length-1&&S.part>2){outcome='SOUND transaction details reserved to incumbent CP1; no simulated request';}else if(['WAVE','TYPE','ON','MODE','RANGE','SPEED','ENV/GATE'].includes(cell.label)){outcome='Existing enum route retained; enum value is a fixed inspection fixture here';}else{const key=S.engine+':'+S.page+':'+k;const shown=parseFloat(fixtureValue(cell.label,S.page,k))||0;if(S.held>=0){S.locks[S.held+':'+key]=(S.locks[S.held+':'+key]??shown)+d;outcome='Step '+S.held+' lock; base unchanged';}else{S.values[key]=(S.values[key]??shown)+d;outcome='Base value; existing step locks retained';}changed=true;}}else if(S.view==='SEQ'){if(k===0&&S.part<2){S.length=Math.max(1,Math.min(64,S.length+d));changed=true;}if(k===1&&S.part<2){S.rate=Math.max(0,Math.min(D.rates.length-1,S.rate+d));changed=true;}if(!changed)outcome='No mapped drum control here; no edit';}else outcome='Source-layout preview; this scalar edit is not implemented in the design model';if(changed)S.dirty=true;}
 else if(ev==='OCTAVE+'||ev==='OCTAVE-'){if(S.view==='SEQ'&&S.part<2){S.stepPage=Math.max(0,Math.min(3,S.stepPage+(ev.endsWith('+')?1:-1)));}else outcome='Context-dependent source route; outside this bounded fixture';}
 else if(ev==='BT')outcome='SAVE request only; existing authoritative result lifecycle remains outside this model';
 else if(ev==='GLOBE'){outcome='Settings component retained with reserved owner; no new Settings layout';}
 S.note=outcome||'Existing input route retained';trace.push({event:ev+(d<0?' −':''),result:S.note,state:{view:S.view,page:S.page,held:S.held}});sync();render();
}
function black(n){if(S.view==='HOME'){S.muted.includes(n)?S.muted.splice(S.muted.indexOf(n),1):S.muted.push(n);}else if(S.part<2){S.track=n;S.page=0;S.soundPage=0;}S.note=S.playing?'Track selection; running audition is source-conditional, no audio here':'Track selection; stopped audition follows KEY SOUND in actual source';trace.push({event:blacks[n],result:S.note});render();}
function pad(n,held=false){const step=S.stepPage*16+n+1;if(held){if(S.view!=='SEQ'){S.note='Start step inspection in SEQ; PART keys normally play sound';render();return;}S.held=step;S.note='Original step held; no new navigation binding';}else if(S.view==='HOME'){S.queued=n+1;S.note='Pattern target requested; no automatic adoption';}else if(S.view==='SEQ'&&S.part<2){const a=S.steps[S.track],at=a.indexOf(step);at<0?a.push(step):a.splice(at,1);S.note='Selected track step toggled';S.dirty=true;}else S.note='Musical key input is live-only in this bounded model; no audio generated';trace.push({event:(held?'Hold ':'')+'pad'+n,result:S.note});render();}
function release(){S.held=-1;S.note='Original step released; next parameter turn edits base';trace.push({event:'Release original step',result:S.note});render();}
function updateLights(){
 const rgb=[[127,74,20],[127,36,32],[106,60,127],[32,103,127],[65,121,45]][S.part],dim=rgb.map(x=>Math.floor(x/4));
 const pads=[];for(let i=0;i<16;i++){let value=0;if(S.lightsEnabled&&S.view==='HOME'){value=i+1===S.pattern?2:i===0?1:0;if(i+1===S.queued&&(S.framePhase&8))value=2;}else if(S.lightsEnabled&&S.part<2&&S.view!=='MIX'){const step=S.stepPage*16+i+1;let on=step<=S.length&&S.steps[S.track].includes(step);if(S.playing&&i===S.playhead)on=!on;value=on?2:0;}if(S.held===S.stepPage*16+i+1)value=2;pads.push(value);}
 document.querySelectorAll('[data-pad]').forEach((b,i)=>{const v=pads[i],color=v===2?rgb:v===1?dim:[0,0,0];b.style.background=`rgb(${color.join(',')})`;b.style.borderColor=v?C.gray:C.line;b.setAttribute('aria-label',`Physical pad${i}, ${v===2?'bright':v===1?'dim':'off'} source snapshot; GRID step${i+1}`)});
 const on={'PATCH':S.view==='HOME'||S.view==='SONG','SEQ':S.view==='SEQ','PARA':S.view==='PART','FX':S.view==='FX','NOTE REPEAT':S.view==='MIX','SCALE':!!S.overlay,'SEQ REC':S.rec,'SEQ PLAY':S.wait?!!(S.framePhase&32):S.playing&&S.playhead%4===0,'OCTAVE+':S.stepPage>0};
 document.querySelectorAll('#panelButtons [data-ev]').forEach(b=>{const lit=!!on[b.dataset.ev];b.setAttribute('data-lit',String(lit));b.setAttribute('aria-label',b.dataset.ev+'; source LED '+(lit?'on':'off'));});
 if($('ledState'))$('ledState').textContent=`Source-only snapshot: pad bright/dim/off=${pads.join(' ')}. PLAY=${on['SEQ PLAY']?'on':'off'}, REC=${S.rec?'on':'off'}. No keybed LEDs; pad pattern does not certify mute or recording success.`;
 return {pads,buttons:on,evidence:'SOURCE SNAPSHOT MODEL; NOT PHYSICAL/EMULATOR OBSERVATION'};
}
function sync(){if($('trackButtons'))[...$('trackButtons').children].forEach((b,i)=>b.setAttribute('aria-pressed',String(i===S.track)));}
window.proposal={reset,event,black,pad,release,render,pages,pageCount,drumGlyph,lights:updateLights,configure(patch){Object.assign(S,patch);render();},get state(){return structuredClone(S)},get trace(){return structuredClone(trace)}};
