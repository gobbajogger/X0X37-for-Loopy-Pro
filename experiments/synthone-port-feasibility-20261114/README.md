# Synth One engine feasibility probe

This is an external-input source feasibility probe, not a standalone/full Synth One engine, an AU implementation, or a finished DnBOX port. Read REFERENCE-PACKET.md for exact source pins, scope, allocation/render results and limits. No production firmware is changed.

## Inputs

The supplied Soundpipe dependency subset and MIT notices are included. Full Synth One app source, the preset corpus, generated tables/parameter arrays/audio, compiler, SDK and executables are not included.

Obtain the official AudioKit/AudioKitSynthOne source at commit `6466a3715c96b7ecf1dd255a218cbd571408a314`. Set `SYNTHONE_SOURCE` to its inner `AudioKitSynthOne` directory, containing `DSP/` and `Presets/`. `public-source-manifest.json` records source identities; verify every source/dependency SHA-256 with `python3 probe/verify_source_inputs.py` before generation; the AudioKit dependency pin is `03fecf80a66be88f6ac0356cd532c72dbe09922d` (v4.9.2).

Set `SYNTHONE_PRESETS_ARCHIVE` to the existing authorized original `Synth one.zip`, SHA-256 `68ad8b5fa0adca8b5da2b03f9303e9321c6a20907f5945e3fe879bc6494009a4`. The generator refuses a mismatched archive. It reads this private file but does not publish it. Generated `probe/host/preset-params.f32` contains supplied preset values and must remain private and uncommitted. The generated tables come from the pinned public source. Do not infer schema/app-version or audible compatibility solely from recognized keys.

## Host reproduction

Requirements: Python 3.12, a C/C++11 compiler, and the two explicit external inputs above. Commands start in this directory:

```sh
export SYNTHONE_SOURCE=/absolute/path/to/source/AudioKitSynthOne
export SYNTHONE_PRESETS_ARCHIVE='/absolute/path/to/Synth one.zip'
python3 probe/verify_source_inputs.py
sh probe/build_probe.sh
python3 probe/generate_voice_probe.py
sh probe/build_voice_probe.sh
```

The first command builds the real packaged Soundpipe modules and reports requested upstream initialization allocations. The generator creates task-local tables/parameters and reproduces the adapted source. The last command renders the defined static note/LFO subset, not the source app's full voice/event/FX system. Host wall time is not AC79 CPU evidence.

## Target compilation only

Set `JIELI_TOOLCHAIN` to an already authorized, verified JieLi compiler bundle root containing `common/bin/clang` and `pi32v2/include`. This experiment neither downloads nor distributes it. The original receipt used compiler clang 4.0.1 with pi32v2 r3/mfprev1.

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/verified/compiler-bundle
sh probe/target_compile.sh
sh probe/target_voice_compile.sh
```

These are object-only checks. Reported text sizes exclude final kernel/linker selection, tables, libc/libm/compiler helpers, other engines and firmware integration. Undefined math symbols, current flash/pool budget, integrated musical workload, source-app audio equivalence and hardware remain unresolved separately. Preserve saved Juno/Drone data and stable identities; this experiment does not reinterpret them.

## Preservation

This public projection retains the frozen source/math subset and original text receipts. It omits the private coordination plan and replaces machine-local paths with explicit environment inputs. DSP source and preset semantics are unchanged. No supplied bank payloads, generated parameter/audio/table binaries, credentials, SDK/toolchain or firmware products belong in commits.

Retain complete notices in `notices/` and source-file attributions. Audit any expanded dependency closure before distribution. The root MIT notices do not grant rights to proprietary J6 code/assets or supplied preset banks. Shared DnBOX-FX substitution is explicitly approximate unless separately proved against the reference.
