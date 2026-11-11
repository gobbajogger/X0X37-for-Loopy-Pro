#!/usr/bin/env python3
"""Compose labelled evidence from real firmware framebuffer PNGs.

This script only crops, nearest-neighbour enlarges and labels existing frames;
it never draws a replacement control, invents motion or changes rendered pixels.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

try:
 FONT=ImageFont.truetype('DejaVuSans.ttf',14)
 SMALL=ImageFont.truetype('DejaVuSans.ttf',11)
except OSError:
 FONT=ImageFont.load_default(size=14)
 SMALL=ImageFont.load_default(size=11)
BOXES={
 'chicago-env':(123,94,153,117),'chicago-adsr':(8,174,233,224),
 'chicago-level':(64,44,96,67),'chicago-bite':(5,144,37,167),
 'miami-level':(123,44,155,67),'miami-wobble':(123,144,155,167),
 '909-decay':(123,44,155,67),'909-tune':(5,44,37,67),
 '909-snappy':(182,44,214,67),'909-tone':(64,44,96,67),
 '909-level':(8,184,52,211),'909-accent':(5,144,37,167),
 '808-decay':(182,44,214,67),'808-tune':(5,44,37,67),
 '808-snappy':(182,44,214,67),'808-tone':(64,44,96,67),
 '808-level':(8,184,52,211),'808-accent':(5,94,37,117),
 '909-attack':(64,44,96,67),'909-pitch-depth':(182,44,214,67),'909-pitch':(5,94,37,117),
 '808-attack':(123,44,155,67),'808-shell-tone':(64,44,96,67),'808-sound':(64,44,96,67),
}
ROWS=[[[0,1,-1,-1],[2,3,4,-1],[5,6,-1,-1],[7,8,9,10],[11,12,-1,-1],[13,15,14,16],[17,18,19,-1]],
      [[0,1,2,3],[6,5,7,8],[9,10,12,-1],[11,13,-1,-1],[4,15,14,-1]]]
# Exact 26x19 glyph bounds exclude labels and value readouts.
for name,box in list(BOXES.items()):
 if name=='chicago-adsr': continue
 if name.endswith('-level') and name.startswith(('909','808')):
  BOXES[name]=(10,186,51,209)
 else:
  x,y,_,_=box
  BOXES[name]=(x+2,y+2,x+28,y+21)
for kind,rows in enumerate(ROWS):
 for page,row in enumerate(rows):
  for col,control in enumerate(row):
   if control<0:continue
   x=7+59*col;y=46+(page%4)*50
   BOXES[f'{"miami" if kind else "chicago"}-control-{control:02}']=(x,y,x+26,y+19)

def label(im,xy,text,small=False):
 ImageDraw.Draw(im).text(xy,text,font=SMALL if small else FONT,fill='#e7e7e7')
def rows_at(folder):
 d=defaultdict(dict)
 with (folder/'frames.csv').open() as f:
  for row in csv.DictReader(f):d[row['series']][(int(row['value']),int(row['frame']))]=row
 return d

def open_frame(folder,row):return Image.open(folder/row['file']).convert('RGB')
def crop_frame(folder,row,series):return open_frame(folder,row).crop(BOXES[series])
def hash_image(im):return hashlib.sha256(im.tobytes()).hexdigest()
def pixel_changes(a,b):
 x,y=a.tobytes(),b.tobytes()
 return sum(x[i:i+3]!=y[i:i+3] for i in range(0,len(x),3))
CONTROL_NAMES={
 'chicago':['WAVE','GLIDE','CUTOFF','PEAK','ENV','BITE','GNAW','ATTACK','DECAY','SUSTAIN','RELEASE','ENV/GATE','LEVEL','ON','MODE','RANGE','SPEED','TYPE','EDIT1','EDIT2'],
 'miami':['WAVE','LEVEL','SUB','PITCH','GLIDE','LEVEL','WAVE','TUNE','DEPTH','CUTOFF','CRUSH','SHAPE','WOBBLE','RATE','OUTPUT','STEREO']}

def compose_original_atlases(phases,out):
 """Consolidate every static source frame, four controls by five values."""
 series=[name for name in phases[0][2] if '-control-' in name]
 for name in series:
  # These are this script's redundant generated files, never source captures.
  for suffix in ['-frames-01.png','-values.png']:
   redundant=out/(name+suffix)
   if redundant.exists(): redundant.unlink()
 for start in range(0,len(series),4):
  roles=series[start:start+4];cw=132;ch=102
  sheet=Image.new('RGB',(170+5*cw,38+len(roles)*len(phases)*ch),'#111719')
  label(sheet,(8,8),'CHICAGO / MIAMI | all original-control glyphs | real native pixels, nearest-neighbour4x')
  for ri,name in enumerate(roles):
   kind,_,control=name.partition('-control-');control=int(control)
   title=f'{kind.upper()}\n{CONTROL_NAMES[kind][control]} (id{control})'
   for pi,(phase,folder,table) in enumerate(phases):
    y=38+(ri*len(phases)+pi)*ch;label(sheet,(8,y+14),title+'\n'+phase,small=True)
    for vi,value in enumerate(sorted({k[0] for k in table[name]})):
     x=170+vi*cw;label(sheet,(x,y),f'{value}%',small=True)
     native=crop_frame(folder,table[name][value,0],name)
     sheet.paste(native.resize((native.width*4,native.height*4),Image.Resampling.NEAREST),(x,y+21))
  sheet.save(out/f'original-controls-atlas-{start//4+1:02}.png',optimize=True)

def compose(before:Path,after:Path|None,out:Path):
 out.mkdir(parents=True,exist_ok=True); bs=rows_at(before);ats=rows_at(after) if after else bs
 records=[]; phases=[('BEFORE',before,bs)]+([('AFTER',after,ats)] if after else [])
 for series,data in bs.items():
  values=sorted({k[0] for k in data});frames=sorted({k[1] for k in data}); animated=len(frames)>1
  bx=BOXES[series];scale=2 if series=='chicago-adsr' else 4
  cw=(bx[2]-bx[0])*scale;ch=(bx[3]-bx[1])*scale
  for phase,folder,table in phases:
   for value in values:
    images=[crop_frame(folder,table[series][value,f],series) for f in frames]
    hashes=[hash_image(x) for x in images]
    changes=[0]+[pixel_changes(images[i-1],images[i]) for i in range(1,len(images))]
    records.append({'series':series,'phase':phase,'value_percent':value,'frame_count':len(frames),'unique_glyph_frames':len(set(hashes)),
                    'changed_pixels_per_step':changes,'glyph_sha256':hashes,'crop':bx})
  if not animated: continue # Static frames are consolidated below.
  # Every source frame gets a labelled nearest-neighbour crop in a sheet.
  per_sheet=3 if series=='chicago-adsr' else 9
  for start in range(0,len(frames),per_sheet):
   fs=frames[start:start+per_sheet];w=100+len(fs)*(cw+10);h=38+len(phases)*len(values)*(ch+25)
   sheet=Image.new('RGB',(w,h),'#111719');label(sheet,(8,8),f'{series} | real firmware framebuffer | every frame {fs[0]}..{fs[-1]}')
   for pi,(phase,folder,table) in enumerate(phases):
    for vi,value in enumerate(values):
     y=38+(pi*len(values)+vi)*(ch+25);label(sheet,(8,y+20),f'{phase}\n{value}%',small=True)
     for fi,f in enumerate(fs):
      row=table[series][value,f];x=100+fi*(cw+10);label(sheet,(x,y),f'f{f:02} {row["time_ms"]} ms',small=True)
      sheet.paste(crop_frame(folder,row,series).resize((cw,ch),Image.Resampling.NEAREST),(x,y+19))
   sheet.save(out/f'{series}-frames-{start//per_sheet+1:02}.png',optimize=True)
  if animated:
   # One GIF shows all representative values and both complete screens.
   gif_frames=[]
   for f in frames:
    canvas=Image.new('RGB',(len(values)*244,38+len(phases)*266),'#111719');label(canvas,(8,8),f'{series} | real framebuffer | t={data[values[0],f]["time_ms"]} ms')
    for pi,(phase,folder,table) in enumerate(phases):
     for vi,value in enumerate(values):
      x=vi*244;y=38+pi*266;label(canvas,(x+3,y),f'{phase}  {value}%',small=True);canvas.paste(open_frame(folder,table[series][value,f]),(x,y+20))
    gif_frames.append(canvas)
   times=[int(data[values[0],f]['time_ms']) for f in frames]
   durations=[160]+[max(20,times[i+1]-times[i]) for i in range(1,len(times)-1)]+[300]
   gif_frames[0].save(out/f'{series}-before-after.gif',save_all=True,append_images=gif_frames[1:],duration=durations,loop=0,optimize=False)
  # A compact value sheet uses actual full screens, static value comparison.
  selected=frames[6] if animated else 0
  sheet=Image.new('RGB',(len(values)*244,38+len(phases)*266),'#111719');label(sheet,(8,8),f'{series} | representative values | frame {selected}')
  for pi,(phase,folder,table) in enumerate(phases):
   for vi,value in enumerate(values):
    x=vi*244;y=38+pi*266;label(sheet,(x+3,y),f'{phase} {value}%',small=True);sheet.paste(open_frame(folder,table[series][value,selected]),(x,y+20))
  sheet.save(out/f'{series}-values.png',optimize=True)
 compose_original_atlases(phases,out)
 # Concise geometry check: a labelled 1-bit mask of actual native pixels.
 # Rec.709 luma threshold128/255 discards dim ghosts and keeps bright glyph ink.
 mono_roles=['chicago-env','chicago-adsr','chicago-level','miami-wobble',
             '909-decay','909-tune','909-snappy','909-tone','909-level','909-accent',
             '808-decay','808-tune','808-snappy','808-tone','808-level','808-accent',
             '808-shell-tone','808-sound']
 for start in range(0,len(mono_roles),6):
  roles=mono_roles[start:start+6];rh=104;cw=228
  sheet=Image.new('RGB',(150+5*cw,58+len(roles)*len(phases)*rh),'#111719')
  label(sheet,(8,8),'1-BIT READABILITY MASK | actual framebuffer pixels | Rec.709 luma >=128/255')
  label(sheet,(8,29),'Sample f06=90ms after hit. No smoothing; dim ghosts fall below threshold.',small=True)
  for ri,series in enumerate(roles):
   for pi,(phase,folder,table) in enumerate(phases):
    y=58+(ri*len(phases)+pi)*rh;label(sheet,(7,y+24),series+'\n'+phase,small=True)
    values=sorted({k[0] for k in table[series]})
    for vi,value in enumerate(values):
     frame=6 if (value,6) in table[series] else 0
     native=crop_frame(folder,table[series][value,frame],series)
     mask=Image.new('1',native.size)
     rgb=native.tobytes()
     mask.putdata([int(.2126*rgb[i]+.7152*rgb[i+1]+.0722*rgb[i+2]>=128) for i in range(0,len(rgb),3)])
     scale=1 if series=='chicago-adsr' else 4
     mask=mask.resize((native.width*scale,native.height*scale),Image.Resampling.NEAREST).convert('RGB')
     x=150+vi*cw;label(sheet,(x,y),str(value)+'%',small=True)
     sheet.paste(mask,(x+(cw-mask.width)//2,y+21))
  sheet.save(out/f'mono-role-atlas-{start//6+1:02}.png',optimize=True)
 (out/'motion-measurements.json').write_text(json.dumps(records,indent=2)+'\n')
 # Compare sequences so stationary ENV=0 is allowed while depth extremes stay distinct.
 differences=[]
 for phase,folder,table in phases:
  for series,data in table.items():
   values=sorted({k[0] for k in data});frames=sorted({k[1] for k in data})
   for low,high in zip(values,values[1:]):
    counts=[]
    for f in frames:
     a=crop_frame(folder,data[low,f],series).tobytes();b=crop_frame(folder,data[high,f],series).tobytes()
     counts.append(sum(a[i:i+3]!=b[i:i+3] for i in range(0,len(a),3)))
    differences.append({'phase':phase,'series':series,'values':[low,high],'changed_pixels_by_frame':counts,'max_changed_pixels':max(counts)})
 (out/'value-differences.json').write_text(json.dumps(differences,indent=2)+'\n')
 if after:
  results=[]
  for rec in records:
   if rec['phase']!='AFTER' or rec['frame_count']==1: continue
   # Zero ENV is stationary by contract; zero noise/level have no hit to show.
   zero_env=rec['series']=='chicago-env' and rec['value_percent']==50
   silent_zero=rec['value_percent']==0 and rec['series'].endswith(('-snappy','-level'))
   ok=rec['unique_glyph_frames']==1 if zero_env else (True if silent_zero else rec['unique_glyph_frames']>1)
   results.append({'series':rec['series'],'value_percent':rec['value_percent'],'check':'stationary zero ENV' if zero_env else 'live motion','pass':ok})
  for diff in differences:
   if diff['phase']=='AFTER' and diff['series']=='chicago-env':
    results.append({'series':diff['series'],'values':diff['values'],'check':'different depth sequences','pass':diff['max_changed_pixels']>0})
  (out/'render-validation.json').write_text(json.dumps(results,indent=2)+'\n')
  failures=[r for r in results if not r['pass']]
  if failures: raise SystemExit('Render checks FAILED: '+json.dumps(failures))
 print(f'Composed {len(bs)} series, {sum(len(x) for x in bs.values())} source frames per phase, {len(records)} measurements')

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();compose(a.before,a.after,a.output)
