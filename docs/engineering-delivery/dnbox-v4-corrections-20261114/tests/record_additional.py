"""Capture actual immutable #899 firmware output; no firmware/runtime edits or device.
WAV encoding only clamps/rounds captured stereo float PCM to PCM16, as prior evidence.
"""
from playwright.sync_api import sync_playwright
from pathlib import Path
import json, wave, struct, hashlib, math

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'evidence'/'recordings-extra';OUT.mkdir(exist_ok=True)
COLORS={'909':64709,'808':64072,'break':54238,'chicago':18047,'miami':34699}
def rgb(r,g,b):return ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
COLORS={n:rgb(*v) for n,v in {'909':(255,150,40),'808':(255,72,64),'break':(214,120,255),'chicago':(70,205,255),'miami':(130,240,90)}.items()}
ORDER=list(COLORS);ORDER=['909','808','break','chicago','miami']
receipts=[]
with sync_playwright() as p:
 b=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True)
 pg=b.new_page();pg.goto('http://127.0.0.1:4322/qualification.html?model=pro');pg.locator('#boot').click()
 pg.wait_for_function('q.boot.length>0',timeout=60000)
 pg.wait_for_function('q.frames.at(-1)?.ms>=12000',timeout=60000)
 def send(m):pg.evaluate('(m)=>send(m)',m);pg.wait_for_timeout(140)
 def key(i):send(dict(type='key',id=i,down=True));send(dict(type='key',id=i,down=False))
 def enc(i,n):send(dict(type='enc',enc=i,clicks=n));pg.wait_for_timeout(250)
 def pad(i):send(dict(type='analog',idx=i,value=700));send(dict(type='analog',idx=i,value=0))
 def select(n):
  for _ in range(12):
   h=pg.evaluate('q.header')
   if h==COLORS[n]:return
   if h not in COLORS.values():raise RuntimeError('Unrecognized actual part header '+str(h))
   cur=next(k for k,v in COLORS.items() if v==h);enc(2,1 if ORDER.index(n)>ORDER.index(cur) else -1)
  raise RuntimeError('No verified part '+n)
 def begin(name):pg.evaluate('(n)=>{mark(n);q.pcm=[];q.audio=[]}',name)
 def finish(name,description):
  pg.wait_for_timeout(100)
  data=pg.evaluate('({pcm:q.pcm,audio:q.audio,frames:q.frames.filter(f=>f.phase===q.phase),events:q.events.filter(e=>e.phase===q.phase),errors:q.errors,boot:q.boot,header:q.header})')
  pcm=data.pop('pcm'); raw=bytearray();wav=bytearray();peak=0
  for l,r in pcm:
   for a,z in zip(l,r):
    assert math.isfinite(a) and math.isfinite(z)
    raw+=struct.pack('<ff',a,z);peak=max(peak,abs(a),abs(z))
    wav+=struct.pack('<hh',max(-32768,min(32767,round(a*32768))),max(-32768,min(32767,round(z*32768))))
  assert peak>.001 and len(wav)>44100*4
  dest=OUT/(name+'.wav')
  with wave.open(str(dest),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(44100);f.writeframes(wav)
  (OUT/(name+'.f32le')).write_bytes(raw)
  pg.locator('#lcd').screenshot(path=str(OUT/(name+'.png')))
  data.update(name=name,description=description,peak=peak,seconds=len(wav)/4/44100,wav_sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),raw_sha256=hashlib.sha256(raw).hexdigest())
  (OUT/(name+'.json')).write_text(json.dumps(data,indent=2));receipts.append({k:data[k] for k in ['name','description','peak','seconds','wav_sha256','raw_sha256']});print(json.dumps(receipts[-1]),flush=True)
 select('909');key(6)
 for i in [0,4,8,12]:pad(i)
 select('chicago')
 for i in [0,3,6,9]:pad(i)
 key(53);begin('length-before');pg.wait_for_timeout(5000);finish('length-before','909 four kicks plus Chicago four A2 notes at steps 1/4/7/10, both 16 steps.');key(53)
 # SEQ Chicago: physical K5 LENGTH, then held step/K2 chance.
 enc(4,-4)
 send(dict(type='analog',idx=9,value=700));enc(1,-10);pg.locator('#lcd').screenshot(path=str(OUT/'chance-setting.png'));send(dict(type='analog',idx=9,value=0))
 key(53);begin('length-chance');pg.wait_for_timeout(8000);finish('length-chance','Same 909 + Chicago, Chicago LENGTH reduced by four detents; held step10 chance reduced by ten detents. Actual LCD is retained, values require review.');key(53)
 key(50);key(40) # PARA, then FILTER 1 / verified source page 1
 pg.locator('#lcd').screenshot(path=str(OUT/'motion-before.png'))
 key(53);key(54);begin('motion-record')
 for _ in range(12):enc(4,1)
 key(54);pg.wait_for_timeout(4000);finish('motion-record','Actual Chicago K5 cutoff moves recorded with SEQ REC on, then REC off and four seconds playback.');
 begin('motion-takeover');enc(4,-16);pg.wait_for_timeout(8000);finish('motion-takeover','REC off, K5 cutoff manually lowered by16 detents; uninterrupted playback lets recorded motion return after one pattern pass.');key(53)
 # Two SONG bars reuse Pattern1; second bar mutes Chicago.
 key(49);key(49);key(5);key(5);enc(4,-1);key(13);enc(4,-1)
 pg.locator('#lcd').screenshot(path=str(OUT/'song-before.png'))
 key(53);begin('song-two-bars');pg.wait_for_timeout(10000);finish('song-two-bars','Actual two-bar SONG: Pattern1 on both bars, Chicago muted in bar2; started from selected bar1. LCD/table retained for qualification.');key(53)
 (OUT/'all-events.json').write_text(json.dumps(pg.evaluate('q.events'),indent=2))
 (OUT/'summary.json').write_text(json.dumps(receipts,indent=2))
 b.close()
