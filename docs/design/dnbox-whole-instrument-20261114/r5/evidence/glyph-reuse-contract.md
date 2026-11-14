# Current glyph reuse contract: exact source identities

Read-only evidence for the existing design owner. No candidate, renderer, runtime, private header/export, build, emulator, review or device action. Effective model/effort is not exposed.

Compared exact3c41ef4bedf9701f549ce2752b73306e2fab1eb8, recovered accepted d158444101d569f9ee64ba05d7cb54f24de1be30, #931 base e581333f1ef2586c6c37f72e80d0b51fe5a00723 and head ea10fb1492527932476b0ecfa668348f5c1a8a03. d158 was independently fetched after its full identity was supplied; it was not assumed equal to e581. Its reported tree0b966725cb55713dcf8817156130cc3f57f61e06 remains parent-supplied metadata.

| Public source |3c41 and d158 Git blob|#931 base and head Git blob|
|---|---|---|
| ui.c |0d1fb3962b3d2773fde935bf032c2167551b321e|0694f20d1e7106d2479aa3846873426af7d114c9|
| ui_original_controls.h |afd18e4d0bc5ed1c58ce92002bfb916cfe530627|same|
| ui_original_wave.h |568bcf906655b7dc4eae7b22e0ae1b72369cf63f|same|
| ui_pictograms.h |7753873439d4cc13845220c43a1f1d3f6bca958c|same|
| juno6.c (ADSR law only) |223e6bdc5ac269b3d6cac97ae890204b16da185c|same|

#931 changes no inspected public UI/glyph file relative to its base. Its six changed filenames are runtime/profile/archive component files; UI composition remains outside that component. The only ui.c delta from3c41/d158 to e581/ea10 gates the question YES card/footer and SCALE LED with action_key_cue(). No Juno/ADSR/glyph/parameter-page/navigation change appears. Seventeen relevant function bodies were compared as exact text and are identical across all four refs; source-and-function-manifest.json records their hashes and line positions. The small public diff is ui-3c41-to-e581.patch.

## Established Chicago ADSR language

draw_part_sound()5742–5842 dispatches adsr_row() before drawing individual cells. adsr_row() at3c41/d1587371–7377 (e581/ea107376–7382) admits only V_PART on a synth slot whose actual engine is Chicago and whose row IDs are7,8,9,10. It draws whether the ENVELOPE row is selected or a neighboring inactive row. This full-row graph and its useful controls are already firmware behavior, not a new design feature.

Reuse the established row family: four-column K5 A/K6 D/K7 S/K8 R percentage labels at rest and during turns; selected-stage/value caption; highlighted segment/underline; fixed illustrative sustain hold; note-off marker; source native stage/level gauge and admitted live VCA cursor. Retain source truth about which elements are a schematic and which read actual runtime state. The labels that follow an edited value and the touch/highlight behavior are pre-existing too; do not advertise them as newly added dynamic labels.

adsr_values() reads compiled native Chicago ADSR when available and has a q fallback. adsr_original_level(), adsr_layout() and draw_adsr_inline() preserve Chicago's original curves and widths: attack toward1.3 clipped at1, RC decay toward sustain, RC release; duration endpoints include 1%-of-sustain decay and.04 release cutoff; sustain width42 is illustrative. Replacing this with a synthetic fixed chart or raw-q-only copy loses existing behavior. Relevant source functions are byte-identical at every compared ref.

## Actual Juno admission at these exact refs

The dedicated V_PART Juno parameter matrix has four editable rows plus SOUND:

| Page | K5 | K6 | K7 | K8 |
|---|---|---|---|---|
| OSCILLATOR |WAVE id0|SUB1|WIDTH12|LEVEL15|
| FILTER |CUTOFF2|RES3|ENV4|HPF9|
| ENVELOPE |ATTACK5|DECAY6|SUSTAIN7|RELEASE8|
| LFO/CHORUS |LFO RATE10|LFO DEPTH11|CHO RATE13|CHORUS14|

Juno already has these human-readable labels, rounded-percent values, neutral value bars, selected-row/knob mapping, engine A/B identity and ordinary shared sequence/mixer/FX vocabulary. Preserve those existing strengths and routes. The absence below is specifically about dedicated Juno V_PART parameter pictograms, not every view used with Juno.

At all four exact refs, wave_shape() explicitly accepts only Chicago/Miami, original_wave_view()/original_wave_slot() exclude Juno, orig_strip_slot() admits only Chicago/Miami, and adsr_row() admits only Chicago. Therefore Juno V_PART parameter cells fall through draw_param_widget() to value text and a neutral bar. Its ENVELOPE does not currently call the full-row ADSR graph. No dedicated Juno oscillator/ADSR glyph implementation was found in these pinned public selection paths. Do not describe bespoke R4 proposal Juno glyphs as current firmware; if retained, call them retained proposal extensions. If another accepted component is intended, its exact artifact/source identity is needed before making a different baseline claim.

K2/physical arrows still select rows with source wrap, PARA cycles the remembered full PART page set, SCALE opens all parameters, and PATCH returns from the list. Juno's fifth SOUND row and sparse/read-only SOUND cells stay in that topology. Those routes are retained baseline behavior, not new access improvements.

## Source-grounded full-row Juno ENVELOPE extension

The proposed next extension can use the established full-row layout and control vocabulary for Juno's IDs5/6/7/8, rather than four independently invented mini-glyphs. The engine law must be Juno's. juno6.c is identical across all four refs; for normalized x=q/127:

- A=.002+3x_A² seconds. Each sample adds 1/max(1,A·Fs), clamps to1, enters decay: a linear rise. A new voice starting at0 reaches1 in about A seconds; a retrigger at a nonzero level has a shorter remaining rise.
- D=.01+3x_D² seconds. Each sample subtracts (1−S)/max(1,D·Fs) until S: a linear decay from1 to S. S=x_S; if S=1, the decay segment has no height and transitions immediately.
- R=.01+4x_R² seconds is the source release coefficient parameter. On release E[n+1]=E[n]·(1−1/max(1,R·Fs)); below1e−5 the voice retires. It is an exponential tail, not a linear fall or a fixed total duration. A tail beginning during attack uses that actual starting level, not an assumed sustain level.
- A schematic complete ADSR may explicitly assume note-off after sustain. Retain the existing fixed illustrative hold convention and minimum visible stage widths; do not label that hold an actual elapsed duration. Derive release extent from the source recurrence/threshold and chosen starting level, rather than Chicago's.04 threshold. No audio/performance execution is established here.

The existing generalized legacy helpers are not an active Juno renderer. adsr_fall(curved,…) selects linear versus exp(−5u); adsr_release() also contains a legacy gate recurrence. They are not called by current draw_adsr_inline(), which calls adsr_original_level() directly. The non-Chicago adsr_values() branch uses legacy BASS303 IDs/display_env_ms (disabled in the original-synth profile) and cannot be activated for Juno just by loosening adsr_row(). Generalize the family explicitly with the Juno law and correct IDs. Existing Chicago native input/curve/labels must continue through their established path.

Live markers remain runtime evidence. The new Juno schematic must not invent a live oscillator/LFO/ADSR cursor from wall time or reuse a Chicago-only native view as Juno evidence. An interactive proposal with no audio can show only an explicitly modeled stage/level fixture; a firmware live extension needs the corresponding admitted Juno source view.

## Existing vocabulary versus novel extension

Preserve current ui_pictograms and common control families (perform_glyph, filter_glyph, semantic_amount_glyph, control_glyph, original/dedicated drum glyphs) where their parameter semantics apply. Adapt layout/family deliberately with engine-specific source values; do not replace meaning with one generic regex curve. New Juno application of the full-row family is an extension, while Chicago's established graph, labels, readouts, selection behavior and common glyph vocabulary are reuse.

Current active original-engine matrix labels come first from ui_original_name(): matrix_name() returns that table before the following legacy FX-name switch. The dormant switch contains MIX/AMOUNT/FREQ/etc, but these exact pins do not prove it runs for the active original profile. Thus neither call those dynamic FX labels a new user benefit nor claim they are an active baseline from that unreachable legacy branch. Keep a version/artifact pin for any separate accepted dynamic-label treatment.

This comparison establishes source selection and unchanged functions, not current rendered pixels or acceptance. No old checks were rerun. Candidate writing and the single admitted reviewer remain with the parent routes.
