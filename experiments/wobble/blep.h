/* SPDX-License-Identifier: MIT */
/* PolyBLEP helper adapted from DaisySP oscillator.cpp, pinned revision
 * 2c72eaf9eac5fc0dca1919d65d606da907832618. Copyright (c) 2020 Electrosmith.
 * Full notice and source provenance: vendor-review/ (retained alongside this).
 * This tiny component is not a Miami oscillator recreation. */
#ifndef X0X_WOBBLE_BLEP_H
#define X0X_WOBBLE_BLEP_H
static inline float wobble_blep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}
#endif
