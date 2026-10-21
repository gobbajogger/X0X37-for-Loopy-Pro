/* Original SVG geometry traced numerically from official M-VAVE product photographs.
 * See REFERENCES.md. Coordinates are image-derived estimates, not measured CAD.
 * No manufacturer image is embedded. */
const esc=s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('"','&quot;');
const rect=(x,y,w,h,fill,rx=2,extra='')=>`<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${rx}" fill="${fill}" ${extra}/>`;
const text=(x,y,s,size=5,fill='#e4e5e3',extra='')=>`<text x="${x}" y="${y}" fill="${fill}" font-size="${size}" text-anchor="middle" font-family="Arial,sans-serif" ${extra}>${esc(s)}</text>`;
const group=(id,label,body)=>`<g data-control="${id}" aria-label="${esc(label)}" role="button" tabindex="0"><title>${esc(label)}</title>${body}</g>`;
function button(id,label,x,y,w,h,color='#dc4b13',ink='#582e1e',size=5){return group(id,label,rect(x,y,w,h,'#343a3f',3)+rect(x+1,y+1,w-2,h-2,color,2)+text(x+w/2,y+h*.66,label,size,ink));}
function knob(id,label,x,y,r,color){return group(id,label,`<circle cx="${x}" cy="${y+1}" r="${r+2}" fill="#242a2d" opacity=".45"/><circle cx="${x}" cy="${y}" r="${r}" fill="${color}" stroke="#24292b" stroke-opacity=".3"/><path data-pointer="true" d="M ${x} ${y-r+3} v 5" stroke="#efc8aa" stroke-width="1.2"/>`);}
function fader(id,label,x,y,h,color,scale=1){return group(id,label,rect(x-3,y,6,h,'#171b1d',2)+rect(x-19*scale,y+h*.42,38*scale,18*scale,color,2,'data-cap="true"')+`<path d="M ${x-17*scale} ${y+h*.42+8*scale} h ${34*scale}" stroke="#ffffff" stroke-opacity=".28"/>`);}
export function renderSmk(){
let a=[`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 395" class="surface smk-surface" role="group" aria-label="M-VAVE SMK-37 Elite physical control surface">`,rect(1,1,998,393,'#60696f',10,'stroke="#919a9c" stroke-width="2"'),rect(8,8,984,378,'#646e74',7)];
// Piano: 22 white keys, 15 orange accidental keys, C through C (three octaves).
const keyX=28,keyY=209,keyW=42.8,keyH=180,whiteSemis=[0,2,4,5,7,9,11];
for(let i=0;i<22;i++){const note=Math.floor(i/7)*12+whiteSemis[i%7];a.push(group(`smk-key-${note}`,`Key ${note}`,rect(keyX+i*keyW,keyY,keyW-1,keyH,'#f1f0e9',3,'data-face="true" stroke="#a4a8a5" stroke-width=".7"')));}
for(let oct=0;oct<3;oct++)for(const [wi,semitone] of [[0,1],[1,3],[3,6],[4,8],[5,10]]){let x=keyX+(oct*7+wi+1)*keyW-12;a.push(group(`smk-key-${oct*12+semitone}`,`Key ${oct*12+semitone}`,rect(x,keyY,23,110,'#9c3510',2)+rect(x+1,keyY+1,21,100,'#d94b13',2,'data-face="true"')+rect(x+3,keyY+2,15,94,'#e55717',1)));}
a.push(text(66,168,'M-VAVE',21,'#e7e8e4','font-style="italic"'));
for(const [label,x] of [['PITCH',25],['MOD',76]]){a.push(text(x+15,38,label,7));a.push(group(`smk-${label.toLowerCase()}`,label,rect(x-3,48,36,91,'#24282c',5)+rect(x,50,31,88,'#db4c14',10)+rect(x+3,56,25,73,'#e75b19',9)));}
a.push(rect(134,18,85,83,'#151d29',11),rect(143,27,67,65,'#243345',2),`<g id="smk-screen" data-display="smk"/>`);
const bank=[['knob-bank','KNOB BANK'],['fader-bank','FADER BANK'],['left','‹'],['right','›'],['octave-minus','OCTAVE-'],['octave-plus','OCTAVE+']];
a.push(rect(139,112,71,53,'#db4d15',5),text(174,108,'PAD BANK',4.8));
for(let i=0;i<bank.length;i++){let [id,label]=bank[i];a.push(button(`smk-${id}`,label,143+(i%2)*32,115+Math.floor(i/2)*15,31,14,'#e65b20','#4d3024',i<2?4:5));}
for(let i=0;i<8;i++){let x=274+i*60.7;a.push(text(x,18,`K${i+1}`,5));a.push(knob(`smk-knob-${i}`,`K${i+1}`,x,38,18,'#e35117'));}
for(let row=0;row<2;row++)for(let col=0;col<8;col++){let x=250+col*60.7,y=70+row*45.5,id=row*8+col;a.push(group(`smk-pad-${id}`,`Pad ${id+1}`,rect(x-1,y-1,50,40,'#424a4e',3)+rect(x,y,48,38,'#f7f4e6',2,'data-face="true"')+rect(x+8,y+4,31,2,'#767a75',1)));}
for(let i=0;i<4;i++){let x=772+i*62.3;a.push(text(x,18,`FADER${i+1}`,5));a.push(fader(`smk-fader-${i}`,`Fader ${i+1}`,x,28,81,'#e35218'));}
a.push(rect(747,126,230,40,'#d54b15',5,'stroke="#353c41" stroke-width=".7"'));
const fun=[['arp','ARP'],['note-repeat','NOTE REPEAT'],['scale','SCALE'],['chord','CHORD'],['globe','GLOBE'],['bt','BT'],['patch','PATCH'],['para','PARA'],['fx','FX'],['seq','SEQ'],['seq-play','SEQ PLAY'],['seq-rec','SEQ REC']];
for(let i=0;i<fun.length;i++){let [id,label]=fun[i];a.push(button(`smk-${id}`,label,751+(i%6)*37.1,130+Math.floor(i/6)*16,36,15,'#e65c21','#653a29',4.6));}
// Printed key-function rail is deliberately subtle, separate from clickable keys.
const legends=['UP','DOWN','INCL','EXCL','RAND','ORDER','REPEAT','OCT+','LATCH','GATE-','GATE+','TAP','SWING-','SWING+','TEMPO-','TEMPO+','SYNC'];
for(let i=0;i<legends.length;i++)a.push(text(28+i*26.2,191,legends[i],4.6));
a.push(text(950,199,'RESET',4.5),'</svg>');return a.join('');
}
export function renderSmc(){
let a=[`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 477" class="surface smc-surface" role="group" aria-label="M-VAVE SMC-Mixer physical control surface">`,rect(1,1,998,475,'#dce0df',28,'stroke="#a6acad" stroke-width="2"'),rect(7,6,986,463,'#e4e7e6',25)];
for(let i=0;i<8;i++){let x=53+i*118.4;a.push(knob(`smc-knob-${i}`,`Encoder ${i+1}`,x,70,31,'#252c2d'));a.push(rect(x-16,114,31,9,'#717a7b',2),rect(x-11,116,21,5,'#272c2d',1));a.push(rect(x-23,139,47,231,'#b9c0c0',10),rect(x-21,141,43,227,'#eef0ee',8));a.push(fader(`smc-fader-${i}`,`Fader ${i+1}`,x,152,205,'#2c3334',1.2));for(let j=0;j<4;j++){let [id,label]=[['mute','M'],['solo','S'],['rec','R'],['select','□']][j];a.push(button(`smc-${id}-${i}`,`${label}`,x+43,162+j*51.8,33,33,'#e1e5e4','#566064',16));}}
a.push(button('smc-bt','BT',932,48,52,24,'#e5e8e7','#596467',11),button('smc-shift','SHIFT',932,106,52,24,'#e5e8e7','#596467',10));
const globals=[['play','▶'],['stop','Ⅱ'],['record','●'],['rewind','◀◀'],['forward','▶▶'],['bank-left','≪'],['bank-right','≫'],['up','△'],['down','▽'],['left','◁'],['right','▷']];
for(let i=0;i<globals.length;i++){let [id,label]=globals[i];let x=64+i*82.7; if(['play','rewind','forward'].includes(id)){let icons=id==='play'?[[0,1]]:id==='rewind'?[[-5,-1],[5,-1]]:[[-5,1],[5,1]];a.push(group(`smc-${id}`,id,rect(x,410,47,23,'#687477',2)+rect(x+1,411,45,21,'#e8ebe9',2)+icons.map(([offset,d])=>`<path d="M ${x+23+offset-5*d} 416 l ${10*d} 6 l ${-10*d} 6 z" fill="#5d696b"/>`).join('')));}else a.push(button(`smc-${id}`,label,x,410,47,23,'#e8ebe9','#5d696b',15));}a.push('</svg>');return a.join('');
}

export const surfaceGeometry = {
 smk:{width:1000,height:395,screen:{x:143,y:27,width:67,height:65},faders:Array.from({length:4},(_,i)=>({x:772+i*62.3,y:28,height:81,capWidth:38,capHeight:18,defaultCapY:28+81*.42})),knobs:Array.from({length:8},(_,i)=>({x:274+i*60.7,y:38,r:18}))},
 smc:{width:1000,height:477,faders:Array.from({length:8},(_,i)=>({x:53+i*118.4,y:152,height:205,capWidth:45.6,capHeight:21.6,defaultCapY:152+205*.42})),knobs:Array.from({length:8},(_,i)=>({x:53+i*118.4,y:70,r:31}))}
};
