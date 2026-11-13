# 26.8b — DnBOX Juno engine fidelity toward AudioKit Synth One J6 (2026-11-13)

ROADMAP **26.8b** on `lane/fmb-presets-format`. This is an **engineering goal**: the DnBOX Juno **engine** should reproduce AudioKit Synth One J6 sounds faithfully, not only approximate presets. Fidelity is a goal. **Never assume or claim identity.** There is still **no** J6 export and **no** J6 reference audio (**MISSING PROOF**, §5).

This document does **not** edit firmware on any lane. Host-only prototypes are in the isolated directory `/workspace/j6-fidelity` (pointer: `docs/plans/2026-11-13-j6-fidelity-experiments-README.md`). A separate FMB task (**26.9**) is measuring Juno/Drone **gain** at `94197fc8` in `/workspace/gain-juno-drone`. Those tests were **not** run here, and that tree was **not** edited. Engine edits are sequenced later, **after both studies**.

No JieLi builds. Every device cost below is a **HOST ESTIMATE** from host cycle timings and host code-size proxies. It is not an AC79 measurement (the host has SIMD; the pi32v2 runs XIP and has no SIMD).

Related: `docs/plans/2026-11-13-audiokit-j6-juno-compat.md` (preset converter; schema still UNVERIFIED). This 26.8b work is the **engine**, not the file map. Native Juno presets and the current render must stay available (§4).

## 1. Architecture comparison

### 1.1 AudioKit Synth One J6 (public sources)

Product identity follows Website UX’s research and the public registry (compat doc §1): **AudioKit Synth One J6 + AUv3**, bundle `com.audiokitpro.superj6`, store version **0.999.8** as of 2026-11-13. J6 **preset schema is UNVERIFIED**. The older Synth One JSON is **not** this schema.

| Topic | Claim | Label | Source |
| --- | --- | --- | --- |
| Oscillators | Oscillators trained from recorded vintage **Juno-106 and Jupiter-8** tones; “analog-like variations in the tuning” (drift). Waveforms named: **sine, saw, square, triangle**. | **VERIFIED** (vendor) | https://audiokitpro.com/synthonej6/ |
| PWM / pulse | Pulse waveform; pulse width is a modulation destination for **LFO 2 and LFO 3**. | **VERIFIED** (App Store update text) | https://apps.apple.com/us/app/audiokit-synth-one-j6-auv3/id6741535277 |
| Noise | “NEW Noise Oscillator.” | **VERIFIED** (App Store) | same |
| Oversampling | 2× on iPhone/iPad, 4× on Mac. | **VERIFIED** (App Store) | same |
| Two DCOs, independent level/tune, Juno-style octaves, square sub | Two DCOs with those controls; square sub. | **UNVERIFIED** (third-party review; vendor says “oscillators” and a sub is consistent but not specified this way on the vendor page) | https://synthanatomy.com/2026/09/audiokit-synth-one-j6-review-free-ios-synth-app-for-mobile-music-producers-and-synthesis-newbies.html |
| Mixer | Each section has its own volume; “doesn’t need a separate mixer.” | **UNVERIFIED** (review) | Synth Anatomy |
| Filter | “Analog-style filters.” “NEW V2 FILTER inspired by the character of the **J106**”; tap the Filter header to switch types. | **VERIFIED** that a switchable J106-inspired V2 exists (App Store). Slope, resonance curve, and which type is default: **UNVERIFIED**. | App Store; https://audiokitpro.com/synthonej6/ (“analog-style filters”) |
| Filter (review) | “Jupiter-8-inspired” resonant LPF and **non-resonant HPF**; cutoff by envelope **and** LFO; creamy below 50% resonance, growl above. | **UNVERIFIED** (review). The official V2 note is J106-inspired, which is a different vendor sentence than “Jupiter-8-inspired.” | Synth Anatomy |
| Key track / filter velocity | Filter envelope has “adjustable keyboard tracking and velocity.” | **UNVERIFIED** (review) | Synth Anatomy |
| HPF | Present (review: non-resonant). Vendor page does not name an HPF. | **UNVERIFIED** as a specified control | Synth Anatomy; Jade Starr walkthrough index also lists “High pass” at https://howtoapponios.com/apps/synth-one-j6/ (no transcript on this box) |
| ADSRs | “ADSR for **Amp & Filter** Envelopes.” Two envelopes. | **VERIFIED** (vendor) | https://audiokitpro.com/synthonej6/ |
| ADSR routing | Envelopes “fixed to the filter and amp”; “linkable.” | **UNVERIFIED** (review) | Synth Anatomy |
| LFOs | **3 LFOs**, **60+ destinations**, LFOs can modulate LFOs; each can be Hz or tempo-sync’d. | **VERIFIED** (vendor, App Store) | vendor page; App Store |
| LFO detail | LFO 1 “simple,” LFO 2/3 “advanced”; LFO 3 has delay, looping, restart; LFO 2/3 each to two destinations, including ADSR stages, FX, sequencer steps. | **UNVERIFIED** (review), except PWM as dest for LFO 2/3 (**VERIFIED**, App Store) | Synth Anatomy; App Store |
| Chorus | Chorus is included. “NEW **J60 CHORUS**”; tap the Chorus header to switch types. | **VERIFIED** that switchable J60-style chorus exists | App Store; vendor “Chorus” |
| Chorus modes I / II / I+II, two stacked choruses | “Modeled Juno chorus with all three distinct modes (I, II, I+II)”; walkthrough “two choruses stacked.” | **UNVERIFIED** (review / walkthrough) | Synth Anatomy; Jade Starr page |
| Delay / reverb | Analog **tape delay**; reverb “based on **Sean Costello’s** code (Valhalla).” Bonus: crush, stereo widen (HAAS), auto-pan, dedicated VCA LFO (tremolo). | **VERIFIED** (vendor) | https://audiokitpro.com/synthonej6/ |
| Second reverb “Sparkle Hall” | Named in the review. | **UNVERIFIED** (review) | Synth Anatomy |
| Voice count / unison | Exact polyphony **UNVERIFIED**. “Supersaw, based on the Roland Jupiter 8000 … stack up to **five** voices” (review) vs walkthrough “Super saw up to **six** voices.” Those two third-party counts **disagree**. | **UNVERIFIED**; do not pick a number | Synth Anatomy; Jade Starr |
| Portamento | “Selectable play modes: polyphonic, mono, and adjustable pitch-glide.” | **VERIFIED** (vendor) | vendor page |
| Velocity | Filter-env velocity: **UNVERIFIED** (review). Amp velocity: **UNVERIFIED** (not stated by the vendor). | **UNVERIFIED** | Synth Anatomy |
| Analog drift | ML oscillators: “random imperfections,” “variations in the tuning.” | **VERIFIED** as a vendor claim (not measured here) | vendor page |
| Not a clone | “Not a clone of the Juno-6, Juno-60, Juno-106, Jupiter-8.” | **VERIFIED** (legal notice) | vendor page; App Store |

No official J6 manual or vendor parameter list with ranges was found. No AudioKit forum posts were found for these details. No video **transcript** is on this box; the Jade Starr page is only a chapter index.

### 1.2 DnBOX Juno (this lane’s real code)

Engine: `firmware/src/dsp/juno6.c` / `juno6.h`, wired in `firmware/src/app/original_runtime.c`. Header: “Compact original equations informed by the Plaits analog oscillator/filter **design**; **no Plaits source is copied**” (`juno6.h:2–4`). This is **not** a Plaits port of the 26.4 research sketch. Six voices (`JUNO6_VOICES`, `juno6.h:7`). Host `sizeof`: voice **36 B**, `juno6_t` **352 B**, chorus **536 B** (256 × int16 delay + state) — measured in `/workspace/j6-fidelity`.

| Topic | Behaviour | File:line |
| --- | --- | --- |
| Oscillator | One **saw + pulse blend**, `saw*(1-macro[0])+pulse*macro[0]`, PolyBLEP. `waveform = (uint8_t)(x*2.99)` is stored (`juno6_set` case 0) and **not used in render**. No sine, no triangle, no second DCO, no detune, no oversampling, no ML drift (deterministic). | `juno6.c:36–41`, `juno6.c:70` |
| Sub | Square, half frequency, level `x*0.5`. **No BLEP** on the sub. | `juno6.c:40–41`, `juno6.c:51`, `juno6.c:71` |
| Noise | **None.** | (absent from `juno6_render`) |
| Mixer | Blend + sub + a 0.7 scale, then `Level/6`. No independent DCO2 / noise / sub-shape mix. | `juno6.c:41`, `juno6.c:53` |
| Filter | **Bounded two-pole low-pass**, “no transcendental work per voice/sample.” Cutoff Hz = `35+cutoff²*12000` plus `envelope*EnvAmt*7000`. Resonance `*0.8`. **No key track** (cutoff is global, not per-note). **No 4-pole / J106 mode.** | `juno6.c:25–26`, `juno6.c:45–48` |
| HPF | One-pole on the **mix** (not per voice), Hz = `15+2000*hpf²`, skipped when `hpf==0`. | `juno6.c:27`, `juno6.c:54` |
| ADSR | **One shared envelope** for amp **and** filter (filter uses the same `v->envelope` via EnvAmt). Times: attack `0.002+x²*3` s, decay `0.01+x²*3`, sustain `x`, release `0.01+x²*4`. | `juno6.c:42–49`, `juno6.c:74–77` |
| LFO | **One** triangle LFO, rate `0.1+8*x²` Hz, **PWM only** (`width += lfo*LFODep*0.2`). No LFO→cutoff, no LFO→pitch, no second or third LFO, no tempo-sync. | `juno6.c:28–36` |
| Chorus | Shared 16-bit, 256-sample delay, **one** sine LFO, opposite wet on L/R. Depth clamp **1.7–5.4 ms** (Juno-60 range). Runtime **fixes depth at 5.4 ms**. Rate `0.1+4.9*ChoRate` Hz, mix `0.5*Chorus`. **No I / II / I+II switch**, no second J60 type. | `juno6.c:58–60`; `original_runtime.c:101`; `original_runtime.c:353–355` |
| Delay / reverb | **None** in the engine. (Project FX is a different path; A/B in this study has it off.) | (absent) |
| Voice count | 6, with steal of the **oldest** sounding voice. | `juno6.c:12`, `juno6.h:7` |
| Unison / portamento | **None.** Poly allocator only. No glide. | `juno6.c:12` |
| Velocity | **Amplitude only**: `mix += velocity*envelope*filter`. Velocity is `vel/127`. **No** velocity-to-cutoff. | `juno6.c:12`, `juno6.c:49` |
| Drift | None. | deterministic BLEP |
| Performance | Pitch bend `(x-0.5)*4` semitones; damper. **Not** patch data (`original_sound.h:37`). | `juno6.c:80–81` |
| Params | 18 macros, 16 patch + 2 performance, 7-bit. Names in `original_sound.c:31–36`. | see those lines |

**CPU per voice (known, host — this box, 2026-11-13).** `/workspace/j6-fidelity` bench, `-O2 -ffp-contract=off`, 44,100 Hz, 3 s work, median of 3 runs, 256-sample blocks. **HOST ESTIMATE, not device:**

- Baseline **1 held voice:** **9.345 ns/sample** (8.898–9.624).
- Baseline **6 held voices:** **51.809 ns/sample** (51.634–53.254) ≈ 8.63 ns/voice on this host when all six sound, plus some constant cost (1-voice is not 51.8/6).
- Baseline **6 voices + stock chorus I:** **68.264 ns/sample**.
- Chorus I alone: **17.937 ns/sample** (does not scale with voices).

ROADMAP 26.4 quoted a **plan** of 32–39 ns/sample per Plaits voice. The **shipped** engine is this compact original, and on this host it is **cheaper** than that sketch. **Device** Juno CPU is **not measured** (no JieLi run in this step). 26.4 already left “does six voices fit under heavy load?” as an open question (`juno-engine-research-026.md`). Chicago 1-note **34.6 ns/sample** host was a 26.4 reference on this box, not a device number.

## 2. Ranked musically audible differences

Ranking is by **impact on real playing**, then cost and evidence. The known gaps (separate amp/filter ADSR; 3 routable LFOs vs one shared envelope and a PWM LFO) **guided** the list. They are **not** an automatic build order, and none is allowed into firmware as-is if it adds CPU without a saving (§3).

| Rank | Difference | Playing impact | Cost (HOST ESTIMATE, §3) | Evidence |
| ---: | --- | --- | --- | --- |
| 1 | **Separate filter ADSR** vs one shared envelope | A J6-style pluck (fast amp, slow filter) or a pad (slow amp, different filter) **cannot** be made today. This is the largest everyday voicing gap. | Fair linked test: **+0.98 ns/sample** at 1 voice (~+11%), **+6.35 ns** at 6 voices (~+14%). RAM **+48 B**. Host `.text` of the new render **1877 B** vs 1499 B. | VERIFIED two J6 ADSRs (vendor). DnBOX one envelope (`juno6.c:42–49`). |
| 2 | **LFO → cutoff** (and, more generally, three routable LFOs) vs PWM-only | Wah, tremolo-filter, moving pads are a J6 staple (60+ dest, LFO-to-LFO). DnBOX LFO never reaches the filter. | Shared-LFO route (E2a): **near zero** at 6 voices (within host noise); **+0.33 ns** at 1 voice. Independent 2nd LFO (E2b): 1-voice median **+4.2 ns** (noisy); 6-voice within noise. RAM **+4 B** (E2a) or **+12 B** (E2b). Host `.text` +64 / +144 B. A **third** LFO and a 60-destination matrix were **not** prototyped; they would cost more. | VERIFIED 3 LFOs and PWM dest for LFO 2/3 (vendor / App Store). Cutoff as dest is UNVERIFIED officially, claimed in the review. DnBOX PWM-only (`juno6.c:30–36`). |
| 3 | **Chorus character** (J60 switch, I / II / I+II) vs one max-depth tap | The “Juno sheen” on chords is a large part of why a J6 preset sounds like J6. DnBOX always uses one LFO and **fixes depth at 5.4 ms**. | I+II on the **same** 256-sample line: chorus-only **+10.35 ns** (~+58%); 6-voice+chorus **+8.59 ns** (~+13%). RAM **+12 B**. Host `.text` 1201 vs 793 B. | VERIFIED switchable J60 chorus (App Store). Modes I/II/I+II UNVERIFIED (review). DnBOX `original_runtime.c:101`. |
| 4 | **Filter family** (J106-inspired 4-pole / switchable types, key track) vs a 2-pole global cutoff | Brightness and resonance behaviour of a Juno-106-style filter will not match a 2-pole with no key track, even when ADSRs match. | 4-pole cascade prototype: **+1.59 ns** at 1 voice, **+7.88 ns** at 6 voices (~+15%). RAM **+48 B**. Host `.text` +80 B. Key track was **not** prototyped. | VERIFIED V2 J106-inspired filter (App Store). Slope UNVERIFIED. DnBOX 2-pole (`juno6.c:45–48`), no key track. |
| 5 | **Oscillator set** (2 DCOs, sine/tri, noise, PWM as a wave, 2×/4× oversampling, ML drift, supersaw) vs one BLEP saw/pulse + sub | This is the “is it the same instrument?” gap. A faithful J6 sound almost certainly needs more than one oscillator and some drift. Cost is the reason it is not an automatic first build. | Noise mix only (E5): **+0.79 ns** at 1 voice, **+7.43 ns** at 6 voices (~+14%). RAM **+8 B**. Host `.text` +92 B. Second DCO, oversampling, and supersaw were **not** bounded here; 26.4 already measured a 7-voice supersaw at ~9 ns/sample **per note** on this box (`juno-engine-research-026.md` chorus bench) — expensive under a 6-note chord. | VERIFIED waves, PWM, noise, oversampling, ML drift (vendor / App Store). Two DCOs and supersaw UNVERIFIED. DnBOX `juno6.c:36–41`. |
| 6 | **In-engine delay, reverb, crush, widen, auto-pan, tremolo** | J6 presets that rely on the built-in Sean Costello reverb or tape delay will sound dry on DnBOX if only the slot engine is heard. Some of this may belong to **project FX**, not the Juno engine — a product choice, not an automatic engine port. | Not prototyped. Prior research rejected large plate/tape FX for RAM (`ROADMAP` 26.7). | VERIFIED FX list (vendor). |
| 7 | **Mono / glide, microtonality, dual arp, sequencer, touch pads, exact polyphony / unison** | Performance and sequence features; audible when a J6 preset depends on them. Lower priority for a **held-note** engine voice. | Not prototyped. | VERIFIED play modes, microtonality, arp, sequencer, pads (vendor / App Store). Unison count UNVERIFIED. DnBOX: none of these. |

Velocity-to-cutoff (UNVERIFIED on J6) would sit with rank 2 or 4 if confirmed. 26.9 will speak to **loudness**, not to these voicing gaps.

## 3. Bounded host experiments (top items)

Directory: `/workspace/j6-fidelity`. Engine copies are SHA-identical to this lane (`PROVENANCE.txt`). Flags: `cc -std=c99 -O2 -ffp-contract=off -DX0X_HOST`. Sample rate 44,100 Hz, block 256, median of 3 runs (E1 fair: 5 runs, second session). WAVs and SHA-256: experiments README.

**Offsetting savings.** No saving was found that pays for a CPU add **and** keeps the current render bit-identical for old patches. The compact engine is already small. **Every CPU-adding item is NOT ALLOWED as-is** (rule: firmware must never get slower). A later Optimizer or DSP saving, measured on the **same** host bench and then on the device, would be required before a ship decision. Versioning (§4) can keep **old** patches on the **current** `juno6_render` (E2a with depth 0 was **byte-identical** to the baseline for 1024 samples), so **existing** projects need not get slower if the v0 function is left unchanged. That does **not** allow a slower v1 path to ship by default.

**Floors and Optimizer.** Floors: flash free ≥ **8,192 B**, pool free ≥ **8,192 B**. At `94197fc8`, code-flash headroom is only **248 B Elite / 256 B Pro**, and pool headroom **148 B** (plan §12–§14). The measured STAGE=4 **codec** already puts Elite flash free at **6,276 B** and Pro at **6,284 B** (below the floor by 1,916 / 1,908 B), with pool still 8,340 B. Any 26.8b engine growth is **in addition** to the codec. It **depends on the Optimizer’s savings**. Even if the Optimizer only returns the 2,164 B codec cost, **248 / 256 B** remain — **less** than the HOST ESTIMATE `.text` of a second render function (see table). Do not wire 26.8b into a default image on that headroom.

**HOST ESTIMATE method for SMK CPU %.** Device cycles were **not** counted. The table gives (1) host ns/sample and the **relative** add vs the same-session baseline, and (2) a **HOST real-time fraction** = `host_ns / 22675.7 × 100` (one 44,100 Hz sample’s wall time). That fraction is what this **x86 host** would use of one core; the SMK (360 MHz AC79, no SIMD, XIP) will be **higher**. Do not treat the fraction as a device %. The **relative** column is the figure to apply, later, to a real device Juno measurement.

**Latency.** All prototypes are per-sample (no added block delay). Chorus I+II keeps the same 256-sample line: max delay **5.80 ms** at 44,100 Hz (already true of stock I). No polyphony cap is proposed from these host numbers alone (6 voices stay 6); a device budget can still force a cap — that is unmeasured.

### 3.1 Cost table (HOST ESTIMATE)

| Item | Host ns/sample (median) | Δ vs baseline | HOST real-time fraction (optimistic) | HOST `.text` proxy | RAM (`sizeof` Δ) | Latency | Polyphony | Allowed? |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| Baseline 1 voice | 9.345 | — | 0.041% | `juno6_render` 1499 B | 352 B synth | none added | 6 max | current |
| Baseline 6 voices | 51.809 | — | 0.228% | same | same | | | current |
| Baseline 6v + chorus I | 68.264 | — | 0.301% | + `juno6_chorus_process` 793 B | +536 B | 5.80 ms max | | current |
| **E1** separate filter ADSR, **fair linked**, 1v / 6v | 10.081 / 52.564 (2nd session; that session’s baseline was 9.099 / 46.217) | **+0.982 / +6.347** (~+11% / ~+14%) | 0.044% / 0.232% on that session | new render **1877 B** (Δ +378 if replaced; **+1877** if **added** beside v0) | **+48 B** (400−352) | none | 6 | **NOT ALLOWED as-is** (CPU up, no saving; flash ≫ 248 B if v1 is added) |
| **E2a** LFO→cutoff, **same** PWM LFO, 1v / 6v | 9.676 / 50.671 | **+0.331 / −1.14** (6v within noise) | 0.043% / 0.223% | new render 1563 B (Δ +64 / add +1563) | **+4 B** | none | 6 | **CPU** maybe near-zero at 6v, **not proven**; **flash** still an add. **NOT ALLOWED as-is** until a same-binary v0 bench shows **no** slowdown and Optimizer flash fits. |
| **E2b** independent LFO 2 → cutoff, 1v / 6v | 13.551 / 51.010 (1v noisy: 10.7–14.2) | **+4.21 / −0.80** | 0.060% / 0.225% | 1643 B (Δ +144) | **+12 B** | none | 6 | **NOT ALLOWED as-is** (1-voice CPU up; 3-LFO matrix not even included) |
| **E3** chorus I+II, chorus-only / 6v+chorus | 28.286 / 76.852 | **+10.35 / +8.59** (~+58% chorus, ~+13% full) | 0.125% / 0.339% | 1201 B vs 793 (Δ +408) | **+12 B** (same 512 B delay) | same 5.80 ms | 6 | **NOT ALLOWED as-is** |
| **E4** 4-pole cascade, 1v / 6v | 10.940 / 59.689 | **+1.59 / +7.88** (~+15% at 6v) | 0.048% / 0.263% | 1579 B (Δ +80) | **+48 B** | none | 6 | **NOT ALLOWED as-is** |
| **E5** noise mix, 1v / 6v | 10.134 / 59.235 | **+0.79 / +7.43** (~+14% at 6v) | 0.045% / 0.261% | 1591 B (Δ +92) | **+8 B** | none | 6 | **NOT ALLOWED as-is** |

The first-session E1 numbers in `results/bench.json` (59 / 102 ns) used a **short-amp unlinked demo** patch and are **not** the cost figures. Use the fair linked row.

E2a with **depth 0** matched the baseline **byte-for-byte** for 1024 samples: a disabled route can preserve v0 sound. That is a versioning fact, not a permission to ship extra CPU or flash.

### 3.2 What the WAVs show (not a J6 A/B)

These are **engine-vs-engine** demos on the host, **not** proof against J6 (**MISSING PROOF**, §5). All SHA-256 values are in the experiments README.

- `e1_filt_env.wav` — fast amp, slow filter: a contour the current engine cannot make with one envelope.
- `e2a_lfo_cutoff.wav` / `e2b_lfo2_cutoff.wav` — periodic filter motion with PWM depth zero.
- `e3_chorus_ii.wav` and `e3_chorus_ii_chord6.wav` — two-rate chorus against `baseline_c3.wav` / `baseline_chord6.wav`.
- `e4_fourpole.wav` — steeper rolloff on the same cutoff macros.
- `e5_noise.wav` — audible noise in the mix.

Do **not** read these as “closer to J6.” There is no J6 reference take.

## 4. Compatibility and versioning

Existing sounds and projects **must render as before**.

**Plan (docs only; not implemented):**

1. Keep the **current** `juno6_render` / `juno6_chorus_process` as **engine version 0**. Do not add branches in that function. Old patches call it unchanged (same CPU, same samples).
2. Give new behaviour an explicit **engine-version** (or param-schema) field. Candidates, to be chosen when firmware is allowed to change: a non-zero use of the reserved byte in the 32-byte cell (`original_sound.h`, byte 31 is reserved zero), or DNPR param-schema **2** for Juno only, or a dedicated field. DNPR v1 **flags must stay 0**; do not overload flags for this.
3. **Old patches map to v0.** Factory JUNO POLY / PAD / PLUCK / BRASS, every existing user cell, and every DNPR Juno record with schema 1 and today’s 18 bytes, stay on v0 **without migration**.
4. **No automatic migration.** A user who wants v1 (separate filter ADSR, etc.) opts in, after the feature is allowed. A one-way export can write a v1 record; it must not rewrite a project in place on upgrade.
5. **v1 records** carry the extra state (filter ADSR times, LFO routes, chorus mode, …) in an extended schema, with honesty labels from the compat doc. Until that schema exists, do not invent DNPR bytes for it.
6. If a v1 path is ever allowed, it still needs an **offsetting CPU saving** or a measured proof that it is **not** slower under the same load rule — otherwise it stays **NOT ALLOWED as-is**, even as opt-in, for default firmware (an experimental compile-time switch is a later product decision, not a waiver of the rule).
7. 26.9 gain results apply to **whichever** version is sounding. Do not mix a loudness change and a fidelity change in one unversioned step.

## 5. Validation plan (once references exist)

**MISSING PROOF today:**

| Reference | Status | Who |
| --- | --- | --- |
| A **user-saved J6 preset export** plus the exact app / AU version | **MISSING PROOF** | Owner (compat doc J1) |
| **J6 reference audio** of that program (and of agreed test notes), lossless, named version | **MISSING PROOF** | Owner, or FMB/Installer QA at the owner’s machine with that install |
| J6 parameter dump / schema for the same program | **MISSING PROOF** (schema UNVERIFIED) | FMB + Website UX after J1 |
| Device (SMK) cycle counts for baseline and any v1 | **MISSING PROOF** | Later JieLi / fm1-emu, not this step |

When those exist, validation is:

- **Matched notes:** same MIDI as the X14-style protocol used here (C3 or C2, mid velocity, 4 beats at 120 BPM, eighth-note stab), plus a 6-note chord, plus at least one mono legato/glide case if J6 uses it.
- **Velocities:** soft / mid / hard, to test amp vs filter velocity (the latter is UNVERIFIED).
- **Chords:** six-note hold, chorus on, to test I vs I+II vs J6 J60.
- **FX tails:** J6 delay and reverb tails vs DnBOX with project FX off and, separately, with a documented project-FX stand-in — do not hide an engine gap behind accidental FX.
- **A/B:** lossless WAV, comparison sheet (peak, RMS, spectrum, audible drift). **No** identical-sound language. Parameter grades stay EXACT / APPROXIMATE / UNSUPPORTED.
- Website UX and Installer QA use the **same** contract as the compat doc, once verified, plus this engine-version rule: a v0 project byte-matches today’s render on a named SHA.

Until **MISSING PROOF** items arrive, do not claim a prototype is faithful to J6.

## 6. Sources

- Website UX AudioKit identity research, relayed 2026-11-13 (America/Chicago); compat doc.
- https://audiokitpro.com/synthonej6/ , https://apps.apple.com/us/app/audiokit-synth-one-j6-auv3/id6741535277 , iTunes lookup `id=6741535277`.
- https://synthanatomy.com/2026/09/audiokit-synth-one-j6-review-free-ios-synth-app-for-mobile-music-producers-and-synthesis-newbies.html (third-party; UNVERIFIED unless it repeats a vendor sentence).
- https://howtoapponios.com/apps/synth-one-j6/ (walkthrough index; no transcript here).
- This lane: `firmware/src/dsp/juno6.c`, `juno6.h`, `firmware/src/app/original_runtime.c`, `original_sound.c`.
- Host prototypes: `/workspace/j6-fidelity` (results in `results/bench.json`, `results/e1fair.txt`).
- `/workspace/juno-engine-research-026.md` (26.4/26.5 research; not a J6 source).
- Flash/pool: `docs/plans/2026-11-13-preset-format-v1.md` §12–§14.
