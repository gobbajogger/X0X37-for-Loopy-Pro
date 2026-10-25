# SPDX-License-Identifier: GPL-3.0-only
"""Private scientific comparison figures from recorded metrics."""
import json,pathlib,numpy as np,matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
root=pathlib.Path(__file__).resolve().parents[3]
s=json.loads((root/'evidence/experimental-slide-calibration.json').read_text());m=json.loads((root/'evidence/measurements.json').read_text())['metrics']
fig,axs=plt.subplots(1,2,figsize=(11,4.3),layout='constrained')
a=axs[0];t=s['centers_seconds'];a.plot(t,s['holdout_reference_hz'],'o-',label='Chicago .5 glide');a.plot(t,s['holdout_baseline_hz'],'s--',label='Production slide64');a.plot(t,s['holdout_experimental_hz'],'^-',label='Experimental tau120ms');a.set(xlabel='Time from first note-on (s)',ylabel='Apparent frequency (Hz)',title='Held-out slide: MIDI60 →67 at0.25s');a.legend(fontsize=8);a.grid(alpha=.2)
a=axs[1];h=np.arange(1,13)
for key,label in [('cut0.6-n60-r0.au','Chicago cutoff.6'),('cut0.6-n60-r0.dsp','Production cutoff64'),('grid-n60-w0-c104.dsp','Experimental cutoff104')]:a.plot(h,m[key]['relative_harmonics_db'],'o-',label=label)
a.set(xlabel='Harmonic number',ylabel='Magnitude relative to fundamental (dB)',title='Held-out spectrum: MIDI60, 0.1–0.4s',ylim=(-65,3));a.legend(fontsize=8);a.grid(alpha=.2)
fig.suptitle('Host-only Chicago correspondence experiments ·44.1kHz\nSeparate pitch and spectral metrics; no claim of full equivalence',fontsize=12)
fig.savefig(root/'evidence/comparison.png',dpi=170)
