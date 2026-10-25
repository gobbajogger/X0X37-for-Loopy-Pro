#!/usr/bin/env python3
"""Unfitted evaluation. Usage: REF_DIR BASE_RENDERS NEW_RENDERS OUTPUT_JSON.

Time windows and partial splits are frozen in REFINEMENT-PLAN.md. Audio stays
outside the source tree. Missing partials use a union mask, never intersection.
"""
import json
import pathlib
import sys
import numpy as np
from scipy.signal import correlate
from compare import CONFIGS, SR, db, digest, load_wav, measure, rms


def window_metrics(x, f, start=.2, stop=.4):
    y=x.mean(axis=1)[int(start*SR):int(stop*SR)]
    t=np.arange(len(y))/SR
    z=np.exp(2j*np.pi*f*t[:,None]*np.arange(1,17))
    mat=np.column_stack([np.ones(len(y)), z.real,z.imag])
    v=np.linalg.lstsq(mat,y,rcond=None)[0]
    c=v[1:17]-1j*v[17:33]
    return y,c


def compare_window(ref, out, f, start=.2, stop=.4):
    x,a=window_metrics(ref,f,start,stop); y,b=window_metrics(out,f,start,stop)
    absolute=20*np.log10(np.maximum(abs(b),1e-20)/np.maximum(abs(a),1e-20))
    relative=absolute-absolute[0]
    strong=(abs(a)>abs(a[0])*.01)|(abs(b)>abs(b[0])*.01)
    held=strong.copy();held[:3]=False
    limit=int(np.ceil(SR/f)); begin=int(start*SR);end=int(stop*SR)
    wide=out.mean(axis=1)[begin-limit:end+limit]
    co=correlate(wide,x,mode='valid',method='fft')/np.maximum(
        np.sqrt(np.maximum(np.convolve(wide**2,np.ones(len(x)),mode='valid'),0))*np.linalg.norm(x),1e-30)
    j=int(np.argmax(co)); shifted=wide[j:j+len(x)]
    gain=float(np.dot(x,shifted)/max(np.dot(x,x),1e-30))
    valid=bool(rms(x)>1e-8 and rms(y)>1e-8 and abs(a[0])>rms(x)*.01 and abs(b[0])>rms(y)*.01)
    return {'fundamentalsValid':valid,'windowSeconds':[start,stop], 'absoluteRmsErrorDb':db(rms(y)/max(rms(x),1e-20)),
        'absoluteHarmonicErrorDb':absolute.tolist(),'relativeHarmonicErrorDb':relative.tolist(),
        'unionStrongHarmonics':(np.flatnonzero(strong)+1).tolist(),
        'heldH4To16UnionStrongMaxDb':float(max(abs(relative[held]))) if held.any() else None,
        'allUnionStrongMaxDb':float(max(abs(relative[strong]))) if strong.any() else None,
        'constantLagSamples':j-limit,'constantLagBoundSamples':limit,
        'constantLagCorrelation':float(co[j]),'fittedGainDiagnosticDb':db(gain),
        'gainAlignedResidualRmsRatio':rms(shifted-gain*x)/max(rms(shifted),1e-20)}


def raw(path, notes=1):
    x=np.fromfile(path,dtype='<f4').reshape(notes,SR,2).astype(float)
    assert np.isfinite(x).all()
    return x


def validate_manifest(refs, name, group, note, cfg):
    manifest=refs/name
    doc=json.loads(manifest.read_text())
    assert doc['complete'] and doc['sampleRate']==SR and doc['channels']==2
    assert doc['componentVersion']==196870
    expected=[0,.7,0,.5,0,0,0,.5,0,1,0,0,0,0,.5,0,.5,0]
    if cfg=='carrier_filter':expected[9]=.6
    if cfg=='square_open':expected[0]=1
    if cfg=='sub_open':expected[1]=0;expected[2]=.7
    if cfg in ['mod_triangle','mod_pulse']:expected[1]=0;expected[5]=.7
    if cfg=='mod_pulse':expected[6]=1
    if cfg=='mix_depth':expected[5]=.7;expected[8]=.15
    for suffix in ['r0','r1']:
        for repeat in [0,1]:
            trial=next(t for t in doc['trials'] if t['group']==group+'_'+suffix and t['repeat']==repeat)
            current_note=note if repeat==0 else 48
            assert trial['fresh']==(repeat==0) and trial['frames']==48000 and trial['leadFrames']==0
            assert trial['blockFrames']==256 and trial['startSampleTimestamp']==repeat*48000
            prefix='Miami-follow-' if name=='Miami-followup.json' else 'Miami-'
            assert trial['file']==f'{prefix}{group}_{suffix}-{repeat}.wav'
            assert [(e['relativeFrame'],e['status'],e['note'],e['velocity']) for e in trial['events']]==[(0,144,current_note,80),(24000,128,current_note,0)]
            params={p['id']:p for p in trial['parameters']}
            assert set(params)==set(range(18))
            assert np.allclose([params[i]['readback'] for i in range(18)],expected,rtol=0,atol=1e-7)
            assert np.allclose([params[i]['requested'] for i in range(18)],expected,rtol=0,atol=1e-7)
    return digest(manifest)


def main():
    refs,base,new,out=map(pathlib.Path,sys.argv[1:])
    result={'method':'Frozen unfitted200–400ms, fixed common nominal frequency, union strong partial mask; phase lag bounded to one period; absolute gain kept separate.',
        'noteCoverage':'MIDI48 all7 fresh; MIDI48 history4; MIDI55 carrier and mix first followed by MIDI48 second. No unfitted isolated pulse note reference exists.',
        'referenceManifestSha256':{n:digest(refs/n) for n in ['Miami-matrix.json','Miami-followup.json']},
        'configurations':{}}
    for cfg in CONFIGS:
        f=440*2**((48-69)/12)/(2 if cfg in ['sub_open','mod_triangle','mod_pulse','mix_depth'] else 1)
        validate_manifest(refs,'Miami-matrix.json',cfg,48,cfg)
        ref=load_wav(refs/f'Miami-{cfg}_r0-0.wav')
        assert np.array_equal(ref,load_wav(refs/f'Miami-{cfg}_r1-0.wav'))
        row={'referenceSha256':digest(refs/f'Miami-{cfg}_r0-0.wav')}
        for label,directory in [('before',base),('after',new)]:
            p=directory/(cfg+'.f32'); x=raw(p)[0]
            a,b=measure(ref,f),measure(x,f)
            m=compare_window(ref,x,f)
            m.update({'pitchValid':a['pitchFitValid'] and b['pitchFitValid'],
                'pitchWindowSeconds':[.1,.4],
                'pitchErrorCents':float(1200*np.log2(b['pitchFitHz']/a['pitchFitHz'])),
                'releaseFrames':b['releaseLastAboveThresholdFrames'],
                'referenceReleaseFrames':a['releaseLastAboveThresholdFrames'],
                'rawSha256':digest(p)})
            row[label]=m
        result['configurations'][cfg]=row
    history={}
    for cfg in ['carrier_open','square_open','sub_open','mix_depth']:
        f=440*2**((48-69)/12)/(2 if cfg in ['sub_open','mix_depth'] else 1)
        rr=[load_wav(refs/f'Miami-{cfg}_r0-{i}.wav') for i in range(2)]
        row={}
        for label,directory in [('before',base),('after',new)]:
            xx=raw(directory/(cfg+'-history.f32'),2)
            refchange=compare_window(rr[0],rr[1],f)
            protochange=compare_window(xx[0],xx[1],f)
            row[label]={'secondNoteAgainstReference':compare_window(rr[1],xx[1],f),
                'referenceH3H5ChangeDb':[refchange['relativeHarmonicErrorDb'][i] for i in [2,4]],
                'prototypeH3H5ChangeDb':[protochange['relativeHarmonicErrorDb'][i] for i in [2,4]]}
        history[cfg]=row
    result['history']=history
    note55={}
    for cfg,refname in [('carrier_open','Miami-open_pitch'),('mix_depth','Miami-follow-mix_pitch')]:
        validate_manifest(refs,'Miami-matrix.json' if cfg=='carrier_open' else 'Miami-followup.json','open_pitch' if cfg=='carrier_open' else 'mix_pitch',55,cfg)
        rr=[load_wav(refs/f'{refname}_r0-{i}.wav') for i in range(2)]
        repeats=[load_wav(refs/f'{refname}_r1-{i}.wav') for i in range(2)]
        assert all(np.array_equal(a,b) for a,b in zip(rr,repeats))
        f=440*2**((55-69)/12)/(2 if cfg=='mix_depth' else 1)
        row={'referenceRepeatExact':True,'referenceSha256':[digest(refs/f'{refname}_r0-{i}.wav') for i in range(2)]}
        for label,directory in [('before',base),('after',new)]:
            xx=raw(directory/(cfg+'-note55.f32'),2)
            row[label]=[compare_window(rr[i],xx[i],f if i==0 else 440*2**((48-69)/12)/(2 if cfg=='mix_depth' else 1)) for i in range(2)]
        note55[cfg]=row
    result['note55Holdout']=note55
    out.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:{stage:{m:row[stage][m] for m in ['absoluteRmsErrorDb','heldH4To16UnionStrongMaxDb','constantLagCorrelation','releaseFrames']} for stage in ['before','after']} for k,row in result['configurations'].items()},indent=2))


if __name__=='__main__':main()
