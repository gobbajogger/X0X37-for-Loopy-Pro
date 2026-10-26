#!/usr/bin/env python3
"""Reproduce declared development fit using an immutable baseline checkout.
Usage: fit_refinement.py BASELINE_REPO PRIVATE_REFERENCE_DIR PRIVATE_WORK_DIR
Outputs generated exploratory C/library and numeric training.json only under WORK.
H1/H2/H3 and samples4800:9600 only; never evaluates holdouts.
"""
import ctypes as ct, numpy as np, subprocess, pathlib, json, sys, hashlib
from scipy.io import wavfile
from scipy.optimize import least_squares
root,refs,work=map(lambda x:pathlib.Path(x).resolve(),sys.argv[1:]);work.mkdir(parents=True,exist_ok=True)
base=root/'experiments/wobble/wobble.c'
assert hashlib.sha256(base.read_bytes()).hexdigest()=='19dac9935a9524407418bb8bd7cc662d7acf36ff2fad31d1f85e5e69d3529e43'
src=base.read_text()
src=src.replace('#include "wobble.h"',f'#include "{root}/experiments/wobble/wobble.h"').replace('#include "blep.h"',f'#include "{root}/experiments/wobble/blep.h"').replace('#include "../../firmware/src/dsp/fastmath.h"',f'#include "{root}/firmware/src/dsp/fastmath.h"')
src=src.replace('static float wrap(', 'float train_gain=.078f, train_cut=1, train_duty=.1f, train_level=1, train_family=0;\nstatic float wrap(')
src=src.replace('float mod = waveform(s->mod_phase, mod_step, s->p[W_XMOD_WAVE] >= .5f);','''float mod = waveform(s->mod_phase, mod_step, 0);
if(s->p[W_XMOD_WAVE]>=.5f) {
float p=s->mod_phase,d=train_duty;
if(train_family==0) mod=(p<d?2*(1-d):-2*d)+wobble_blep(p,mod_step)-wobble_blep(wrap(p+1-d),mod_step);
else mod=p<d ? 1-2*p/d : -1+2*(p-d)/(1-d);
mod*=train_level;
}''')
src=src.replace('float mixed = .1f *','float mixed = train_gain *').replace('40.0f * fm_exp2f(cutoff_control * 8.7f)','train_cut * 40.0f * fm_exp2f(cutoff_control * 8.7f)')
(work/'train.c').write_text(src); subprocess.run(['clang','-std=c99','-O2','-ffp-contract=off','-dynamiclib',str(work/'train.c'),'-o',str(work/'train.dylib')],check=True)
lib=ct.CDLL(str(work/'train.dylib')); lib.wobble_init.argtypes=[ct.c_void_p,ct.c_float];lib.wobble_set.argtypes=[ct.c_void_p,ct.c_uint,ct.c_float]
f0=440*2**(-21/12)
def coeff(y,f):
 t=np.arange(4800)/48000;z=np.exp(2j*np.pi*f*t[:,None]*np.arange(1,4));a=np.linalg.lstsq(np.column_stack([np.ones(4800),z.real,z.imag]),y,rcond=None)[0];return abs(a[1:4]-1j*a[4:7])
def render(name):
 s=ct.create_string_buffer(144);lib.wobble_init(s,48000);lib.wobble_set(s,9,.6 if name=='carrier_filter' else 1)
 if name=='mod_pulse':lib.wobble_set(s,1,0);lib.wobble_set(s,5,.7);lib.wobble_set(s,6,1)
 l=np.zeros(9600,dtype='float32');r=l.copy();lib.wobble_note_on(s,48,80)
 for start in range(0,9600,240):
  lib.wobble_render(s,l[start:].ctypes.data_as(ct.POINTER(ct.c_float)),r[start:].ctypes.data_as(ct.POINTER(ct.c_float)),240)
 return coeff((l[4800:]+r[4800:])*.5,f0/(2 if name=='mod_pulse' else 1))
def setp(**kw):
 for k,v in kw.items():ct.c_float.in_dll(lib,'train_'+k).value=v
ref={}
for name in ['carrier_open','carrier_filter','mod_pulse']:
 sr,y=wavfile.read(refs/f'Miami-{name}_r0-0.wav');ref[name]=coeff(y.mean(axis=1)[4800:9600],f0/(2 if name=='mod_pulse' else 1))
def loss_common(p):
 setp(gain=p[0],cut=p[1]);v=[]
 for name in ['carrier_open','carrier_filter']:
  c=render(name); v.extend(20*np.log10(c[[0,2]]/ref[name][[0,2]]))
 return v
res=least_squares(loss_common,[.078,.35],bounds=([.05,.1],[.1,1.5]),diff_step=.005,xtol=1e-7)
setp(gain=res.x[0],cut=res.x[1]); print('COMMON',res.x,res.fun)
rows=[]
for family,duty in [(0,.5),(0,.09),(1,.1)]:
 setp(family=family)
 def loss(p):
  setp(duty=p[0],level=p[1]);return 20*np.log10(np.maximum(render('mod_pulse'),1e-15)/ref['mod_pulse'])
 bounds=([.49999,.1],[.50001,2]) if duty==.5 else ([.015,.1],[.49,2])
 res2=least_squares(loss,[duty,.7],bounds=bounds,diff_step=.005,xtol=1e-7)
 row=dict(family='symmetric pulse' if duty==.5 else ['asymmetric pulse','asymmetric triangle'][family],duty=float(res2.x[0]),level=float(res2.x[1]),trainingH123Db=res2.fun.tolist(),rmsLossDb=float(np.sqrt(np.mean(res2.fun**2))))
 rows.append(row);print(row)
(work/'training.json').write_text(json.dumps({'gain':float(res.x[0]),'cutoffScale':float(res.x[1]),'commonH1H3ErrorsDb':res.fun.tolist(),'families':rows},indent=2)+'\n')

# Select small rounded constants, retaining the unfitted general model.
setp(gain=.07725,cut=.3,family=0,duty=.1,level=1)
print('ROUNDED pulse H1/H2/H3 error dB',20*np.log10(render('mod_pulse')/ref['mod_pulse']))
