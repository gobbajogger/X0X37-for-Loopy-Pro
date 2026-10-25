# SPDX-License-Identifier: GPL-3.0-only
"""Observable comparison and experimental host-only cutoff lookup; no DSP mutation.
Fit only relative harmonics on note48, test the selected pot on held-out note60.
A lookup entry is the best finite grid candidate, never a claim of equivalence.
"""
import json,pathlib,numpy as np
from scipy.optimize import minimize_scalar
root=pathlib.Path(__file__).resolve().parents[3]; sr=44100
manifest=json.loads((root/'evidence/manifest.json').read_text())
def load(path):
 x=np.fromfile(path,dtype='<f4').astype(float)
 if '.au.' in path.name:x=x.reshape(-1,2).mean(axis=1)
 assert np.isfinite(x).all()
 return x
def db(x):return 20*np.log10(np.maximum(x,1e-12))
def features(x,note=48,start=.1,end=.4):
 y=x[round(start*sr):round(end*sr)]; nominal=440*2**((note-69)/12)
 # Joint harmonic fit on an antialias-free decimation for pitch search: signals
 # already filtered, pitch fit uses only first 4 partials. Full-rate final fit.
 from scipy.signal import resample_poly
 ys=resample_poly(y,1,8);ts=np.arange(len(ys))*8/sr
 def matrix(f,t,h=4):
  a=2*np.pi*f*t[:,None]*np.arange(1,h+1)
  return np.column_stack([np.ones(len(t)),np.cos(a),np.sin(a)])
 def loss(f):
  m=matrix(f,ts);c=np.linalg.lstsq(m,ys,rcond=None)[0];return np.mean((ys-np.sum(m*c[None,:],axis=1))**2)
 opt=minimize_scalar(loss,bounds=(nominal*.99,nominal*1.01),method='bounded',options={'xatol':1e-6})
 f=float(opt.x);boundary=bool(abs(f-nominal*.99)<.001 or abs(f-nominal*1.01)<.001);t=np.arange(len(y))/sr;m=matrix(f,t,12);c=np.linalg.lstsq(m,y,rcond=None)[0];a=np.hypot(c[1:13],c[13:25]);rel=db(a/a[0])
 env=[float(np.sqrt(np.mean(x[i:i+2205]**2))) for i in range(0,len(x),2205)]
 nz=np.flatnonzero(abs(x)>max(abs(x).max()*1e-4,1e-10))
 return dict(pitch_fit_at_search_boundary=boundary,pitch_interpretation='apparent frequency from finite window; changing timbre can bias estimate',f0=f,cents=float(1200*np.log2(f/nominal)),rms=float(np.sqrt(np.mean(y*y))),peak=float(abs(x).max()),relative_harmonics_db=rel.tolist(),envelope50ms=env,onset_threshold_frame=int(nz[0]) if len(nz) else None,tail_last100ms_rms=float(np.sqrt(np.mean(x[-4410:]**2))))
metrics={}; waves={}
for t in manifest:
 for engine in ['au','dsp']:
  key=f"{t['id']}.{engine}";path=root/'audio'/f'{key}.f32';x=load(path);waves[key]=x
  metrics[key]=features(x,t['note']) if t['sequence']<2 else features(x,t['note']+7,.45,.7)
  if t['sequence']>=2:metrics[key]['window']='posttransition .45-.7s, note55 target; stationarity unproven'
for path in (root/'audio').glob('*grid*.f32'):
 note=int(path.name.split('-')[1][1:]);metrics[path.name[:-4]]=features(load(path),note)
repeats={}
for t in manifest:
 if t['id'].endswith('-r0'):
  repeats[t['name']]={e:float(np.max(abs(waves[f"{t['name']}-r0.{e}"]-waves[f"{t['name']}-r1.{e}"]))) for e in ['au','dsp']}
def distance(a,b):
 # H2..H12 at or above -40dB in reference; reject fewer than three.
 ar=np.array(a['relative_harmonics_db']);br=np.array(b['relative_harmonics_db']);mask=ar[1:]>=-40
 if np.count_nonzero(mask)<3:return 999.0
 return float(np.sqrt(np.mean((ar[1:][mask]-br[1:][mask])**2)))
cal=[]
for cut in [.2,.4,.6,.8,1.]:
 a=metrics[f'cut{cut:.1f}-n48-r0.au'];opts=[]
 for w in [0,1]:
  for p in list(range(0,128,8))+[127]:
   k=f'grid-n48-w{w}-c{p}.dsp';opts.append((distance(a,metrics[k]),w,p,k))
 score,w,p,k=min(opts);b=metrics[k];ah=metrics[f'cut{cut:.1f}-n60-r0.au'];bh=metrics[f'grid-n60-w{w}-c{p}.dsp'];baseline=metrics[f'cut{cut:.1f}-n48-r0.dsp'];baseh=metrics[f'cut{cut:.1f}-n60-r0.dsp']
 cal.append(dict(chicago_cutoff=cut,production_cutoff=p,production_wave=w,train_db=score,holdout_db=distance(ah,bh),baseline_train_db=distance(a,baseline),baseline_holdout_db=distance(ah,baseh),strong_nonfundamental_count=int(sum(np.array(a['relative_harmonics_db'])[1:]>=-40)),usable_exploratory_anchor=bool(score<=3 and distance(ah,bh)<=3 and abs(float(db(bh['rms']/ah['rms'])+db(a['rms']/b['rms'])))<=1),gain_db=float(db(a['rms']/b['rms'])),holdout_gain_error_db=float(db(bh['rms']/ah['rms'])+db(a['rms']/b['rms']))))
expanded=[]
for cut in [.2,.4,.6,.8,1.]:
 a=metrics[f'cut{cut:.1f}-n48-r0.au'];opts=[(distance(metrics[f'cut{cut:.1f}-n48-r0.au'],metrics[f'grid-n48-w0-c{p}.dsp']),0,p,f'grid-n48-w0-c{p}.dsp') for p in list(range(0,128,8))+[127]]
 for env in [0,32,64,96,127]:
  for pot in list(range(0,128,16))+[127]:
   k=f'expanded-n48-e{env}-c{pot}.dsp'
   if k not in metrics:metrics[k]=features(load(root/'audio'/f'{k}.f32'),48)
   opts.append((distance(a,metrics[k]),env,pot,k))
 score,env,pot,k=min(opts);b=metrics[k];hk=f'grid-n60-w0-c{pot}.dsp' if k.startswith('grid') else f'expanded-n60-e{env}-c{pot}.dsp';bh=features(load(root/'audio'/f'{hk}.f32'),60);metrics[hk]=bh
 ah=metrics[f'cut{cut:.1f}-n60-r0.au'];holdout=distance(ah,bh);gain=float(db(a['rms']/b['rms']));gerr=float(db(bh['rms']/ah['rms'])+gain)
 expanded.append(dict(chicago_cutoff=cut,production_cutoff=pot,production_envmod=env,production_wave=0,train_db=score,holdout_db=holdout,gain_db=gain,holdout_gain_error_db=gerr,usable_exploratory_anchor=bool(score<=3 and holdout<=3 and abs(gerr)<=1)))
# History: compare first vs second in each engine, retain level and spectra.
history={}
for name in ['repeat','accent_repeat']:
 history[name]={}
 for e in ['au','dsp']:
  x=waves[f'{name}-r0.{e}'];a=features(x[:44100]);b=features(x[44100:]);history[name][e]=dict(gain_db=float(db(b['rms']/a['rms'])),harmonic_distance_db=distance(a,b),pitch_change_cents=float(b['cents']-a['cents']))
# Preserve phase separate from gain, using a single integer lag only.
def phase_pair(x,y):
 x=x[4410:17640];y=y[4410:17640];best=None
 for lag in range(-338,339):
  a=x[max(0,lag):min(len(x),len(x)+lag)];b=y[max(0,-lag):min(len(y),len(y)-lag)]
  c=float(np.dot(a,b)/np.sqrt(np.dot(a,a)*np.dot(b,b)))
  if best is None or c>best['correlation']:best=dict(lag=lag,correlation=c,gain=float(np.dot(a,b)/np.dot(a,a)))
 return best
phase={}
for e in ['au','dsp']:
 x=waves[f'repeat-r0.{e}'];phase[e]=phase_pair(x[:44100],x[44100:])
for row in cal+expanded:
 if row['train_db']>=999:
  row['holdout_db']=None
  row['rejection_reason']='insufficient strong reference harmonics: fewer than three above -40dB'
  for field in ['production_cutoff','production_wave','production_envmod','gain_db','holdout_gain_error_db']:
   if field in row:row[field]=None
 for field in ['train_db','holdout_db','baseline_train_db','baseline_holdout_db']:
  if field in row and row[field]>=999:row[field]=None
result=dict(sample_rate=sr,metrics=metrics,fresh_repeat_max_error=repeats,calibration=cal,history=history,history_phase=phase,expanded_calibration=expanded)
(root/'evidence/measurements.json').write_text(json.dumps(result,indent=2))
(root/'evidence/experimental-cutoff-lookup.json').write_text(json.dumps(dict(name='EXPERIMENTAL_HOST_ONLY_CHICAGO_SPECTRAL_LOOKUP',scope='note48 training, note60 held out; velocity100; Chicago waveform0 Gate; production constant filter-envelope diagnostic; reso0; 44.1kHz',acceptance="Exploratory post-analysis screen: >=3 reference partials above -40dB; <=3dB spectral RMSE on train and holdout; <=1dB holdout gain error. Not a fidelity target.",entries=expanded),indent=2))
print(json.dumps(dict(repeat_max=max(v for x in repeats.values() for v in x.values()),calibration=cal,expanded=expanded,history=history,phase=phase),indent=2))
