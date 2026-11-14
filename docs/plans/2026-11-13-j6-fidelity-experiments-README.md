# J6 fidelity host experiments (26.8b) — pointer

The prototypes live **outside this repo**, in `/workspace/j6-fidelity` on the box. They are host-only. They do **not** edit lane firmware. They are **not** JieLi builds. 26.9 loudness tests were not run.

Full write-up: `docs/plans/2026-11-13-j6-juno-engine-fidelity.md`.

## What is there

- `engine/` — byte-identical copies of `firmware/src/dsp/juno6.c`, `juno6.h`, `fastmath.h` from this lane (SHA-256 in `PROVENANCE.txt`).
- `proto/` — bounded C prototypes: separate filter ADSR, LFO→cutoff (shared and independent LFO), chorus I+II, four-pole cascade, noise mix.
- `build/bench` — host bench (`-O2 -ffp-contract=off`). Timings in `results/bench.json` and fair E1 in `results/e1fair.txt`.
- `renders/*.wav` — 3 s, 44.1 kHz, stereo, 16-bit, ~529,244 B each. **Not committed** (no big binaries). SHA-256:

```
20d4818e190d7ef545d27e2aea4a2c971450c47f2d8a0938d600c0e53463e237  baseline_c3.wav
effaab6ec88573efe2820be14f831e7305bc46ecc6a8d7b95189b07555b452a0  baseline_chord6.wav
fa3e381ef866fcd00e7635f7e3479e760aa011ddbeab7c090fe5fe721ea7b5b0  e1_filt_env.wav
c43c6df6fedb8da24a3a33612cbda4748defc6af1679143ecb685b4daeabea19  e2a_lfo_cutoff.wav
b45f0a220466c86601cda3b7cb5876aaf290233d78458f8c90af3330ebf20c72  e2b_lfo2_cutoff.wav
f291e70d104740a8fe24540dd10fd49fe9c417c4a1fdf48ed29959ee35a91560  e3_chorus_ii.wav
9ea7cd7335e288f26a42b156ab2d308890d1677b12aba5d374a30c6f0be0ab34  e3_chorus_ii_chord6.wav
b196958969bdefef81c9b78b443bcfb8cb29db22c7eaa727cc1bbc4fe6607756  e4_fourpole.wav
bc3d3301e7268494993336d6c7ab861cdbacf73a10b9940afdfa421090df34c4  e5_noise.wav
```

Protocol: C3 (MIDI 60), velocity 80, 4 beats held at 120 BPM (2.0 s), one eighth-note stab (0.25 s), 0.75 s tail. Chord renders hold C3–E3–G3–C4–E4–G4. Project FX is off; only the engine chorus is in the file.

Rebuild on the box: `cc -std=c99 -O2 -ffp-contract=off -DX0X_HOST -Iengine -Iproto -o build/bench bench.c proto/engines.c engine/juno6.c -lm && build/bench`.
