#!/usr/bin/env python3
"""Regression checks specific to frozen holdout windows/partial masks."""
import numpy as np
from compare import SR
from evaluate_refinement import compare_window
from evaluate_rate_convergence import range_pitch

f=130.81278265
t=np.arange(SR)/SR
base=np.sin(2*np.pi*f*t)
rich=base+.2*np.sin(2*np.pi*5*f*t)
stereo=lambda x:np.column_stack([x,x])
a,b=stereo(rich),stereo(base)
assert compare_window(a,b,f)['heldH4To16UnionStrongMaxDb']>20
assert compare_window(b,a,f)['heldH4To16UnionStrongMaxDb']>20
r=compare_window(a,a*2,f)
assert abs(r['absoluteRmsErrorDb']-6.020599913)<1e-6
assert r['heldH4To16UnionStrongMaxDb']<1e-5
altered=a.copy();altered[:9600]*=2
assert compare_window(a,altered,f)['absoluteRmsErrorDb']==0
assert abs(compare_window(a,a,f)['constantLagSamples'])<=1
assert not compare_window(a,np.zeros_like(a),f)['fundamentalsValid']
assert not compare_window(np.zeros_like(a),a,f)['fundamentalsValid']
print('PASS holdout missing/added partials, gain separation, training-window exclusion, lag bound')

for hz in [4186.009044809578,5760.0]:
    y=np.sin(2*np.pi*hz*t)+.1*np.sin(4*np.pi*hz*t)
    assert abs(range_pitch(y,hz)-hz)<.002
print("PASS high-note pitch initialization avoids local harmonic-fit minima")
