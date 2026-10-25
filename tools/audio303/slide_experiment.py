# SPDX-License-Identifier: GPL-3.0-only
"""Fit explicit host-only slide tau using apparent pitch trajectory.
Train note48->55 and evaluate note60->67 without retuning. No firmware changes.
"""
import pathlib,json,subprocess,numpy as np
from scipy.signal import resample_poly
from scipy.optimize import minimize_scalar
root=pathlib.Path(__file__).resolve().parents[3]
def read(path,stereo=False):
 x=np.fromfile(path,dtype='<f4').astype(float)
 return x.reshape(-1,2).mean(axis=1) if stereo else x
def contour(x,note):
 values=[];scale=2**((note-48)/12)
 for center in [.3,.4,.5,.6,.7]:
  y=resample_poly(x[round((center-.025)*44100):round((center+.025)*44100)],1,8);t=np.arange(len(y))*8/44100
  def loss(f):
   ph=2*np.pi*f*t[:,None]*np.arange(1,5);m=np.column_stack([np.ones(len(t)),np.cos(ph),np.sin(ph)]);c=np.linalg.lstsq(m,y,rcond=None)[0]
   return float(np.mean((y-np.sum(m*c[None,:],axis=1))**2))
  fs=np.arange(120,211,2)*scale;best=fs[np.argmin([loss(f) for f in fs])];opt=minimize_scalar(loss,bounds=(best-2*scale,best+2*scale),method='bounded');values.append(float(opt.x))
 return values
def render(note,tau,out):
 args=[44100,note,100,64,0,0,0,1,0,64,127,0,64,2,0,0,0]
 if tau is not None:args+=['--experimental-slide-tau-ms',tau]
 subprocess.run([str(root/'production303'),str(out),*map(str,args)],check=True)
def err(a,b):return float(np.sqrt(np.mean((1200*np.log2(np.array(a)/b))**2)))
ref=contour(read(root/'audio/overlap_glide_mid-r0.au.f32',True),48);candidates=[]
for tau in [80,90,100,110,120,130,140,150,160]:
 out=root/'audio'/f'experimental-slide-tau{tau}-n48.f32';render(48,tau,out);hz=contour(read(out),48);candidates.append(dict(tau_ms=tau,apparent_hz=hz,rmse_cents=err(hz,ref)))
best=min(candidates,key=lambda x:x['rmse_cents']);holdref=contour(read(root/'audio/slide-holdout-n60-r0.au.f32',True),60)
heldout=root/'audio/experimental-slide-n60.f32';render(60,best['tau_ms'],heldout);hold=contour(read(heldout),60)
baseline=root/'audio/baseline-slide-n60.f32';render(60,None,baseline);basehold=contour(read(baseline),60)
base=contour(read(root/'audio/overlap_glide_mid-r0.dsp.f32'),48)
result=dict(name='EXPERIMENTAL_HOST_ONLY_CHICAGO_SLIDE_TAU',chicago_glide=.5,source_slide_pot=64,rate=44100,centers_seconds=[.3,.4,.5,.6,.7],train_reference_hz=ref,candidates=candidates,selected=best,baseline_train_rmse_cents=err(base,ref),holdout_reference_hz=holdref,holdout_experimental_hz=hold,holdout_baseline_hz=basehold,holdout_experimental_rmse_cents=err(hold,holdref),holdout_baseline_rmse_cents=err(basehold,holdref),limitations='Apparent frequency from 50ms local harmonic fits, not internal oscillator trajectory; one glide setting and upward interval, only two octaves; no amplitude/spectral/envelope match claimed')
(root/'evidence/experimental-slide-calibration.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
