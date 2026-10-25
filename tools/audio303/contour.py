# SPDX-License-Identifier: GPL-3.0-only
"""Bounded apparent pitch trajectories, using local harmonic fits (not internal pitch)."""
import pathlib,json,numpy as np
from scipy.optimize import minimize_scalar
from scipy.signal import resample_poly
root=pathlib.Path(__file__).resolve().parents[3]
out={}
for name in ['overlap_glide0','overlap_glide_mid','overlap_retrigger']:
 out[name]={}
 for eng in ['au','dsp']:
  x=np.fromfile(root/'audio'/f'{name}-r0.{eng}.f32',dtype='<f4').astype(float)
  if eng=='au':x=x.reshape(-1,2).mean(axis=1)
  estimates=[]
  for center in [.3,.4,.5,.6,.7]:
   y=resample_poly(x[round((center-.025)*44100):round((center+.025)*44100)],1,8);t=np.arange(len(y))*8/44100
   def loss(f):
    ph=2*np.pi*f*t[:,None]*np.arange(1,5);m=np.column_stack([np.ones(len(t)),np.cos(ph),np.sin(ph)]);c=np.linalg.lstsq(m,y,rcond=None)[0]
    return float(np.mean((y-np.sum(m*c[None,:],axis=1))**2))
   fs=np.arange(120,211,2);best=fs[np.argmin([loss(f) for f in fs])];opt=minimize_scalar(loss,bounds=(best-2,best+2),method='bounded')
   estimates.append(dict(center_seconds=center,apparent_hz=float(opt.x),normalized_fit_residual=loss(opt.x)/float(np.mean(y*y))))
  out[name][eng]=estimates
(root/'evidence/apparent-pitch-contours.json').write_text(json.dumps(out,indent=2))
print(json.dumps(out,indent=2))
