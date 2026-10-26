#!/usr/bin/env python3
"""Alias-sensitive rate-convergence probe, NOT an alias-only measurement.

Usage BASE_DIR NEW_DIR OUTPUT_JSON. Inputs MODE-noteN-rateSR.f32 generated with
qa_render at48k/192k. Differences also include rate-dependent filter response and
float phase accumulation; residual cannot be attributed entirely to aliasing.
"""
import json
import pathlib
import sys
import numpy as np
from scipy.signal import resample_poly, correlate
from scipy.optimize import minimize_scalar
from compare import db, rms, digest


def range_pitch(y,expected):
    # FFT initialization avoids the multi-minimum broad local fit at high notes.
    x=y[4800:19200]; t=np.arange(len(x))/48000
    nfft=8*len(x)
    spectrum=abs(np.fft.rfft(x*np.hanning(len(x)),n=nfft))
    frequencies=np.fft.rfftfreq(nfft,1/48000)
    mask=(frequencies>=expected*.98)&(frequencies<=expected*1.02)
    peak=frequencies[np.flatnonzero(mask)[np.argmax(spectrum[mask])]]
    def loss(f):
        z=np.exp(2j*np.pi*f*t)
        mat=np.column_stack([np.ones(len(x)),z.real,z.imag])
        v=np.linalg.lstsq(mat,x,rcond=None)[0]
        return np.mean((x-np.einsum('ij,j->i',mat,v))**2)
    fit=minimize_scalar(loss,bounds=(peak-.5,peak+.5),method='bounded',options={'xatol':1e-7})
    return float(fit.x)


def main():
    before,after,out=map(pathlib.Path,sys.argv[1:])
    result={'method':'48k engine versus192k engine antialias-decimated by scipy resample_poly(1,4); steady100–400ms; integer lag bounded to one nominal carrier period; no gain normalization.',
        'limits':'Alias-sensitive convergence, not alias-only energy: filter cutoff caps differ with sample rate, numerical phase differs, and integer lag leaves fractional delay. Note127 has different capped pitch at48k and192k and must not be interpreted as rate convergence. No alias pass/fidelity claim.',
        'rows':[]}
    for mode in ['carrier_open','mod_pulse','mix_depth','mix_extreme']:
        for note in [48,84,108,127]:
            row={'mode':mode,'note':note,'nominalCarrierHz':440*2**((note-69)/12),
                'effectiveCarrier48kHz':min(440*2**((note-69)/12),48000*.12),
                'sameCarrierFrequencyAcrossRates':bool(440*2**((note-69)/12)<=48000*.12)}
            for label,directory in [('before',before),('after',after)]:
                def load(rate):
                    p=directory/f'{mode}-note{note}-rate{rate}.f32'
                    x=np.fromfile(p,dtype='<f4').reshape(rate,2).astype(float).mean(axis=1)
                    assert np.isfinite(x).all()
                    return x
                lo=load(48000);hi=resample_poly(load(192000),1,4)
                expected=row['effectiveCarrier48kHz']/(1 if mode=='carrier_open' else 2)
                pitch_hz=range_pitch(lo,expected)
                x=hi[4800:19200]
                limit=int(np.ceil(48000/row['effectiveCarrier48kHz']))
                w=lo[4800-limit:19200+limit]
                co=correlate(w,x,mode='valid',method='fft')/np.maximum(
                    np.sqrt(np.maximum(np.convolve(w*w,np.ones(len(x)),mode='valid'),0))*np.linalg.norm(x),1e-30)
                j=int(np.argmax(co));y=w[j:j+len(x)]
                row[label]={'measuredPitchHz48k':pitch_hz,
                    'pitchFitValid48k':bool(mode in ['carrier_open','mod_pulse'] and rms(lo[4800:19200])>1e-8),
                    'pitchFitUse':'isolated-pitch diagnostic' if mode in ['carrier_open','mod_pulse'] else 'mixed-PM fit diagnostic only; no pitch acceptance',
                    'pitchCentsFromCappedExpected':float(1200*np.log2(pitch_hz/expected)),
                    'pitchCentsFromUncappedNominal':float(1200*np.log2(pitch_hz/(row['nominalCarrierHz']/(1 if mode=='carrier_open' else 2)))),
                    'raw48kSha256':digest(directory/f'{mode}-note{note}-rate48000.f32'),
                    'raw192kSha256':digest(directory/f'{mode}-note{note}-rate192000.f32'),
                    'absoluteRms48kDb':db(rms(y)),'absoluteRmsErrorVs192kDb':db(rms(y)/max(rms(x),1e-20)),
                    'lagSamples':j-limit,'correlation':float(co[j]),
                    'unalignedGainResidualDbRelative192k':db(rms(y-x)/max(rms(x),1e-20)),
                    'pitchCapActive48k':bool(row['nominalCarrierHz']>48000*.12)}
            result['rows'].append(row)
    out.write_text(json.dumps(result,indent=2)+'\n')
    print('Measured',len(result['rows']),'rate-convergence cases; no alias pass claimed')


if __name__=='__main__':main()
