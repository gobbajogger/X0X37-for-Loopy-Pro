#!/usr/bin/env python3
"""Regression checks for analysis failure modes, not reference-fidelity claims."""
import numpy as np

from compare import SR, compare, measure


def stereo(y):
    return np.column_stack([y, y])


def main():
    f = 130.81278265
    t = np.arange(SR)/SR
    base = np.sin(2*np.pi*f*t)
    rich = base+.2*np.sin(2*np.pi*3*f*t)
    a, b = stereo(rich), stereo(base)
    result = compare(a, b, measure(a, f), measure(b, f))
    assert not result['strongRelativeSpectrumGoal1Db'], 'Missing reference partial escaped'
    assert 3 in result['referenceStrongHarmonics']
    result = compare(b, a, measure(b, f), measure(a, f))
    assert 3 in result['unexpectedPrototypeStrongHarmonics']
    assert not result['strongRelativeSpectrumGoal1Db']
    silent = np.zeros_like(a)
    result = compare(a, silent, measure(a, f), measure(silent, f))
    assert not result['pitchGoal2Cents']
    assert not result['strongRelativeSpectrumGoal1Db']
    result = compare(a, a*2, measure(a, f), measure(a*2, f))
    assert result['strongRelativeSpectrumGoal1Db']
    assert not result['steadyEnvelopeGoal1Db']
    assert abs(result['noteRmsErrorDb']-6.020599913) < 1e-6
    print('PASS missing/added partials, silent pitch refusal, gain versus spectrum separation')


if __name__ == '__main__':
    main()
