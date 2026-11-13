# Component selection and license audit

This is an original host experiment within the GPL-3.0-only X0X repository.
It contains no KORG source, assets, factory presets or reference recordings.
The Miami name identifies the privately measured comparison instrument only.

The only adapted third-party implementation compiled here is the small PolyBLEP
helper in `blep.h`, derived from DaisySP's `Source/Synthesis/oscillator.cpp` at
`2c72eaf9eac5fc0dca1919d65d606da907832618`. The matching `oscillator.h` has
Copyright (c) 2020 Electrosmith, Corp and an explicit MIT-style license reference.
The pinned root `LICENSE` includes the complete MIT permission and warranty text.
Those exact source/header/license bytes are retained under `vendor-review/`.
The helper's C adaptation changes argument order and names, with attribution.

The audited upstream oscillator dependency is `Utility/dsp.h`, whose header
names Electrosmith and Emilie Gillet under MIT. It includes C++ standard headers
including `random` and various unrelated helpers. This experiment does not compile
that header or copy its unrelated utilities. Wrapping and float math use the
repository's existing GPL `firmware/src/dsp/fastmath.h`. No upstream tables,
random generators, allocation, constructors or C++ runtime enter the engine.

Also inspected at the same exact DaisySP revision: `fm2.h/.cpp` and
`Filters/svf.h/.cpp`, with MIT file-header grants inherited through the pinned
license. Fm2 depends on the oscillator; SVF depends on `dsp.h` and math functions.
They are retained as review evidence, but are **not compiled or adapted** here.
The new filter has its own bounded update and explicit experimental mapping.

Plaits `fm_engine.cc` and `fm_engine.h` at
`f4cfa7697095a28d292bff758beb51deb49b5a86` were considered. Both carry Emilie
Gillet's complete MIT grant. The engine's dependency list includes `engine.h`,
stmlib's parameter interpolator, sine oscillator, 4x downsampler, and generated
FM frequency tables. That transitive closure was not fully licensed/audited for
extraction here, and no Plaits code was copied. Its oversampling and frequency
dependent modulation limits remain useful future design ideas, not evidence of
Miami's implementation. No differently licensed optional module was included.

The exact pinned files were obtained through the connected GitHub source reader.
The ordinary HTTPS shell fetch failed DNS before reading bytes; no security
denial was bypassed. Current notices remain with corresponding source.
