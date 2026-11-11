#!/usr/bin/env python3
"""Compare corresponding first/second notes without treating retained phase as fidelity."""
import json
import pathlib
import sys

import numpy as np

from compare import SR, compare, digest, load_wav, measure


def main():
    refs, renders, output = map(pathlib.Path, sys.argv[1:])
    rows = {}
    for cfg in ['carrier_open', 'square_open', 'sub_open', 'mix_depth']:
        nominal = 440*2**((48-69)/12)/(2 if cfg in ['sub_open', 'mix_depth'] else 1)
        reference = [load_wav(refs/f'Miami-{cfg}_r0-{i}.wav') for i in range(2)]
        raw = renders/(cfg+'-history.f32')
        proto = np.fromfile(raw, dtype='<f4').reshape(2, 48000, 2).astype(float)
        a = [measure(x, nominal) for x in reference]
        b = [measure(x, nominal) for x in proto]
        refchange = compare(reference[0], reference[1], a[0], a[1])
        protochange = compare(proto[0], proto[1], b[0], b[1])
        rows[cfg] = {'referenceSha256': [digest(refs/f'Miami-{cfg}_r0-{i}.wav') for i in range(2)],
                     'prototypeRawSha256': digest(raw), 'referenceFirstToSecond': refchange,
                     'prototypeFirstToSecond': protochange,
                     'historyRmsChangeMismatchDb': protochange['noteRmsErrorDb']-refchange['noteRmsErrorDb'],
                     'relativeHarmonicChangeMismatchDb': (np.array(protochange['relativeHarmonicErrorDb'])-np.array(refchange['relativeHarmonicErrorDb'])).tolist()}
    output.write_text(json.dumps({'sampleRate': SR, 'gateFrames': 24000,
        'gapFrames': 24000, 'method': 'Separate same-engine first-to-second changes; no architecture inference.',
        'configurations': rows}, indent=2)+'\n')
    for cfg, row in rows.items():
        print(cfg, 'RMS change mismatch dB', row['historyRmsChangeMismatchDb'],
              'H3/H5 change mismatch dB', [row['relativeHarmonicChangeMismatchDb'][i] for i in [2, 4]])


if __name__ == '__main__':
    main()
