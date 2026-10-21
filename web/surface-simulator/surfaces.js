/* Original projected-solid model of verified M-VAVE hardware footprints.
 * No photo, third-party code, SVG skew, or raster asset is embedded.
 * Plan coordinates retain the verified 1000-unit top view; heights are illustrative.
 * Camera: orthographic azimuth35/elevation40; +z is up. See geometry-notes.md. */
const esc=s=>String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('"','&quot;');
const n=v=>Number(v.toFixed(4));
const clamp=v=>Math.max(0,Math.min(127,Number(v)||0));
const A=35*Math.PI/180,E=40*Math.PI/180;
const basis={a:Math.cos(A),b:Math.sin(A)*Math.sin(E),c:-Math.sin(A),d:Math.cos(A)*Math.sin(E),z:-Math.cos(E)};
/** Fit all projected corners, including raised parts, with a fixed margin. */
export function fitProjection(points,width=1000,pad=24){
 const raw=([x,y,z])=>({x:basis.a*x+basis.c*y,y:basis.b*x+basis.d*y+basis.z*z});
 const p=points.map(raw),loX=Math.min(...p.map(p=>p.x)),hiX=Math.max(...p.map(p=>p.x)),loY=Math.min(...p.map(p=>p.y)),hiY=Math.max(...p.map(p=>p.y));
 const scale=(width-2*pad)/(hiX-loX),tx=pad-scale*loX,ty=pad-scale*loY,height=Math.ceil((hiY-loY)*scale+2*pad);
 const project=(x,y,z=0)=>({x:scale*(basis.a*x+basis.c*y)+tx,y:scale*(basis.b*x+basis.d*y+basis.z*z)+ty});
 const unproject=(sx,sy,z=0)=>{const x=(sx-tx)/scale,y=(sy-ty)/scale-basis.z*z,det=basis.a*basis.d-basis.b*basis.c;return {x:(x*basis.d-y*basis.c)/det,y:(y*basis.a-x*basis.b)/det};};
 const planeTransform=(z=0)=>`matrix(${[scale*basis.a,scale*basis.b,scale*basis.c,scale*basis.d,tx,ty+scale*basis.z*z].map(n).join(' ')})`;
 return {width,height,scale,project,unproject,planeTransform};
}
const depth=(x,y,z=0)=>x*Math.sin(A)*Math.cos(E)+y*Math.cos(A)*Math.cos(E)+z*Math.sin(E);
function geom(kind,physicalHeight){
 const P=fitProjection([...[0,1000].flatMap(x=>[0,physicalHeight].flatMap(y=>[-5,90].map(z=>[x,y,z]))) ]);
 const isSmk=kind==='smk',knobs=Array.from({length:8},(_,i)=>({x:isSmk?274+i*60.7:53+i*118.4,y:isSmk?38:70,r:isSmk?18:31,z:isSmk?63:78}));
 const faders=Array.from({length:isSmk?4:8},(_,i)=>({x:isSmk?772+i*62.3:53+i*118.4,y:isSmk?28:152,height:isSmk?81:205,capWidth:isSmk?38:45.6,capHeight:isSmk?18:21.6,z:32,capDepth:14,defaultCapY:(isSmk?28:152)+(isSmk?81:205)*.42}));
 return {...P,physicalWidth:1000,physicalHeight,caseTop:30,screen:{x:143,y:27,width:67,height:65,z:33},knobs,faders,
 knobTransform(index,degrees){const k=knobs[index];return `${P.planeTransform(k.z)} rotate(${n(Number(degrees)||0)} ${k.x} ${k.y})`;},
 faderTransform(index,value){const f=faders[index],dest=f.y+(1-clamp(value)/127)*(f.height-f.capHeight),delta=dest-f.defaultCapY;return `translate(${n(P.scale*basis.c*delta)} ${n(P.scale*basis.d*delta)})`;},
 faderValue(index,svgX,svgY){const f=faders[index],p=P.unproject(svgX,svgY,f.z+f.capDepth);return clamp(127*(1-(p.y-f.y-f.capHeight/2)/(f.height-f.capHeight)));},
 keyTransform(pressed){return `translate(0 ${pressed?n(-P.scale*basis.z*3):0})`;}
 };
}
export const surfaceGeometry={smk:geom('smk',395),smc:geom('smc',477)};
const palettes={orange:['#f37b3d','#df571e','#b43e14','#7f3019'],ivory:['#faf9ef','#dfdfd5','#b8beb8','#838e8b'],grey:['#818e94','#606e76','#46535d','#303e49'],silver:['#f1f3ef','#cdd4d2','#a6b1b0','#768584'],black:['#586063','#343d41','#252c31','#151d22']};
const plan=(x,y,w,h,r=0)=>[[x+r,y],[x+w-r,y],[x+w,y+r],[x+w,y+h-r],[x+w-r,y+h],[x+r,y+h],[x,y+h-r],[x,y+r]];
function drawing(kind){
 const G=surfaceGeometry[kind],P=G.project;
 const poly=(pts,fill,attr='')=>`<polygon points="${pts.map(([x,y,z])=>{const p=P(x,y,z);return `${n(p.x)},${n(p.y)}`;}).join(' ')}" fill="${fill}" ${attr}/>`;
 const top=(pts,z,fill,attr='')=>poly(pts.map(([x,y])=>[x,y,z]),fill,attr);
 const line=(p1,p2,color,width=.7)=>{const a=P(...p1),b=P(...p2);return `<path d="M${n(a.x)} ${n(a.y)}L${n(b.x)} ${n(b.y)}" stroke="${color}" stroke-width="${width}" fill="none"/>`;};
 const txt=(x,y,z,label,size=6,color='#e6eae7',extra='')=>`<text transform="${G.planeTransform(z)}" x="${x}" y="${y}" font-size="${size}" text-anchor="middle" font-family="Arial,sans-serif" fill="${color}" ${extra}>${esc(label)}</text>`;
 // Rounded-rectangle prism: outward side faces, bevel ring, and one lit top.
 function solid(x,y,w,h,z,t,palette,r=2,face=false){
  const out=plan(x,y,w,h,r),inset=Math.min(1.4,w/8,h/8),inn=plan(x+inset,y+inset,w-inset*2,h-inset*2,Math.max(0,r-inset));
  let s='';for(let i=0;i<out.length;i++){const j=(i+1)%out.length,a=out[i],b=out[j],dx=b[0]-a[0],dy=b[1]-a[1];if(dy*Math.sin(A)-dx*Math.cos(A)>0){s+=poly([[...a,z],[...b,z],[...b,z+t-inset],[...a,z+t-inset]],dy>0?palette[3]:palette[2],'data-solid-side="true"');s+=poly([[...a,z+t-inset],[...b,z+t-inset],[...inn[j],z+t],[...inn[i],z+t]],palette[1]);}}
  s+=top(inn,z+t,palette[0],face?'data-face="true"':'');return s;
 }
 function cylinder(x,y,r,z,t,palette){
  const points=Array.from({length:16},(_,i)=>{const a=i*2*Math.PI/16;return [x+Math.cos(a)*r,y+Math.sin(a)*r];});let s='';
  const faces=[];for(let i=0;i<16;i++){const j=(i+1)%16,a=points[i],b=points[j],mx=(a[0]+b[0])/2-x,my=(a[1]+b[1])/2-y;if(mx*Math.sin(A)+my*Math.cos(A)>0)faces.push({d:depth((a[0]+b[0])/2,(a[1]+b[1])/2),a,b,mx,my});}
  faces.sort((a,b)=>a.d-b.d).forEach(({a,b,mx,my})=>{s+=poly([[...a,z],[...b,z],[x+(b[0]-x)*.86,y+(b[1]-y)*.86,z+t],[x+(a[0]-x)*.86,y+(a[1]-y)*.86,z+t]],mx>r*.4?palette[3]:my>r*.75?palette[2]:palette[1],'data-solid-side="true"');});
  s+=top(points.map(([px,py])=>[x+(px-x)*.86,y+(py-y)*.86]),z+t,palette[0]);return s;
 }
 const control=(id,label,body,extra='')=>`<g data-control="${id}" aria-label="${esc(label)}" role="button" tabindex="0" ${extra}><title>${esc(label)}</title>${body}</g>`;
 const items=[];const put=(x,y,z,svg)=>items.push({d:depth(x,y,z),svg});
 function button(id,label,x,y,w,h,pal=palettes.orange,size=5){const z=31,t=5;put(x+w/2,y+h/2,z,control(id,label,solid(x,y,w,h,z,t,pal,3,true)+txt(x+w/2,y+h*.68,z+t+.1,label,size,pal===palettes.orange?'#63341f':'#35434b')));}
 function knob(id,i){const k=G.knobs[i],pal=kind==='smk'?palettes.orange:palettes.black;let s=cylinder(k.x,k.y,k.r+2,30,3,palettes.black)+cylinder(k.x,k.y,k.r,33,k.z-33,pal);
  // Pointer lives in world-plane coordinates, under its own complete affine transform.
  s+=`<g data-pointer="true" transform="${G.knobTransform(i,0)}"><path d="M ${k.x} ${k.y-k.r*.69} v ${k.r*.36}" fill="none" stroke="#f7e5c9" stroke-width="2.1" stroke-linecap="round"/></g>`;
  put(k.x,k.y,45,control(id,`Encoder ${i+1}`,s));
 }
 function fader(id,i){const f=G.faders[i];let s=top(plan(f.x-3,f.y,6,f.height,2),30.2,'#18252c');
  s+=`<g data-cap="true">${solid(f.x-f.capWidth/2,f.defaultCapY,f.capWidth,f.capHeight,f.z,f.capDepth,kind==='smk'?palettes.orange:palettes.black,2)}${line([f.x-f.capWidth/2+2,f.defaultCapY+f.capHeight/2,f.z+f.capDepth+.2],[f.x+f.capWidth/2-2,f.defaultCapY+f.capHeight/2,f.z+f.capDepth+.2],'#afbbba',.9)}</g>`;
  put(f.x,f.y+f.height*.45,32,control(id,`Fader ${i+1}`,s));
 }
 let base=`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${G.width} ${G.height}" class="surface ${kind}-surface" role="group" aria-label="${kind==='smk'?'M-VAVE SMK-37 Elite':'M-VAVE SMC-Mixer'} interactive isometric hardware">`;
 // Ground contact shadow, bottom seam, then closed case with actual visible walls.
 base+=top(plan(9,10,990,G.physicalHeight-5,16),-5,'#06131b','opacity=".15"');
 base+=solid(0,0,1000,G.physicalHeight,0,9,palettes.black,12);
 base+=solid(0,0,1000,G.physicalHeight,9,21,kind==='smk'?palettes.grey:palettes.silver,12);
 return {G,top,txt,line,solid,cylinder,control,put,button,knob,fader,finish:()=>base+items.sort((a,b)=>a.d-b.d).map(i=>i.svg).join('')+'</svg>',addBase:s=>base+=s};
}
export function renderSmk(){
 const D=drawing('smk'),{G,top,txt,line,solid,control,put,button,knob,fader}=D;
 D.addBase(top(plan(24,205,951,187,3),30.1,'#1d2931'));
 // Naturals are ordered as an assembly; raised accidentals sit on the keybed above them.
 let keys='';const white=[0,2,4,5,7,9,11],x0=28,w=42.8;
 for(let i=0;i<22;i++){const id=Math.floor(i/7)*12+white[i%7];keys+=control(`smk-key-${id}`,`Key ${id}`,solid(x0+i*w,209,w-1,180,22,13,palettes.ivory,2,true));}
 for(let oct=0;oct<3;oct++)for(const[wi,note]of[[0,1],[1,3],[3,6],[4,8],[5,10]])keys+=control(`smk-key-${oct*12+note}`,`Key ${oct*12+note}`,solid(x0+(oct*7+wi+1)*w-12,209,23,110,35,17,palettes.orange,2,true));
 put(500,310,26,keys);
 D.addBase(txt(66,168,30.2,'M-VAVE',19,'#e9ece7','font-style="italic"'));
 for(const[label,x]of[['PITCH',25],['MOD',76]]){D.addBase(txt(x+15,38,30.2,label,7));put(x+15,94,40,control(`smk-${label.toLowerCase()}`,label,top(plan(x-3,48,36,91,4),30.2,'#1c2a32')+solid(x,50,31,88,31,16,palettes.orange,8)+[0,1,2,3,4,5,6].map(i=>line([x+4,59+i*10,47.2],[x+27,59+i*10,47.2],'#b84a20',.5)).join('')));}
 put(176,60,31,solid(134,18,85,83,30,3,palettes.black,9)+top(plan(143,27,67,65,2),33.1,'#162a3e')+`<g id="smk-screen" data-display="smk" transform="${G.planeTransform(33.2)}"/>`);
 D.addBase(top(plan(139,112,71,53,4),30.1,'#b94920')+txt(174,108,30.2,'PAD BANK',4.8));
 const banks=[['knob-bank','KNOB BANK'],['fader-bank','FADER BANK'],['left','‹'],['right','›'],['octave-minus','OCTAVE-'],['octave-plus','OCTAVE+']];
 banks.forEach(([id,label],i)=>button(`smk-${id}`,label,143+i%2*32,115+Math.floor(i/2)*15,31,14,palettes.orange,i<2?4:5));
 for(let i=0;i<8;i++){knob(`smk-knob-${i}`,i);D.addBase(txt(274+i*60.7,18,30.2,`K${i+1}`,5));}
 for(let row=0;row<2;row++)for(let col=0;col<8;col++){const x=250+col*60.7,y=70+row*45.5,i=row*8+col;put(x+24,y+19,34,control(`smk-pad-${i}`,`Pad ${i+1}`,solid(x-1,y-1,50,40,30,2,palettes.black,2)+solid(x,y,48,38,32,4,palettes.ivory,2,true)+line([x+8,y+5,36.2],[x+39,y+5,36.2],'#657375',1.4)));}
 for(let i=0;i<4;i++){fader(`smk-fader-${i}`,i);D.addBase(txt(772+i*62.3,18,30.2,`FADER${i+1}`,5));}
 D.addBase(top(plan(747,126,230,40,5),30.1,'#a74222'));
 const fun=[['arp','ARP'],['note-repeat','NOTE REPEAT'],['scale','SCALE'],['chord','CHORD'],['globe','GLOBE'],['bt','BT'],['patch','PATCH'],['para','PARA'],['fx','FX'],['seq','SEQ'],['seq-play','SEQ PLAY'],['seq-rec','SEQ REC']];
 fun.forEach(([id,label],i)=>button(`smk-${id}`,label,751+i%6*37.1,130+Math.floor(i/6)*16,36,15,palettes.orange,4.6));
 const legends=['UP','DOWN','INCL','EXCL','RAND','ORDER','REPEAT','OCT+','LATCH','GATE-','GATE+','TAP','SWING-','SWING+','TEMPO-','TEMPO+','SYNC'];legends.forEach((s,i)=>D.addBase(txt(28+i*26.2,191,30.2,s,4.6)));D.addBase(txt(950,199,30.2,'RESET',4.5));
 return D.finish();
}
export function renderSmc(){
 const D=drawing('smc'),{G,top,button,knob,fader}=D;
 for(let i=0;i<8;i++){const x=53+i*118.4;knob(`smc-knob-${i}`,i);D.addBase(top(plan(x-16,114,31,9,2),30.1,'#8a9796')+top(plan(x-11,116,21,5,1),30.2,'#263b40',`data-led="smc-${i}"`)+top(plan(x-23,139,47,231,8),30.1,'#b3bfbd')+top(plan(x-21,141,43,227,7),30.2,'#ecf0e9'));fader(`smc-fader-${i}`,i);
  [['mute','M'],['solo','S'],['rec','R'],['select','□']].forEach(([id,label],j)=>button(`smc-${id}-${i}`,label,x+43,162+j*51.8,33,33,palettes.silver,16));}
 button('smc-bt','BT',932,48,52,24,palettes.silver,11);button('smc-shift','SHIFT',932,106,52,24,palettes.silver,10);
 const globals=[['play','▶'],['stop','Ⅱ'],['record','●'],['rewind','◀◀'],['forward','▶▶'],['bank-left','≪'],['bank-right','≫'],['up','△'],['down','▽'],['left','◁'],['right','▷']];
 globals.forEach(([id,label],i)=>{const x=64+i*82.7;if(['play','rewind','forward'].includes(id)){const icons=id==='play'?[[0,1]]:id==='rewind'?[[-5,-1],[5,-1]]:[[-5,1],[5,1]];D.put(x+23.5,421.5,31,D.control(`smc-${id}`,id,D.solid(x,410,47,23,31,5,palettes.silver,3,true)+icons.map(([offset,d])=>top([[x+23+offset-5*d,416],[x+23+offset+5*d,422],[x+23+offset-5*d,428]],36.2,'#4c5c63')).join('')));}else button(`smc-${id}`,label,x,410,47,23,palettes.silver,15);});
 return D.finish();
}
