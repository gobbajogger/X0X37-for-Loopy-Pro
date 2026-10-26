#!/usr/bin/env python3
"""Private reference comparison. Never copies reference audio into the source tree.

Usage: python3 compare.py REFERENCE_DIRECTORY RENDER_DIRECTORY OUTPUT_JSON
Requires numpy/scipy; renderer produces CONFIG.f32, stereo float32 at 48kHz.

Miami AU 3.1.6 matrix params only — owner W_ATTACK/W_DECAY/W_SUSTAIN/W_RELEASE
are not in the reference manifest; render.c leaves them at AU-like defaults.
"""
import hashlib
import json
import pathlib
import sys
import warnings

_REPO = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(_REPO / "tools"))
import au_compare_battery  # noqa: E402

import numpy as np
from scipy.io import wavfile
from scipy.optimize import minimize_scalar
from scipy.signal import correlate

CONFIGS = ['carrier_filter', 'carrier_open', 'square_open', 'sub_open',
           'mod_triangle', 'mod_pulse', 'mix_depth']
SR = 48000


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rms(x):
    return float(np.sqrt(np.mean(x*x)))


def db(x):
    return float(20*np.log10(max(abs(x), 1e-20)))


def _active_frames():
    return au_compare_battery.active_timing().frames


def _steady_window():
    t = au_compare_battery.active_timing()
    return t.steady_start, t.steady_end


def load_wav(path, frames=None):
    if frames is None:
        frames = _active_frames()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', wavfile.WavFileWarning)
        sr, x = wavfile.read(path)
    assert sr == SR and x.ndim == 2 and x.shape[1] == 2 and x.shape[0] >= frames
    assert np.isfinite(x).all()
    assert np.issubdtype(x.dtype, np.floating)
    return x[:frames].astype(float)


def measure(x, expected, steady_start=4800, steady_end=19200):
    assert x.ndim == 2 and x.shape[1] == 2 and x.shape[0] >= steady_end and np.isfinite(x).all()
    y = x.mean(axis=1)
    a = y[steady_start:steady_end]
    tm = np.arange(len(a))/SR

    def fit(f, count):
        z = np.exp(2j*np.pi*f*tm[:, None]*np.arange(1, count+1))
        mat = np.column_stack([np.ones(len(a)), z.real, z.imag])
        v = np.linalg.lstsq(mat, a, rcond=None)[0]
        return v, mat

    def loss(f):
        v, mat = fit(f, 8)
        # einsum avoids BLAS floating-status warnings on this host; finite
        # residuals are checked rather than suppressing numerical warnings.
        residual = a-np.einsum('ij,j->i', mat, v)
        assert np.isfinite(residual).all()
        return np.mean(residual**2)

    f = minimize_scalar(loss, bounds=(expected*.98, expected*1.02),
                        method='bounded', options={'xatol': 1e-7}).x
    v, _ = fit(expected, 16)
    c = v[1:17]-1j*v[17:33]
    mid = rms(a)
    crossings = np.flatnonzero(abs(y) > 1e-7)
    tail_start = min(24000, x.shape[0])
    tail = np.flatnonzero(np.max(abs(x[tail_start:]), axis=1) > 1e-7)
    valid_pitch = mid > 1e-8 and abs(c[0]) > mid*.01 and abs(f/expected-1) < .019
    return {'pitchFitValid': bool(valid_pitch), 'pitchFitHz': float(f), 'pitchCentsFromNominal': float(1200*np.log2(f/expected)),
            'nominalHz': expected, 'noteRms': rms(y[:min(24000, len(y))]), 'steadyRms': mid,
            'channelRms': [rms(x[:min(24000, x.shape[0]), i]) for i in range(2)],
            'sideMidRatio': rms((x[steady_start:steady_end, 0]-x[steady_start:steady_end, 1])*.5)/max(mid, 1e-20),
            'peak': float(abs(x).max()), 'dc': float(np.mean(a)),
            'thresholdOnsetSample': int(crossings[0]) if len(crossings) else None,
            'releaseLastAboveThresholdFrames': int(tail[-1]+1) if len(tail) else 0,
            'last100msPeak': float(abs(x[-4800:]).max()),
            'envelope50msRms': [rms(y[i:i+2400]) for i in range(0, x.shape[0], 2400)],
            'fixedHarmonicAbs': abs(c).tolist(), 'fixedHarmonicPhase': np.angle(c).tolist()}


def compare(ref, proto, a, b, steady_start=4800, steady_end=19200):
    x, y = ref.mean(axis=1), proto.mean(axis=1)
    xx = x[steady_start:steady_end]
    limit = int(np.ceil(SR/a['nominalHz']))
    window = y[steady_start-limit:steady_end+limit]
    cs = correlate(window, xx, mode='valid', method='fft')
    en = np.sqrt(np.maximum(np.convolve(window**2, np.ones(len(xx)), mode='valid'), 0))
    co = cs/np.maximum(en*np.linalg.norm(xx), 1e-30)
    j = int(np.argmax(co))
    lag = j-limit
    yy = y[steady_start+lag:steady_end+lag]
    gain = float(np.dot(xx, yy)/max(np.dot(xx, xx), 1e-30))
    ha, hb = np.array(a['fixedHarmonicAbs']), np.array(b['fixedHarmonicAbs'])
    # Report every partial, but phase summaries exclude weak partials in either signal.
    reference_strong = ha > ha[0]*.01
    prototype_strong = hb > hb[0]*.01
    mask = reference_strong & prototype_strong
    spectrum_mask = reference_strong | prototype_strong
    absolute = 20*np.log10(np.maximum(hb, 1e-20)/np.maximum(ha, 1e-20))
    relative = absolute-absolute[0]
    phase = np.array(b['fixedHarmonicPhase'])-np.array(a['fixedHarmonicPhase'])
    phase = np.angle(np.exp(1j*(phase-np.arange(1, 17)*phase[0])))
    env = 20*np.log10(np.maximum(b['envelope50msRms'], 1e-20)/np.maximum(a['envelope50msRms'], 1e-20))
    return {'pitchErrorCents': float(1200*np.log2(b['pitchFitHz']/a['pitchFitHz'])),
            'noteRmsErrorDb': db(b['noteRms']/a['noteRms']),
            'steadyEnvelopeErrorDb': env[2:8].tolist(), 'envelopeErrorDb': env.tolist(),
            'absoluteHarmonicErrorDb': absolute.tolist(),
            'relativeHarmonicErrorDb': relative.tolist(),
            'commonStrongHarmonics': (np.flatnonzero(mask)+1).tolist(),
            'referenceStrongHarmonics': (np.flatnonzero(reference_strong)+1).tolist(),
            'unexpectedPrototypeStrongHarmonics': (np.flatnonzero(prototype_strong & ~reference_strong)+1).tolist(),
            'strongRelativeHarmonicMaxDb': float(max(abs(relative[spectrum_mask]))) if spectrum_mask.any() else None,
            'constantLagSamples': lag, 'constantLagCorrelation': float(co[j]),
            'fittedGainDb': db(gain),
            'gainAlignedResidualRmsRatio': rms(yy-gain*xx)/max(rms(yy), 1e-20),
            'strongHarmonicPhaseResidualRad': phase[mask].tolist(),
            'pitchGoal2Cents': bool(a['pitchFitValid'] and b['pitchFitValid'] and abs(1200*np.log2(b['pitchFitHz']/a['pitchFitHz'])) <= 2),
            'steadyEnvelopeGoal1Db': bool(max(abs(env[2:8])) <= 1),
            'strongRelativeSpectrumGoal1Db': bool(a['pitchFitValid'] and b['pitchFitValid'] and spectrum_mask.any() and max(abs(relative[spectrum_mask])) <= 1)}


def main():
    refdir, renders, output = map(pathlib.Path, sys.argv[1:])
    manifest = refdir/'Miami-matrix.json'
    doc = json.loads(manifest.read_text())
    assert doc['complete'] and doc['sampleRate'] == SR and doc['channels'] == 2
    assert doc['componentVersion'] == 196870
    frames = _active_frames()
    steady_start, steady_end = _steady_window()
    note_off = au_compare_battery.miami_matrix_note_off_frame()
    results = {'method': 'Separate pitch, absolute envelope, fixed-frequency spectrum, bounded constant-lag phase; no fidelity aggregate.',
               'referenceManifestSha256': digest(manifest), 'sampleRate': SR,
               'harness': au_compare_battery.harness_summary_line(),
               'compareFrames': frames, 'configurations': {}}
    for cfg in CONFIGS:
        trial = next(t for t in doc['trials'] if t['group'] == cfg+'_r0' and t['repeat'] == 0)
        rep = next(t for t in doc['trials'] if t['group'] == cfg+'_r1' and t['repeat'] == 0)
        assert trial['fresh'] and trial['frames'] == 48000
        assert trial['leadFrames'] == 0 and trial['blockFrames'] == 256
        assert [(e['relativeFrame'], e['status'], e['note'], e['velocity']) for e in trial['events']] == [(0, 144, 48, 80), (24000, 128, 48, 0)]
        refpath = refdir/trial['file']
        ref = load_wav(refpath, frames)
        assert np.array_equal(ref, load_wav(refdir/rep['file'], frames)), 'Fresh reference repeat failed'
        rawpath = renders/(cfg+'.f32')
        proto = np.fromfile(rawpath, dtype='<f4').reshape(-1, 2).astype(float)
        if proto.shape[0] > frames:
            proto = proto[:frames]
        assert proto.shape[0] == frames
        nominal = 440*2**((48-69)/12)
        if cfg in ['sub_open', 'mod_triangle', 'mod_pulse', 'mix_depth']:
            nominal /= 2
        a, b = measure(ref, nominal, steady_start, steady_end), measure(proto, nominal, steady_start, steady_end)
        results['configurations'][cfg] = {'referenceFile': trial['file'],
            'compareNoteOffFrame': note_off,
            'referenceSha256': digest(refpath), 'replicateSha256': digest(refdir/rep['file']),
            'referenceRepeatExactStereo': bool(np.array_equal(ref, load_wav(refdir/rep['file']))),
            'prototypeRawSha256': digest(rawpath), 'events': trial['events'],
            'parameters': trial['parameters'], 'reference': a, 'prototype': b,
            'comparison': compare(ref, proto, a, b, steady_start, steady_end)}
        # Only original prototype audio is written. Reference stays at its original private location.
        wavfile.write(renders/(cfg+'.wav'), SR, proto.astype(np.float32))
    output.write_text(json.dumps(results, indent=2)+'\n')
    for name, row in results['configurations'].items():
        c = row['comparison']
        print(name, 'pitch cents', round(c['pitchErrorCents'], 3), 'RMS dB', round(c['noteRmsErrorDb'], 3),
              'relative spectrum dB', None if c['strongRelativeHarmonicMaxDb'] is None else round(c['strongRelativeHarmonicMaxDb'], 3),
              'correlation', round(c['constantLagCorrelation'], 6))


if __name__ == '__main__':
    main()
