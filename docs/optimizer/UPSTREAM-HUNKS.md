# UPSTREAM-HUNKS — построчный review апстрим-изменений

Дата: 2026-11-06 (Asia/Vladivostok). Режим: только чтение. Ничего не запушено, PR и комментарии не создавались, компьютеры пользователя не использовались.

## Кто что сделал
- **Построчный review выполнил OpenAI Codex CLI** (`codex-cli 0.166.0`, `codex exec --skip-git-repo-check`). Было три параллельных фоновых прогона:
  - (1) 9W9 v2.7.0 → 8e9aebde → v2.7.1 → v2.7.2: `codex-hunks/out-9w9.md`, лог `run1.log`;
  - (2) 8W8 base 94aa2716 → v1.5.1, DSP-файлы: `codex-hunks/out-8w8.md`, лог `run2.log`;
  - (3) общие хелперы (`sc_ugens.h`→`w8_dsp.h`, `webaudio.h`, `9w9_circuit.h`, `perc909.h`, `tom909.h`) и сравнение с нашим кодом: `codex-hunks/out-helpers.md`, лог `run3.log`.
  - Промпты лежат в `codex-hunks/p1..p3.txt`.
- **Проверку выполнил я (Grok Bot).** До Codex я сам сделал нормализованный diff 9W9 и 8W8, а также golden-рендеры: 9W9 `render.c` — 12/12 WAV SAME, 8W8 `golden_check` — identical. Затем выборочно сверил заявления Codex с реальными исходниками `/tmp/cmp/*` (подробности ниже).

## Маппинг файлов upstream → наши
| upstream | наш файл |
|---|---|
| 9W9 `src/dsp/{er99,9w9}_circuit.h`, `_perc909.h`, `_tom909.h`, `_pots.h` | `firmware/src/dsp/drum909.c` |
| 9W9 `webaudio.h` | `drum909_dsp.h` |
| 9W9 `_engine.c/.h` | `drum909.c/.h` (формат состояния у нас свой) |
| 9W9 `_params.h`, `_plugin.c` | NONE |
| 8W8 `sc808_*_circuit.h`, `circuit_common.h`, `shape.h`, `fx.h`, `engine.cpp/.h` | `drum808.c/.h` (`fxbus.c` — только мастер-шина) |
| 8W8 `sc808_voices.h` (удалён; RimClave → `w8_rim.h` RimLane) | `drum808.c` (rim/clave lane) |
| 8W8 `sc_ugens.h` → `w8_dsp.h` | `drum808.c` (фильтры/Env), `fastmath.h` |
| 8W8 `params.h`, `plugin.cpp` | NONE |

## Итоги по вердиктам (подсчёт по строкам таблиц Codex)
| область | cosmetic | structure | bugfix | speedup | risk |
|---|---|---|---|---|---|
| 9W9 | 12 | 94 | 3 | 0 | 9 |
| 8W8 DSP | 110 | 7 | 0 | 0 | 0 |
| хелперы / сравнение с нашим кодом | 46 | 33 | 0 | 0 | 41 |

- **Применимых к нам speedup: 0. Применимых bugfix: 0.**
- Все 3 bugfix относятся к одному изменению: толерантность state-тега в v2.7.1 (`w9_state_tag_ok`, `9w9_engine.c:958` — функция, `:978` — вызов). К нам не применимо: у нас другой формат проекта.
- 9 risk в 9W9 — это строка Copyright/«GPL-3.0 with additional terms: see NOTICE», добавленная в v2.7.2 в каждый файл. Исполняемый код в них не меняется; юридическая сторона разобрана в UPSTREAM-TAKE.md.
- 41 risk в хелперах означает «наш порт численно отличается от апстрима»: float вместо double, LUT/полиномы, TDF-II delta-форма, предвычисленные обратные величины и т.п. Это не регрессии апстрима и не кандидаты на перенос. Их перенос изменил бы звук, а не дал бы ускорения.
- Scratch-ветка не создавалась, коммитов нет, бенч NOT_RUN: не было кандидатов, которые могли бы оказаться bit-identical.

## SuperCollider-код, заменённый в v1.5.1
- Шумовой генератор `RGen` → `NoiseGen`: та же taus88, те же сиды, та же формула `0x40000000u | (next32()>>9)` (`w8_dsp.h:62`), минус 3. Я проверил это вручную, Codex тоже.
- Фильтры LPF/HPF/Biquad: формулы коэффициентов и порядок вычислений не изменились. Удалены BPF, HiPass4, Limiter, shelf-фильтры, envPerc, nextHeld/peek и NULLTEST-хуки — всё это мёртвый код.
- Константы `kPi`/`kTwoPi`/`kSqrt2` (`w8_dsp.h:16-18`, double) не изменились.
- golden-вывод обеих версий 8W8 совпадает байт в байт.

## Что я проверил (выборка против реальных исходников)
- `9w9_engine.c`: `w9_state_tag_ok` — определение на :958, вызов на :978. **Поправка к Codex:** в его строках указаны :949/:959/:974. Это заголовки hunk'ов (`@@`), а не строки самих функций; суть описана верно.
- `9w9_circuit.h:1`: строка Copyright есть, verified.
- `9w9_circuit.h:106-110` (`w9_diode_round`, `tanhf(k*x)/tanhf(k)`): совпадает с описанием Codex, verified.
- `w8_dsp.h:16-18` и `:62`; наши `drum808.c:29-30` (`TWO_PI_SR`/`PI_SR` float): verified.
- Строки «cosmetic» в 8W8 выборочно совпали с моим более ранним нормализованным diff'ом (sc::→dsp::, frand2→white, переименования членов RimLane).
- Остальные номера строк в таблицах Codex (≈300 строк) я поштучно не проверял. Номера в хелперах указывают на диапазоны строк, а не на hunk'и. Поэтому считать их **UNVERIFIED по номеру строки**, хотя ни одно проверенное содержательное утверждение не оказалось ложным.
- Рабочее дерево `/workspace/opt-909/repo`: Codex ничего не изменил. Единственное изменение — mode-only у `tests/host/run_drum909_emit_equivalence.sh`, mtime 01:56, то есть ещё до запуска Codex в 02:29.

## Полные таблицы hunk'ов (Codex, без правок)

---
### out-9w9.md

# 9W9 DSP line-level upstream review

## Upstream -> firmware mapping

All firmware paths below are relative to `/workspace/opt-909/repo/firmware/src/dsp/`. A mapping names a semantic counterpart, not a bit-identical implementation. `NONE` means the changed facility has no counterpart in the assigned firmware DSP interface.

| Upstream base -> renamed path | Our file(s) and entry anchors |
|---|---|
| src/dsp/er99_circuit.h -> src/dsp/9w9_circuit.h | drum909.h:28 (BD/SD state), drum909.c:14 (constants), :525 (trigger), :708/:762 (render); drum909_dsp.h:234/:313 (shaping) |
| src/dsp/er99_engine.c -> src/dsp/9w9_engine.c | drum909.c:420/:586/:1020 (init/trigger/render), :927 (sample playback), :316/:389/:225 (parameters); fxbus.c:84/:240/:311/:430/:444 (delay/reverb/glue/mix). NONE for malloc WAV loading/free and upstream text-state codec. |
| src/dsp/er99_engine.h -> src/dsp/9w9_engine.h | drum909.h:21/:28/:46/:56/:66/:77/:94; fxbus.h:37. NONE for text-state tag/API. |
| src/dsp/er99_perc909.h -> src/dsp/9w9_perc909.h | drum909.h:56/:66; drum909.c:304/:298 (retune), :586 (trigger), :859/:890 (render) |
| src/dsp/er99_tom909.h -> src/dsp/9w9_tom909.h | drum909.h:46; drum909.c:420/:586/:803 |
| src/dsp/er99_pots.h -> src/dsp/9w9_pots.h | drum909.c:71 (voice descriptors), :233 (conversion), :389 (setter); fxbus.c:34/:108/:134 |
| src/dsp/webaudio.h -> same | drum909_dsp.h:79/:100/:126 (envelopes), :158/:185 (biquads), :196/:199 (phase/triangle), :207/:234/:313 (tanh/shapes), :321 (noise). NONE for legacy oversampled wa_shaper node. |
| src/dsp/er99_plugin.c -> src/dsp/9w9_plugin.c | NONE (Schwung host ABI, Move pad/MIDI mapping, dynamic JSON/UI and plugin instance lifecycle). Our drum909/fxbus APIs are integration boundaries, not this wrapper. |
| src/dsp/er99_params.h -> src/dsp/9w9_params.h | NONE for generated Schwung JSON; nearest parameter descriptors drum909.c:71 and fxbus.c:34 |
| Upstream master_glue/master rendering inside engine.c and master type inside engine.h | fxbus.c:98/:430/:444 and fxbus.h:61. Our master.c:1/:64/:215/:294 is a separate downstream compressor/filter/limiter, not a one-to-one copy of upstream glue. |

## Inputs, method, and limits

Hunk counts: 9pin -> 9ren: 106, 9ren -> 9v271: 3, 9v271 -> 9v272: 9; total **118**.

Reviewed chain: `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` (v2.7.0, 9pin) -> `8e9aebde0c40a4c017f640e5c30a4f70936bb595` (9ren) -> `25e03ddc33c2a4dfff702927061381a6e8bd6a3d` (v2.7.1, 9v271) -> `a9e030d9bad46e3e53aafce92c7104b8e7bba47f` (v2.7.2, 9v272). Histories are unrelated across repositories; comparison uses extracted trees, not ancestry. All 9 DSP blobs in each extracted tree (36 total) were verified against `git ls-tree -r <commit> -- src/dsp` using Git blob SHA-1 (`blob <length>\0<bytes>`). Git commands ran inside the respective upstream clones. No missing/new DSP files were silently omitted.

Firmware reviewed at current HEAD `263a055879f48919881f50812183aee3f4f2dc1e`. Its DSP files are clean; pre-existing modification `tests/host/run_drum909_emit_equivalence.sh` was left untouched. The authorized candidate base is `a18561f17977be8592cef5a3d3cbebf70e4dbf1b`; current drum909.c differs from that base (78 insertions, 23 deletions). Counterpart line numbers refer to current firmware, not a fabricated checkout at that base. No candidate met the requested creation condition.

Primary tables use unified diff with three context lines after exactly the requested textual normalization, also normalizing uppercase ER99->9W9. Paths er99_* become 9w9_*; content replacements: er99->9w9, sc808->w8, sc::->dsp::, RGen->NoiseGen, frand2->white, trand->next32, SinOsc->SineOsc, LFTri->TriOsc, LFPulse->PulseOsc. Actual renamed C identifiers are **w9_ / W9_**, not 9w9_ / 9W9_. These remaining identifier hunks are retained in the tables. A second token comparison aliases 9w9_/9W9_ to w9_/W9_ and removes comments/whitespace solely to distinguish renames from executable edits; it does not replace primary hunks. No line insertion/deletion occurs during normalization, so new-side line numbers remain the real source positions. Each row covers exactly one `-U3` hunk; ranges include context. The appendix preserves every primary hunk in full.

String normalization would hide `er99v3;` -> `9w9v3;`; raw originals were checked separately and this runtime format change is explicitly retained in the verdicts. No whole-upstream-vs-firmware PCM identity is asserted. `ALREADY` below describes the specific unchanged facility, never wholesale numerical equivalence of this port. Applies Y means an actionable upstream change to transplant into our assigned code; merely having a corresponding voice is insufficient.

## 9pin -> 9ren: every normalized hunk

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies Y/N | ALREADY same / differs | Notes |
|---|---|---|---|---|---|
| src/dsp/9w9_circuit.h:1 (`@@ -1,8 +1,8 @@`) | drum909.c:1 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Port provenance; upstream explanatory prose is abbreviated here. Comment/guard prose change has no executable effect. New-side cue: `* A plain "triangle + VCA envelope" voice gives thin toms and a kick with no`. |
| src/dsp/9w9_circuit.h:11 (`@@ -11,19 +11,19 @@`) | drum909.c:1 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Port provenance; upstream explanatory prose is abbreviated here. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_CIRCUIT_H`. |
| src/dsp/9w9_circuit.h:93 (`@@ -93,7 +93,7 @@`) | drum909.h:28 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | BD/SD runtime structure; integer phases/countdowns differ. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_bt_t;`. |
| src/dsp/9w9_circuit.h:102 (`@@ -102,7 +102,7 @@`) | drum909.c:32 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Diode helper: our LUT tanh and reciprocal multiplication differ from upstream libm division. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_diode_round(const float _x, const float _drive)`. |
| src/dsp/9w9_circuit.h:138 (`@@ -138,23 +138,23 @@`) | drum909.c:14 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same VCA/snappy constants; only upstream macro spelling changes. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_SNAPPY_MIX 1.95f`. |
| src/dsp/9w9_circuit.h:175 (`@@ -175,7 +175,7 @@`) | drum909_dsp.h:234 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Shape setup and d9_shape_other/d9_shape split; our coefficients are precomputed. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_shape_st(const float _x, const float _drive,`. |
| src/dsp/9w9_circuit.h:246 (`@@ -246,32 +246,32 @@`) | drum909.c:23 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same BD constants; shape helper is drum909_dsp.h:313 and initialization drum909.c:404. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `return w9_diode_round(_x, k);`. |
| src/dsp/9w9_circuit.h:298 (`@@ -298,14 +298,14 @@`) | drum909.c:525 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | BD/SD init/trigger port; float/integer time and phase representation differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `v->bd_base = W9_BD_BASE_HZ;`. |
| src/dsp/9w9_circuit.h:329 (`@@ -329,10 +329,10 @@`) | drum909.c:539 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | BD sweep constants and operations; no upstream expression changed. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `v->bd_base  = W9_BD_BASE_HZ * (1.0f + m * (pm - 1.0f));`. |
| src/dsp/9w9_circuit.h:370 (`@@ -370,7 +370,7 @@`) | drum909.c:708 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | BD render; SD counterpart drum909.c:762; block loops differ. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_bt_render(w9_bt_t *v, const float _noise)`. |
| src/dsp/9w9_circuit.h:394 (`@@ -394,7 +394,7 @@`) | drum909.c:18 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same soft VCA formula factored as d9_vca; called from SD render. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `amp = amp > W9_VCA_VT ? (amp - W9_VCA_VT) * W9_VCA_NORM : 0.0f;`. |
| src/dsp/9w9_circuit.h:408 (`@@ -408,7 +408,7 @@`) | drum909.c:32 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Fixed 1.6 diode helper used by BD; LUT/reciprocal port already exists. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `o = w9_diode_round(o, 1.6f);`. |
| src/dsp/9w9_circuit.h:421 (`@@ -421,13 +421,13 @@`) | drum909.c:762 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | SD oscillator mix and shape; fixed diode helper drum909.c:33. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `o = w9_diode_round(o, 2.0f);`. |
| src/dsp/9w9_circuit.h:454 (`@@ -454,15 +454,15 @@`) | drum909.c:762 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | SD noise gate/snappy path; same constants at drum909.c:14 and :23. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `env = env > W9_VCA_VT ? (env - W9_VCA_VT) * W9_VCA_NORM : 0.0f;`. |
| src/dsp/9w9_engine.c:1 (`@@ -1,10 +1,5 @@`) | drum909.c:1 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Provenance comment only. Comment/guard prose change has no executable effect. New-side cue: `* 9w9_engine.c — 9W9 drum voices, sample playback, sends and master bus.`. |
| src/dsp/9w9_engine.c:11 (`@@ -16,7 +11,7 @@`) | drum909.h:21 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | DR_* identifiers replace upstream trigger names; ordering of sampler IDs differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `const char *const w9_trigger_names[W9_NUM_TRIGGERS] = {`. |
| src/dsp/9w9_engine.c:21 (`@@ -26,7 +21,7 @@`) | NONE | cosmetic | N | differs: upstream-only integration/metadata | No upstream-style WAV allocator/loader in these DSP files; our samples use flash IMA ADPCM (drum909.c:927). Comment/guard prose change has no executable effect. New-side cue: `/* WAV loading (mono 16/24-bit PCM)                                    */`. |
| src/dsp/9w9_engine.c:99 (`@@ -104,17 +99,17 @@`) | drum909.c:420 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Engine initialization; FX initialization is fxbus.c:161; delay retime fxbus.c:84. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static void w9_dly_retime(w9_dly_t *d, float _sr);`. |
| src/dsp/9w9_engine.c:174 (`@@ -179,13 +174,13 @@`) | drum909.c:404 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Voice initialization split across d9_bt_init and drum909_init. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_bt_init(b, _sr);`. |
| src/dsp/9w9_engine.c:196 (`@@ -201,9 +196,9 @@`) | drum909.c:420 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim/clap initialization in our combined initializer. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_rim909_init(r9, _sr);`. |
| src/dsp/9w9_engine.c:208 (`@@ -213,20 +208,20 @@`) | drum909.c:420 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Sampler defaults and clap initialization; flash sample representation differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_clap909_init(c9, _sr);`. |
| src/dsp/9w9_engine.c:235 (`@@ -240,9 +235,9 @@`) | drum909.c:420 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Hat data setup; our sampler uses IMA flash bytes, not malloc/free. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `s->buffer = e->sample_data[W9_SAMP_OHH];`. |
| src/dsp/9w9_engine.c:258 (`@@ -263,7 +258,7 @@`) | fxbus.c:84 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Delay retime; our capacity-dependent beat-halving differs from upstream clamp. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_dly_retime(&e->dly, _sr);`. |
| src/dsp/9w9_engine.c:279 (`@@ -284,12 +279,12 @@`) | drum909.c:420 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Initialization exists; NONE for free routine (static/flash storage). Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_seed_pots(e);`. |
| src/dsp/9w9_engine.c:296 (`@@ -301,7 +296,7 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Trigger dispatch; our API takes voice/float velocity. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `void w9_engine_trigger(w9_engine_t *e, const w9_trigger_t _which, const int _velocity)`. |
| src/dsp/9w9_engine.c:326 (`@@ -331,7 +326,7 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Analog trigger dispatch and pinned circuitry. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `case W9_BD: case W9_SD: case W9_LT: case W9_MT: case W9_HT:`. |
| src/dsp/9w9_engine.c:337 (`@@ -342,9 +337,9 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | BD fixed circuitry trigger path. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `if(_which == W9_BD)`. |
| src/dsp/9w9_engine.c:351 (`@@ -356,16 +351,16 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Clap and SD fixed circuitry trigger paths. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `if(_which == W9_HC)`. |
| src/dsp/9w9_engine.c:376 (`@@ -381,52 +376,52 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | SD/tom/rim/clap/sampler dispatch; renamed enums and helper calls only. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_set_raw(e, "sd_c_noise_hp", 1000.0f);`. |
| src/dsp/9w9_engine.c:443 (`@@ -448,7 +443,7 @@`) | drum909.c:927 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Sample playback uses ADPCM and fixed-point positions. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static float render_sampler(w9_sampler_t *s)`. |
| src/dsp/9w9_engine.c:461 (`@@ -466,15 +461,15 @@`) | drum909.c:927 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Sample distortion counterpart; reverb constants fxbus.c:7, reverb fxbus.c:240. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `v = w9_shape_st(v * (1.0f + s->drive * 0.5f), s->drive, s->dist_type, s->crush_st);`. |
| src/dsp/9w9_engine.c:479 (`@@ -484,7 +479,7 @@`) | fxbus.c:240 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Comb loop; same lengths at fxbus.c:7, int16 storage/block processing differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `const int n = W9_VERB_CL[i];`. |
| src/dsp/9w9_engine.c:495 (`@@ -500,7 +495,7 @@`) | fxbus.c:240 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Allpass loop; same left lengths at fxbus.c:8, extra stereo path exists. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `const int n = W9_VERB_AL[j];`. |
| src/dsp/9w9_engine.c:506 (`@@ -511,25 +506,25 @@`) | fxbus.c:84 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Delay retime; beats at fxbus.c:13; capacity policy differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static const float w9_dly_beats[W9_DLY_DIVS] = {`. |
| src/dsp/9w9_engine.c:532 (`@@ -537,13 +532,13 @@`) | fxbus.c:311 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Delay interpolation/wrap guards; guards were ALREADY in upstream base, not new bugfixes. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `while(rp < 0.0f) rp += (float)W9_DLY_MAX;`. |
| src/dsp/9w9_engine.c:549 (`@@ -554,7 +549,7 @@`) | fxbus.c:311 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Delay write-pointer wrap; upstream only renamed capacity macro. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `if(++d->w >= W9_DLY_MAX) d->w = 0;`. |
| src/dsp/9w9_engine.c:559 (`@@ -564,7 +559,7 @@`) | fxbus.c:430 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Glue gain counterpart plus fx_comp_prep at :98 and process at :444; master.c:215 is a separate compressor. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static float master_glue(w9_master_t *m, const float _in, const float _sr)`. |
| src/dsp/9w9_engine.c:614 (`@@ -619,35 +614,35 @@`) | drum909.c:1020 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Block voice dispatch; FX mixing is fxbus.c:444; upstream loops are unchanged after renaming. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `void w9_engine_render(w9_engine_t *e, float *out, const int frames)`. |
| src/dsp/9w9_engine.c:653 (`@@ -658,11 +653,11 @@`) | fxbus.c:444 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Send FX plus master distortion/glue chain. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `mix += w9_verb_tick(&e->verb, rbus);`. |
| src/dsp/9w9_engine.c:673 (`@@ -678,14 +673,14 @@`) | drum909.c:316 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Field setter is explicit enum dispatch, no upstream offsetof table. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define BTF(n) { #n, offsetof(w9_bt_t, n) }`. |
| src/dsp/9w9_engine.c:694 (`@@ -699,7 +694,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | No upstream text-state key inventory in our DSP. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_OTHER_KEY_COUNT (sizeof(g_other_state_keys)/sizeof(g_other_state_keys[0]))`. |
| src/dsp/9w9_engine.c:709 (`@@ -714,11 +709,11 @@`) | drum909.c:316 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Raw setter counterpart; send values stored in drum909.h:100. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_SEND_KEYS ((int)(sizeof(g_send_keys)/sizeof(g_send_keys[0])))`. |
| src/dsp/9w9_engine.c:732 (`@@ -737,14 +732,14 @@`) | fxbus.c:108 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Delay parameter setter and retime. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `: (value > W9_DLY_DIVS - 1 ? W9_DLY_DIVS - 1 : value);`. |
| src/dsp/9w9_engine.c:752 (`@@ -757,13 +752,13 @@`) | drum909.c:316 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Voice field update; numeric API replaces string lookup. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)`. |
| src/dsp/9w9_engine.c:772 (`@@ -777,29 +772,29 @@`) | drum909.c:304 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim/clap retune and d9_apply (:316); sampler fields handled in d9_apply. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_rim909_t *r9 = &e->rim909; w9_clap909_t *c9 = &e->clap909;`. |
| src/dsp/9w9_engine.c:827 (`@@ -832,9 +827,9 @@`) | drum909.c:225 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Pot getter; NONE for a public upstream-style raw string getter. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `int w9_engine_get_raw(const w9_engine_t *e, const char *key, float *out)`. |
| src/dsp/9w9_engine.c:846 (`@@ -851,20 +846,20 @@`) | drum909.c:225 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Pot getter; raw string lookup not implemented by this DSP interface. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)`. |
| src/dsp/9w9_engine.c:879 (`@@ -884,7 +879,7 @@`) | drum909.c:225 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same limitation: getter surface differs. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `const w9_rim909_t *r9 = &e->rim909; const w9_clap909_t *c9 = &e->clap909;`. |
| src/dsp/9w9_engine.c:918 (`@@ -923,7 +918,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | No semicolon-separated upstream text-state serializer. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `int w9_engine_get_state(const w9_engine_t *e, char *buf, const int len)`. |
| src/dsp/9w9_engine.c:927 (`@@ -932,29 +927,29 @@`) | NONE | structure | N | differs: NONE for upstream text-state writer/reader | State writer: actual old literal er99v3; becomes W9_STATE_TAG = 9w9v3;. Runtime bytes DO change. New tag is six bytes, old tag seven; sizeof-minus-one uses correct individual prefix lengths. This is format compatibility, not render optimization. New-side cue: `n += snprintf(buf + n, (size_t)(len - n), W9_STATE_TAG);`. |
| src/dsp/9w9_engine.c:959 (`@@ -964,7 +959,8 @@`) | NONE | bugfix | N | differs: NONE for upstream text-state writer/reader | State loader preserves old er99v3; and accepts new 9w9v3; (lengths 7 and 6). No counterpart in our DSP. New tag is six bytes, old tag seven; sizeof-minus-one uses correct individual prefix lengths. This is format compatibility, not render optimization. New-side cue: `if(strncmp(blob, W9_STATE_TAG, sizeof(W9_STATE_TAG) - 1) != 0 &&`. |
| src/dsp/9w9_engine.c:977 (`@@ -981,7 +977,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Text-state key parser calls renamed setter; our API is numeric. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_set_param(e, seg, (float)atof(eq + 1));`. |
| src/dsp/9w9_engine.c:1003 (`@@ -1007,46 +1003,46 @@`) | drum909.c:389 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Pot set/init/get counterparts at :389/:420/:225; lookup uses indices and precomputed curves. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `for(int i=0; i<W9_POT_COUNT; ++i)`. |
| src/dsp/9w9_engine.h:1 (`@@ -1,14 +1,11 @@`) | drum909.h:1 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Engine provenance/include guards; our header uses pragma once. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_ENGINE_H`. |
| src/dsp/9w9_engine.h:15 (`@@ -18,24 +15,24 @@`) | drum909.h:21 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Voice IDs; sampler ID order differs, so do not transplant enums. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `W9_BD = 0, W9_SD, W9_LT, W9_MT, W9_HT,`. |
| src/dsp/9w9_engine.h:49 (`@@ -52,7 +49,7 @@`) | drum909.h:77 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Sampler shape fields exist; representation differs. Comment/guard prose change has no executable effect. New-side cue: `float  drive;         /* per-sampler distortion (w9_shape) */`. |
| src/dsp/9w9_engine.h:59 (`@@ -62,7 +59,7 @@`) | drum909.h:77 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Sampler type rename only. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_sampler_t;`. |
| src/dsp/9w9_engine.h:71 (`@@ -74,18 +71,18 @@`) | fxbus.h:37 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | FX runtime structure; capacity is supplied, delay data is int16. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_DLY_MAX 88200            /* 2 s at 44.1k: a dotted half at 90 BPM */`. |
| src/dsp/9w9_engine.h:92 (`@@ -95,20 +92,18 @@`) | fxbus.h:37 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Reverb/master types; comments do not remove compressor code; independent master.c:1 has compressor/filter/limiter. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_verb_t;`. |
| src/dsp/9w9_engine.h:128 (`@@ -133,64 +128,66 @@`) | drum909.h:94 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Aggregate engine struct is split across drum909/fxbus; NONE for text-state API/tag macros. Adds W9_STATE_TAG="9w9v3;" and W9_STATE_TAG_OLD="er99v3;"; string bytes must not be treated as cosmetic renames. New-side cue: `} w9_master_t;`. |
| src/dsp/9w9_params.h:1 (`@@ -1,10 +1,10 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Generated Schwung chain-parameter JSON; our descriptors at drum909.c:71 and fxbus.c:34 serve a different API. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_PARAMS_H`. |
| src/dsp/9w9_params.h:86 (`@@ -86,8 +86,8 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung UI hierarchy JSON; no matching DSP JSON. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_UI_HIERARCHY_LEN 6147`. |
| src/dsp/9w9_params.h:153 (`@@ -153,8 +153,8 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | GM UI hierarchy JSON; no matching DSP JSON. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#define W9_UI_HIERARCHY_GM_LEN 6147`. |
| src/dsp/9w9_perc909.h:1 (`@@ -1,13 +1,5 @@`) | drum909.c:1 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Deleted historical rim/clap commentary; no code deletion. Comment/guard prose change has no executable effect. New-side cue: `deletion`. |
| src/dsp/9w9_perc909.h:18 (`@@ -26,11 +18,11 @@`) | drum909_dsp.h:313 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Include guard/include helper name; pragma once in our DSP header. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_PERC909_H`. |
| src/dsp/9w9_perc909.h:42 (`@@ -50,9 +42,9 @@`) | drum909.h:56 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim type and init; our initialization drum909.c:420. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_rim909_t;`. |
| src/dsp/9w9_perc909.h:62 (`@@ -70,13 +62,13 @@`) | drum909.c:298 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim retune; trigger is in drum909_trigger (:586). Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline void w9_rim909_retune(w9_rim909_t *v)`. |
| src/dsp/9w9_perc909.h:78 (`@@ -86,7 +78,7 @@`) | drum909.c:859 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim block render; ring shortcut exists in our code, absent from these upstream changes. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_rim909_render(w9_rim909_t *v, const float _noise)`. |
| src/dsp/9w9_perc909.h:105 (`@@ -113,7 +105,7 @@`) | drum909_dsp.h:313 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Rim shape call counterpart in drum909.c:859; symbol rename only. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `y = w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);`. |
| src/dsp/9w9_perc909.h:130 (`@@ -138,9 +130,9 @@`) | drum909.h:66 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Clap type/init; init drum909.c:420. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_clap909_t;`. |
| src/dsp/9w9_perc909.h:145 (`@@ -153,12 +145,12 @@`) | drum909.c:304 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Clap retune and trigger (:586). Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline void w9_clap909_retune(w9_clap909_t *v)`. |
| src/dsp/9w9_perc909.h:164 (`@@ -172,7 +164,7 @@`) | drum909.c:890 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Clap block render. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_clap909_render(w9_clap909_t *v, const float _noise)`. |
| src/dsp/9w9_perc909.h:197 (`@@ -205,9 +197,9 @@`) | drum909.c:920 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Clap shape and output path; renamed call only. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `y = w9_shape_st(y, v->drive, v->dist_type, v->crush_st);`. |
| src/dsp/9w9_plugin.c:1 (`@@ -1,5 +1,5 @@`) | NONE | cosmetic | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Comment/guard prose change has no executable effect. New-side cue: `* 9w9_plugin.c — Schwung plugin_api_v2 wrapper for the 9W9 engine.`. |
| src/dsp/9w9_plugin.c:20 (`@@ -20,7 +20,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_t engine;`. |
| src/dsp/9w9_plugin.c:39 (`@@ -39,7 +39,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_instance_t;`. |
| src/dsp/9w9_plugin.c:61 (`@@ -61,14 +61,14 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `case 68: return W9_BD;   case 69: return W9_SD;`. |
| src/dsp/9w9_plugin.c:91 (`@@ -91,12 +91,12 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `case 36: return W9_BD;   case 37: return W9_SD;`. |
| src/dsp/9w9_plugin.c:115 (`@@ -115,17 +115,17 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `case 36: return W9_BD;   case 35: return W9_BD;`. |
| src/dsp/9w9_plugin.c:136 (`@@ -136,7 +136,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_instance_t *inst = (w9_instance_t*)calloc(1, sizeof(w9_instance_t));`. |
| src/dsp/9w9_plugin.c:145 (`@@ -145,7 +145,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_init(&inst->engine, sr, _module_dir);`. |
| src/dsp/9w9_plugin.c:154 (`@@ -154,15 +154,15 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_instance_t *inst = (w9_instance_t*)_instance;`. |
| src/dsp/9w9_plugin.c:185 (`@@ -185,7 +185,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_trigger(&inst->engine, (w9_trigger_t)t, (int)vel);`. |
| src/dsp/9w9_plugin.c:201 (`@@ -201,7 +201,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_instance_t *inst = (w9_instance_t*)_instance;`. |
| src/dsp/9w9_plugin.c:226 (`@@ -226,7 +226,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_set_state(&inst->engine, _val);`. |
| src/dsp/9w9_plugin.c:234 (`@@ -234,21 +234,21 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_set_param(&inst->engine, _key, (float)atof(_val));`. |
| src/dsp/9w9_plugin.c:279 (`@@ -279,9 +279,9 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `const char *const j = gm ? w9_ui_hierarchy_gm_json`. |
| src/dsp/9w9_plugin.c:304 (`@@ -304,11 +304,11 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static const char *const level_of[W9_NUM_TRIGGERS] = {`. |
| src/dsp/9w9_plugin.c:324 (`@@ -324,7 +324,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `int n = w9_engine_get_state(&inst->engine, _buf, _len);`. |
| src/dsp/9w9_plugin.c:336 (`@@ -336,7 +336,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `if(w9_engine_get_param(&inst->engine, _key, &v))`. |
| src/dsp/9w9_plugin.c:350 (`@@ -350,7 +350,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_instance_t *inst = (w9_instance_t*)_instance;`. |
| src/dsp/9w9_plugin.c:364 (`@@ -364,7 +364,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_set_raw(&inst->engine, "dly_bpm", bpm);`. |
| src/dsp/9w9_plugin.c:377 (`@@ -377,7 +377,7 @@`) | NONE | structure | N | differs: upstream-only integration/metadata | Schwung plugin ABI wrapper does not exist in our firmware; drum909/fxbus numeric APIs replace this integration. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `w9_engine_render(&inst->engine, mono, chunk);`. |
| src/dsp/9w9_pots.h:13 (`@@ -13,195 +13,195 @@`) | drum909.c:71 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Pot tables and conversion; fxbus.c:34 for globals; d9_pot_value (:233) uses existing LUTs instead of powf. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_POTS_H`. |
| src/dsp/9w9_pots.h:211 (`@@ -211,4 +211,4 @@`) | drum909.h:16 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Guard-only rename; pragma once in our header. Comment/guard prose change has no executable effect. New-side cue: `#endif /* W9_POTS_H */`. |
| src/dsp/9w9_tom909.h:41 (`@@ -41,16 +41,16 @@`) | drum909.c:803 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Tom ratios in block render; same 1/1.5/2.75 constants; guards differ. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_TOM909_H`. |
| src/dsp/9w9_tom909.h:64 (`@@ -64,9 +64,9 @@`) | drum909.h:46 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Tom struct/init in drum909_init (:420). Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `} w9_tom_t;`. |
| src/dsp/9w9_tom909.h:93 (`@@ -93,7 +93,7 @@`) | drum909.c:586 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Tom trigger nested in our dispatcher. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline void w9_tom909_trigger(w9_tom_t *t, const w9_bt_t *p,`. |
| src/dsp/9w9_tom909.h:131 (`@@ -131,7 +131,7 @@`) | drum909.c:803 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Tom render function signature rename. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `static inline float w9_tom909_render(w9_tom_t *t, const w9_bt_t *p,`. |
| src/dsp/9w9_tom909.h:142 (`@@ -142,20 +142,20 @@`) | drum909.c:803 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Tom render: same partial ratios; block loops and float arithmetic substrate differ. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `o += w9_diode_round(wa_osc_triangle(&t->osc[0], f * W9_TOM_R1), 2.0f)`. |
| src/dsp/webaudio.h:1 (`@@ -1,21 +1,19 @@`) | drum909_dsp.h:1 | structure | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Provenance/guard only; port explicitly documents arithmetic differences. Alias-only token equality verified; no load, branch, clamp, constant, or evaluation-order change. New-side cue: `#ifndef W9_WEBAUDIO_H`. |
| src/dsp/webaudio.h:23 (`@@ -25,15 +23,15 @@`) | drum909_dsp.h:49 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same 1e-9 envelope floor; ramp/tick at :100/:126 differ from closed-form upstream. Comment/guard prose change has no executable effect. New-side cue: `/* Web Audio clamps exponential ramp endpoints away from zero; envelopes rely`. |
| src/dsp/webaudio.h:163 (`@@ -165,8 +163,8 @@`) | drum909_dsp.h:158 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Biquad coefficient computation differs (fm sine/cos identity); upstream arithmetic unchanged. Comment/guard prose change has no executable effect. New-side cue: `* bandpass/notch/peaking — the voices leave Q at default (1) for LP/HP/notch`. |
| src/dsp/webaudio.h:262 (`@@ -264,7 +262,7 @@`) | NONE | cosmetic | N | differs: upstream-only integration/metadata | Legacy wa_shaper oversampling curve is not our seven-mode d9_shape helper; header comment change only. Comment/guard prose change has no executable effect. New-side cue: `* Saturation curve:`. |
| src/dsp/webaudio.h:364 (`@@ -366,7 +364,7 @@`) | drum909_dsp.h:321 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Same xorshift32 recurrence; float formula discussed below; only upstream comment changes. Comment/guard prose change has no executable effect. New-side cue: `/* White noise, uniform in [-1, 1)                                        */`. |
| src/dsp/webaudio.h:383 (`@@ -385,4 +383,4 @@`) | drum909_dsp.h:20 | cosmetic | N | ALREADY has counterpart; port representation differs; no changed DSP arithmetic | Include guard close only; pragma once in our header. Comment/guard prose change has no executable effect. New-side cue: `#endif /* W9_WEBAUDIO_H */`. |

## 9ren -> 9v271: every normalized hunk

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies Y/N | ALREADY same / differs | Notes |
|---|---|---|---|---|---|
| src/dsp/9w9_engine.c:949 (`@@ -949,6 +949,21 @@`) | NONE | bugfix | N | differs: no upstream text-state loader/tag API | Adds compatibility helper: exact current prefix, or first segment length 3..8 ending v3 without equals; broad acceptance described below. New-side cue: `static int w9_state_tag_ok(const char *blob)`. |
| src/dsp/9w9_engine.c:974 (`@@ -959,8 +974,7 @@`) | NONE | bugfix | N | differs: no upstream text-state loader/tag API | Replaces two fixed-prefix checks with w9_state_tag_ok. No voice, filter, noise, or FX arithmetic changes. New-side cue: `if(!w9_state_tag_ok(blob))`. |
| src/dsp/9w9_engine.h:181 (`@@ -181,10 +181,10 @@`) | NONE | structure | N | differs: no upstream text-state loader/tag API | Deletes old-tag macro and updates compatibility comment; helper now recognizes old er99v3 generically. New-side cue: `/* State blob tag. Blobs saved by v2.7.0 and earlier carry an older v3 tag`. |

## 9v271 -> 9v272: every normalized hunk

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies Y/N | ALREADY same / differs | Notes |
|---|---|---|---|---|---|
| src/dsp/9w9_circuit.h:1 (`@@ -1,3 +1,4 @@`) | drum909.c:1 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_engine.c:1 (`@@ -1,3 +1,4 @@`) | drum909.c:1 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_engine.h:1 (`@@ -1,3 +1,4 @@`) | drum909.h:1 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_params.h:1 (`@@ -1,3 +1,4 @@`) | NONE | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_perc909.h:1 (`@@ -1,3 +1,4 @@`) | drum909.c:1 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_plugin.c:1 (`@@ -1,3 +1,4 @@`) | NONE | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_pots.h:1 (`@@ -1,3 +1,4 @@`) | drum909.c:71 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/9w9_tom909.h:1 (`@@ -1,3 +1,4 @@`) | drum909.c:803 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |
| src/dsp/webaudio.h:1 (`@@ -1,3 +1,4 @@`) | drum909_dsp.h:1 | risk | N | differs: our files retain GPL-3.0-only SPDX/provenance | Adds copyright/additional-terms NOTICE reference only. Zero executable tokens changed. Legal compatibility/meaning of added terms UNKNOWN in this technical review; no code imported. New-side cue: `/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */`. |

## State compatibility: raw-source review

The v2.7.0 writer at er99_engine.c:935 writes `"er99v3;"`; its reader at :967 checks seven bytes of that prefix. Rename-commit 9w9_engine.c:930 writes `W9_STATE_TAG`; 9w9_engine.h:186/:187 define `"9w9v3;"` and `"er99v3;"`. The reader at 9w9_engine.c:962/:963 accepts either, comparing `sizeof(tag)-1` bytes. This preserves existing patches while changing newly written tags. It is not just a source rename, and normalizing string contents must not erase it.

v2.7.1 9w9_engine.c:957-965 adds `w9_state_tag_ok`, called at :977. For non-null strings it accepts the current `9w9v3;` prefix; otherwise it locates the first semicolon, requires its offset 3..8, checks the preceding two bytes equal `v3`, and rejects equals signs in that first segment. `er99v3;` and other short names pass. The short-circuit length guard protects `semi-2`. Null input is rejected in the caller before invoking the helper. Missing semicolon, too-short/long segment, wrong suffix, and equals-containing tag are rejected. Example `abcv3;volume=...` is accepted regardless of whether `abc` is a known historical name: **risk** of intentionally broad format acceptance, not memory corruption established by this hunk. Whether every such vendor's v3 pot layout has identical meaning is **UNKNOWN**; source comments alone do not prove it. Accepted key/value parsing is unchanged.

Our drum909.h:112 onward exposes init/trigger/render/get/set, not engine_get_state/engine_set_state; drum909.c:225/:389 get/set numeric pots. A literal/function-name search across firmware finds no `er99v3`, `9w9v3`, `W9_STATE_TAG`, or upstream engine_set_state/get_state integration. Consequently this upstream compatibility bugfix does not apply to our assigned DSP files. No parser was invented to make it applicable.

## Hidden speedups and numerical checks

- After the actual symbol aliases and comment/whitespace removal, the entire circuit, percussion, tom, pots, generated params, plugin and webaudio executable token streams match at 9pin->9ren. Engine.c/header differ only in the state writer/reader/macros described above. 9ren->9v271 changes only that state predicate and header declaration/comment; 9v271->9v272 has identical executable tokens in every one of nine files. Therefore no upstream render loads were hoisted, branches/clamps removed, loops reordered, LUTs introduced, constants changed, or float expression evaluation order changed in this chain. Existing protective delay wrap checks and truncation-to-zero quantization are already present in v2.7.0, not discoveries added by later releases.
- **Noise:** 9ren webaudio.h:369-383 (v2.7.2 :370-384) is still xorshift32, shifts `13,17,5`, zero-seed fallback `0x12345678u`; engine initialization seed remains `0xC0FFEEu`. Output remains `(float)((double)state / 2147483648.0) - 1.0f`. Our drum909_dsp.h:321-329 uses the same recurrence, then `(float)x * (1.0f / 2147483648.0f) - 1.0f`; drum909.c:997 advances it once per generated sample. Under IEEE binary32/binary64 round-to-nearest arithmetic, conversion/division by this exact power of two commute here: scaling uint32 by 2^-31 stays normal and cannot overflow, so intermediate rounding gives the same scaled binary32; subtraction is the same float operation. This is an algebraic observation, not an executed exhaustive state/PCM memcmp, and says nothing about whole-port phase/envelope/filter identity. The upstream comment “[-1,1)” is not a strict float bound: high uint32 values round to 2^32 at float conversion, so the implementation can output +1.0f. This is pre-existing and unchanged. No SuperCollider RGen replacement occurs anywhere in these nine 9W9 DSP files.
- **Filter coefficients:** upstream wa_biquad_set at 9v272 webaudio.h:170 uses `w0=(float)M_PI*fc`, `cosf(w0)`, `sinf(w0)`, `alpha=sinw0/(2*q)`, then reciprocal-a0 multiplication. Frequency clamps 0.0001/0.9999 and Q floor 0.0001 persist unchanged. Our drum909_dsp.h:158-180 uses `fm_sinf` and `1-2*sin(w0/2)^2` for cosine, fixed D9_SR, and supports LP/HP/BP. Coefficients and libm rounding are **not proven bit-identical** to upstream. The comment-only Web Audio edits do not replace those formulas. Our direct-form-1 tick at :185 keeps the familiar recurrence; no new upstream filter speedup exists to import.
- **Envelope/phase/shaping:** upstream closed-form powf ramp, double oscillator phase and floating sample position are unchanged; our drum909_dsp.h:100/:126/:140 uses recursive ramps with block re-anchoring, :196/:199 uses uint32 phase, drum909.c:927 uses fixed-point/ADPCM sample playback, and :207 uses tanh LUT interpolation. Our d9_shape_prep at :234 already prepares shape constants; circuit.h's renamed functions still compute their old expressions. These are existing port differences, not evidence of an upstream bit-identical optimization. Whole-port bit identity is **UNKNOWN and not claimed**.
- **Bus/master:** reverb lengths 1116/1188/1277/1356 and allpasses 556/441, 13 beat divisions, delay wrap corrections, quantization and master_glue arithmetic remain unchanged upstream. Our fxbus.c:7/:13/:84/:240/:311/:430 uses a different block/storage implementation, cached glue parameters and extra stereo/tape paths; master.c:64/:215/:294 adds an independent downstream stage. Comment removal about the historic Web Audio compressor does not delete the actual master_glue implementation.

## Candidate, tests, and benchmark decision

No applicable bit-identical speedup or bugfix was found, so the user's **if and only if** condition forbids making a scratch worktree/branch/commit. None was made. No source edits, network Git, pushes, posts, device or user-computer actions were performed.

Host emit equivalence, integral, 808 tests, PCM/state memcmp and performance benchmark: **NOT RUN**, because there is no candidate. No passing equivalence claim or invented timing is made. `/workspace/dnbox/optimizer/909-bench/run_interleaved.sh` was read: it rotates base/tip/tipm1 order over N rounds (default 9), pins CPU 2, executes prebuilt worstcase binaries in three specific existing worktrees and writes per-round JSON. Those binaries are not a new candidate off the requested base; running them here would not measure this review's changes. Peak, mean, median, min and noise: **N/A (no candidate measurements)**. Whether a speedup is within noise: **N/A**, not zero or a claim of significance.

## Complete normalized -U3 evidence

The following retains every primary hunk, including rename-only rows. Raw state literals and their changed semantics are documented above because the requested textual er99 normalization masks their spelling. Each pair uses its own new-side line numbers, not the v2.7.2 line numbers for earlier hunks.

```diff
--- 9pin/src/dsp/9w9_circuit.h
+++ 9ren/src/dsp/9w9_circuit.h
@@ -1,8 +1,8 @@
 /*
  * 9w9_circuit.h — circuit-informed TR-909 voice models (BD / SD / toms).
  *
- * er-99's oscillator voices are "triangle + VCA envelope", which is why its
- * toms sound thin and its kick lacks attack. The real 909 analog voice is:
+ * A plain "triangle + VCA envelope" voice gives thin toms and a kick with no
+ * attack. The real 909 analog voice is:
  *
  *   - an oscillator producing a TRIANGLE that is rounded toward a SINE by
  *     back-to-back diodes conducting ~0.5-0.6 V either side of ground,
@@ -11,19 +11,19 @@
  *   - an AMPLITUDE envelope (Decay),
  *   - and a separate CLICK path — a short impulse plus lowpass-filtered noise,
  *     summed and given their own fast envelope (the Attack control). This is
- *     the beater contact, and it is what er-99 is missing entirely.
+ *     the beater contact.
  *
  * The snare adds a second tuned oscillator and a noise path with its own
  * envelope and highpass (Snappy).
  *
- * Sources: Network-909 circuit analysis (cited by er-99 itself), Roland
+ * Sources: Network-909 circuit analysis, Roland
  * TR-909 service documentation, and the sound-mod analysis at firstpr.com.au.
  *
  * GPL-3.0.
  */
 
-#ifndef 9W9_CIRCUIT_H
-#define 9W9_CIRCUIT_H
+#ifndef W9_CIRCUIT_H
+#define W9_CIRCUIT_H
 
 #include "webaudio.h"
 
@@ -93,7 +93,7 @@
     int         impulse;        /* samples of initial impulse left */
     double      mute_countdown;
     float       sample_rate;
-} 9w9_bt_t;
+} w9_bt_t;
 
 /*
  * Back-to-back diode rounding. A triangle wave's sharp peaks are what make it
@@ -102,7 +102,7 @@
  * model of that pair. drive ~2 reproduces the 909's rounded triangle; higher
  * values push it back toward a harder, more aggressive tone.
  */
-static inline float 9w9_diode_round(const float _x, const float _drive)
+static inline float w9_diode_round(const float _x, const float _drive)
 {
     const float k = _drive > 0.01f ? _drive : 0.01f;
     return tanhf(k * _x) / tanhf(k);
@@ -138,23 +138,23 @@
  * preset is quieter still on the noise leg (0.12 against his hardware's 0.18
  * at 5 ms) — its Snappy was evidently set below noon, so the hardware stays
  * the reference. */
-#define 9W9_SNAPPY_MIX 1.95f
+#define W9_SNAPPY_MIX 1.95f
 
 /* Turn-on voltage of the single-transistor VCAs. Fitted, not assumed: at 0.027
  * the snare's shells were cut off entirely by 100 ms where the real drum still
  * has them at -30 dB and audible to 150. Fitted again once the shell decay was
  * measured directly (tau 28 ms, not 36): at 0.005 the tail still gated off
  * before 150 ms where the references are at -39 dB and running to -46 at
- * 200 ms. Overridable so the fitting rig can sweep it (-D9W9_VCA_VT=...).
+ * 200 ms. Overridable so the fitting rig can sweep it (-DW9_VCA_VT=...).
  *
  * 0.0002 after fitting the shells against Roland's own TR-909 with Snappy at
  * zero — the cleanest shell reference there is. At 0.001 the tail still cut
  * short of theirs (0.005 against 0.015 at 150 ms, both around -40 dB, so this
  * is tidiness rather than anything audible). */
-#ifndef 9W9_VCA_VT
-#define 9W9_VCA_VT    0.0002f
+#ifndef W9_VCA_VT
+#define W9_VCA_VT    0.0002f
 #endif
-#define 9W9_VCA_NORM  (1.0f / (1.0f - 9W9_VCA_VT))
+#define W9_VCA_NORM  (1.0f / (1.0f - W9_VCA_VT))
 
 /*
  * The voice/master distortion stage. Seven characters:
@@ -175,7 +175,7 @@
  * All types are level-normalised below unity drive, so Drive at minimum is
  * transparent for every one of them.
  */
-static inline float 9w9_shape_st(const float _x, const float _drive,
+static inline float w9_shape_st(const float _x, const float _drive,
                                   const float _type, float *_st)
 {
     const int t = (int)(_type + 0.5f);
@@ -246,32 +246,32 @@
     }
     case 0:
     default:
-        return 9w9_diode_round(_x, k);
-    }
-}
-
-static inline float 9w9_shape(const float _x, const float _drive, const float _type)
-{
-    return 9w9_shape_st(_x, _drive, _type, (float*)0);
+        return w9_diode_round(_x, k);
+    }
+}
+
+static inline float w9_shape(const float _x, const float _drive, const float _type)
+{
+    return w9_shape_st(_x, _drive, _type, (float*)0);
 }
 
 /* The measured resting pitch of the stock kick — every Tune position on the
  * reference machine settles here. */
-#define 9W9_BD_BASE_HZ 49.0f
+#define W9_BD_BASE_HZ 49.0f
 /* Fitted against all four lettered Tune positions at once (24 windowed pitch
  * readings, total squared error 26 — about 1 Hz per point): the sweep rises
  * 4.6 Hz per ms of its time constant, and HOLDS for ~16 ms before falling —
  * C9's charge time, Fraser's "fixed attack time of the sweep". Without the
  * hold no exponential matches the recordings; the early readings sit far
  * above any curve that starts falling at t=0. */
-#ifndef 9W9_BD_DF_PER_MS
-#define 9W9_BD_DF_PER_MS 4.6f     /* sweep height per ms of tau */
+#ifndef W9_BD_DF_PER_MS
+#define W9_BD_DF_PER_MS 4.6f     /* sweep height per ms of tau */
 #endif
-#ifndef 9W9_BD_PHOLD_MS
-#define 9W9_BD_PHOLD_MS  16.0f    /* sweep hold before the fall */
+#ifndef W9_BD_PHOLD_MS
+#define W9_BD_PHOLD_MS  16.0f    /* sweep hold before the fall */
 #endif
 
-static inline void 9w9_bt_init(9w9_bt_t *v, const float _sr)
+static inline void w9_bt_init(w9_bt_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_osc_init(&v->osc,  _sr);
@@ -298,14 +298,14 @@
     v->noise_gated = 0;
     v->amp_hold_left = 0;
     v->bd_sweep = 0; v->bd_df = 0.0f; v->bd_mult = 0.0f; v->bd_phold = 0;
-    v->bd_base = 9W9_BD_BASE_HZ;
+    v->bd_base = W9_BD_BASE_HZ;
     if(v->pitch_mod <= 0.0f) v->pitch_mod = 1.0f;
     v->out_gain = 0.0f;
     v->impulse = 0;
     v->mute_countdown = 0.0;
 }
 
-static inline void 9w9_bt_trigger(9w9_bt_t *v, const float _accent)
+static inline void w9_bt_trigger(w9_bt_t *v, const float _accent)
 {
     const float sr = v->sample_rate;
     const float ms = 0.001f * sr;
@@ -329,10 +329,10 @@
         const float m  = v->sweep_depth < 0.0f ? 0.0f
                        : (v->sweep_depth > 1.0f ? 1.0f : v->sweep_depth);
         const float pm = v->pitch_mod > 0.0f ? v->pitch_mod : 1.0f;
-        v->bd_base  = 9W9_BD_BASE_HZ * (1.0f + m * (pm - 1.0f));
-        v->bd_df    = 9W9_BD_DF_PER_MS * tau * (1.0f + 1.2f * m);
+        v->bd_base  = W9_BD_BASE_HZ * (1.0f + m * (pm - 1.0f));
+        v->bd_df    = W9_BD_DF_PER_MS * tau * (1.0f + 1.2f * m);
         v->bd_mult  = expf(-1.0f / (tau * ms));
-        v->bd_phold = (int)(9W9_BD_PHOLD_MS * ms);
+        v->bd_phold = (int)(W9_BD_PHOLD_MS * ms);
         wa_set_value(&v->pitch, v->bd_base);
     }
     else
@@ -370,7 +370,7 @@
     v->mute_countdown = (longest + 20.0f) * ms;
 }
 
-static inline float 9w9_bt_render(9w9_bt_t *v, const float _noise)
+static inline float w9_bt_render(w9_bt_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -394,7 +394,7 @@
      * continuously, which is what the junction does. Zeroing gain outright
      * (what this did first) cuts the shell mid-cycle and clicks. */
     if(v->tune2 > 0.0f && v->osc2_mix > 0.0f)
-        amp = amp > 9W9_VCA_VT ? (amp - 9W9_VCA_VT) * 9W9_VCA_NORM : 0.0f;
+        amp = amp > W9_VCA_VT ? (amp - W9_VCA_VT) * W9_VCA_NORM : 0.0f;
 
     /* Oscillator: triangle, rounded toward sine by the diode pair. The pair
      * is ALWAYS in circuit (Whittle: conduction at ~0.5-0.6 V at the stock
@@ -408,7 +408,7 @@
          * and H2/H1 2-4% — the pair conducts slightly asymmetrically, and a
          * perfectly symmetric shaper produces none. The square term's DC goes
          * through the blocker below. */
-        o = 9w9_diode_round(o, 1.6f);
+        o = w9_diode_round(o, 1.6f);
         o += 0.05f * o * o;
         o = wa_biquad_tick(&v->dc_block, o);
     }
@@ -421,13 +421,13 @@
          * grow a ~120 Hz difference tone, a phantom pitch no 909 makes. The
          * user Drive stage after this still distorts the mix on purpose;
          * at low Drive it stays clean. */
-        o = 9w9_diode_round(o, 2.0f);
+        o = w9_diode_round(o, 2.0f);
         const float ratio = v->tune > 1.0f ? v->tune2 / v->tune : 1.0f;
-        o += 9w9_diode_round(wa_osc_triangle(&v->osc2, f * ratio), 2.0f)
+        o += w9_diode_round(wa_osc_triangle(&v->osc2, f * ratio), 2.0f)
            * v->osc2_mix;
         o /= (1.0f + v->osc2_mix);
     }
-    float body = 9w9_shape_st(o, v->drive, v->dist_type, v->crush_st) * amp;
+    float body = w9_shape_st(o, v->drive, v->dist_type, v->crush_st) * amp;
 
     /* Click: impulse + lowpass-filtered noise, own envelope (Attack). */
     float click = 0.0f;
@@ -454,15 +454,15 @@
         {
             /* Same junction, same soft cutoff — and once it is shut, stay shut
              * (the envelope only falls further). */
-            env = env > 9W9_VCA_VT ? (env - 9W9_VCA_VT) * 9W9_VCA_NORM : 0.0f;
+            env = env > W9_VCA_VT ? (env - W9_VCA_VT) * W9_VCA_NORM : 0.0f;
             if(env <= 0.0f) v->noise_gated = 1;
         }
         snare = wa_biquad_tick(&v->noise_lpf,
                     wa_biquad_tick(&v->noise_hpf, _noise))
-              * env * v->snappy * 9W9_SNAPPY_MIX;
+              * env * v->snappy * W9_SNAPPY_MIX;
     }
 
     return (body + click + snare) * v->out_gain;
 }
 
-#endif /* 9W9_CIRCUIT_H */
+#endif /* W9_CIRCUIT_H */
--- 9pin/src/dsp/9w9_engine.c
+++ 9ren/src/dsp/9w9_engine.c
@@ -1,10 +1,5 @@
 /*
- * 9w9_engine.c — ER-99 drum voices.
- *
- * Signal paths follow matthewcieplak/er-99 exactly (src/instrument.ts,
- * generator.ts, sampler.ts, index.ts). Where the original has a quirk, the
- * quirk is reproduced and commented rather than silently "fixed" — fidelity
- * first; deviations are opt-in flags.
+ * 9w9_engine.c — 9W9 drum voices, sample playback, sends and master bus.
  *
  * GPL-3.0.
  */
@@ -16,7 +11,7 @@
 
 #include "9w9_engine.h"
 
-const char *const 9w9_trigger_names[9W9_NUM_TRIGGERS] = {
+const char *const w9_trigger_names[W9_NUM_TRIGGERS] = {
     "bd", "sd", "lt", "mt", "ht", "rs", "hc", "ohh", "chh", "rc", "cr"
 };
 
@@ -26,7 +21,7 @@
 }
 
 /* ===================================================================== */
-/* WAV loading (mono 16/24-bit PCM — that's what er-99 ships)            */
+/* WAV loading (mono 16/24-bit PCM)                                    */
 /* ===================================================================== */
 
 static float *load_wav_mono(const char *_path, uint32_t *_out_len)
@@ -104,17 +99,17 @@
 /* Init                                                                   */
 /* ===================================================================== */
 
-static void 9w9_dly_retime(9w9_dly_t *d, float _sr);
-
-void 9w9_engine_init(9w9_engine_t *e, const float _sr, const char *_module_dir)
+static void w9_dly_retime(w9_dly_t *d, float _sr);
+
+void w9_engine_init(w9_engine_t *e, const float _sr, const char *_module_dir)
 {
     memset(e, 0, sizeof(*e));
     e->sample_rate = _sr;
     wa_noise_init(&e->noise, 0xC0FFEEu);
 
-    for(int i=0; i<9W9_NUM_INSTRUMENTS; ++i)
-    {
-        9w9_bt_t *b = &e->bt[i];
+    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
+    {
+        w9_bt_t *b = &e->bt[i];
         memset(b, 0, sizeof(*b));
         b->drive = 2.0f;  b->level = 0.8f;  b->click_tone = 3000.0f;
         switch(i)
@@ -179,13 +174,13 @@
             break;
         default: break;
         }
-        9w9_bt_init(b, _sr);
-    }
-    for(int i=0; i<3; ++i) 9w9_tom909_init(&e->tom909[i], _sr);
+        w9_bt_init(b, _sr);
+    }
+    for(int i=0; i<3; ++i) w9_tom909_init(&e->tom909[i], _sr);
 
     /* ---- Rim shot and hand clap (circuit models) ---- */
     {
-        9w9_rim909_t *r9 = &e->rim909;
+        w9_rim909_t *r9 = &e->rim909;
         memset(r9, 0, sizeof(*r9));
         /* Measured on a real 909 rim: the two resonances are 210 and 480 Hz —
          * a ratio of 2.3, not the near-decade this had. The 1350 Hz "tick" was
@@ -201,9 +196,9 @@
          * it at half. Distortion stays available on the knob. */
         r9->decay = 200.0f; r9->noise_mix = 0.35f;
         r9->drive = 1.4f;  r9->dist_type = 0.0f; r9->level = 1.1f;
-        9w9_rim909_init(r9, _sr);
-
-        9w9_clap909_t *c9 = &e->clap909;
+        w9_rim909_init(r9, _sr);
+
+        w9_clap909_t *c9 = &e->clap909;
         memset(c9, 0, sizeof(*c9));
         /* Fitted to Clap 909 Clean: bandpass centred ~950 Hz (Q ~2, burst
          * spectrum 925 with content to 3 k), echoes 12 ms apart, each dying in
@@ -213,20 +208,20 @@
         c9->spread = 12.0f; c9->burst_decay = 50.0f;
         c9->tail_decay = 345.0f; c9->tail_level = 0.58f;
         c9->drive = 1.6f;  c9->dist_type = 0.0f; c9->level = 1.3f;
-        9w9_clap909_init(c9, _sr);
+        w9_clap909_init(c9, _sr);
     }
 
     /* ---- Samplers ---- */
     /* Balanced against the measured kit: at the old defaults the closed hat
      * peaked 16 dB under the kick and the ride 17 — inaudible next to the
      * (then overpowered) toms. ohh/rc/cr/chh. */
-    static const float vols[9W9_NUM_SAMPLERS]   = { 0.85f, 0.45f, 0.5f, 0.95f };
-    static const float sdec[9W9_NUM_SAMPLERS]  = { 450.0f, 1800.0f, 1800.0f, 110.0f };
+    static const float vols[W9_NUM_SAMPLERS]   = { 0.85f, 0.45f, 0.5f, 0.95f };
+    static const float sdec[W9_NUM_SAMPLERS]  = { 450.0f, 1800.0f, 1800.0f, 110.0f };
     /* The closed hat plays the open hat's buffer — same cymbals, shorter gate. */
-    static const char *files[9W9_NUM_SAMPLERS]  = { "hh.wav", "ride.wav", "crash.wav", NULL };
-    for(int i=0; i<9W9_NUM_SAMPLERS; ++i)
-    {
-        9w9_sampler_t *s = &e->sampler[i];
+    static const char *files[W9_NUM_SAMPLERS]  = { "hh.wav", "ride.wav", "crash.wav", NULL };
+    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
+    {
+        w9_sampler_t *s = &e->sampler[i];
         s->decay = 450.0f;      /* open hat; ride/crash overridden below */
         s->decay_closed = 110.0f;
         s->volume = vols[i];
@@ -240,9 +235,9 @@
         if(!files[i])
         {
             /* Shares the open hat's data: not loaded again, and not freed
-             * through this index either (see 9w9_engine_free). */
-            s->buffer = e->sample_data[9W9_SAMP_OHH];
-            s->length = e->sample_len[9W9_SAMP_OHH];
+             * through this index either (see w9_engine_free). */
+            s->buffer = e->sample_data[W9_SAMP_OHH];
+            s->length = e->sample_len[W9_SAMP_OHH];
         }
         else if(_module_dir)
         {
@@ -263,7 +258,7 @@
     e->dly.bpm  = 120.0f;
     e->dly.fdbk = 0.35f; e->dly.tone = 0.4f;
     e->dly.hpf_hz = 150.0f; e->dly.level = 0.8f;
-    9w9_dly_retime(&e->dly, _sr);
+    w9_dly_retime(&e->dly, _sr);
     e->dly.dcur = e->dly.time_ms * 0.001f * _sr;
     wa_biquad_set(&e->dly.hp, WA_HIGHPASS, e->dly.hpf_hz, 0.7071f, 0.0f, _sr);
     wa_biquad_reset(&e->dly.hp);
@@ -284,12 +279,12 @@
     e->master.vel_depth    = 1.0f;   /* velocity live by default */
 
     /* Derive initial pot positions from the defaults above. */
-    9w9_engine_seed_pots(e);
-}
-
-void 9w9_engine_free(9w9_engine_t *e)
-{
-    for(int i=0; i<9W9_NUM_SAMPLERS; ++i)
+    w9_engine_seed_pots(e);
+}
+
+void w9_engine_free(w9_engine_t *e)
+{
+    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
     {
         free(e->sample_data[i]);        /* NULL for the closed hat: borrowed */
         e->sample_data[i] = NULL;
@@ -301,7 +296,7 @@
 /* Triggering                                                             */
 /* ===================================================================== */
 
-void 9w9_engine_trigger(9w9_engine_t *e, const 9w9_trigger_t _which, const int _velocity)
+void w9_engine_trigger(w9_engine_t *e, const w9_trigger_t _which, const int _velocity)
 {
 
     /* How loud this hit is, from its velocity.
@@ -331,7 +326,7 @@
 
     switch(_which)
     {
-    case 9W9_BD: case 9W9_SD: case 9W9_LT: case 9W9_MT: case 9W9_HT:
+    case W9_BD: case W9_SD: case W9_LT: case W9_MT: case W9_HT:
         /* Everything on the snare that is not a panel pot is fixed circuitry:
          * the shell interval (one pitch CV, D-network ratio 1.585), the pitch
          * pulse (ENV1 — "the same for all SD notes"), the shell decay
@@ -342,9 +337,9 @@
         /* Same for the kick: everything not on its panel is fixed circuitry.
          * The sub layer, tube stage and drift were our additions — no 909 has
          * them — and the diode pair provides the tone now. */
-        if(_which == 9W9_BD)
-        {
-            9w9_bt_t *bd = &e->bt[9W9_BD];
+        if(_which == W9_BD)
+        {
+            w9_bt_t *bd = &e->bt[W9_BD];
             bd->tune2 = 0.0f; bd->osc2_mix = 0.0f;
             bd->click_tone = 2500.0f;
             bd->amp_hold = 40.0f;
@@ -356,16 +351,16 @@
         /* Clap: the real 909 gives it ONE pot (Level). Tune and Tail stay
          * because Gus likes them; the burst geometry is the circuit's and is
          * pinned — echo spacing, echo decay, tail share and the filter's Q. */
-        if(_which == 9W9_HC)
-        {
-            9w9_clap909_t *c9 = &e->clap909;
+        if(_which == W9_HC)
+        {
+            w9_clap909_t *c9 = &e->clap909;
             c9->spread = 12.0f; c9->burst_decay = 50.0f;
             c9->tail_level = 0.58f;
-            if(c9->res != 2.0f) { c9->res = 2.0f; 9w9_clap909_retune(c9); }
-        }
-        if(_which == 9W9_SD)
-        {
-            9w9_bt_t *sd = &e->bt[9W9_SD];
+            if(c9->res != 2.0f) { c9->res = 2.0f; w9_clap909_retune(c9); }
+        }
+        if(_which == W9_SD)
+        {
+            w9_bt_t *sd = &e->bt[W9_SD];
             sd->tune2       = sd->tune * 1.585f;
             sd->osc2_mix    = 0.40f;   /* ~28% of shell 1 at the output */
             sd->sweep_depth = 1.045f;  /* 206->198 Hz measured, not 8% */
@@ -381,52 +376,52 @@
             sd->decay       = 340.0f;
             sd->attack      = 0.0f;    /* the 909 SD has no click path */
             if(sd->noise_hp < 990.0f || sd->noise_hp > 1010.0f)
-                9w9_engine_set_raw(e, "sd_c_noise_hp", 1000.0f);
+                w9_engine_set_raw(e, "sd_c_noise_hp", 1000.0f);
         }
         /* The toms are their own circuit: three oscillators with separate
          * envelopes plus a noise attack (9w9_tom909.h). They read the same
          * panel values as the two-oscillator voice, so nothing else changes. */
-        if(_which >= 9W9_LT && _which <= 9W9_HT)
-            9w9_tom909_trigger(&e->tom909[_which - 9W9_LT], &e->bt[_which], vgain);
+        if(_which >= W9_LT && _which <= W9_HT)
+            w9_tom909_trigger(&e->tom909[_which - W9_LT], &e->bt[_which], vgain);
         else
-            9w9_bt_trigger(&e->bt[_which], vgain);
+            w9_bt_trigger(&e->bt[_which], vgain);
         break;
 
-    case 9W9_RS: {
+    case W9_RS: {
         {
             /* Fixed circuitry, not pots: the upper resonance is the same
              * network as the lower (measured 480/210 = 2.286, so it tracks
              * Tune), both ring at Q ~10, and the trigger pulse carries a set
              * amount of noise. Pinned here so no saved state can drift them. */
-            9w9_rim909_t *r9 = &e->rim909;
+            w9_rim909_t *r9 = &e->rim909;
             const float t2 = r9->tune * 2.286f, q = 10.0f;
             if(r9->tune2 != t2 || r9->res != q)
-            { r9->tune2 = t2; r9->res = q; 9w9_rim909_retune(r9); }
+            { r9->tune2 = t2; r9->res = q; w9_rim909_retune(r9); }
             r9->noise_mix = 0.35f;   /* inside the 2-sample pulse only */
             r9->decay     = 200.0f;  /* -30 dB at ~30 ms, as measured */
-            9w9_rim909_trigger(r9, vgain);
+            w9_rim909_trigger(r9, vgain);
             break;
         }
     }
 
-    case 9W9_HC: {
-        9w9_clap909_trigger(&e->clap909, vgain);
+    case W9_HC: {
+        w9_clap909_trigger(&e->clap909, vgain);
         break;
     }
 
-    case 9W9_OHH: case 9W9_CHH: case 9W9_RC: case 9W9_CR: {
-        const int idx = _which == 9W9_OHH ? 9W9_SAMP_OHH
-                      : _which == 9W9_CHH ? 9W9_SAMP_CHH
-                      : _which == 9W9_RC  ? 9W9_SAMP_RC : 9W9_SAMP_CR;
-        9w9_sampler_t *s = &e->sampler[idx];
+    case W9_OHH: case W9_CHH: case W9_RC: case W9_CR: {
+        const int idx = _which == W9_OHH ? W9_SAMP_OHH
+                      : _which == W9_CHH ? W9_SAMP_CHH
+                      : _which == W9_RC  ? W9_SAMP_RC : W9_SAMP_CR;
+        w9_sampler_t *s = &e->sampler[idx];
 
         /* One pair of cymbals: closing the pedal stops the open hat ringing,
          * and opening it cuts the closed sound short. Anything else lets two
          * hats sound at once, which the hardware cannot do. */
-        if(idx == 9W9_SAMP_OHH || idx == 9W9_SAMP_CHH)
-        {
-            9w9_sampler_t *other = &e->sampler[idx == 9W9_SAMP_OHH
-                                                ? 9W9_SAMP_CHH : 9W9_SAMP_OHH];
+        if(idx == W9_SAMP_OHH || idx == W9_SAMP_CHH)
+        {
+            w9_sampler_t *other = &e->sampler[idx == W9_SAMP_OHH
+                                                ? W9_SAMP_CHH : W9_SAMP_OHH];
             /* A short fade, not a hard stop: cutting a ringing buffer mid-cycle
              * is a click. 3 ms is under a hat's own attack. */
             wa_exp_ramp(&other->out, 0.00001f, ms_to_samples(3.0f, sr));
@@ -448,7 +443,7 @@
 /* Per-voice rendering                                                    */
 /* ===================================================================== */
 
-static float render_sampler(9w9_sampler_t *s)
+static float render_sampler(w9_sampler_t *s)
 {
     if(s->mute_countdown <= 0.0 || s->pos < 0.0 || !s->buffer) return 0.0f;
     s->mute_countdown -= 1.0;
@@ -466,15 +461,15 @@
     }
     v *= wa_param_tick(&s->out);
     if(s->drive > 0.25f)                     /* below this the shape is inaudible */
-        v = 9w9_shape_st(v * (1.0f + s->drive * 0.5f), s->drive, s->dist_type, s->crush_st);
+        v = w9_shape_st(v * (1.0f + s->drive * 0.5f), s->drive, s->dist_type, s->crush_st);
     return v;
 }
 
 
-static const int 9W9_VERB_CL[4] = { 1116, 1188, 1277, 1356 };
-static const int 9W9_VERB_AL[2] = { 556, 441 };
-
-static float 9w9_verb_tick(9w9_verb_t *r, const float _in)
+static const int W9_VERB_CL[4] = { 1116, 1188, 1277, 1356 };
+static const int W9_VERB_AL[2] = { 556, 441 };
+
+static float w9_verb_tick(w9_verb_t *r, const float _in)
 {
     const float x = wa_biquad_tick(&r->hp, _in);
     /* Tone: bright opens the loop's damping, dark closes it. */
@@ -484,7 +479,7 @@
     for(int i = 0; i < 4; ++i)
     {
         float *b = r->comb[i];
-        const int n = 9W9_VERB_CL[i];
+        const int n = W9_VERB_CL[i];
         const float y = b[r->cpos[i]];
         acc += y;
         r->cdmp[i] = y + (r->cdmp[i] - y) * damp;
@@ -500,7 +495,7 @@
     for(int j = 0; j < 2; ++j)
     {
         float *b = r->apb[j];
-        const int n = 9W9_VERB_AL[j];
+        const int n = W9_VERB_AL[j];
         const float bo = b[r->apos[j]];
         b[r->apos[j]] = y + bo * 0.5f;
         y = bo - y * 0.5f;
@@ -511,25 +506,25 @@
 
 /* Note divisions, shortest to longest, in beats. Matches the dly_time enum:
  * 1/32 1/16T 1/16 1/8T 1/16. 1/8 1/4T 1/8. 1/4 1/2T 1/4. 1/2 1/2. */
-static const float 9w9_dly_beats[9W9_DLY_DIVS] = {
+static const float w9_dly_beats[W9_DLY_DIVS] = {
     0.125f, 1.0f/6.0f, 0.25f, 1.0f/3.0f, 0.375f, 0.5f, 2.0f/3.0f,
     0.75f, 1.0f, 4.0f/3.0f, 1.5f, 2.0f, 3.0f
 };
 
-static void 9w9_dly_retime(9w9_dly_t *d, float _sr)
+static void w9_dly_retime(w9_dly_t *d, float _sr)
 {
     int i = (int)(d->divi + 0.5f);
     if(i < 0) i = 0;
-    if(i >= 9W9_DLY_DIVS) i = 9W9_DLY_DIVS - 1;
+    if(i >= W9_DLY_DIVS) i = W9_DLY_DIVS - 1;
     const float bpm = d->bpm > 20.0f ? d->bpm : 120.0f;
-    float ms = 9w9_dly_beats[i] * 60000.0f / bpm;
+    float ms = w9_dly_beats[i] * 60000.0f / bpm;
     /* The line is 2 s; longer divisions at slow tempos clamp to it. */
-    const float max_ms = (float)(9W9_DLY_MAX - 256) / _sr * 1000.0f;
+    const float max_ms = (float)(W9_DLY_MAX - 256) / _sr * 1000.0f;
     if(ms > max_ms) ms = max_ms;
     d->time_ms = ms;
 }
 
-static float 9w9_dly_tick(9w9_dly_t *d, const float _in, const float _sr)
+static float w9_dly_tick(w9_dly_t *d, const float _in, const float _sr)
 {
     const float x = wa_biquad_tick(&d->hp, _in);
     const float target = d->time_ms * 0.001f * _sr;
@@ -537,13 +532,13 @@
      * clicking. ~30 ms to settle. */
     d->dcur += (target - d->dcur) * 0.0008f;
     float rp = (float)d->w - d->dcur;
-    while(rp < 0.0f) rp += (float)9W9_DLY_MAX;
+    while(rp < 0.0f) rp += (float)W9_DLY_MAX;
     int i0 = (int)rp;
     const float fr = rp - (float)i0;
     /* float spacing at 36000 is 1/256: a read a hair under the wrap point
-     * rounds UP to exactly 9W9_DLY_MAX and indexes past the buffer. */
-    if(i0 >= 9W9_DLY_MAX) i0 -= 9W9_DLY_MAX;
-    const int   i1 = i0 + 1 >= 9W9_DLY_MAX ? 0 : i0 + 1;
+     * rounds UP to exactly W9_DLY_MAX and indexes past the buffer. */
+    if(i0 >= W9_DLY_MAX) i0 -= W9_DLY_MAX;
+    const int   i1 = i0 + 1 >= W9_DLY_MAX ? 0 : i0 + 1;
     const float y  = d->buf[i0] + (d->buf[i1] - d->buf[i0]) * fr;
 
     /* Feedback through a one-pole: every repeat gets darker, like tape and
@@ -554,7 +549,7 @@
     float st = x + d->lp;
     st = truncf(st * 2048.0f) * (1.0f / 2048.0f);   /* toward zero: no DC */
     d->buf[d->w] = st;
-    if(++d->w >= 9W9_DLY_MAX) d->w = 0;
+    if(++d->w >= W9_DLY_MAX) d->w = 0;
     return y * d->level;
 }
 
@@ -564,7 +559,7 @@
  * is smoothed — 8 ms attack so transients punch through, 150 ms release, 6 dB
  * soft knee. The old inherited compressor's sins are not repeated: no zero-
  * attack waveshaping, and amount zero means the stage is not in the path. */
-static float master_glue(9w9_master_t *m, const float _in, const float _sr)
+static float master_glue(w9_master_t *m, const float _in, const float _sr)
 {
     const float a = m->comp;
     /* Gentler curve than the first cut: -26 dB threshold at 5:1 measured
@@ -619,35 +614,35 @@
     return _in * powf(10.0f, (m->comp_env_db + makeup) / 20.0f);
 }
 
-void 9w9_engine_render(9w9_engine_t *e, float *out, const int frames)
+void w9_engine_render(w9_engine_t *e, float *out, const int frames)
 {
     for(int n=0; n<frames; ++n)
     {
         const float noise = wa_noise_tick(&e->noise);
 
         float mix = 0.0f, rbus = 0.0f, dbus = 0.0f;
-        for(int i=0; i<9W9_NUM_INSTRUMENTS; ++i)
+        for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
         {
             float v;
-            if(i >= 9W9_LT && i <= 9W9_HT)
-                v = 9w9_tom909_render(&e->tom909[i - 9W9_LT], &e->bt[i], noise);
+            if(i >= W9_LT && i <= W9_HT)
+                v = w9_tom909_render(&e->tom909[i - W9_LT], &e->bt[i], noise);
             else
-                v = 9w9_bt_render(&e->bt[i], noise);
+                v = w9_bt_render(&e->bt[i], noise);
             mix += v;
             rbus += v * e->send_rev[i];
             dbus += v * e->send_dly[i];
         }
         {
-            const float v = 9w9_rim909_render(&e->rim909, noise);
-            mix += v; rbus += v * e->send_rev[9W9_RS]; dbus += v * e->send_dly[9W9_RS];
-        }
-        {
-            const float v = 9w9_clap909_render(&e->clap909, noise);
-            mix += v; rbus += v * e->send_rev[9W9_HC]; dbus += v * e->send_dly[9W9_HC];
-        }
-        {
-            static const int strig[9W9_NUM_SAMPLERS] = { 9W9_OHH, 9W9_RC, 9W9_CR, 9W9_CHH };
-            for(int i=0; i<9W9_NUM_SAMPLERS; ++i)
+            const float v = w9_rim909_render(&e->rim909, noise);
+            mix += v; rbus += v * e->send_rev[W9_RS]; dbus += v * e->send_dly[W9_RS];
+        }
+        {
+            const float v = w9_clap909_render(&e->clap909, noise);
+            mix += v; rbus += v * e->send_rev[W9_HC]; dbus += v * e->send_dly[W9_HC];
+        }
+        {
+            static const int strig[W9_NUM_SAMPLERS] = { W9_OHH, W9_RC, W9_CR, W9_CHH };
+            for(int i=0; i<W9_NUM_SAMPLERS; ++i)
             {
                 const float v = render_sampler(&e->sampler[i]);
                 mix += v;
@@ -658,11 +653,11 @@
 
         /* FX returns join the bus before the master stages, so master
          * distortion and the Comp work on the wet signal too. */
-        mix += 9w9_verb_tick(&e->verb, rbus);
-        mix += 9w9_dly_tick(&e->dly, dbus, e->sample_rate);
+        mix += w9_verb_tick(&e->verb, rbus);
+        mix += w9_dly_tick(&e->dly, dbus, e->sample_rate);
 
         if(e->master.dist_mode >= 0.5f)
-            mix = 9w9_shape_st(mix * e->master.drive, e->master.drive,
+            mix = w9_shape_st(mix * e->master.drive, e->master.drive,
                              e->master.dist_mode - 1.0f, e->master.crush_st) * 0.7f;
         if(e->master.comp > 0.001f)
             mix = master_glue(&e->master, mix, e->sample_rate);
@@ -678,14 +673,14 @@
 
 /* Circuit-voice fields, shared by set_param/get_param and state save/load. */
 typedef struct { const char *n; size_t off; } bt_field_t;
-#define BTF(n) { #n, offsetof(9w9_bt_t, n) }
+#define BTF(n) { #n, offsetof(w9_bt_t, n) }
 static const bt_field_t g_bt_fields[] = {
     BTF(tune), BTF(sweep_depth), BTF(sweep_time), BTF(decay), BTF(attack),
     BTF(click_tone), BTF(drive), BTF(level), BTF(dist_type),
     BTF(tune2), BTF(osc2_mix), BTF(snappy), BTF(noise_decay), BTF(noise_hp),
     BTF(pitch_mod),
 };
-#define 9W9_BT_FIELD_COUNT (sizeof(g_bt_fields)/sizeof(g_bt_fields[0]))
+#define W9_BT_FIELD_COUNT (sizeof(g_bt_fields)/sizeof(g_bt_fields[0]))
 
 /* Everything else that must round-trip in a saved state. */
 static const char *const g_other_state_keys[] = {
@@ -699,7 +694,7 @@
     "rs_dist_type", "hc_dist_type", "ohh_dist_type", "chh_dist_type",
     "rc_dist_type", "cr_dist_type",
 };
-#define 9W9_OTHER_KEY_COUNT (sizeof(g_other_state_keys)/sizeof(g_other_state_keys[0]))
+#define W9_OTHER_KEY_COUNT (sizeof(g_other_state_keys)/sizeof(g_other_state_keys[0]))
 
 /* Per-voice FX sends. The kick has none, deliberately. */
 static const struct { const char *k; unsigned char voice, dly; } g_send_keys[] = {
@@ -714,11 +709,11 @@
     {"rc_rev",9,0},{"rc_dly",9,1},
     {"cr_rev",10,0},{"cr_dly",10,1},
 };
-#define 9W9_SEND_KEYS ((int)(sizeof(g_send_keys)/sizeof(g_send_keys[0])))
-
-int 9w9_engine_set_raw(9w9_engine_t *e, const char *key, const float value)
-{
-    for(int i=0; i<9W9_SEND_KEYS; ++i)
+#define W9_SEND_KEYS ((int)(sizeof(g_send_keys)/sizeof(g_send_keys[0])))
+
+int w9_engine_set_raw(w9_engine_t *e, const char *key, const float value)
+{
+    for(int i=0; i<W9_SEND_KEYS; ++i)
         if(!strcmp(key, g_send_keys[i].k))
         {
             float *a = g_send_keys[i].dly ? e->send_dly : e->send_rev;
@@ -737,14 +732,14 @@
     if(!strcmp(key, "dly_time"))
     {
         e->dly.divi = value < 0.0f ? 0.0f
-                    : (value > 9W9_DLY_DIVS - 1 ? 9W9_DLY_DIVS - 1 : value);
-        9w9_dly_retime(&e->dly, e->sample_rate);
+                    : (value > W9_DLY_DIVS - 1 ? W9_DLY_DIVS - 1 : value);
+        w9_dly_retime(&e->dly, e->sample_rate);
         return 1;
     }
     if(!strcmp(key, "dly_bpm"))
     {
         e->dly.bpm = value;
-        9w9_dly_retime(&e->dly, e->sample_rate);
+        w9_dly_retime(&e->dly, e->sample_rate);
         return 1;
     }
     if(!strcmp(key, "dly_fdbk"))  { e->dly.fdbk = value; return 1; }
@@ -757,13 +752,13 @@
         return 1;
     }
 
-    for(int i=0; i<9W9_NUM_INSTRUMENTS; ++i)
+    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
         {
             char pre[16];
-            snprintf(pre, sizeof(pre), "%s_c_", 9w9_trigger_names[i]);
+            snprintf(pre, sizeof(pre), "%s_c_", w9_trigger_names[i]);
             const size_t plen = strlen(pre);
             if(strncmp(key, pre, plen)) continue;
-            for(size_t k=0; k<9W9_BT_FIELD_COUNT; ++k)
+            for(size_t k=0; k<W9_BT_FIELD_COUNT; ++k)
             {
                 if(strcmp(key + plen, g_bt_fields[k].n)) continue;
                 *(float*)((char*)&e->bt[i] + g_bt_fields[k].off) = value;
@@ -777,29 +772,29 @@
             }
         }
     {
-        9w9_rim909_t *r9 = &e->rim909; 9w9_clap909_t *c9 = &e->clap909;
+        w9_rim909_t *r9 = &e->rim909; w9_clap909_t *c9 = &e->clap909;
         if(!strcmp(key,"rs_decay"))     { r9->decay = value; return 1; }
         if(!strcmp(key,"rs_volume"))    { r9->level = value; return 1; }
         if(!strcmp(key,"rs_saturation")){ r9->drive = value; return 1; }
-        if(!strcmp(key,"rs_tune"))      { r9->tune = value;  9w9_rim909_retune(r9); return 1; }
-        if(!strcmp(key,"rs_tune2"))     { r9->tune2 = value; 9w9_rim909_retune(r9); return 1; }
-        if(!strcmp(key,"rs_res"))       { r9->res = value;   9w9_rim909_retune(r9); return 1; }
+        if(!strcmp(key,"rs_tune"))      { r9->tune = value;  w9_rim909_retune(r9); return 1; }
+        if(!strcmp(key,"rs_tune2"))     { r9->tune2 = value; w9_rim909_retune(r9); return 1; }
+        if(!strcmp(key,"rs_res"))       { r9->res = value;   w9_rim909_retune(r9); return 1; }
         if(!strcmp(key,"rs_noise"))     { r9->noise_mix = value; return 1; }
         if(!strcmp(key,"hc_decay"))     { c9->tail_decay = value; return 1; }
         if(!strcmp(key,"hc_spread"))    { c9->spread = value; return 1; }
         if(!strcmp(key,"hc_volume"))    { c9->level = value; return 1; }
         if(!strcmp(key,"hc_tone_decay")){ c9->burst_decay = value; return 1; }
-        if(!strcmp(key,"hc_tune"))      { c9->tune = value; 9w9_clap909_retune(c9); return 1; }
+        if(!strcmp(key,"hc_tune"))      { c9->tune = value; w9_clap909_retune(c9); return 1; }
         if(!strcmp(key,"hc_tail"))      { c9->tail_level = value; return 1; }
         if(!strcmp(key,"hc_drive"))     { c9->drive = value; return 1; }
     }
 
     /* Patches saved before the closed hat became its own voice carry its decay
      * as ohh_decay_closed. Keep accepting it. */
-    if(!strcmp(key, "ohh_decay_closed")) { e->sampler[9W9_SAMP_CHH].decay = value; return 1; }
-
-    static const char *sn[9W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
-    for(int i=0; i<9W9_NUM_SAMPLERS; ++i)
+    if(!strcmp(key, "ohh_decay_closed")) { e->sampler[W9_SAMP_CHH].decay = value; return 1; }
+
+    static const char *sn[W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
+    for(int i=0; i<W9_NUM_SAMPLERS; ++i)
     {
         char buf[32];
         snprintf(buf, sizeof(buf), "%s_decay", sn[i]);
@@ -832,9 +827,9 @@
     return 0;
 }
 
-int 9w9_engine_get_raw(const 9w9_engine_t *e, const char *key, float *out)
-{
-    for(int i=0; i<9W9_SEND_KEYS; ++i)
+int w9_engine_get_raw(const w9_engine_t *e, const char *key, float *out)
+{
+    for(int i=0; i<W9_SEND_KEYS; ++i)
         if(!strcmp(key, g_send_keys[i].k))
         {
             const float *a = g_send_keys[i].dly ? e->send_dly : e->send_rev;
@@ -851,20 +846,20 @@
     if(!strcmp(key, "dly_level")) { *out = e->dly.level; return 1; }
     if(!strcmp(key, "dly_hpf"))   { *out = e->dly.hpf_hz; return 1; }
 
-    for(int i=0; i<9W9_NUM_INSTRUMENTS; ++i)
+    for(int i=0; i<W9_NUM_INSTRUMENTS; ++i)
     {
         char pre[16];
-        snprintf(pre, sizeof(pre), "%s_c_", 9w9_trigger_names[i]);
+        snprintf(pre, sizeof(pre), "%s_c_", w9_trigger_names[i]);
         const size_t plen = strlen(pre);
         if(strncmp(key, pre, plen)) continue;
-        for(size_t k=0; k<9W9_BT_FIELD_COUNT; ++k)
+        for(size_t k=0; k<W9_BT_FIELD_COUNT; ++k)
             if(!strcmp(key + plen, g_bt_fields[k].n))
             { *out = *(const float*)((const char*)&e->bt[i] + g_bt_fields[k].off); return 1; }
     }
     {
-        if(!strcmp(key, "ohh_decay_closed")) { *out = e->sampler[9W9_SAMP_CHH].decay; return 1; }
-        static const char *sn[9W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
-        for(int i=0; i<9W9_NUM_SAMPLERS; ++i)
+        if(!strcmp(key, "ohh_decay_closed")) { *out = e->sampler[W9_SAMP_CHH].decay; return 1; }
+        static const char *sn[W9_NUM_SAMPLERS] = { "ohh", "rc", "cr", "chh" };
+        for(int i=0; i<W9_NUM_SAMPLERS; ++i)
         {
             char b[32];
             snprintf(b, sizeof(b), "%s_decay", sn[i]);
@@ -884,7 +879,7 @@
     if(!strcmp(key, "rs_dist_type")) { *out = e->rim909.dist_type; return 1; }
     if(!strcmp(key, "hc_dist_type")) { *out = e->clap909.dist_type; return 1; }
     {
-        const 9w9_rim909_t *r9 = &e->rim909; const 9w9_clap909_t *c9 = &e->clap909;
+        const w9_rim909_t *r9 = &e->rim909; const w9_clap909_t *c9 = &e->clap909;
         if(!strcmp(key,"rs_decay"))     { *out = r9->decay; return 1; }
         if(!strcmp(key,"rs_volume"))    { *out = r9->level; return 1; }
         if(!strcmp(key,"rs_saturation")){ *out = r9->drive; return 1; }
@@ -923,7 +918,7 @@
 /* State — self-contained, so slot autosave and User Presets round-trip   */
 /* ===================================================================== */
 
-int 9w9_engine_get_state(const 9w9_engine_t *e, char *buf, const int len)
+int w9_engine_get_state(const w9_engine_t *e, char *buf, const int len)
 {
     if(!buf || len <= 0) return -1;
     int n = 0;
@@ -932,29 +927,29 @@
      * old octave tune2, the 1.6x sweep, osc2_mix 0.7) — restoring one
      * resurrects the pre-rebuild sound no matter how the defaults are set,
      * which is exactly what kept happening. Old blobs are discarded whole. */
-    n += snprintf(buf + n, (size_t)(len - n), "9w9v3;");
+    n += snprintf(buf + n, (size_t)(len - n), W9_STATE_TAG);
 
     /* Pot positions: what the panel shows, and exactly what restores it. */
-    for(int i=0; i<9W9_POT_COUNT && n < len - 1; ++i)
+    for(int i=0; i<W9_POT_COUNT && n < len - 1; ++i)
         n += snprintf(buf + n, (size_t)(len - n), "%s=%d;",
-                      g_9w9_pots[i].key, (int)e->pots[i]);
+                      g_w9_pots[i].key, (int)e->pots[i]);
 
     /* Switches keep their own values. */
-    for(size_t k=0; k<9W9_OTHER_KEY_COUNT && n < len - 1; ++k)
+    for(size_t k=0; k<W9_OTHER_KEY_COUNT && n < len - 1; ++k)
     {
         if(!is_enum_key(g_other_state_keys[k])) continue;
         float v = 0.0f;
-        if(9w9_engine_get_raw(e, g_other_state_keys[k], &v))
+        if(w9_engine_get_raw(e, g_other_state_keys[k], &v))
             n += snprintf(buf + n, (size_t)(len - n), "%s=%.0f;", g_other_state_keys[k], v);
     }
-    for(int i=0; i<9W9_NUM_INSTRUMENTS && n < len - 1; ++i)
+    for(int i=0; i<W9_NUM_INSTRUMENTS && n < len - 1; ++i)
         n += snprintf(buf + n, (size_t)(len - n), "%s_c_dist_type=%.0f;",
-                      9w9_trigger_names[i], e->bt[i].dist_type);
+                      w9_trigger_names[i], e->bt[i].dist_type);
 
     return n < len ? n : len - 1;
 }
 
-int 9w9_engine_set_state(9w9_engine_t *e, const char *blob)
+int w9_engine_set_state(w9_engine_t *e, const char *blob)
 {
     if(!blob) return 0;
 
@@ -964,7 +959,8 @@
      * i.e. total silence. Anything that isn't v2 is discarded so the engine
      * keeps its (audible) defaults instead of being silently muted.
      */
-    if(strncmp(blob, "9w9v3;", 7) != 0)
+    if(strncmp(blob, W9_STATE_TAG, sizeof(W9_STATE_TAG) - 1) != 0 &&
+       strncmp(blob, W9_STATE_TAG_OLD, sizeof(W9_STATE_TAG_OLD) - 1) != 0)
         return 0;      /* v1/v2/unknown: keep the (correct) defaults */
 
     const char *p = blob;
@@ -981,7 +977,7 @@
             if(eq)
             {
                 *eq = '\0';
-                9w9_engine_set_param(e, seg, (float)atof(eq + 1));
+                w9_engine_set_param(e, seg, (float)atof(eq + 1));
             }
         }
         if(!semi) break;
@@ -1007,46 +1003,46 @@
 static int pot_index(const char *key)
 {
     key = pot_alias(key);
-    for(int i=0; i<9W9_POT_COUNT; ++i)
-        if(!strcmp(key, g_9w9_pots[i].key)) return i;
+    for(int i=0; i<W9_POT_COUNT; ++i)
+        if(!strcmp(key, g_w9_pots[i].key)) return i;
     return -1;
 }
 
 
-void 9w9_engine_seed_pots(9w9_engine_t *e)
-{
-    for(int i=0; i<9W9_POT_COUNT; ++i)
+void w9_engine_seed_pots(w9_engine_t *e)
+{
+    for(int i=0; i<W9_POT_COUNT; ++i)
     {
         float v = 0.0f;
-        if(9w9_engine_get_raw(e, g_9w9_pots[i].key, &v))
-            e->pots[i] = (unsigned char)9w9_value_to_pot(&g_9w9_pots[i], v);
-    }
-}
-
-int 9w9_engine_set_param(9w9_engine_t *e, const char *key, const float pot)
+        if(w9_engine_get_raw(e, g_w9_pots[i].key, &v))
+            e->pots[i] = (unsigned char)w9_value_to_pot(&g_w9_pots[i], v);
+    }
+}
+
+int w9_engine_set_param(w9_engine_t *e, const char *key, const float pot)
 {
     if(is_enum_key(key))
-        return 9w9_engine_set_raw(e, key, pot);
+        return w9_engine_set_raw(e, key, pot);
 
     const int idx = pot_index(key);
     if(idx < 0)
-        return 9w9_engine_set_raw(e, key, pot);   /* unknown: treat as raw */
+        return w9_engine_set_raw(e, key, pot);   /* unknown: treat as raw */
 
     int p = (int)(pot + 0.5f);
     if(p < 0)   p = 0;
     if(p > 127) p = 127;
     e->pots[idx] = (unsigned char)p;
-    return 9w9_engine_set_raw(e, key, 9w9_pot_to_value(&g_9w9_pots[idx], p));
-}
-
-int 9w9_engine_get_param(const 9w9_engine_t *e, const char *key, float *pot)
+    return w9_engine_set_raw(e, key, w9_pot_to_value(&g_w9_pots[idx], p));
+}
+
+int w9_engine_get_param(const w9_engine_t *e, const char *key, float *pot)
 {
     if(is_enum_key(key))
-        return 9w9_engine_get_raw(e, key, pot);
+        return w9_engine_get_raw(e, key, pot);
 
     const int idx = pot_index(key);
     if(idx < 0)
-        return 9w9_engine_get_raw(e, key, pot);
+        return w9_engine_get_raw(e, key, pot);
 
     *pot = (float)e->pots[idx];
     return 1;
--- 9pin/src/dsp/9w9_engine.h
+++ 9ren/src/dsp/9w9_engine.h
@@ -1,14 +1,11 @@
 /*
- * 9w9_engine.h — ER-99 drum voices, ported from matthewcieplak/er-99.
+ * 9w9_engine.h — 9W9 drum engine.
  *
- * The original is a Web Audio node graph in TypeScript; this is a faithful
- * reimplementation against the same spec-defined node behaviours (webaudio.h).
- *
- * GPL-3.0 (er-99 is GPL-3.0).
+ * GPL-3.0.
  */
 
-#ifndef 9W9_ENGINE_H
-#define 9W9_ENGINE_H
+#ifndef W9_ENGINE_H
+#define W9_ENGINE_H
 
 #include <stdint.h>
 #include "webaudio.h"
@@ -18,24 +15,24 @@
 #include "9w9_perc909.h"
 
 /* Trigger IDs. 'chh' is not a separate voice — it retriggers the hi-hat with
- * the closed decay time, exactly as er-99's playNote() does. */
+ * the closed decay time. */
 typedef enum {
-    9W9_BD = 0, 9W9_SD, 9W9_LT, 9W9_MT, 9W9_HT,
-    9W9_RS, 9W9_HC,
-    9W9_OHH, 9W9_CHH, 9W9_RC, 9W9_CR,
-    9W9_NUM_TRIGGERS
-} 9w9_trigger_t;
+    W9_BD = 0, W9_SD, W9_LT, W9_MT, W9_HT,
+    W9_RS, W9_HC,
+    W9_OHH, W9_CHH, W9_RC, W9_CR,
+    W9_NUM_TRIGGERS
+} w9_trigger_t;
 
-#define 9W9_NUM_INSTRUMENTS 5   /* bd sd lt mt ht */
+#define W9_NUM_INSTRUMENTS 5   /* bd sd lt mt ht */
 /* Open hat, ride, crash, closed hat. The closed hat is its own sampler rather
  * than a second decay on the open one, so it can have its own tuning, level and
  * drive; it plays the same buffer, and the two choke each other (see
- * 9w9_trigger) because they are one physical hat. */
-#define 9W9_NUM_SAMPLERS    4   /* ohh ride crash chh */
-#define 9W9_SAMP_OHH        0
-#define 9W9_SAMP_RC         1
-#define 9W9_SAMP_CR         2
-#define 9W9_SAMP_CHH        3
+ * w9_trigger) because they are one physical hat. */
+#define W9_NUM_SAMPLERS    4   /* ohh ride crash chh */
+#define W9_SAMP_OHH        0
+#define W9_SAMP_RC         1
+#define W9_SAMP_CR         2
+#define W9_SAMP_CHH        3
 
 /* ---- Oscillator+noise voices: BD, SD, LT, MT, HT ---- */
 
@@ -52,7 +49,7 @@
     float  decay_closed;  /* ms, hh only   */
     float  volume;
     float  pitch;         /* playbackRate  */
-    float  drive;         /* per-sampler distortion (9w9_shape) */
+    float  drive;         /* per-sampler distortion (w9_shape) */
     float  dist_type;
 
     float  crush_st[2];
@@ -62,7 +59,7 @@
     double       pos;
     wa_param_t   out;
     double       mute_countdown;
-} 9w9_sampler_t;
+} w9_sampler_t;
 
 /* ---- Send FX: one reverb, one delay, fed by per-voice sends ----
  *
@@ -74,18 +71,18 @@
  * clicking. Both take a highpass on the send input so low end stays out.
  * Sends default to zero: with them down the engine is bit-identical to one
  * that has no FX at all. The kick has no sends, deliberately. */
-#define 9W9_DLY_MAX 88200            /* 2 s at 44.1k: a dotted half at 90 BPM */
-#define 9W9_DLY_DIVS 13
+#define W9_DLY_MAX 88200            /* 2 s at 44.1k: a dotted half at 90 BPM */
+#define W9_DLY_DIVS 13
 typedef struct {
     float time_ms, fdbk, tone, hpf_hz, level;
     float divi;                       /* note division index, 0..12 */
     float bpm;                        /* fed by the host each block */
     wa_biquad_t hp;
-    float buf[9W9_DLY_MAX];
+    float buf[W9_DLY_MAX];
     int   w;
     float dcur;                       /* slewed delay, samples */
     float lp;                         /* loop damping state    */
-} 9w9_dly_t;
+} w9_dly_t;
 
 typedef struct {
     float decay, tone, hpf_hz, level;
@@ -95,20 +92,18 @@
     float cdmp[4];                    /* per-comb damping state */
     float apb[2][556];
     int   apos[2];
-} 9w9_verb_t;
+} w9_verb_t;
 
 /* ---- Master bus ---- */
 typedef struct {
-    /* No compressor and no limiter. er-99 inherited a Web Audio
-     * DynamicsCompressorNode on the master bus; a TR-909 has nothing of the
-     * kind — its voices sum through a mixer straight into the output amp. The
+    /* No compressor and no limiter: a TR-909 has nothing of the kind — its voices sum through a mixer straight into the output amp. The
      * headroom comes from the gain structure instead (voice Level tops out at
      * the ceiling, master volume 0.35), and the only thing left on the bus is
      * the distortion, which is ours and optional.
      *
      * Master distortion: whole-kit drive.
      * dist_mode: 0=Off, 1=Diode, 2=Hard Clip, 3=Wavefolder, 4=Bitcrush
-     * (modes 1..4 map to 9w9_shape types 0..3). */
+     * (modes 1..4 map to w9_shape types 0..3). */
     float drive;
     float dist_mode;
     float volume;
@@ -133,64 +128,66 @@
     float comp;         /* 0..1 amount; 0 = hard bypass  */
     float comp_env_db;  /* smoothed gain reduction state */
     float comp_det;     /* rectified level follower      */
-} 9w9_master_t;
+} w9_master_t;
 
 typedef struct {
     float sample_rate;
 
-    /* Circuit-informed BD/SD/tom models (9w9_circuit.h). The er-99 legacy
-     * selects which engine the 5 oscillator voices use: 1 = circuit (default,
-     * the good one), 0 = stock er-99 for comparison. */
-    9w9_bt_t         bt[9W9_NUM_INSTRUMENTS];
-    9w9_tom_t        tom909[3]; /* LT/MT/HT, three oscillators each */
-    9w9_rim909_t     rim909;   /* circuit models (default)      */
-    9w9_clap909_t    clap909;
-    9w9_sampler_t    sampler[9W9_NUM_SAMPLERS];
+    /* Circuit-informed BD/SD/tom models (9w9_circuit.h). */
+    w9_bt_t         bt[W9_NUM_INSTRUMENTS];
+    w9_tom_t        tom909[3]; /* LT/MT/HT, three oscillators each */
+    w9_rim909_t     rim909;   /* circuit models (default)      */
+    w9_clap909_t    clap909;
+    w9_sampler_t    sampler[W9_NUM_SAMPLERS];
 
-    9w9_verb_t   verb;
-    9w9_dly_t    dly;
-    float         send_rev[9W9_NUM_TRIGGERS];
-    float         send_dly[9W9_NUM_TRIGGERS];
+    w9_verb_t   verb;
+    w9_dly_t    dly;
+    float         send_rev[W9_NUM_TRIGGERS];
+    float         send_dly[W9_NUM_TRIGGERS];
 
-    9w9_master_t master;
+    w9_master_t master;
     wa_noise_t    noise;
 
 
     /* Pot positions (0..127) — the external parameter surface. Real
-     * engineering values are derived from these via g_9w9_pots. */
-    unsigned char pots[9W9_POT_COUNT];
+     * engineering values are derived from these via g_w9_pots. */
+    unsigned char pots[W9_POT_COUNT];
 
     /* decoded sample data, owned by the engine */
-    float   *sample_data[9W9_NUM_SAMPLERS];
-    uint32_t sample_len[9W9_NUM_SAMPLERS];
-} 9w9_engine_t;
+    float   *sample_data[W9_NUM_SAMPLERS];
+    uint32_t sample_len[W9_NUM_SAMPLERS];
+} w9_engine_t;
 
-/* Initialise with stock er-99 parameters. module_dir may be NULL, in which
+/* Initialise with default parameters. module_dir may be NULL, in which
  * case the samplers stay silent (engine still runs). */
-void 9w9_engine_init(9w9_engine_t *e, float sample_rate, const char *module_dir);
-void 9w9_engine_free(9w9_engine_t *e);
+void w9_engine_init(w9_engine_t *e, float sample_rate, const char *module_dir);
+void w9_engine_free(w9_engine_t *e);
 
 /* velocity 0..127; scales the voice, reaching the Accent gain at 127. */
-void 9w9_engine_trigger(9w9_engine_t *e, 9w9_trigger_t which, int velocity);
+void w9_engine_trigger(w9_engine_t *e, w9_trigger_t which, int velocity);
 
 /* Render mono into out (engine is mono; the plugin duplicates to stereo). */
-void 9w9_engine_render(9w9_engine_t *e, float *out, int frames);
+void w9_engine_render(w9_engine_t *e, float *out, int frames);
 
 /* Raw access in engineering units (ms, Hz, ...). Internal. */
-int 9w9_engine_set_raw(9w9_engine_t *e, const char *key, float value);
-int 9w9_engine_get_raw(const 9w9_engine_t *e, const char *key, float *out);
+int w9_engine_set_raw(w9_engine_t *e, const char *key, float value);
+int w9_engine_get_raw(const w9_engine_t *e, const char *key, float *out);
 
 /* Public parameter surface: pot positions 0..127, like the hardware panel.
  * Enum-style keys (dist_type, master_dist) pass through as
  * their own small integer values rather than being scaled. */
-void 9w9_engine_seed_pots(9w9_engine_t *e);
-int 9w9_engine_set_param(9w9_engine_t *e, const char *key, float pot);
-int 9w9_engine_get_param(const 9w9_engine_t *e, const char *key, float *pot);
+void w9_engine_seed_pots(w9_engine_t *e);
+int w9_engine_set_param(w9_engine_t *e, const char *key, float pot);
+int w9_engine_get_param(const w9_engine_t *e, const char *key, float *pot);
 
 /* Self-contained state blob — required for slot autosave and User Presets. */
-int 9w9_engine_get_state(const 9w9_engine_t *e, char *buf, int len);
-int 9w9_engine_set_state(9w9_engine_t *e, const char *blob);
+/* State blob tag. Blobs saved by v2.7.0 and earlier carry the old tag and
+ * still load, so existing patches and slot autosaves keep their sound. */
+#define W9_STATE_TAG     "9w9v3;"
+#define W9_STATE_TAG_OLD "9w9v3;"
+int w9_engine_get_state(const w9_engine_t *e, char *buf, int len);
+int w9_engine_set_state(w9_engine_t *e, const char *blob);
 
-extern const char *const 9w9_trigger_names[9W9_NUM_TRIGGERS];
+extern const char *const w9_trigger_names[W9_NUM_TRIGGERS];
 
-#endif /* 9W9_ENGINE_H */
+#endif /* W9_ENGINE_H */
--- 9pin/src/dsp/9w9_params.h
+++ 9ren/src/dsp/9w9_params.h
@@ -1,10 +1,10 @@
 /* Generated by scripts/gen_params.py — do not edit by hand.
  * module.json is capped at 8 KB by Schwung's loader, so these are served
  * dynamically from the DSP via get_param(). */
-#ifndef 9W9_PARAMS_H
-#define 9W9_PARAMS_H
-#define 9W9_CHAIN_PARAMS_LEN 8007
-static const char 9w9_chain_params_json[] =
+#ifndef W9_PARAMS_H
+#define W9_PARAMS_H
+#define W9_CHAIN_PARAMS_LEN 8007
+static const char w9_chain_params_json[] =
     "[{\"key\":\"note_map\",\"name\":\"Note Map\",\"type\":\"enum\",\"options\":[\"Drum Rack (36+)\",\"General MIDI\"]},{\"k"
     "ey\":\"bd_c_tune\",\"name\":\"BD Tune\",\"type\":\"int\",\"min\":0,\"max\":127},{\"key\":\"bd_c_attack\",\"name\":\"BD Att"
     "ack\",\"type\":\"int\",\"min\":0,\"max\":127,\"viz\":false},{\"key\":\"bd_c_decay\",\"name\":\"BD Decay\",\"type\":\"int\","
@@ -86,8 +86,8 @@
     "ame\":\"Dly Tone\",\"type\":\"int\",\"min\":0,\"max\":127},{\"key\":\"dly_hpf\",\"name\":\"Dly HPF\",\"type\":\"int\",\"min\""
     ":0,\"max\":127},{\"key\":\"dly_level\",\"name\":\"Dly Level\",\"type\":\"int\",\"min\":0,\"max\":127,\"viz\":{\"kind\":\"fa"
     "der\"}}]";
-#define 9W9_UI_HIERARCHY_LEN 6147
-static const char 9w9_ui_hierarchy_json[] =
+#define W9_UI_HIERARCHY_LEN 6147
+static const char w9_ui_hierarchy_json[] =
     "{\"pad_layout\":\"drums\",\"focus_param\":\"ui_focus_level\",\"levels\":{\"bd\":{\"name\":\"Bass Drum\",\"knobs\":[\"bd"
     "_c_tune\",\"bd_c_attack\",\"bd_c_decay\",\"bd_c_sweep_depth\",\"bd_c_pitch_mod\",\"bd_c_drive\",\"bd_c_dist_type"
     "\",\"bd_c_level\"],\"params\":[{\"key\":\"bd_c_tune\",\"label\":\"Tune\"},{\"key\":\"bd_c_attack\",\"label\":\"Attack\"},"
@@ -153,8 +153,8 @@
 /* The same contract with the General MIDI note map, for g_note_map == 1.
  * The map is switchable at runtime, so one static hierarchy would be wrong
  * half the time; get_param picks. */
-#define 9W9_UI_HIERARCHY_GM_LEN 6147
-static const char 9w9_ui_hierarchy_gm_json[] =
+#define W9_UI_HIERARCHY_GM_LEN 6147
+static const char w9_ui_hierarchy_gm_json[] =
     "{\"pad_layout\":\"drums\",\"focus_param\":\"ui_focus_level\",\"levels\":{\"bd\":{\"name\":\"Bass Drum\",\"knobs\":[\"bd"
     "_c_tune\",\"bd_c_attack\",\"bd_c_decay\",\"bd_c_sweep_depth\",\"bd_c_pitch_mod\",\"bd_c_drive\",\"bd_c_dist_type"
     "\",\"bd_c_level\"],\"params\":[{\"key\":\"bd_c_tune\",\"label\":\"Tune\"},{\"key\":\"bd_c_attack\",\"label\":\"Attack\"},"
--- 9pin/src/dsp/9w9_perc909.h
+++ 9ren/src/dsp/9w9_perc909.h
@@ -1,13 +1,5 @@
 /*
  * 9w9_perc909.h — circuit-informed TR-909 RIM SHOT and HAND CLAP.
- *
- * Replaces er-99's versions, which are not the target any more:
- *
- *  - er-99's rim is a single 220 Hz bandpass (its "parallel" topology only ever
- *    feeds filter 0 — the other two are wired to the output but nothing drives
- *    them), so it comes out dull and one-dimensional.
- *  - er-99's clap approximates the burst with five scheduled gain taps and no
- *    real delay line.
  *
  * The hardware:
  *
@@ -26,11 +18,11 @@
  * GPL-3.0.
  */
 
-#ifndef 9W9_PERC909_H
-#define 9W9_PERC909_H
+#ifndef W9_PERC909_H
+#define W9_PERC909_H
 
 #include "webaudio.h"
-#include "9w9_circuit.h"   /* 9w9_shape() */
+#include "9w9_circuit.h"   /* w9_shape() */
 
 /* ===================== RIM SHOT ===================== */
 typedef struct {
@@ -50,9 +42,9 @@
     double      mute_countdown;
     float       sample_rate;
     float       accent_gain_;
-} 9w9_rim909_t;
-
-static inline void 9w9_rim909_init(9w9_rim909_t *v, const float _sr)
+} w9_rim909_t;
+
+static inline void w9_rim909_init(w9_rim909_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune  > 0 ? v->tune  : 220.0f,
@@ -70,13 +62,13 @@
     v->accent_gain_ = 1.0f;
 }
 
-static inline void 9w9_rim909_retune(9w9_rim909_t *v)
+static inline void w9_rim909_retune(w9_rim909_t *v)
 {
     wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
     wa_biquad_set(&v->bp2, WA_BANDPASS, v->tune2, v->res * 1.1f, 0.0f, v->sample_rate);
 }
 
-static inline void 9w9_rim909_trigger(9w9_rim909_t *v, const float _accent)
+static inline void w9_rim909_trigger(w9_rim909_t *v, const float _accent)
 {
     const float ms = 0.001f * v->sample_rate;
     wa_set_value(&v->amp, 1.0f);
@@ -86,7 +78,7 @@
     v->mute_countdown = (v->decay + 20.0f) * ms;
 }
 
-static inline float 9w9_rim909_render(9w9_rim909_t *v, const float _noise)
+static inline float w9_rim909_render(w9_rim909_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -113,7 +105,7 @@
      * had been living off that hiss, and lost most of its level when it went. */
     float y = wa_biquad_tick(&v->bp1, exc) * 60.0f
             + wa_biquad_tick(&v->bp2, exc) * 32.0f;
-    y = 9w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);
+    y = w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);
     y = wa_biquad_tick(&v->hp, y);
     return y * v->level * v->accent_gain_;
 }
@@ -138,9 +130,9 @@
     double      mute_countdown;
     float       sample_rate;
     float       accent_gain_;
-} 9w9_clap909_t;
-
-static inline void 9w9_clap909_init(9w9_clap909_t *v, const float _sr)
+} w9_clap909_t;
+
+static inline void w9_clap909_init(w9_clap909_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_biquad_set(&v->bp, WA_BANDPASS, v->tune > 0 ? v->tune : 1000.0f,
@@ -153,12 +145,12 @@
     v->accent_gain_ = 1.0f;
 }
 
-static inline void 9w9_clap909_retune(9w9_clap909_t *v)
+static inline void w9_clap909_retune(w9_clap909_t *v)
 {
     wa_biquad_set(&v->bp, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
 }
 
-static inline void 9w9_clap909_trigger(9w9_clap909_t *v, const float _accent)
+static inline void w9_clap909_trigger(w9_clap909_t *v, const float _accent)
 {
     const float ms = 0.001f * v->sample_rate;
     v->pulse_index = 0;
@@ -172,7 +164,7 @@
     v->mute_countdown = (v->tail_decay + v->spread * 4.0f + 40.0f) * ms;
 }
 
-static inline float 9w9_clap909_render(9w9_clap909_t *v, const float _noise)
+static inline float w9_clap909_render(w9_clap909_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -205,9 +197,9 @@
 
     const float env = wa_param_tick(&v->burst) + wa_param_tick(&v->tail);
     float y = wa_biquad_tick(&v->bp, _noise) * env;
-    y = 9w9_shape_st(y, v->drive, v->dist_type, v->crush_st);
+    y = w9_shape_st(y, v->drive, v->dist_type, v->crush_st);
     y = wa_biquad_tick(&v->hp, y);
     return y * v->level * v->accent_gain_;
 }
 
-#endif /* 9W9_PERC909_H */
+#endif /* W9_PERC909_H */
--- 9pin/src/dsp/9w9_plugin.c
+++ 9ren/src/dsp/9w9_plugin.c
@@ -1,5 +1,5 @@
 /*
- * 9w9_plugin.c — Schwung plugin_api_v2 wrapper for the ER-99 engine.
+ * 9w9_plugin.c — Schwung plugin_api_v2 wrapper for the 9W9 engine.
  *
  * Runs in-process inside the shim's SPI callback (SCHED_FIFO 90). render_block
  * therefore does no allocation, no file I/O and takes no locks — all of that
@@ -20,7 +20,7 @@
 static const host_api_v1_t *g_host = NULL;
 
 typedef struct {
-    9w9_engine_t engine;
+    w9_engine_t engine;
     float last_bpm;
     char          module_dir[512];
     /* Pad-follow state. Deliberately NOT updated while the transport runs —
@@ -39,7 +39,7 @@
 
     /* Per-lane performance mutes (Mute+Pad in the editor). Bit n = lane n. */
     uint16_t      lane_mutes;
-} 9w9_instance_t;
+} w9_instance_t;
 
 /* ---- MIDI note map -------------------------------------------------- */
 /*
@@ -61,14 +61,14 @@
     switch(_note)
     {
     /* row 0 */
-    case 68: return 9W9_BD;   case 69: return 9W9_SD;
-    case 70: return 9W9_LT;   case 71: return 9W9_MT;
+    case 68: return W9_BD;   case 69: return W9_SD;
+    case 70: return W9_LT;   case 71: return W9_MT;
     /* row 1 */
-    case 76: return 9W9_HT;   case 77: return 9W9_RS;
-    case 78: return 9W9_HC;   case 79: return 9W9_CHH;
+    case 76: return W9_HT;   case 77: return W9_RS;
+    case 78: return W9_HC;   case 79: return W9_CHH;
     /* row 2 */
-    case 84: return 9W9_OHH;  case 85: return 9W9_CR;
-    case 86: return 9W9_RC;
+    case 84: return W9_OHH;  case 85: return W9_CR;
+    case 86: return W9_RC;
     default: return -1;
     }
 }
@@ -91,12 +91,12 @@
 {
     switch(_note)
     {
-    case 36: return 9W9_BD;   case 37: return 9W9_SD;
-    case 38: return 9W9_LT;   case 39: return 9W9_MT;
-    case 40: return 9W9_HT;   case 41: return 9W9_RS;
-    case 42: return 9W9_HC;   case 43: return 9W9_CHH;
-    case 44: return 9W9_OHH;  case 45: return 9W9_CR;
-    case 46: return 9W9_RC;
+    case 36: return W9_BD;   case 37: return W9_SD;
+    case 38: return W9_LT;   case 39: return W9_MT;
+    case 40: return W9_HT;   case 41: return W9_RS;
+    case 42: return W9_HC;   case 43: return W9_CHH;
+    case 44: return W9_OHH;  case 45: return W9_CR;
+    case 46: return W9_RC;
     default: return -1;
     }
 }
@@ -115,17 +115,17 @@
     switch(_note)
     {
     /* GM drum map */
-    case 36: return 9W9_BD;   case 35: return 9W9_BD;
-    case 38: return 9W9_SD;   case 40: return 9W9_SD;
-    case 41: return 9W9_LT;   case 45: return 9W9_LT;
-    case 47: return 9W9_MT;   case 48: return 9W9_MT;
-    case 50: return 9W9_HT;   case 43: return 9W9_HT;
-    case 37: return 9W9_RS;
-    case 39: return 9W9_HC;
-    case 42: return 9W9_CHH;
-    case 46: return 9W9_OHH;
-    case 51: return 9W9_RC;
-    case 49: return 9W9_CR;
+    case 36: return W9_BD;   case 35: return W9_BD;
+    case 38: return W9_SD;   case 40: return W9_SD;
+    case 41: return W9_LT;   case 45: return W9_LT;
+    case 47: return W9_MT;   case 48: return W9_MT;
+    case 50: return W9_HT;   case 43: return W9_HT;
+    case 37: return W9_RS;
+    case 39: return W9_HC;
+    case 42: return W9_CHH;
+    case 46: return W9_OHH;
+    case 51: return W9_RC;
+    case 49: return W9_CR;
     default: break;
     }
     return -1;
@@ -136,7 +136,7 @@
 static void *create_instance(const char *_module_dir, const char *_json_defaults)
 {
     (void)_json_defaults;
-    9w9_instance_t *inst = (9w9_instance_t*)calloc(1, sizeof(9w9_instance_t));
+    w9_instance_t *inst = (w9_instance_t*)calloc(1, sizeof(w9_instance_t));
     if(!inst) return NULL;
 
     if(_module_dir)
@@ -145,7 +145,7 @@
     const float sr = g_host && g_host->sample_rate > 0
                    ? (float)g_host->sample_rate : 44100.0f;
 
-    9w9_engine_init(&inst->engine, sr, _module_dir);
+    w9_engine_init(&inst->engine, sr, _module_dir);
 
     if(g_host && g_host->log)
         g_host->log("9w9: engine ready");
@@ -154,15 +154,15 @@
 
 static void destroy_instance(void *_instance)
 {
-    9w9_instance_t *inst = (9w9_instance_t*)_instance;
+    w9_instance_t *inst = (w9_instance_t*)_instance;
     if(!inst) return;
-    9w9_engine_free(&inst->engine);
+    w9_engine_free(&inst->engine);
     free(inst);
 }
 
 static void on_midi(void *_instance, const uint8_t *_msg, const int _len, const int _source)
 {
-    9w9_instance_t *inst = (9w9_instance_t*)_instance;
+    w9_instance_t *inst = (w9_instance_t*)_instance;
     if(!inst || _len < 3) return;
 
     const uint8_t status = _msg[0] & 0xF0u;
@@ -185,7 +185,7 @@
         if(inst->mute_one) { inst->mute_one = 0; return; }
     }
 
-    9w9_engine_trigger(&inst->engine, (9w9_trigger_t)t, (int)vel);
+    w9_engine_trigger(&inst->engine, (w9_trigger_t)t, (int)vel);
 
     /* Only a hand-played hit moves the editor. While Move's sequencer is
      * running, notes arrive constantly and following them makes the UI
@@ -201,7 +201,7 @@
 
 static void set_param(void *_instance, const char *_key, const char *_val)
 {
-    9w9_instance_t *inst = (9w9_instance_t*)_instance;
+    w9_instance_t *inst = (w9_instance_t*)_instance;
     if(!inst || !_key || !_val) return;
 
     if(!strcmp(_key, "note_map"))
@@ -226,7 +226,7 @@
     /* Slot autosave / User Presets restore through this key. */
     if(!strcmp(_key, "state"))
     {
-        9w9_engine_set_state(&inst->engine, _val);
+        w9_engine_set_state(&inst->engine, _val);
         /* Plugin-level keys piggyback on the same blob. */
         {
             const char *mq = strstr(_val, "mutes=");
@@ -234,21 +234,21 @@
         }
         return;
     }
-    9w9_engine_set_param(&inst->engine, _key, (float)atof(_val));
+    w9_engine_set_param(&inst->engine, _key, (float)atof(_val));
 }
 
 static int get_param(void *_instance, const char *_key, char *_buf, const int _len)
 {
-    9w9_instance_t *inst = (9w9_instance_t*)_instance;
+    w9_instance_t *inst = (w9_instance_t*)_instance;
     if(!inst || !_key || !_buf || _len <= 0) return -1;
 
     /* Served dynamically: module.json is capped at 8 KB by the loader and our
      * parameter surface is much larger than that. */
     if(!strcmp(_key, "chain_params"))
     {
-        if(_len <= 9W9_CHAIN_PARAMS_LEN) return -1;   /* refuse to truncate */
-        memcpy(_buf, 9w9_chain_params_json, 9W9_CHAIN_PARAMS_LEN + 1);
-        return 9W9_CHAIN_PARAMS_LEN;
+        if(_len <= W9_CHAIN_PARAMS_LEN) return -1;   /* refuse to truncate */
+        memcpy(_buf, w9_chain_params_json, W9_CHAIN_PARAMS_LEN + 1);
+        return W9_CHAIN_PARAMS_LEN;
     }
     /* ui_hierarchy is deliberately served EMPTY, not refused. The Shadow UI's
      * enterComponentEdit uses the hierarchy editor whenever a module offers
@@ -279,9 +279,9 @@
          * it would put every voice in the wrong place with nothing to say why.
          * Two strings, one pointer choice, nothing built on the audio thread. */
         const int gm = (g_note_map != 0);
-        const char *const j = gm ? 9w9_ui_hierarchy_gm_json
-                                 : 9w9_ui_hierarchy_json;
-        const int n = gm ? 9W9_UI_HIERARCHY_GM_LEN : 9W9_UI_HIERARCHY_LEN;
+        const char *const j = gm ? w9_ui_hierarchy_gm_json
+                                 : w9_ui_hierarchy_json;
+        const int n = gm ? W9_UI_HIERARCHY_GM_LEN : W9_UI_HIERARCHY_LEN;
         if(_len <= n) return -1;
         memcpy(_buf, j, (size_t)n + 1);
         return n;
@@ -304,11 +304,11 @@
          * crash as four separate pages. A host resolving a focus answer
          * against the declared levels found nothing for any of the four, so
          * the two hats and the two cymbals silently never followed. */
-        static const char *const level_of[9W9_NUM_TRIGGERS] = {
+        static const char *const level_of[W9_NUM_TRIGGERS] = {
             "bd","sd","lt","mt","ht","rim","clap","ohh","chh","ride","crash"
         };
         const int v = inst->focus_voice;
-        if(inst->focus_count == 0 || v < 0 || v >= 9W9_NUM_TRIGGERS) return -1;
+        if(inst->focus_count == 0 || v < 0 || v >= W9_NUM_TRIGGERS) return -1;
         return snprintf(_buf, (size_t)_len, "%u:%s",
                         inst->focus_count, level_of[v]);
     }
@@ -324,7 +324,7 @@
         return snprintf(_buf, (size_t)_len, "%u", (unsigned)inst->lane_mutes);
     if(!strcmp(_key, "state"))
     {
-        int n = 9w9_engine_get_state(&inst->engine, _buf, _len);
+        int n = w9_engine_get_state(&inst->engine, _buf, _len);
         if(n < 0) return n;
         if(n < _len - 1)
             n += snprintf(_buf + n, (size_t)(_len - n), "mutes=%u;",
@@ -336,7 +336,7 @@
         return snprintf(_buf, (size_t)_len, "%d", g_note_map);
 
     float v = 0.0f;
-    if(9w9_engine_get_param(&inst->engine, _key, &v))
+    if(w9_engine_get_param(&inst->engine, _key, &v))
         return snprintf(_buf, (size_t)_len, "%.4f", v);
 
     return -1;
@@ -350,7 +350,7 @@
 
 static void render_block(void *_instance, int16_t *_out_lr, const int _frames)
 {
-    9w9_instance_t *inst = (9w9_instance_t*)_instance;
+    w9_instance_t *inst = (w9_instance_t*)_instance;
     if(!inst) { memset(_out_lr, 0, (size_t)_frames * 2 * sizeof(int16_t)); return; }
 
     inst->samples_rendered += (uint64_t)_frames;
@@ -364,7 +364,7 @@
         if(bpm > 20.0f && bpm != inst->last_bpm)
         {
             inst->last_bpm = bpm;
-            9w9_engine_set_raw(&inst->engine, "dly_bpm", bpm);
+            w9_engine_set_raw(&inst->engine, "dly_bpm", bpm);
         }
     }
 
@@ -377,7 +377,7 @@
         int chunk = _frames - done;
         if(chunk > (int)(sizeof(mono)/sizeof(mono[0]))) chunk = (int)(sizeof(mono)/sizeof(mono[0]));
 
-        9w9_engine_render(&inst->engine, mono, chunk);
+        w9_engine_render(&inst->engine, mono, chunk);
 
         for(int i=0; i<chunk; ++i)
         {
--- 9pin/src/dsp/9w9_pots.h
+++ 9ren/src/dsp/9w9_pots.h
@@ -13,195 +13,195 @@
  * GPL-3.0.
  */
 
-#ifndef 9W9_POTS_H
-#define 9W9_POTS_H
+#ifndef W9_POTS_H
+#define W9_POTS_H
 
 #include <math.h>
 
-typedef enum { 9W9_LIN = 0, 9W9_EXP = 1 } 9w9_curve_t;
+typedef enum { W9_LIN = 0, W9_EXP = 1 } w9_curve_t;
 
 typedef struct {
     const char  *key;    /* same key the engine's raw setter uses */
     float        min;
     float        max;
-    9w9_curve_t curve;
-} 9w9_pot_t;
+    w9_curve_t curve;
+} w9_pot_t;
 
 /* Order is the storage order for pot values and for the state blob. */
-static const 9w9_pot_t g_9w9_pots[] = {
+static const w9_pot_t g_w9_pots[] = {
     /* --- Bass Drum --- */
-    { "bd_c_tune",          6.0f,   32.0f, 9W9_LIN },
+    { "bd_c_tune",          6.0f,   32.0f, W9_LIN },
     /* From the board: ENV1 is C8 0.33 uF discharging through VR5 1M(A) plus
      * R58 47K — tau 15 ms with the pot closed, 345 ms wide open. This ramp's
      * tau is nominal/11.5, so 100..4000 nominal covers exactly that span; the
      * measured "Long" recording (tau ~120 ms, t60 760 ms) sits inside it.
      * The old 15-400 ms range topped out at a 35 ms tau and could not reach
      * a 909 kick at any setting. */
-    { "bd_c_decay",       100.0f, 4000.0f, 9W9_EXP },
-    { "bd_c_attack",        0.0f,    1.0f, 9W9_LIN },
+    { "bd_c_decay",       100.0f, 4000.0f, W9_EXP },
+    { "bd_c_attack",        0.0f,    1.0f, W9_LIN },
     /* The two kick mod pots. P.Depth 0 = stock kick bit for bit, and Pitch is
      * inert until P.Depth opens it (RD-9 behaviour, per Gus). Pitch spans the
      * doc's 0.43x..4.7x of the usual frequency; unity sits at pot ~45. */
-    { "bd_c_sweep_depth",   0.0f,    1.0f, 9W9_LIN },
-    { "bd_c_pitch_mod",     0.43f,   4.7f, 9W9_EXP },
-    { "bd_c_sweep_time",     8.0f,  136.0f, 9W9_EXP },
+    { "bd_c_sweep_depth",   0.0f,    1.0f, W9_LIN },
+    { "bd_c_pitch_mod",     0.43f,   4.7f, W9_EXP },
+    { "bd_c_sweep_time",     8.0f,  136.0f, W9_EXP },
     /* Drive pots start at 0.85 — just under the shaping knee — instead of 0.2:
      * below unity every type is level-normalised transparent, so the old range
      * spent pots 0-52 doing nothing ("only kinda kicks in around 53"). Bite
      * now arrives by pot ~12; the top reaches half again further. */
-    { "bd_c_drive",         0.85f,   12.0f, 9W9_EXP },
+    { "bd_c_drive",         0.85f,   12.0f, W9_EXP },
     /* Level tops out AT the output ceiling, not past it: 1.35 x accent (2.0)
      * x master volume (0.35) = 0.945 FS. Ranges that went to 2.0 (3.0 on the
      * rim and clap) put the last third of every pot into the limiter, which is
      * why turning Level up past a point stopped doing anything. */
-    { "bd_c_level",         0.0f,    1.35f, 9W9_LIN },
-    { "bd_c_click_tone",  500.0f, 8000.0f, 9W9_EXP },
-    { "bd_c_tune2",        20.0f,  200.0f, 9W9_EXP },
-    { "bd_c_osc2_mix",      0.0f,    1.0f, 9W9_LIN },
+    { "bd_c_level",         0.0f,    1.35f, W9_LIN },
+    { "bd_c_click_tone",  500.0f, 8000.0f, W9_EXP },
+    { "bd_c_tune2",        20.0f,  200.0f, W9_EXP },
+    { "bd_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
 
     /* --- Snare --- */
-    { "sd_c_tune",         130.0f,  320.0f, 9W9_EXP },
-    { "sd_c_decay",         60.0f,  700.0f, 9W9_EXP },
-    { "sd_c_attack",        0.0f,    1.0f, 9W9_LIN },
-    { "sd_c_sweep_depth",   1.0f,    6.0f, 9W9_LIN },
-    { "sd_c_sweep_time",    1.0f,   60.0f, 9W9_EXP },
-    { "sd_c_drive",         0.85f,   12.0f, 9W9_EXP },
-    { "sd_c_level",         0.0f,    1.35f, 9W9_LIN },
-    { "sd_c_click_tone",  500.0f, 8000.0f, 9W9_EXP },
-    { "sd_c_tune2",       150.0f,  800.0f, 9W9_EXP },
-    { "sd_c_osc2_mix",      0.0f,    1.0f, 9W9_LIN },
-    { "sd_c_snappy",        0.0f,    1.0f, 9W9_LIN },
-    { "sd_c_noise_decay",  300.0f, 4000.0f, 9W9_EXP },
-    { "sd_c_noise_hp",    200.0f, 6000.0f, 9W9_EXP },
+    { "sd_c_tune",         130.0f,  320.0f, W9_EXP },
+    { "sd_c_decay",         60.0f,  700.0f, W9_EXP },
+    { "sd_c_attack",        0.0f,    1.0f, W9_LIN },
+    { "sd_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
+    { "sd_c_sweep_time",    1.0f,   60.0f, W9_EXP },
+    { "sd_c_drive",         0.85f,   12.0f, W9_EXP },
+    { "sd_c_level",         0.0f,    1.35f, W9_LIN },
+    { "sd_c_click_tone",  500.0f, 8000.0f, W9_EXP },
+    { "sd_c_tune2",       150.0f,  800.0f, W9_EXP },
+    { "sd_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
+    { "sd_c_snappy",        0.0f,    1.0f, W9_LIN },
+    { "sd_c_noise_decay",  300.0f, 4000.0f, W9_EXP },
+    { "sd_c_noise_hp",    200.0f, 6000.0f, W9_EXP },
 
     /* --- Toms --- */
     /* Tom Tune pots are trims around each voice's own center (68.5 / 102 /
      * 131.5 Hz measured from a real 909), as the 10K VR on the board is: the
      * ranges only meet at the edges, so the toms can never trade places. */
-    { "lt_c_tune",           50.0f,   90.0f, 9W9_EXP },
-    { "lt_c_decay",          80.0f, 2600.0f, 9W9_EXP },
-    { "lt_c_attack",        0.0f,    1.0f, 9W9_LIN },
-    { "lt_c_sweep_depth",   1.0f,    6.0f, 9W9_LIN },
-    { "lt_c_sweep_time",    1.0f,  250.0f, 9W9_EXP },
-    { "lt_c_drive",         0.85f,   12.0f, 9W9_EXP },
-    { "lt_c_level",         0.0f,    1.35f, 9W9_LIN },
-    { "lt_c_tune2",        20.0f,  300.0f, 9W9_EXP },
-    { "lt_c_osc2_mix",      0.0f,    1.0f, 9W9_LIN },
-
-    { "mt_c_tune",           80.0f,  125.0f, 9W9_EXP },
-    { "mt_c_decay",          70.0f, 2200.0f, 9W9_EXP },
-    { "mt_c_attack",        0.0f,    1.0f, 9W9_LIN },
-    { "mt_c_sweep_depth",   1.0f,    6.0f, 9W9_LIN },
-    { "mt_c_sweep_time",    1.0f,  250.0f, 9W9_EXP },
-    { "mt_c_drive",         0.85f,   12.0f, 9W9_EXP },
-    { "mt_c_level",         0.0f,    1.35f, 9W9_LIN },
-    { "mt_c_tune2",        30.0f,  400.0f, 9W9_EXP },
-    { "mt_c_osc2_mix",      0.0f,    1.0f, 9W9_LIN },
-
-    { "ht_c_tune",          110.0f,  170.0f, 9W9_EXP },
-    { "ht_c_decay",          60.0f, 2000.0f, 9W9_EXP },
-    { "ht_c_attack",        0.0f,    1.0f, 9W9_LIN },
-    { "ht_c_sweep_depth",   1.0f,    6.0f, 9W9_LIN },
-    { "ht_c_sweep_time",    1.0f,  250.0f, 9W9_EXP },
-    { "ht_c_drive",         0.85f,   12.0f, 9W9_EXP },
-    { "ht_c_level",         0.0f,    1.35f, 9W9_LIN },
-    { "ht_c_tune2",        50.0f,  600.0f, 9W9_EXP },
-    { "ht_c_osc2_mix",      0.0f,    1.0f, 9W9_LIN },
+    { "lt_c_tune",           50.0f,   90.0f, W9_EXP },
+    { "lt_c_decay",          80.0f, 2600.0f, W9_EXP },
+    { "lt_c_attack",        0.0f,    1.0f, W9_LIN },
+    { "lt_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
+    { "lt_c_sweep_time",    1.0f,  250.0f, W9_EXP },
+    { "lt_c_drive",         0.85f,   12.0f, W9_EXP },
+    { "lt_c_level",         0.0f,    1.35f, W9_LIN },
+    { "lt_c_tune2",        20.0f,  300.0f, W9_EXP },
+    { "lt_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
+
+    { "mt_c_tune",           80.0f,  125.0f, W9_EXP },
+    { "mt_c_decay",          70.0f, 2200.0f, W9_EXP },
+    { "mt_c_attack",        0.0f,    1.0f, W9_LIN },
+    { "mt_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
+    { "mt_c_sweep_time",    1.0f,  250.0f, W9_EXP },
+    { "mt_c_drive",         0.85f,   12.0f, W9_EXP },
+    { "mt_c_level",         0.0f,    1.35f, W9_LIN },
+    { "mt_c_tune2",        30.0f,  400.0f, W9_EXP },
+    { "mt_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
+
+    { "ht_c_tune",          110.0f,  170.0f, W9_EXP },
+    { "ht_c_decay",          60.0f, 2000.0f, W9_EXP },
+    { "ht_c_attack",        0.0f,    1.0f, W9_LIN },
+    { "ht_c_sweep_depth",   1.0f,    6.0f, W9_LIN },
+    { "ht_c_sweep_time",    1.0f,  250.0f, W9_EXP },
+    { "ht_c_drive",         0.85f,   12.0f, W9_EXP },
+    { "ht_c_level",         0.0f,    1.35f, W9_LIN },
+    { "ht_c_tune2",        50.0f,  600.0f, W9_EXP },
+    { "ht_c_osc2_mix",      0.0f,    1.0f, W9_LIN },
 
     /* --- Rim (909 model) --- */
-    { "rs_tune",          150.0f,  300.0f, 9W9_EXP },
-    { "rs_tune2",         340.0f,  680.0f, 9W9_EXP },
-    { "rs_res",             4.0f,   20.0f, 9W9_EXP },
-    { "rs_decay",          40.0f,  700.0f, 9W9_EXP },
-    { "rs_noise",           0.0f,    1.0f, 9W9_LIN },
-    { "rs_saturation",      0.85f,   12.0f, 9W9_EXP },
-    { "rs_volume",          0.0f,    1.35f, 9W9_LIN },
+    { "rs_tune",          150.0f,  300.0f, W9_EXP },
+    { "rs_tune2",         340.0f,  680.0f, W9_EXP },
+    { "rs_res",             4.0f,   20.0f, W9_EXP },
+    { "rs_decay",          40.0f,  700.0f, W9_EXP },
+    { "rs_noise",           0.0f,    1.0f, W9_LIN },
+    { "rs_saturation",      0.85f,   12.0f, W9_EXP },
+    { "rs_volume",          0.0f,    1.35f, W9_LIN },
 
     /* --- Clap (909 model) --- */
-    { "hc_tune",          650.0f, 1400.0f, 9W9_EXP },
-    { "hc_spread",          2.0f,   30.0f, 9W9_EXP },
-    { "hc_tone_decay",      2.0f,   40.0f, 9W9_EXP },   /* burst pulse decay */
-    { "hc_decay",         120.0f, 1000.0f, 9W9_EXP },   /* room tail         */
-    { "hc_tail",            0.0f,    1.0f, 9W9_LIN },
-    { "hc_drive",           0.85f,   12.0f, 9W9_EXP },
-    { "hc_volume",          0.0f,    1.35f, 9W9_LIN },
+    { "hc_tune",          650.0f, 1400.0f, W9_EXP },
+    { "hc_spread",          2.0f,   30.0f, W9_EXP },
+    { "hc_tone_decay",      2.0f,   40.0f, W9_EXP },   /* burst pulse decay */
+    { "hc_decay",         120.0f, 1000.0f, W9_EXP },   /* room tail         */
+    { "hc_tail",            0.0f,    1.0f, W9_LIN },
+    { "hc_drive",           0.85f,   12.0f, W9_EXP },
+    { "hc_volume",          0.0f,    1.35f, W9_LIN },
 
     /* --- Hats / cymbals --- */
-    { "ohh_decay",         20.0f, 1200.0f, 9W9_EXP },
-    { "ohh_pitch",          0.25f,   4.0f, 9W9_EXP },
-    { "ohh_volume",         0.0f,    1.35f, 9W9_LIN },
-    { "ohh_drive",          0.85f,   12.0f, 9W9_EXP },
-    { "chh_decay",         15.0f,  300.0f, 9W9_EXP },
-    { "chh_pitch",          0.25f,   4.0f, 9W9_EXP },
-    { "chh_volume",         0.0f,    1.35f, 9W9_LIN },
-    { "chh_drive",          0.85f,   12.0f, 9W9_EXP },
-    { "rc_decay",         100.0f, 3000.0f, 9W9_EXP },
-    { "rc_pitch",           0.25f,   4.0f, 9W9_EXP },
-    { "rc_volume",          0.0f,    1.35f, 9W9_LIN },
-    { "rc_drive",           0.85f,   12.0f, 9W9_EXP },
-    { "cr_decay",         100.0f, 3000.0f, 9W9_EXP },
-    { "cr_pitch",           0.25f,   4.0f, 9W9_EXP },
-    { "cr_volume",          0.0f,    1.35f, 9W9_LIN },
-    { "cr_drive",           0.85f,   12.0f, 9W9_EXP },
+    { "ohh_decay",         20.0f, 1200.0f, W9_EXP },
+    { "ohh_pitch",          0.25f,   4.0f, W9_EXP },
+    { "ohh_volume",         0.0f,    1.35f, W9_LIN },
+    { "ohh_drive",          0.85f,   12.0f, W9_EXP },
+    { "chh_decay",         15.0f,  300.0f, W9_EXP },
+    { "chh_pitch",          0.25f,   4.0f, W9_EXP },
+    { "chh_volume",         0.0f,    1.35f, W9_LIN },
+    { "chh_drive",          0.85f,   12.0f, W9_EXP },
+    { "rc_decay",         100.0f, 3000.0f, W9_EXP },
+    { "rc_pitch",           0.25f,   4.0f, W9_EXP },
+    { "rc_volume",          0.0f,    1.35f, W9_LIN },
+    { "rc_drive",           0.85f,   12.0f, W9_EXP },
+    { "cr_decay",         100.0f, 3000.0f, W9_EXP },
+    { "cr_pitch",           0.25f,   4.0f, W9_EXP },
+    { "cr_volume",          0.0f,    1.35f, W9_LIN },
+    { "cr_drive",           0.85f,   12.0f, W9_EXP },
 
     /* --- Global --- */
     /* FX sends (kick has none, deliberately) and the two send FX. */
-    { "sd_c_rev",           0.0f,    1.0f, 9W9_LIN },
-    { "sd_c_dly",           0.0f,    1.0f, 9W9_LIN },
-    { "lt_c_rev",           0.0f,    1.0f, 9W9_LIN },
-    { "lt_c_dly",           0.0f,    1.0f, 9W9_LIN },
-    { "mt_c_rev",           0.0f,    1.0f, 9W9_LIN },
-    { "mt_c_dly",           0.0f,    1.0f, 9W9_LIN },
-    { "ht_c_rev",           0.0f,    1.0f, 9W9_LIN },
-    { "ht_c_dly",           0.0f,    1.0f, 9W9_LIN },
-    { "rs_rev",             0.0f,    1.0f, 9W9_LIN },
-    { "rs_dly",             0.0f,    1.0f, 9W9_LIN },
-    { "hc_rev",             0.0f,    1.0f, 9W9_LIN },
-    { "hc_dly",             0.0f,    1.0f, 9W9_LIN },
-    { "ohh_rev",            0.0f,    1.0f, 9W9_LIN },
-    { "ohh_dly",            0.0f,    1.0f, 9W9_LIN },
-    { "chh_rev",            0.0f,    1.0f, 9W9_LIN },
-    { "chh_dly",            0.0f,    1.0f, 9W9_LIN },
-    { "rc_rev",             0.0f,    1.0f, 9W9_LIN },
-    { "rc_dly",             0.0f,    1.0f, 9W9_LIN },
-    { "cr_rev",             0.0f,    1.0f, 9W9_LIN },
-    { "cr_dly",             0.0f,    1.0f, 9W9_LIN },
-    { "rev_decay",          0.2f,    0.93f, 9W9_LIN },
-    { "rev_tone",           0.0f,    1.0f, 9W9_LIN },
-    { "rev_hpf",           30.0f,  800.0f, 9W9_EXP },
-    { "rev_level",          0.0f,    1.2f, 9W9_LIN },
-    { "dly_fdbk",           0.0f,    0.85f, 9W9_LIN },
-    { "dly_tone",           0.0f,    1.0f, 9W9_LIN },
-    { "dly_hpf",           30.0f,  800.0f, 9W9_EXP },
-    { "dly_level",          0.0f,    1.2f, 9W9_LIN },
-    { "master_drive",       0.85f,   12.0f, 9W9_EXP },
-    { "master_comp",        0.0f,    1.0f, 9W9_LIN },
-    { "accent",             1.0f,    4.0f, 9W9_LIN },
-    { "vel_depth",          0.0f,    1.0f, 9W9_LIN },
-    { "volume",             0.0f,    1.0f, 9W9_LIN },
+    { "sd_c_rev",           0.0f,    1.0f, W9_LIN },
+    { "sd_c_dly",           0.0f,    1.0f, W9_LIN },
+    { "lt_c_rev",           0.0f,    1.0f, W9_LIN },
+    { "lt_c_dly",           0.0f,    1.0f, W9_LIN },
+    { "mt_c_rev",           0.0f,    1.0f, W9_LIN },
+    { "mt_c_dly",           0.0f,    1.0f, W9_LIN },
+    { "ht_c_rev",           0.0f,    1.0f, W9_LIN },
+    { "ht_c_dly",           0.0f,    1.0f, W9_LIN },
+    { "rs_rev",             0.0f,    1.0f, W9_LIN },
+    { "rs_dly",             0.0f,    1.0f, W9_LIN },
+    { "hc_rev",             0.0f,    1.0f, W9_LIN },
+    { "hc_dly",             0.0f,    1.0f, W9_LIN },
+    { "ohh_rev",            0.0f,    1.0f, W9_LIN },
+    { "ohh_dly",            0.0f,    1.0f, W9_LIN },
+    { "chh_rev",            0.0f,    1.0f, W9_LIN },
+    { "chh_dly",            0.0f,    1.0f, W9_LIN },
+    { "rc_rev",             0.0f,    1.0f, W9_LIN },
+    { "rc_dly",             0.0f,    1.0f, W9_LIN },
+    { "cr_rev",             0.0f,    1.0f, W9_LIN },
+    { "cr_dly",             0.0f,    1.0f, W9_LIN },
+    { "rev_decay",          0.2f,    0.93f, W9_LIN },
+    { "rev_tone",           0.0f,    1.0f, W9_LIN },
+    { "rev_hpf",           30.0f,  800.0f, W9_EXP },
+    { "rev_level",          0.0f,    1.2f, W9_LIN },
+    { "dly_fdbk",           0.0f,    0.85f, W9_LIN },
+    { "dly_tone",           0.0f,    1.0f, W9_LIN },
+    { "dly_hpf",           30.0f,  800.0f, W9_EXP },
+    { "dly_level",          0.0f,    1.2f, W9_LIN },
+    { "master_drive",       0.85f,   12.0f, W9_EXP },
+    { "master_comp",        0.0f,    1.0f, W9_LIN },
+    { "accent",             1.0f,    4.0f, W9_LIN },
+    { "vel_depth",          0.0f,    1.0f, W9_LIN },
+    { "volume",             0.0f,    1.0f, W9_LIN },
 };
 
-#define 9W9_POT_COUNT ((int)(sizeof(g_9w9_pots)/sizeof(g_9w9_pots[0])))
+#define W9_POT_COUNT ((int)(sizeof(g_w9_pots)/sizeof(g_w9_pots[0])))
 
 /* pot (0..127) -> engineering value */
-static inline float 9w9_pot_to_value(const 9w9_pot_t *p, int pot)
+static inline float w9_pot_to_value(const w9_pot_t *p, int pot)
 {
     if(pot < 0)   pot = 0;
     if(pot > 127) pot = 127;
     const float t = (float)pot / 127.0f;
 
-    if(p->curve == 9W9_EXP && p->min > 0.0001f)
+    if(p->curve == W9_EXP && p->min > 0.0001f)
         return p->min * powf(p->max / p->min, t);
     return p->min + (p->max - p->min) * t;
 }
 
 /* engineering value -> nearest pot, used to seed pots from engine defaults */
-static inline int 9w9_value_to_pot(const 9w9_pot_t *p, float v)
+static inline int w9_value_to_pot(const w9_pot_t *p, float v)
 {
     if(v < p->min) v = p->min;
     if(v > p->max) v = p->max;
     float t;
-    if(p->curve == 9W9_EXP && p->min > 0.0001f)
+    if(p->curve == W9_EXP && p->min > 0.0001f)
         t = logf(v / p->min) / logf(p->max / p->min);
     else
         t = (p->max - p->min) > 0.0f ? (v - p->min) / (p->max - p->min) : 0.0f;
@@ -211,4 +211,4 @@
     return pot;
 }
 
-#endif /* 9W9_POTS_H */
+#endif /* W9_POTS_H */
--- 9pin/src/dsp/9w9_tom909.h
+++ 9ren/src/dsp/9w9_tom909.h
@@ -41,16 +41,16 @@
  *
  * GPL-3.0.
  */
-#ifndef 9W9_TOM909_H
-#define 9W9_TOM909_H
+#ifndef W9_TOM909_H
+#define W9_TOM909_H
 
 #include "webaudio.h"
 #include "9w9_circuit.h"
 
 /* VCO2 : VCO1 : VCO3 from the timing capacitors above. */
-#define 9W9_TOM_R1  1.00f
-#define 9W9_TOM_R2  1.50f
-#define 9W9_TOM_R3  2.75f
+#define W9_TOM_R1  1.00f
+#define W9_TOM_R2  1.50f
+#define W9_TOM_R3  2.75f
 
 typedef struct {
     wa_osc_t    osc[3];
@@ -64,9 +64,9 @@
     float       crush_st[2];
     double      mute_countdown;
     float       sample_rate;
-} 9w9_tom_t;
+} w9_tom_t;
 
-static inline void 9w9_tom909_init(9w9_tom_t *t, const float _sr)
+static inline void w9_tom909_init(w9_tom_t *t, const float _sr)
 {
     t->sample_rate = _sr;
     for(int i = 0; i < 3; ++i)
@@ -93,7 +93,7 @@
  * same Tune / Decay / Attack / Drive / Dist / Level the UI already has and no
  * new parameter has to exist for it.
  */
-static inline void 9w9_tom909_trigger(9w9_tom_t *t, const 9w9_bt_t *p,
+static inline void w9_tom909_trigger(w9_tom_t *t, const w9_bt_t *p,
                                        const float _accent)
 {
     const float sr = t->sample_rate;
@@ -131,7 +131,7 @@
     t->mute_countdown = (d1 + 60.0f) * ms;
 }
 
-static inline float 9w9_tom909_render(9w9_tom_t *t, const 9w9_bt_t *p,
+static inline float w9_tom909_render(w9_tom_t *t, const w9_bt_t *p,
                                        const float _noise)
 {
     if(t->mute_countdown <= 0.0) return 0.0f;
@@ -142,20 +142,20 @@
     /* Three oscillators at the board's ratios, each rounded by its own diode
      * pair and gated by its own envelope. */
     float o = 0.0f;
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[0], f * 9W9_TOM_R1), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[0], f * W9_TOM_R1), 2.0f)
        * wa_param_tick(&t->env[0]);
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[1], f * 9W9_TOM_R2), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[1], f * W9_TOM_R2), 2.0f)
        * wa_param_tick(&t->env[1]);
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[2], f * 9W9_TOM_R3), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[2], f * W9_TOM_R3), 2.0f)
        * wa_param_tick(&t->env[2]);
     o *= 0.55f;                       /* three partials summed, keep headroom */
 
     const float stick = wa_biquad_tick(&t->noise_bp, _noise)
                       * wa_param_tick(&t->noise_env) * t->noise_level * 1.5f;
 
-    float y = 9w9_shape_st(o + stick, p->drive, p->dist_type, t->crush_st);
+    float y = w9_shape_st(o + stick, p->drive, p->dist_type, t->crush_st);
     y = wa_biquad_tick(&t->dc_block, y);
     return y * t->out_gain;
 }
 
-#endif /* 9W9_TOM909_H */
+#endif /* W9_TOM909_H */
--- 9pin/src/dsp/webaudio.h
+++ 9ren/src/dsp/webaudio.h
@@ -1,21 +1,19 @@
 /*
- * webaudio.h — the handful of Web Audio API nodes er-99 uses, in C.
+ * webaudio.h — a handful of W3C Web Audio API nodes, in C.
  *
- * er-99's DSP is a pure Web Audio node graph (no AudioWorklet), so every
- * behaviour it relies on is defined mathematically by the W3C Web Audio spec.
- * This header implements exactly those definitions, which is what makes the
- * port verifiable against reference renders instead of tuned by ear.
+ * Each node implements the behaviour the W3C Web Audio spec defines
+ * mathematically, so results are verifiable rather than tuned by ear.
  *
  * Spec references:
  *   BiquadFilterNode  https://www.w3.org/TR/webaudio/#filters-characteristics
  *   AudioParam ramps  https://www.w3.org/TR/webaudio/#dom-audioparam-*
  *   WaveShaperNode    https://www.w3.org/TR/webaudio/#WaveShaperNode
  *
- * GPL-3.0 (er-99 is GPL-3.0).
+ * GPL-3.0.
  */
 
-#ifndef 9W9_WEBAUDIO_H
-#define 9W9_WEBAUDIO_H
+#ifndef W9_WEBAUDIO_H
+#define W9_WEBAUDIO_H
 
 #include <math.h>
 #include <stdint.h>
@@ -25,15 +23,15 @@
 #define M_PI 3.14159265358979323846
 #endif
 
-/* Web Audio clamps exponential ramp endpoints away from zero; er-99 relies on
- * this by ramping to 0.00001 rather than 0. */
+/* Web Audio clamps exponential ramp endpoints away from zero; envelopes rely
+ * on this by ramping to 0.00001 rather than 0. */
 #define WA_MIN_EXP 1.0e-9f
 
 /* ===================================================================== */
 /* AudioParam — the automation timeline                                   */
 /* ===================================================================== */
 /*
- * er-99 uses four automation methods. Rather than model a full event list we
+ * The voices use four automation methods. Rather than model a full event list we
  * keep the single active segment, which is sufficient because every voice
  * re-arms its envelopes on trigger (the one exception, the clap's 5 scheduled
  * taps, is handled by the clap voice itself).
@@ -165,8 +163,8 @@
  * Coefficients exactly as the Web Audio spec defines them (which is the RBJ
  * cookbook). Note the spec's BANDPASS is the constant-0dB-peak-gain variant,
  * and Q is interpreted in dB for lowpass/highpass but linearly for
- * bandpass/notch/peaking — er-99 leaves Q at default (1) for LP/HP/notch and
- * sets it explicitly only on bandpass, so we take Q linearly throughout.
+ * bandpass/notch/peaking — the voices leave Q at default (1) for LP/HP/notch
+ * and set it explicitly only on bandpass, so we take Q linearly throughout.
  */
 static inline void wa_biquad_set(wa_biquad_t *f, const wa_filter_type_t _type,
                                  const float _freq, const float _q,
@@ -264,7 +262,7 @@
 /* WaveShaperNode                                                         */
 /* ===================================================================== */
 /*
- * er-99's curve, verbatim from index.ts:
+ * Saturation curve:
  *   curve[i] = (PI + amount) * x / (PI + amount * |x|),  x = i*2/n - 1
  *
  * The node runs at oversample='2x'. We reproduce that: upsample x2 (linear),
@@ -366,7 +364,7 @@
 }
 
 /* ===================================================================== */
-/* White noise — er-99 uses lib/noise.js: Math.random() * 2 - 1           */
+/* White noise, uniform in [-1, 1)                                        */
 /* ===================================================================== */
 
 typedef struct { uint32_t state; } wa_noise_t;
@@ -385,4 +383,4 @@
     return (float)((double)n->state / 2147483648.0) - 1.0f;
 }
 
-#endif /* 9W9_WEBAUDIO_H */
+#endif /* W9_WEBAUDIO_H */
--- 9ren/src/dsp/9w9_engine.c
+++ 9v271/src/dsp/9w9_engine.c
@@ -949,6 +949,21 @@
     return n < len ? n : len - 1;
 }
 
+/*
+ * The current tag, or any v3 tag written before it: a short first segment
+ * ending in "v3" and holding no '='. v3 is the pot-position format, so the
+ * values that follow mean the same thing whichever name it was saved under.
+ */
+static int w9_state_tag_ok(const char *blob)
+{
+    if(strncmp(blob, W9_STATE_TAG, sizeof(W9_STATE_TAG) - 1) == 0) return 1;
+    const char *semi = strchr(blob, ';');
+    if(!semi) return 0;
+    const size_t n = (size_t)(semi - blob);
+    return n >= 3 && n <= 8 && memcmp(semi - 2, "v3", 2) == 0 &&
+           memchr(blob, '=', n) == NULL;
+}
+
 int w9_engine_set_state(w9_engine_t *e, const char *blob)
 {
     if(!blob) return 0;
@@ -959,8 +974,7 @@
      * i.e. total silence. Anything that isn't v2 is discarded so the engine
      * keeps its (audible) defaults instead of being silently muted.
      */
-    if(strncmp(blob, W9_STATE_TAG, sizeof(W9_STATE_TAG) - 1) != 0 &&
-       strncmp(blob, W9_STATE_TAG_OLD, sizeof(W9_STATE_TAG_OLD) - 1) != 0)
+    if(!w9_state_tag_ok(blob))
         return 0;      /* v1/v2/unknown: keep the (correct) defaults */
 
     const char *p = blob;
--- 9ren/src/dsp/9w9_engine.h
+++ 9v271/src/dsp/9w9_engine.h
@@ -181,10 +181,10 @@
 int w9_engine_get_param(const w9_engine_t *e, const char *key, float *pot);
 
 /* Self-contained state blob — required for slot autosave and User Presets. */
-/* State blob tag. Blobs saved by v2.7.0 and earlier carry the old tag and
- * still load, so existing patches and slot autosaves keep their sound. */
+/* State blob tag. Blobs saved by v2.7.0 and earlier carry an older v3 tag
+ * and still load (see w9_state_tag_ok), so existing patches and slot
+ * autosaves keep their sound. */
 #define W9_STATE_TAG     "9w9v3;"
-#define W9_STATE_TAG_OLD "9w9v3;"
 int w9_engine_get_state(const w9_engine_t *e, char *buf, int len);
 int w9_engine_set_state(w9_engine_t *e, const char *blob);
 
--- 9v271/src/dsp/9w9_circuit.h
+++ 9v272/src/dsp/9w9_circuit.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_circuit.h — circuit-informed TR-909 voice models (BD / SD / toms).
  *
--- 9v271/src/dsp/9w9_engine.c
+++ 9v272/src/dsp/9w9_engine.c
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_engine.c — 9W9 drum voices, sample playback, sends and master bus.
  *
--- 9v271/src/dsp/9w9_engine.h
+++ 9v272/src/dsp/9w9_engine.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_engine.h — 9W9 drum engine.
  *
--- 9v271/src/dsp/9w9_params.h
+++ 9v272/src/dsp/9w9_params.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /* Generated by scripts/gen_params.py — do not edit by hand.
  * module.json is capped at 8 KB by Schwung's loader, so these are served
  * dynamically from the DSP via get_param(). */
--- 9v271/src/dsp/9w9_perc909.h
+++ 9v272/src/dsp/9w9_perc909.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_perc909.h — circuit-informed TR-909 RIM SHOT and HAND CLAP.
  *
--- 9v271/src/dsp/9w9_plugin.c
+++ 9v272/src/dsp/9w9_plugin.c
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_plugin.c — Schwung plugin_api_v2 wrapper for the 9W9 engine.
  *
--- 9v271/src/dsp/9w9_pots.h
+++ 9v272/src/dsp/9w9_pots.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_pots.h — every continuous control is a 0..127 pot, like the hardware.
  *
--- 9v271/src/dsp/9w9_tom909.h
+++ 9v272/src/dsp/9w9_tom909.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_tom909.h — the TR-909's tom voice, from the voicing-board schematic.
  *
--- 9v271/src/dsp/webaudio.h
+++ 9v272/src/dsp/webaudio.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * webaudio.h — a handful of W3C Web Audio API nodes, in C.
  *

```

---
### out-8w8.md

# 8W8 base → v1.5.1: voice and engine hunk review

## Upstream → firmware mapping

| Base upstream file → v1.5.1 file | Our counterpart (firmware/src/dsp/) | Relationship |
|---|---|---|
| `src/dsp/sc808_bd_circuit.h` → `src/dsp/w8_bd_circuit.h` | drum808.c:416; drum808.h:66 | BD loop/trigger/tick; d8_bd_t |
| `src/dsp/sc808_sd_circuit.h` → `src/dsp/w8_sd_circuit.h` | drum808.c:493; drum808.h:74 | SD shell/noise; d8_sd_t |
| `src/dsp/sc808_cp_circuit.h` → `src/dsp/w8_cp_circuit.h` | drum808.c:769; drum808.h:113 | Clap burst train and tail; d8_cp_t |
| `src/dsp/sc808_rs_circuit.h` → `src/dsp/w8_rs_circuit.h` | drum808.c:632; drum808.c:666; drum808.h:92 | ClaveCircuit / MaracasCircuit; d8_clave_t / d8_ma_t |
| `src/dsp/sc808_tom_circuit.h` → `src/dsp/w8_tom_circuit.h` | drum808.c:551; drum808.h:83 | Tom/conga loop and strike; d8_tom_t |
| `src/dsp/sc808_mt_circuit.h` → `src/dsp/w8_mt_circuit.h` | drum808.c:821; drum808.c:920; drum808.c:1039; drum808.c:1104; drum808.h:122 | Shared bank, cymbal, hats, cowbell |
| `src/dsp/sc808_circuit_common.h` → `src/dsp/w8_circuit_common.h` | drum808.c:226; drum808.c:239; drum808.c:242; drum808.c:319 | Pulse shaper, clip, bridged-T, one-pole HP |
| `src/dsp/sc808_shape.h` → `src/dsp/w8_shape.h` | drum808.c:1135; drum808.c:1149; drum808.h:169 | Resolved per-sound drive and seven shapes |
| `src/dsp/sc808_fx.h` → `src/dsp/w8_fx.h` | NONE | Upstream internal wet returns/master glue omitted from drum808; shared firmware FX is outside this comparison |
| `src/dsp/sc808_engine.cpp` → `src/dsp/w8_engine.cpp` | drum808.c:1196; drum808.c:1259; drum808.c:1600; drum808.c:1664; drum808.c:1953 | Pot maps, trims, init, trigger, render; upstream state/mutes/master are absent |
| `src/dsp/sc808_engine.h` → `src/dsp/w8_engine.h` | drum808.h:1; drum808.h:26; drum808.h:176 | Public API / roster / engine state, adapted to firmware |
| `src/dsp/sc808_params.h` → `src/dsp/w8_params.h` | drum808.c:1196; drum808.c:1290; drum808.h:38 | Per-sound pot specs and firmware parameter presentation; upstream JSON has no counterpart |
| `src/dsp/sc808_plugin.cpp` → `src/dsp/w8_plugin.cpp` | NONE | Schwung ABI wrapper/pad mapping/focus/JSON serving omitted from drum808 |
| `src/dsp/sc808_voices.h` → `src/dsp/w8_rim.h` | drum808.c:716; drum808.c:743; drum808.h:102 | Only RimClave mode 0 is ported; claves use ClaveCircuit, other transcription voices absent |
| `src/dsp/sc_ugens.h` → `src/dsp/w8_dsp.h` (dependency audit) | drum808.c:208–394; drum808.h:51–63 | RNG, filters, envelope; rim oscillators at drum808.c:743 |

## Pins, method and limits

Base: `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997`, `/workspace/upstream/michaelmenesescs_schwung-8W8`. New: `1dbef27e13a6c63cb1fc078dd9b4d86172237c51`, `/workspace/upstream/athousanddetails_schwung-8W8`. Both IDs verified by git inside those clones. All 15 extracted src/dsp files on each side were also verified with git ls-tree and git hash-object against the corresponding pin (30 blob matches). Compared extracted full trees `/tmp/cmp/8pin` and `/tmp/cmp/8v151`, not an ancestry diff. Firmware inspected at HEAD `263a055879f48919881f50812183aee3f4f2dc1e`; `git diff a18561f17977be8592cef5a3d3cbebf70e4dbf1b HEAD -- firmware/src/dsp/drum808.c firmware/src/dsp/drum808.h` is empty, so these line references also describe the requested scratch base.

Primary tables use unified diff with three context lines, after exactly the requested case-sensitive replacements: er99→9w9, sc808→w8, sc::→dsp::, RGen→NoiseGen, frand2→white, trand→next32, SinOsc→SineOsc, LFTri→TriOsc, LFPulse→PulseOsc. Uppercase SC808 identifiers are intentionally still visible and reviewed. Upstream references are new-side hunk start plus count; original old-side span is included. File paths below are under `src/dsp/`; firmware paths are under `firmware/src/dsp/`. NONE means no direct code counterpart, not a missing requested feature.

“Applies” asks whether this upstream change supplies an actionable firmware speedup/fix, rather than whether the surrounding model exists. “Same” concerns the reviewed change, not bit identity between the C float firmware port and the upstream C++ double implementation. Where the firmware differs, that distinction is explicit. No external posting, push, computer access, source edit, branch or worktree creation occurred. Existing modification observed in tests/host/run_drum909_emit_equivalence.sh was left untouched; drum808.c/.h have no working-tree differences from HEAD.

## Every normalized hunk in the requested surviving files

| Upstream file:line (new side; old span) | Our counterpart file:line or NONE | Verdict | Applies Y/N | Our code already has the same / differs | Notes |
|---|---|---|---|---|---|
| `w8_bd_circuit.h:1` (+9; `sc808_bd_circuit.h:1`, 9 lines) | drum808.c:416 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright and kick-description rewrite; no model equation change. |
| `w8_bd_circuit.h:37` (+7; `sc808_bd_circuit.h:37`, 7 lines) | drum808.c:416 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | “Transcribed” becomes “copied” in coefficient provenance. |
| `w8_bd_circuit.h:48` (+13; `sc808_bd_circuit.h:48`, 14 lines) | drum808.c:416 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Removes null-test comparison prose and renames include guard; coefficients untouched. |
| `w8_bd_circuit.h:350` (+7; `sc808_bd_circuit.h:351`, 7 lines) | drum808.c:416 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Engine A level-reference wording only; kClickLevel and output scaling unchanged. |
| `w8_bd_circuit.h:381` (+4; `sc808_bd_circuit.h:382`, 4 lines) | drum808.c:416 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing include guard rename. |
| `w8_sd_circuit.h:1` (+4; `sc808_sd_circuit.h:1`, 3 lines) | drum808.c:493 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_sd_circuit.h:14` (+7; `sc808_sd_circuit.h:13`, 7 lines) | drum808.c:493 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Engine A comparison wording only. |
| `w8_sd_circuit.h:44` (+17; `sc808_sd_circuit.h:43`, 17 lines) | drum808.c:493 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Provenance wording, include guard and primitive-header replacement; dependency audited below. |
| `w8_sd_circuit.h:201` (+7; `sc808_sd_circuit.h:200`, 7 lines) | drum808.c:493 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Noise-stage provenance wording; HP corner/decay unchanged. |
| `w8_sd_circuit.h:224` (+7; `sc808_sd_circuit.h:223`, 15 lines) | drum808.c:493 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Removes A/B matching rationale; shell/noise gains remain unchanged. |
| `w8_sd_circuit.h:252` (+4; `sc808_sd_circuit.h:259`, 4 lines) | drum808.c:493 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_cp_circuit.h:1` (+14; `sc808_cp_circuit.h:1`, 17 lines) | drum808.c:769 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Removes Engine A/transcription prose; the described old clap fault is historical, not a fix in this patch. |
| `w8_cp_circuit.h:27` (+7; `sc808_cp_circuit.h:30`, 8 lines) | drum808.c:769 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Removes historical decay-default explanation; 330 ms circuit tail is unchanged. |
| `w8_cp_circuit.h:55` (+12; `sc808_cp_circuit.h:59`, 12 lines) | drum808.c:769 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Guard and primitive-header rename; no circuit arithmetic change. |
| `w8_cp_circuit.h:92` (+7; `sc808_cp_circuit.h:96`, 7 lines) | drum808.c:769 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Engine A loudness wording only; output scale unchanged. |
| `w8_cp_circuit.h:244` (+4; `sc808_cp_circuit.h:248`, 4 lines) | drum808.c:769 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_rs_circuit.h:1` (+21; `sc808_rs_circuit.h:1`, 21 lines) | drum808.c:632 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Historical rim rejection prose rewritten; no rim model or branch removed here. |
| `w8_rs_circuit.h:33` (+12; `sc808_rs_circuit.h:33`, 12 lines) | drum808.c:632 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Guard and primitive-header rename. |
| `w8_rs_circuit.h:155` (+7; `sc808_rs_circuit.h:155`, 7 lines) | drum808.c:632 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Attack-pot bug-fix description already exists in base; only its historical label changes. |
| `w8_rs_circuit.h:236` (+4; `sc808_rs_circuit.h:236`, 4 lines) | drum808.c:632 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_tom_circuit.h:1` (+6; `sc808_tom_circuit.h:1`, 9 lines) | drum808.c:551 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright and removal of A/B introduction; no voice removal in this file. |
| `w8_tom_circuit.h:47` (+12; `sc808_tom_circuit.h:50`, 12 lines) | drum808.c:551 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Guard and primitive-header rename. |
| `w8_tom_circuit.h:114` (+13; `sc808_tom_circuit.h:117`, 15 lines) | drum808.c:551 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Compensation/level prose only: powers 0.224/0.90 and output scales 9.2/6.7 unchanged. |
| `w8_tom_circuit.h:312` (+4; `sc808_tom_circuit.h:317`, 4 lines) | drum808.c:551 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_mt_circuit.h:1` (+4; `sc808_mt_circuit.h:1`, 3 lines) | drum808.c:821 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_mt_circuit.h:61` (+12; `sc808_mt_circuit.h:60`, 12 lines) | drum808.c:821 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Guard and primitive-header rename. |
| `w8_mt_circuit.h:111` (+7; `sc808_mt_circuit.h:110`, 7 lines) | drum808.c:821 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Tune-control provenance wording only; ratio clamp 0.25..4.0 unchanged. |
| `w8_mt_circuit.h:878` (+4; `sc808_mt_circuit.h:877`, 4 lines) | drum808.c:821 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_circuit_common.h:1` (+4; `sc808_circuit_common.h:1`, 3 lines) | drum808.c:226 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_circuit_common.h:15` (+8; `sc808_circuit_common.h:14`, 8 lines) | drum808.c:226 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Include guard rename; shared filter/shaper definitions unchanged. |
| `w8_circuit_common.h:188` (+4; `sc808_circuit_common.h:187`, 4 lines) | drum808.c:226 | cosmetic | N | ALREADY same model intent; differs in float implementation and local naming. | Closing guard rename. |
| `w8_shape.h:1` (+4; `sc808_shape.h:1`, 3 lines) | drum808.c:1149 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_shape.h:40` (+14; `sc808_shape.h:39`, 14 lines) | drum808.c:1149 | cosmetic | N | ALREADY same 0.001 bypass / two-float Crush state; differs in cached coefficients and fast math. | Guard and DRIVE_BYPASS symbol rename; threshold remains 1.0e-3f. |
| `w8_shape.h:55` (+11; `sc808_shape.h:54`, 11 lines) | drum808.c:1149 | cosmetic | N | ALREADY same 0.001 bypass / two-float Crush state; differs in cached coefficients and fast math. | CRUSH_STATE and bypass symbol rename; state size remains 2 and diode expression unchanged. |
| `w8_shape.h:109` (+13; `sc808_shape.h:108`, 13 lines) | drum808.c:1149 | cosmetic | N | ALREADY same 0.001 bypass / two-float Crush state; differs in cached coefficients and fast math. | Bypass name/comment rename; same early return and same k=1+drive calculation. |
| `w8_shape.h:171` (+4; `sc808_shape.h:170`, 4 lines) | drum808.c:1149 | cosmetic | N | ALREADY same 0.001 bypass / two-float Crush state; differs in cached coefficients and fast math. | Closing guard rename. |
| `w8_fx.h:1` (+4; `sc808_fx.h:1`, 3 lines) | NONE | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_fx.h:18` (+28; `sc808_fx.h:17`, 28 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Guard/header/table symbol renames; delay remains 88200, 13 divisions, comb/allpass lengths and beat values identical. |
| `w8_fx.h:57` (+20; `sc808_fx.h:56`, 20 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Buffer-size and send-RQ symbol rename; RQ stays 1.4142135623730951; highpass setter audited below. |
| `w8_fx.h:86` (+10; `sc808_fx.h:85`, 10 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Delay-init names only; same buffer-clear loop bound. |
| `w8_fx.h:102` (+7; `sc808_fx.h:101`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Comb table name only; same indexed load, damping and accumulator order. |
| `w8_fx.h:118` (+7; `sc808_fx.h:117`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Allpass table name only; same indexed load and recursion. |
| `w8_fx.h:135` (+15; `sc808_fx.h:134`, 15 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Delay wrap names only. The post-cast bounds fix is present on BOTH sides; no added/removed clamp. |
| `w8_fx.h:155` (+7; `sc808_fx.h:154`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Delay write-wrap constant rename; same branch. |
| `w8_fx.h:165` (+9; `sc808_fx.h:164`, 9 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Division clamp and beat-table names only; BPM fallback and multiplication/division order unchanged. |
| `w8_fx.h:175` (+7; `sc808_fx.h:174`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | MAX name only; 256-sample safety margin and max-time clamp already exist in base. |
| `w8_fx.h:236` (+4; `sc808_fx.h:235`, 4 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Closing guard rename. |
| `w8_engine.cpp:1` (+15; `sc808_engine.cpp:1`, 15 lines) | drum808.h:2 | structure | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Copyright/provenance and voices-to-rim include extraction; only used rim class survives, audited separately. |
| `w8_engine.cpp:39` (+7; `sc808_engine.cpp:39`, 7 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | State-version symbol rename; value remains 4. |
| `w8_engine.cpp:51` (+7; `sc808_engine.cpp:51`, 7 lines) | drum808.c:1267 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Full-velocity symbol rename; value remains 1.9921260f. |
| `w8_engine.cpp:59` (+39; `sc808_engine.cpp:59`, 61 lines) | drum808.c:1264 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Roster-size symbol and pitch-provenance prose; all base-note literals unchanged, including 42.02/48.58/54.83, 55.88/62.49/67.55, 91.62 and 99.2. |
| `w8_engine.cpp:100` (+7; `sc808_engine.cpp:122`, 8 lines) | drum808.c:1259 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Kit-balance prose only. |
| `w8_engine.cpp:122` (+7; `sc808_engine.cpp:145`, 7 lines) | drum808.c:1259 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Trim array bound rename; all 16 gain literals unchanged. |
| `w8_engine.cpp:134` (+7; `sc808_engine.cpp:157`, 7 lines) | drum808.c:1261 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Clap trim comment only; 3.9640f remains. |
| `w8_engine.cpp:150` (+7; `sc808_engine.cpp:173`, 7 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Compile-time completeness recursion bound rename; no runtime branch removal. |
| `w8_engine.cpp:161` (+12; `sc808_engine.cpp:184`, 12 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Compile-time ID-check bound/message rename. |
| `w8_engine.cpp:185` (+19; `sc808_engine.cpp:208`, 19 lines) | drum808.c:1232 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Crush state size and pot/enum lookup loop bound renames; searches were already outside sample loop. |
| `w8_engine.cpp:209` (+7; `sc808_engine.cpp:232`, 7 lines) | drum808.c:1269 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | EXP enum rename; same min*powf(max/min,t) expression and linear fallback. |
| `w8_engine.cpp:219` (+12; `sc808_engine.cpp:242`, 12 lines) | drum808.h:176 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Array-bound renames only; dimensions unchanged. |
| `w8_engine.cpp:232` (+7; `sc808_engine.cpp:255`, 7 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Master Crush array-bound rename; still two floats. |
| `w8_engine.cpp:268` (+11; `sc808_engine.cpp:291`, 14 lines) | drum808.h:102 | structure | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | RimClave becomes RimLane and explanatory prose changes; no member removed from live engine state besides type spelling. |
| `w8_engine.cpp:297` (+14; `sc808_engine.cpp:323`, 14 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Send RQ symbol rename in cached highpass updates; float comparisons/caching already exist in base. |
| `w8_engine.cpp:320` (+7; `sc808_engine.cpp:346`, 7 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Voice-ID bounds symbol rename. |
| `w8_engine.cpp:346` (+18; `sc808_engine.cpp:372`, 18 lines) | drum808.c:1600 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Initialization/slot-resolution loop-bound renames; no new hoisting or caching. |
| `w8_engine.cpp:427` (+8; `sc808_engine.cpp:453`, 8 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Mute-mask and fade-loop bound rename; no algorithm change. |
| `w8_engine.cpp:463` (+7; `sc808_engine.cpp:489`, 7 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Trigger bounds symbol rename. |
| `w8_engine.cpp:488` (+7; `sc808_engine.cpp:514`, 7 lines) | drum808.c:1680 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Velocity constant symbol rename; same multiplication/subtraction order. |
| `w8_engine.cpp:512` (+12; `sc808_engine.cpp:538`, 12 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Hat-choke and BD case labels renamed; same two conditionals and choke calls. |
| `w8_engine.cpp:540` (+7; `sc808_engine.cpp:566`, 7 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | SD case label rename only. |
| `w8_engine.cpp:564` (+10; `sc808_engine.cpp:590`, 10 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Tom/conga case labels and index origin rename; same local pointer array and selection. |
| `w8_engine.cpp:577` (+15; `sc808_engine.cpp:603`, 15 lines) | drum808.c:719 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Rim case-label/provenance wording only; decay conversion and tune-following filters unchanged. |
| `w8_engine.cpp:593` (+7; `sc808_engine.cpp:619`, 7 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Clave case-label rename only. |
| `w8_engine.cpp:601` (+21; `sc808_engine.cpp:627`, 21 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Maracas, clap and cowbell case-label renames; trigger parameters and 10 ms spacing unchanged. |
| `w8_engine.cpp:623` (+7; `sc808_engine.cpp:649`, 7 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Closed-hat case-label rename only. |
| `w8_engine.cpp:635` (+12; `sc808_engine.cpp:661`, 12 lines) | drum808.c:1664 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Open-hat/cymbal case-label renames; same trigger and shared-bank tuning. |
| `w8_engine.cpp:699` (+7; `sc808_engine.cpp:725`, 7 lines) | drum808.c:1765 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | LANE macro rename; choke/active guards already present in base. |
| `w8_engine.cpp:715` (+18; `sc808_engine.cpp:741`, 18 lines) | drum808.c:1953 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Per-sample LANE calls renamed; same voice order, calls and guards; no loop fusion or removed branch. |
| `w8_engine.cpp:734` (+16; `sc808_engine.cpp:760`, 16 lines) | drum808.c:1953 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Metal macro/voice IDs renamed; shared bank still ticks once per sample unconditionally upstream. |
| `w8_engine.cpp:785` (+9; `sc808_engine.cpp:811`, 9 lines) | drum808.c:1413 | cosmetic | N | ALREADY corresponding firmware behavior; differs in API, layout and names. | Default-reset explanation only; reset logic unchanged. |
| `w8_engine.cpp:824` (+11; `sc808_engine.cpp:850`, 11 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | State serialization version/loop-bound names only; same schema and loops. |
| `w8_engine.cpp:926` (+15; `sc808_engine.cpp:952`, 15 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Deserialize scratch-size and scan-bound names only; same allocation extent. |
| `w8_engine.cpp:942` (+17; `sc808_engine.cpp:968`, 17 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Deserialize pot/enum bounds names only; migration/clamping logic unchanged. |
| `w8_engine.cpp:970` (+7; `sc808_engine.cpp:996`, 7 lines) | NONE | cosmetic | N | Differs: this upstream facility has no direct drum808 counterpart. | Stored mute-mask bound rename only. |
| `w8_engine.h:1` (+10; `sc808_engine.h:1`, 9 lines) | drum808.h:38 | cosmetic | N | ALREADY sixteen sound IDs; differs in eleven-track API and combined sound switches. | Provenance/orchestration description and copyright. |
| `w8_engine.h:12` (+8; `sc808_engine.h:11`, 8 lines) | drum808.h:38 | cosmetic | N | ALREADY sixteen sound IDs; differs in eleven-track API and combined sound switches. | Include guard rename. |
| `w8_engine.h:31` (+9; `sc808_engine.h:30`, 9 lines) | drum808.h:38 | cosmetic | N | ALREADY sixteen sound IDs; differs in eleven-track API and combined sound switches. | Roster/pad explanation only. |
| `w8_engine.h:41` (+11; `sc808_engine.h:40`, 11 lines) | drum808.h:38 | cosmetic | N | ALREADY sixteen sound IDs; differs in eleven-track API and combined sound switches. | Enum names renamed; numeric order remains sixteen sounds. |
| `w8_engine.h:84` (+4; `sc808_engine.h:83`, 4 lines) | drum808.h:38 | cosmetic | N | ALREADY sixteen sound IDs; differs in eleven-track API and combined sound switches. | Closing guard rename. |
| `w8_params.h:1` (+4; `sc808_params.h:1`, 3 lines) | drum808.c:1196 | cosmetic | N | Differs in header/provenance text; no executable delta to port. | Copyright header. |
| `w8_params.h:7` (+10; `sc808_params.h:6`, 10 lines) | drum808.c:1196 | cosmetic | N | ALREADY corresponding pot defaults; differs in compact table/track organization. | Guard and LIN/EXP enum names only; values remain 0/1. |
| `w8_params.h:26` (+123; `sc808_params.h:25`, 123 lines) | drum808.c:1196 | cosmetic | N | ALREADY corresponding pot defaults; differs in compact table/track organization. | NUM_POTS/NUM_ENUMS and curve symbol renames throughout table; 110 pots, 20 enums, every min/max/default and enum cardinality unchanged. |
| `w8_params.h:165` (+7; `sc808_params.h:164`, 7 lines) | NONE | cosmetic | N | Differs: firmware does not serve upstream JSON. | Chain JSON length symbol rename; bytes/length remain 11949. |
| `w8_params.h:288` (+7; `sc808_params.h:287`, 7 lines) | NONE | cosmetic | N | Differs: firmware does not serve upstream JSON. | UI JSON length symbol rename; bytes/length remain 7924. |
| `w8_params.h:374` (+7; `sc808_params.h:373`, 7 lines) | NONE | cosmetic | N | Differs: firmware does not serve upstream JSON. | GM UI JSON length symbol rename; bytes/length remain 7924. |
| `w8_params.h:457` (+4; `sc808_params.h:456`, 4 lines) | NONE | cosmetic | N | Differs: firmware does not serve upstream JSON. | Closing guard rename. |
| `w8_plugin.cpp:1` (+4; `sc808_plugin.cpp:1`, 3 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Copyright header. |
| `w8_plugin.cpp:6` (+8; `sc808_plugin.cpp:5`, 9 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Wrapper-provenance prose only. |
| `w8_plugin.cpp:30` (+7; `sc808_plugin.cpp:30`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Voice-array bound rename; same sixteen level IDs. |
| `w8_plugin.cpp:79` (+8; `sc808_plugin.cpp:79`, 8 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Pad-roster prose only. |
| `w8_plugin.cpp:94` (+17; `sc808_plugin.cpp:94`, 17 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Grid note-map enum names only; same MIDI numbers and return values. |
| `w8_plugin.cpp:134` (+22; `sc808_plugin.cpp:134`, 22 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | GM note-map enum names only; same aliases and return values. |
| `w8_plugin.cpp:287` (+9; `sc808_plugin.cpp:287`, 9 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Chain JSON length symbol rename; same refusal-to-truncate branch and memcpy length. |
| `w8_plugin.cpp:323` (+7; `sc808_plugin.cpp:323`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | UI JSON length symbols only; same map selection and copy. |
| `w8_plugin.cpp:338` (+7; `sc808_plugin.cpp:338`, 7 lines) | NONE | cosmetic | N | Differs: upstream-only facility absent from drum808. | Focus voice-bound symbol rename; same validation and format. |

Coverage: 104 primary normalized hunks, one row each. No scoped surviving file was unchanged under the exact requested normalization, largely because of the added copyright line and uppercase macro renames.

## Removed voices header and rim extraction

The literal deletion of `sc808_voices.h` is one whole-file deletion hunk; the addition of `w8_rim.h` is one whole-file addition hunk. These are accounted for below, then matched class-to-class so the retained arithmetic is not hidden by file deletion. No new-side line exists for a deleted file.

| Upstream file:line (new side) | Our counterpart file:line or NONE | Verdict | Applies Y/N | Our code already has the same / differs | Notes |
|---|---|---|---|---|---|
| `sc808_voices.h:NONE` (old 1–885; new count 0) | drum808.c:716; drum808.h:102 | structure | N | ALREADY only transcription rim mode 0 is retained. | Removes MetalBank, BassDrum, Snare, Tom, Maracas, Clap, Cowbell, ClosedHat, OpenHat and Cymbal transcription classes; live engine already uses the separate circuit classes in base. This is dead-source removal, not a proven render speedup. |
| `w8_rim.h:1` (+126; old count 0) | drum808.c:716; drum808.h:102 | structure | N | ALREADY retained rim algorithm; differs in C float/fixed-point realization. | Extracts VoiceBase and RimClave as RimLane. Retains mode-1 sine-claves code even though live engine uses ClaveCircuit. No live A/B selection removal. |
| `w8_rim.h:45` (+10; old `sc808_voices.h:379`) | drum808.c:719; drum808.h:102 | cosmetic | N | ALREADY equivalent rim graph; differs in precision, filter form and oscillator phase representation. | Class name and peak_→tock_; same init seed 0x8081DDA and PeakEQ(464,0.44,8). |
| `w8_rim.h:61` (+11; old `sc808_voices.h:395`) | drum808.c:719; drum808.h:102 | cosmetic | N | ALREADY equivalent rim graph; differs in precision, filter form and oscillator phase representation. | triHz_/pulHz_/tri_/pul_→toneHz_/pulseHz_/tone_/pulse_; exact 1.1, powf(2,-22/12), reset values and order retained. |
| `w8_rim.h:74` (+12; old `sc808_voices.h:408`) | drum808.c:719; drum808.h:102 | cosmetic | N | ALREADY equivalent rim graph; differs in precision, filter form and oscillator phase representation. | clvHz_/sin_→claveHz_/clave_; comments rephrased, +7-semitone expression and pi/2 reset retained. |
| `w8_rim.h:92` (+15; old `sc808_voices.h:426`) | drum808.c:743; drum808.h:102 | cosmetic | N | ALREADY equivalent rim graph; differs in precision, filter form and oscillator phase representation. | tri1/tri2→tone/body, peak_→tock_, sin_→clave_; same left-associated sum, env multiplications, noise 0.46f and HP→LP order. |
| `w8_rim.h:110` (+12; old `sc808_voices.h:444`) | drum808.c:743; drum808.h:102 | cosmetic | N | ALREADY equivalent rim graph; differs in precision, filter form and oscillator phase representation. | Private member names change in place; types, ordering and initial values unchanged. |

`VoiceBase` is exactly unchanged after requested normalization. After additionally mapping the rim member/local identifiers listed above and removing comments, the two rim class bodies have the same tokens. This is source evidence that the extraction itself does not change arithmetic; it is not a firmware/upstream bit-identity claim.

## Numerical and hidden-speedup audit

- All six circuit bodies and shared circuit helper bodies have identical executable tokens after identifier/header normalization and comment removal. No changed constants, evaluation-order changes, removed clamps, coefficient update intervals, loop bounds, lookup tables, hoisted loads or removed active branches were found in those deltas. The shape and FX executable tokens likewise match after macro/table-name normalization.
- Engine token differences after that normalization are exactly `#include "w8_voices.h"` → `#include "w8_rim.h"` and `RimClave` → `RimLane`. The base engine already instantiates only circuit voices plus rim, and already gates process calls. Removing unused header class definitions does not establish a CPU improvement. Compiler output size/timing effects are UNKNOWN and were not measured.
- The engine pitch and trim tables, velocity scale 1.9921260f, pot endpoints/defaults, enum counts, 2 ms choke, per-sample shared-bank tick and render accumulation order do not change. Historical “BUG FIX” and “pot used to do nothing” comments describe code already present in base; they are not newly applicable fixes.
- Firmware already differs from upstream: fixed 44.1 kHz, C floats and fast-math, transposed filters storing denominator offsets, cached/approximated coefficients, fixed-point pulse/metal phases, block voice rendering and local silence-tail policy. Upstream double DF-II/oscillator code cannot safely replace this as a bit-identical optimization. See drum808.c:242–317 (filters), :344–394 (envelope), :719–765 (rim), :852 (metal bank), :1871–2001 (render); drum808.h:51–63 (state forms). These are existing differences, not upstream v1.5.1 improvements.

### Replacement primitive header: dependency verification

Although sc_ugens.h/w8_dsp.h is outside the named file list, all changed includes were followed. Shared surviving algorithms were compared directly. The following table records the important numerical details; a full normalized dependency diff follows in the appendix.

| New source | Our counterpart | Verdict | Applies Y/N | Already same / differs | Numerical result |
|---|---|---|---|---|---|
| `w8_dsp.h:40` NoiseGen | drum808.c:208; :216 | cosmetic | N | ALREADY same seed/xorshift/bit-stuff formula. | seed constants 1243598713,3093459404,1821928721 and minima 2/8/16 unchanged; masks FE/F8/F0 and shifts 12/13/19,4/2/25,17/3/11 unchanged. white uses 0x40000000 OR (next32 >> 9), then float minus 3.0f. No scaling/division replacement. |
| `w8_dsp.h:96` Env | drum808.c:346; :369 | structure | N | ALREADY no SC construction priming or nextHeld test hook in this port; differs in float arithmetic. | Removes NULLTEST priming branches, nextHeld/peek and envPerc. Production build already compiled priming branches out. Surviving next/armSegment/stepControl expressions unchanged, control block 64, truncated integer segment count, minimum 1, one-step linear override unchanged. |
| `w8_dsp.h:209` LPF / `:233` HPF | drum808.c:273; :284; :266 | cosmetic | N | Differs: float fast-tan setters and transposed filter form. | Surviving upstream set/reset/process method tokens unchanged. Same tan prewarp, sqrt(2), double coefficient/state operations and final float cast. No coefficient hoist or reordered recursion. |
| `w8_dsp.h:262` Biquad | drum808.c:307; :266; NONE for upstream FX setter | structure | N | Differs in coefficient form and float implementation. | Keeps HiPass, PeakEQ, reset/process unchanged. Deletes unused BandPass, LowShelf, HiShelf setters; no substitution of retained filter computation. PeakEQ still pow(10,db*0.025), alpha=sin(w0)*0.5*rq and existing parenthesization. |
| `w8_dsp.h:315` SineOsc / `:340` TriOsc / `:364` PulseOsc | drum808.c:743; NONE for sine-clave mode | cosmetic | N | Differs: firmware phase increments/representations are precomputed float/fixed point. | Retained upstream oscillator method tokens unchanged; pulse wrap polarity, unipolar output, triangle wrap and emit-before-increment retained. No sine LUT added. |
| Removed BPF, HiPass4, Limiter (no new-side line) | NONE | structure | N | ALREADY absent as such from drum808. | Removes unused transcription support, including lookahead buffers; neither base live engine nor firmware was calling these removed classes. No measured CPU/RAM saving inferred. |

## Verification and decision

`cmp /tmp/cmp/g_8pin.txt /tmp/cmp/g_8v151.txt` returned 0. Both existing files have 34 lines (2 headers + 32 lane/velocity renders), SHA-256 `01270f55983c88a2557e8e0b5468a174a3e74161a3a34d90ce580a27c062c87d`. This verifies the prior saved golden-check outputs are identical, including hashes and descriptors. The generators were not rerun in this read-only audit. Hash equality is evidence for those renders, not a direct PCM/state memcmp and not exhaustive control coverage.

No speedup or bugfix in this area qualifies for a possibly bit-identical firmware patch. Therefore the user’s “if and only if” worktree condition is false: no scratch worktree or commit was made, and firmware host tests/PCM-state comparisons and benchmarking were not run. Peak, mean, median, min and timing noise are NOT_MEASURED; a within-noise claim is UNKNOWN. Read `/workspace/dnbox/optimizer/909-bench/run_interleaved.sh`: it runs rotating base/tip/tipm1 worstcase binaries pinned to CPU 2 using pre-existing `/workspace/opt/wt-909b-*` checkouts, writes per-round row.json, and defaults to 9 rounds. It is a 909 harness; it supplies no measured 808 speedup here.

## Appendix: exact primary normalized unified diff

```diff
--- sc808_bd_circuit.h
+++ w8_bd_circuit.h
@@ -1,9 +1,9 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_bd_circuit.h — the TR-808 bass drum, as a circuit.
  *
- * This is the voice 8W8 exists for. Everything else in the kit can be a
- * faithful transcription of w8 and be fine; the kick cannot, because
- * w8's kick is a sine on a pitch envelope and an 808's kick is not.
+ * This is the voice 8W8 exists for. A sine on a pitch envelope is a perfectly
+ * good kick drum, and it is not an 808's kick.
  *
  * WHAT AN 808 KICK ACTUALLY IS
  *
@@ -37,7 +37,7 @@
  * does, the pulse shaper's shelf-plus-diode form, the decay pot as loop gain,
  * the attack-time frequency shift and the leakage pitch sigh are all theirs.
  *
- * The coefficients here are RE-DERIVED rather than transcribed, deliberately.
+ * The coefficients here are RE-DERIVED rather than copied, deliberately.
  * The paper's published coefficient list for the bridged-T in feedback is
  * dimensionally inconsistent as printed (its s^2 and s^0 terms differ by an
  * ohm), and the component values legible in the schematic figure give a
@@ -48,14 +48,13 @@
  * is marked and its source given.
  *
  * That distinction matters: this is a physically-informed model built to a
- * published analysis, not a transcription, and unlike Engine A it has no null
- * test behind it. It is checked against the paper's own figures — the
+ * published analysis, and it is checked against the paper's own figures — the
  * instantaneous-frequency trace and the retrigger test — by tools/bd_check.
  *
  * GPL-3.0.
  */
-#ifndef SC808_BD_CIRCUIT_H
-#define SC808_BD_CIRCUIT_H
+#ifndef W8_BD_CIRCUIT_H
+#define W8_BD_CIRCUIT_H
 
 #include <math.h>
 #include <stdint.h>
@@ -351,7 +350,7 @@
     static constexpr double kClickLevel  = 0.16;
     /*
      * Set so an UNACCENTED hit at the default pots peaks at 1.0 — the same
-     * place the w8 kick lands. The two engines share one per-lane trim, so
+     * place the Engine A kick lands. The two engines share one per-lane trim, so
      * if they do not agree on what "a kick" comes out at, switching engines
      * changes the level by 11 dB and the kit balance is wrong for one of them.
      */
@@ -382,4 +381,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_BD_CIRCUIT_H */
+#endif /* W8_BD_CIRCUIT_H */
--- sc808_sd_circuit.h
+++ w8_sd_circuit.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_sd_circuit.h — the TR-808 snare drum, as a circuit.
  *
@@ -13,7 +14,7 @@
  * networks. The noise runs through a swing-type VCA and a high-pass and is
  * summed in.
  *
- * Three things follow that w8's two-sines-and-a-crossfade does not do:
+ * Three things follow that Engine A's two-sines-and-a-crossfade does not do:
  *
  *   - TONE moves the balance between a 173 Hz shell and a 336 Hz one, so it
  *     changes which drum you are hearing rather than how bright it is
@@ -43,17 +44,17 @@
  *
  * WHAT IS NOT DERIVED, and is marked where it happens: the noise high-pass
  * corner, and the envelope times. Those stages' component values are not in
- * the sources used here, so they take w8's fitted values instead. That is
+ * the sources used here, so they take Engine A's fitted values instead. That is
  * a weaker footing than the kick, where every block came from the paper.
  *
  * GPL-3.0.
  */
-#ifndef SC808_SD_CIRCUIT_H
-#define SC808_SD_CIRCUIT_H
+#ifndef W8_SD_CIRCUIT_H
+#define W8_SD_CIRCUIT_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
+#include "w8_dsp.h"
 #include "w8_circuit_common.h"
 
 namespace w8 {
@@ -200,7 +201,7 @@
 
 private:
     /* NOT derived from the circuit — the service notes' values for the noise
-     * stage are not in the sources this file is built from. w8's fitted
+     * stage are not in the sources this file is built from. Engine A's fitted
      * high-pass corner (its `hpf` default, MIDI note 93) stands in, and the
      * noise decay is fitted by ear against an 808 snare's tail. Marked
      * because the rest of this file is not like this. */
@@ -223,15 +224,7 @@
 
     /*
      * Level staging, set so an unaccented hit at the engine's default pot
-     * positions peaks at 2.09 — which is exactly where the w8 snare lands
-     * at ITS defaults.
-     *
-     * The two engines share one per-lane trim, so they have to agree about
-     * what a snare comes out at or switching engines moves the whole kit
-     * balance by 12 dB. Matching is done HERE rather than by scaling the
-     * w8 voice, because that voice is what the null test renders and
-     * putting a gain in it would break the one measurement this project's
-     * fidelity claim rests on.
+     * positions peaks at 2.09, the level the lane trim was set for.
      *
      * The ratio between the two numbers is the shell/noise balance and is
      * fitted by ear: shells alone reach about three quarters of the total,
@@ -259,4 +252,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_SD_CIRCUIT_H */
+#endif /* W8_SD_CIRCUIT_H */
--- sc808_cp_circuit.h
+++ w8_cp_circuit.h
@@ -1,17 +1,14 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_cp_circuit.h — the TR-808 hand clap, from the circuit.
  *
- * Engine B for the clap lane, the same arrangement the bass drum and the
- * snare already use: w8's transcription stays exactly as it is and keeps
- * its place in the null test, this runs alongside it, and the panel switches.
- *
  * WHY THIS EXISTS
  *
- * w8's clap is one immediate noise burst, a second one 26 ms later, and a
- * long diffuse tail. Two things follow from that shape and both of them were
+ * The first clap was one immediate noise burst, a second one 26 ms later, and
+ * a long diffuse tail. Two things follow from that shape and both of them were
  * reported as faults, correctly:
  *
- *   - "Decay doesn't work". In the SynthDef `decay` sets the attack burst's
+ *   - "Decay doesn't work". `decay` set the attack burst's
  *     envelope, whose curve is -160 — so steep that the audible part lasts
  *     decay/160, under two milliseconds across the whole pot. The part you
  *     actually hear is a second envelope hard-coded to six seconds, which no
@@ -30,8 +27,7 @@
  *
  *   - C143 (1 uF) charges on the trigger and bleeds off through R362 (330k).
  *     That is the tail, and its time constant is a number you can read
- *     straight off the schematic: 330k x 1 uF = 330 ms. w8's `decay`
- *     default is 0.3, so somebody knew; it just never reached the tail.
+ *     straight off the schematic: 330k x 1 uF = 330 ms.
  *
  *   - The noise runs through IC22 (BA662, the same swing-type VCA the snare
  *     uses) and IC21's bandpass: R342 15k in, C128 = C129 = 0.0047 uF, R334
@@ -59,12 +55,12 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_CP_CIRCUIT_H
-#define SC808_CP_CIRCUIT_H
+#ifndef W8_CP_CIRCUIT_H
+#define W8_CP_CIRCUIT_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
+#include "w8_dsp.h"
 #include "w8_circuit_common.h"
 
 namespace w8 {
@@ -96,7 +92,7 @@
 static const double kCP_PulseTau  = 0.0026;  /* per-pulse collapse, by ear  */
 
 /*
- * Output level, fitted so that the circuit clap and the w8 clap sit at the
+ * Output level, fitted so that the circuit clap and the Engine A clap sit at the
  * same loudness when the panel switches between them. Neither engine should
  * change the mix.
  */
@@ -248,4 +244,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_CP_CIRCUIT_H */
+#endif /* W8_CP_CIRCUIT_H */
--- sc808_rs_circuit.h
+++ w8_rs_circuit.h
@@ -1,21 +1,21 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_rs_circuit.h — the TR-808 claves, from the circuit, and the maracas.
  *
  * WHAT HAPPENED TO THE RIM SHOT. This file used to model both sides of the
  * hardware's RS/CL channel. The rim was built twice from the schematic —
  * first as the two bridged-T networks alone, then with the trigger edge's
- * wideband crack over them — and both lost to w8's algorithm on
- * hardware, the second badly ("not good at all, I prefer w8"). The rim
- * lane now runs the w8 voice with no switch, tuned to rim808.wav, and
- * the rim modelling is GONE from here rather than left as unreachable code
- * pretending to be a rim shot.
+ * wideband crack over them — and both lost on hardware to the oscillator rim
+ * in w8_rim.h, the second badly. The rim lane runs that voice, tuned to a
+ * recorded 808 rim, and the rim modelling is GONE from here rather than left
+ * as unreachable code pretending to be a rim shot.
  *
  * The measurements that pass are kept in the git history for whoever tries
  * again: the reference's first four milliseconds are the 1788 Hz network
  * under a crack reaching past 13 kHz, and by six milliseconds the tock is
  * 19 dB down while the 461 Hz body carries the note alone. What none of the
  * component values explain is the CHARACTER of that crack, and guessing at
- * it twice is what produced two voices worse than the transcription.
+ * it twice is what produced two voices worse than the oscillator rim.
  *
  * WHAT IS DERIVED — service schematic, RIM SHOT / CLAVES block:
  *
@@ -33,12 +33,12 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_RS_CIRCUIT_H
-#define SC808_RS_CIRCUIT_H
+#ifndef W8_RS_CIRCUIT_H
+#define W8_RS_CIRCUIT_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
+#include "w8_dsp.h"
 #include "w8_circuit_common.h"
 
 namespace w8 {
@@ -155,7 +155,7 @@
     /*
      * attack01  the Attack pot's POSITION, 0..1 — not a time in seconds.
      *
-     * THE POT USED TO DO NOTHING. It fed the w8 maracas, and when the
+     * THE POT USED TO DO NOTHING. It fed the Engine A maracas, and when the
      * Engine switches came off that branch went with it while this trigger
      * never took an attack at all: the knob stayed on the panel, resolved a
      * slot, and changed not one sample. Measured, not guessed — every
@@ -236,4 +236,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_RS_CIRCUIT_H */
+#endif /* W8_RS_CIRCUIT_H */
--- sc808_tom_circuit.h
+++ w8_tom_circuit.h
@@ -1,9 +1,6 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_tom_circuit.h — the TR-808 tom / conga channel, from the circuit.
- *
- * Engine B for all six tom and conga lanes. w8's transcription stays
- * exactly as it is and keeps its place in the null test; this runs alongside
- * it and the panel switches.
  *
  * SECOND VERSION, and the first one is worth an honest paragraph. It guessed
  * where it had no data: a 33% pitch lift, a loud white-noise "skin" on the
@@ -50,12 +47,12 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_TOM_CIRCUIT_H
-#define SC808_TOM_CIRCUIT_H
+#ifndef W8_TOM_CIRCUIT_H
+#define W8_TOM_CIRCUIT_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
+#include "w8_dsp.h"
 #include "w8_circuit_common.h"
 
 namespace w8 {
@@ -117,15 +114,13 @@
  */
 static const double kTOM_FwdRefHz = 120.0;
 /* per mode: the tom's fixed-corner strike RC adds its own frequency slope,
- * so the tom's compensation is flatter and larger; solved against the
- * engines-agree measurement in tools/tom_check */
+ * so the tom's compensation is flatter and larger */
 static const double kTOM_FwdPowerTom = 0.224;
 static const double kTOM_FwdPowerCga = 0.90;
 
 /*
- * Fitted so an unaccented hit at the default pots peaks where the w8
- * voice does — each lane has ONE trim shared by both engines, and
- * tools/tom_check asserts they still agree.
+ * Fitted so an unaccented hit at the default pots peaks at the level the
+ * lane trim in w8_engine.cpp was set for.
  */
 static const double kTOM_OutScaleTom = 9.2;
 static const double kTOM_OutScaleCga = 6.7;
@@ -317,4 +312,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_TOM_CIRCUIT_H */
+#endif /* W8_TOM_CIRCUIT_H */
--- sc808_mt_circuit.h
+++ w8_mt_circuit.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_mt_circuit.h — the TR-808 metal voices, from the circuit.
  *
@@ -60,12 +61,12 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_MT_CIRCUIT_H
-#define SC808_MT_CIRCUIT_H
+#ifndef W8_MT_CIRCUIT_H
+#define W8_MT_CIRCUIT_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
+#include "w8_dsp.h"
 #include "w8_circuit_common.h"
 
 namespace w8 {
@@ -110,7 +111,7 @@
     }
 
     /* Tune, as a ratio on the whole bank — the "circuit bend" every 808
-     * modder does first, and the same control the w8 metal had. */
+     * modder does first, and the same control the Engine A metal had. */
     void setRatio(const double _r)
     {
         const double r = _r < 0.25 ? 0.25 : (_r > 4.0 ? 4.0 : _r);
@@ -877,4 +878,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_MT_CIRCUIT_H */
+#endif /* W8_MT_CIRCUIT_H */
--- sc808_circuit_common.h
+++ w8_circuit_common.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_circuit_common.h — the blocks the 808's voice circuits share.
  *
@@ -14,8 +15,8 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_CIRCUIT_COMMON_H
-#define SC808_CIRCUIT_COMMON_H
+#ifndef W8_CIRCUIT_COMMON_H
+#define W8_CIRCUIT_COMMON_H
 
 #include <math.h>
 
@@ -187,4 +188,4 @@
 
 } /* namespace w8 */
 
-#endif /* SC808_CIRCUIT_COMMON_H */
+#endif /* W8_CIRCUIT_COMMON_H */
--- sc808_shape.h
+++ w8_shape.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_shape.h — the post-voice drive stage.
  *
@@ -39,14 +40,14 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_SHAPE_H
-#define SC808_SHAPE_H
+#ifndef W8_SHAPE_H
+#define W8_SHAPE_H
 
 #include <math.h>
 
 /* Below this the stage is a wire. Explicit, so "0 means off" is a promise
  * about the code and not about float behaviour. */
-#define SC808_DRIVE_BYPASS 1.0e-3f
+#define W8_DRIVE_BYPASS 1.0e-3f
 
 /*
  * Crush decimates as well as quantises, so it carries state: the held sample
@@ -54,11 +55,11 @@
  * master stage — sharing one would make the hats' sample-and-hold audible on
  * the kick.
  */
-#define SC808_CRUSH_STATE 2
+#define W8_CRUSH_STATE 2
 
 static inline float w8_diode_round(const float _x, const float _k)
 {
-    if(_k < SC808_DRIVE_BYPASS) return _x;
+    if(_k < W8_DRIVE_BYPASS) return _x;
     /* Normalised at x = 0.5: tanh(k*0.5) * 0.5/tanh(0.5k) == 0.5. */
     return tanhf(_k * _x) * (0.5f / tanhf(0.5f * _k));
 }
@@ -108,13 +109,13 @@
  * Fold moved 2 -> 5 and Crush 3 -> 6. w8_deserialize remaps blobs written
  * before that. Never reorder again without extending the migration.
  *
- * `_st` is SC808_CRUSH_STATE floats, or null for a stateless call — only
+ * `_st` is W8_CRUSH_STATE floats, or null for a stateless call — only
  * Crush reads it, and without it Crush quantises but does not decimate.
  */
 static inline float w8_shape_st(const float _x, const float _drive,
                                    const int _type, float *_st)
 {
-    if(_drive < SC808_DRIVE_BYPASS) return _x;
+    if(_drive < W8_DRIVE_BYPASS) return _x;
     /* k = 1 + drive: k = 1 is the transparent point every curve below is
      * written around, and the knob climbs from there. */
     const float k = 1.0f + _drive;
@@ -170,4 +171,4 @@
     return w8_shape_st(_x, _drive, _type, 0);
 }
 
-#endif /* SC808_SHAPE_H */
+#endif /* W8_SHAPE_H */
--- sc808_fx.h
+++ w8_fx.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_fx.h — the two send buses and the bus glue.
  *
@@ -17,28 +18,28 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_FX_H
-#define SC808_FX_H
+#ifndef W8_FX_H
+#define W8_FX_H
 
 #include <math.h>
 
-#include "sc_ugens.h"
-
-#define SC808_DLY_MAX  88200        /* 2 s at 44.1k: a dotted half at 90 BPM */
-#define SC808_DLY_DIVS 13
+#include "w8_dsp.h"
+
+#define W8_DLY_MAX  88200        /* 2 s at 44.1k: a dotted half at 90 BPM */
+#define W8_DLY_DIVS 13
 
 /* Note divisions, shortest to longest, in beats. Must match the dly_time enum
  * option list in gen_params.py: 1/32 1/16T 1/16 1/8T 1/16. 1/8 1/4T 1/8. 1/4
  * 1/2T 1/4. 1/2 1/2. */
-static const float kSC808DlyBeats[SC808_DLY_DIVS] = {
+static const float kW8DlyBeats[W8_DLY_DIVS] = {
     0.125f, 1.0f/6.0f, 0.25f, 1.0f/3.0f, 0.375f, 0.5f, 2.0f/3.0f,
     0.75f, 1.0f, 4.0f/3.0f, 1.5f, 2.0f, 3.0f
 };
 
 /* Freeverb's canonical comb and allpass lengths at 44.1 k. Not sample-rate
  * scaled — the module runs at one rate. */
-static const int kSC808VerbCL[4] = { 1116, 1188, 1277, 1356 };
-static const int kSC808VerbAL[2] = { 556, 441 };
+static const int kW8VerbCL[4] = { 1116, 1188, 1277, 1356 };
+static const int kW8VerbAL[2] = { 556, 441 };
 
 struct w8_verb_t {
     float decay, tone, hpf_hz, level;
@@ -56,20 +57,20 @@
     int   divi;                     /* note-division index, 0..12 */
     float bpm;                      /* fed by the host each block */
     dsp::Biquad hp;
-    float buf[SC808_DLY_MAX];
+    float buf[W8_DLY_MAX];
     int   w;
     float dcur;                     /* slewed delay, in samples */
     float lp;                       /* loop damping state       */
 };
 
-/* The send highpass, Q = 0.7071 — sc_ugens' Biquad takes the RECIPROCAL of Q,
+/* The send highpass, Q = 0.7071 — the Biquad takes the RECIPROCAL of Q,
  * so the argument is sqrt(2) and not its inverse. Getting that backwards
  * gives a resonant peak where a gentle corner belongs. */
-static const double kSC808_SendRQ = 1.4142135623730951;
+static const double kW8_SendRQ = 1.4142135623730951;
 
 static inline void w8_verb_init(w8_verb_t *r, float sr)
 {
-    r->hp.setHiPass(r->hpf_hz, kSC808_SendRQ, sr);
+    r->hp.setHiPass(r->hpf_hz, kW8_SendRQ, sr);
     r->hp.reset();
     for(int i = 0; i < 4; ++i)
     {
@@ -85,10 +86,10 @@
 
 static inline void w8_dly_init(w8_dly_t *d, float sr)
 {
-    d->hp.setHiPass(d->hpf_hz, kSC808_SendRQ, sr);
+    d->hp.setHiPass(d->hpf_hz, kW8_SendRQ, sr);
     d->hp.reset();
     d->w = 0; d->lp = 0.0f;
-    for(int k = 0; k < SC808_DLY_MAX; ++k) d->buf[k] = 0.0f;
+    for(int k = 0; k < W8_DLY_MAX; ++k) d->buf[k] = 0.0f;
 }
 
 static inline float w8_verb_tick(w8_verb_t *r, const float _in)
@@ -101,7 +102,7 @@
     for(int i = 0; i < 4; ++i)
     {
         float *b = r->comb[i];
-        const int n = kSC808VerbCL[i];
+        const int n = kW8VerbCL[i];
         const float y = b[r->cpos[i]];
         acc += y;
         r->cdmp[i] = y + (r->cdmp[i] - y) * damp;
@@ -117,7 +118,7 @@
     for(int j = 0; j < 2; ++j)
     {
         float *b = r->apb[j];
-        const int n = kSC808VerbAL[j];
+        const int n = kW8VerbAL[j];
         const float bo = b[r->apos[j]];
         b[r->apos[j]] = y + bo * 0.5f;
         y = bo - y * 0.5f;
@@ -134,15 +135,15 @@
      * clicking. ~30 ms to settle. */
     d->dcur += (target - d->dcur) * 0.0008f;
     float rp = (float)d->w - d->dcur;
-    while(rp < 0.0f) rp += (float)SC808_DLY_MAX;
+    while(rp < 0.0f) rp += (float)W8_DLY_MAX;
     int i0 = (int)rp;
     const float fr = rp - (float)i0;
     /* BUG FIX (2 of 3): float spacing at 36000 is 1/256, so a read a hair
-     * under the wrap point rounds UP to exactly SC808_DLY_MAX and indexes one
+     * under the wrap point rounds UP to exactly W8_DLY_MAX and indexes one
      * past the buffer — it leaked the write counter into the audio as
      * denormals. Clamp AFTER the cast. */
-    if(i0 >= SC808_DLY_MAX) i0 -= SC808_DLY_MAX;
-    const int   i1 = i0 + 1 >= SC808_DLY_MAX ? 0 : i0 + 1;
+    if(i0 >= W8_DLY_MAX) i0 -= W8_DLY_MAX;
+    const int   i1 = i0 + 1 >= W8_DLY_MAX ? 0 : i0 + 1;
     const float y  = d->buf[i0] + (d->buf[i1] - d->buf[i0]) * fr;
 
     /* Feedback through a one-pole: every repeat gets darker, like tape and
@@ -154,7 +155,7 @@
     float st = x + d->lp;
     st = truncf(st * 2048.0f) * (1.0f / 2048.0f);   /* toward zero: no DC */
     d->buf[d->w] = st;
-    if(++d->w >= SC808_DLY_MAX) d->w = 0;
+    if(++d->w >= W8_DLY_MAX) d->w = 0;
     return y * d->level;
 }
 
@@ -164,9 +165,9 @@
 {
     int i = d->divi;
     if(i < 0) i = 0;
-    if(i >= SC808_DLY_DIVS) i = SC808_DLY_DIVS - 1;
+    if(i >= W8_DLY_DIVS) i = W8_DLY_DIVS - 1;
     const float bpm = d->bpm > 20.0f ? d->bpm : 120.0f;
-    float ms = kSC808DlyBeats[i] * 60000.0f / bpm;
+    float ms = kW8DlyBeats[i] * 60000.0f / bpm;
     /*
      * 9W9's margin, not 6W6's. 6W6 clamps to (MAX - 1) samples, which lets a
      * fully-slewed read land on the sample the writer is about to overwrite —
@@ -174,7 +175,7 @@
      * the longest divisions at slow tempos. 256 samples of daylight costs
      * 6 ms of maximum delay and cannot collide.
      */
-    const float max_ms = (float)(SC808_DLY_MAX - 256) * 1000.0f / sr;
+    const float max_ms = (float)(W8_DLY_MAX - 256) * 1000.0f / sr;
     if(ms > max_ms) ms = max_ms;
     d->time_ms = ms;
 }
@@ -235,4 +236,4 @@
     return _in * powf(10.0f, (g->env_db + makeup) / 20.0f);
 }
 
-#endif /* SC808_FX_H */
+#endif /* W8_FX_H */
--- sc808_engine.cpp
+++ w8_engine.cpp
@@ -1,15 +1,15 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_engine.cpp — see w8_engine.h.
  *
- * GPL-3.0. The voices under src/dsp/w8_voices.h are a transcription of
- * w8.scd (MIT); see THIRD_PARTY.md.
+ * GPL-3.0.
  */
 #include <math.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
-#include "w8_voices.h"
+#include "w8_rim.h"
 #include "w8_bd_circuit.h"
 #include "w8_cp_circuit.h"
 #include "w8_sd_circuit.h"
@@ -39,7 +39,7 @@
  *    kV1PotKeys is the order all three of them wrote and older blobs are
  *    placed by name against it.
  */
-#define SC808_STATE_VERSION 4
+#define W8_STATE_VERSION 4
 
 /*
  * The gain a FULL-velocity hit reaches — the old Accent pot's default,
@@ -51,7 +51,7 @@
  * back 6 dB quieter. Anchoring here keeps every existing pattern at exactly
  * the level it had.
  */
-#define SC808_FULL_VELOCITY_GAIN 1.9921260f
+#define W8_FULL_VELOCITY_GAIN 1.9921260f
 
 /* A choke is a 2 ms fade, not a hard stop — cutting a ringing open hat dead
  * puts a click on the front of the closed hat that follows it. */
@@ -59,61 +59,39 @@
 
 namespace {
 
-constexpr const char *kVoiceIds[SC808_NUM_VOICES] = {
+constexpr const char *kVoiceIds[W8_NUM_VOICES] = {
     "bd", "sd", "lt", "mt", "ht", "lc", "mc", "hc",
     "rs", "cl", "ma", "cp", "cb", "ch", "oh", "cy"
 };
 
 /*
- * Base MIDI notes, taken from w8's SynthDef defaults. Every Tune pot is a
- * SEMITONE OFFSET around these, so pot 64 is the w8 sound whatever the
- * voice happens to be tuned to, and the null test's verified defaults are
- * what a fresh patch loads with.
+ * Base MIDI notes. Every Tune pot is a SEMITONE OFFSET around these, so pot 64
+ * is the reference sound whatever the voice happens to be tuned to, and that
+ * is what a fresh patch loads with.
  *
  * The metal voices (cowbell, both hats, cymbal) have no note — they are six
  * fixed Schmitt oscillators — so their Tune is a frequency RATIO instead and
  * their entry here is unused.
  *
- * ONE DELIBERATE DEPARTURE: w8 gives congahi note 52, the same as congalo,
- * which is plainly a copy-paste slip since congamid sits at 57 between them.
- * 62 continues the progression. w8_voices.h is untouched and the null test
- * still runs against 52 — this is the engine choosing a knob position, not a
- * change to the transcription.
+ * The high conga continues the low/mid progression (52, 57, 62).
  */
-const float kBaseNote[SC808_NUM_VOICES] = {
+const float kBaseNote[W8_NUM_VOICES] = {
     34.0f,   /* bd */
     65.0f,   /* sd */
-    /*
-     * The six tom/conga notes are MEASURED, not w8's. Hardware samples
-     * with note names in the filenames, fundamentals confirmed by
-     * autocorrelation: toms F2 / C3 / G3, congas G3 / D4 / A4 — seven
-     * semitones between neighbours, congas more than an octave above their
-     * toms. w8's declared notes (40/44/52 and 52/57/62) are both mistuned
-     * and wrongly spaced against the real machine, which is why the lanes
-     * neither sounded right nor overlapped. The w8 engine is still
-     * exactly the transcription — it just gets played at the right pitch.
-     */
-    /* Settled fundamentals of Roland's own model at the default preset —
-     * the user's reference render (toms808.wav) matches within a cent. The
-     * earlier F2/C3/G3 came from third-party hardware samples recorded at
-     * other TUNING positions. */
+    /* The six tom/conga notes are MEASURED: settled fundamentals of the
+     * reference 808 at the default preset, matched within a cent. */
     42.02f,  /* lt — 92.6 Hz */
     48.58f,  /* mt — 135.3 Hz */
     54.83f,  /* ht — 183.0 Hz */
     /* Congas: Roland's model with the kit switch flipped to the conga
-     * side of each channel, measured settled — 206.1 / 302.1 / 404.6 Hz.
-     * The 196/293.7/440 that stood here came from third-party samples at
-     * unknown knob positions; the hi conga was a semitone and a half off. */
+     * side of each channel, measured settled — 206.1 / 302.1 / 404.6 Hz. */
     55.88f,  /* lc — 206.1 Hz */
     62.49f,  /* mc — 302.1 Hz */
     67.55f,  /* hc — 404.6 Hz */
-    /* MEASURED from rim808.wav: the tock sits at 1788 Hz and w8 puts it
-     * at note x 1.1, so the note is 91.62 — its declared 92 landed the rim
-     * a third of a semitone sharp, which is the "needs better tuning on the
-     * default" from the first field report. */
+    /* MEASURED from a recorded 808 rim: the tock sits at 1788 Hz and the rim
+     * voice puts it at note x 1.1, so the note is 91.62. */
     91.62f,  /* rs */
-    99.2f,   /* cl — MEASURED: Roland's own model pings at 2518 Hz, which is
-              *      99.2; w8's 99 (2489 Hz) sat 20 cents flat */
+    99.2f,   /* cl — MEASURED: the reference claves ping at 2518 Hz, 99.2 */
    113.0f,   /* ma — the highpass corner, this voice has no oscillator */
     71.0f,   /* cp — the highpass corner */
      0.0f, 0.0f, 0.0f, 0.0f    /* cb, ch, oh, cy: ratio-tuned */
@@ -122,8 +100,7 @@
 /*
  * Kit balance.
  *
- * Left alone the voices arrive at wildly different levels, because each
- * SynthDef was written to be played on its own with its own `amp`: the closed
+ * Left alone the voices arrive at wildly different levels: the closed
  * hat peaks near 17 while the hand clap peaks at 0.38, a spread of 33 dB.
  * Sixteen of those summed is not a drum machine, it is a fault.
  *
@@ -145,7 +122,7 @@
  * prints what to paste back here. Two earlier metrics are documented in that
  * file as failures worth not repeating — peak, and RMS over a fixed window.
  */
-constexpr float kVoiceTrim[SC808_NUM_VOICES] = {
+constexpr float kVoiceTrim[W8_NUM_VOICES] = {
     0.2964f,   /* bd — the reference: everything else is set against the kick */
     0.1953f,   /* sd */
     0.4110f,   /* lt */
@@ -157,7 +134,7 @@
     0.2643f,   /* rs — a click with a crest factor of 11 */
     2.6005f,   /* cl */
     0.6142f,   /* ma */
-    3.9640f,   /* cp — the quietest voice in w8 by a long way */
+    3.9640f,   /* cp — the quietest voice in Engine A by a long way */
     0.1219f,   /* cb */
     0.0021f,   /* ch — raw peak near 17 before the drive stage catches it */
     0.0402f,   /* oh */
@@ -173,7 +150,7 @@
  */
 constexpr bool trim_table_filled(const int i = 0)
 {
-    return i >= SC808_NUM_VOICES
+    return i >= W8_NUM_VOICES
          ? true
          : (kVoiceTrim[i] > 0.0f && trim_table_filled(i + 1));
 }
@@ -184,12 +161,12 @@
 /* Same trap, same table shape. */
 constexpr bool voice_ids_filled(const int i = 0)
 {
-    return i >= SC808_NUM_VOICES
+    return i >= W8_NUM_VOICES
          ? true
          : (kVoiceIds[i] != nullptr && kVoiceIds[i][0] != '\0'
             && voice_ids_filled(i + 1));
 }
-static_assert(voice_ids_filled(), "kVoiceIds is short of SC808_NUM_VOICES");
+static_assert(voice_ids_filled(), "kVoiceIds is short of W8_NUM_VOICES");
 
 /* Per-voice pot/enum slots, resolved once at create time so the audio path
  * never searches by string. */
@@ -208,19 +185,19 @@
     float choke_step;    /* < 0 while choking, else 0           */
     /* Crush's sample-and-hold. Per lane, because one shared state would
      * put the closed hat's decimator steps on the kick. */
-    float crush_st[SC808_CRUSH_STATE];
+    float crush_st[W8_CRUSH_STATE];
 };
 
 int find_pot(const char *key)
 {
-    for(int i = 0; i < SC808_NUM_POTS; ++i)
+    for(int i = 0; i < W8_NUM_POTS; ++i)
         if(!strcmp(g_w8_pots[i].key, key)) return i;
     return -1;
 }
 
 int find_enum(const char *key)
 {
-    for(int i = 0; i < SC808_NUM_ENUMS; ++i)
+    for(int i = 0; i < W8_NUM_ENUMS; ++i)
         if(!strcmp(g_w8_enums[i].key, key)) return i;
     return -1;
 }
@@ -232,7 +209,7 @@
 {
     const w8_pot_t &p = g_w8_pots[slot];
     const float t = (float)pot / 127.0f;
-    if(p.curve == SC808_EXP && p.min > 0.0f)
+    if(p.curve == W8_EXP && p.min > 0.0f)
         return p.min * powf(p.max / p.min, t);
     return p.min + (p.max - p.min) * t;
 }
@@ -242,12 +219,12 @@
 struct w8_engine {
     float sample_rate;
 
-    int   pot[SC808_NUM_POTS];       /* raw 0..127, the stored form   */
-    float potv[SC808_NUM_POTS];      /* resolved, recomputed on write */
-    int   env[SC808_NUM_ENUMS];      /* enum selections               */
-
-    VoiceSlots slot[SC808_NUM_VOICES];
-    VoiceRt    rt[SC808_NUM_VOICES];
+    int   pot[W8_NUM_POTS];       /* raw 0..127, the stored form   */
+    float potv[W8_NUM_POTS];      /* resolved, recomputed on write */
+    int   env[W8_NUM_ENUMS];      /* enum selections               */
+
+    VoiceSlots slot[W8_NUM_VOICES];
+    VoiceRt    rt[W8_NUM_VOICES];
 
     /* Voice-specific extras that only some lanes have. */
     int bd_attack, bd_tone, sd_snappy;
@@ -255,7 +232,7 @@
     /* Globals. */
     int e_master_dist, e_choke, e_note_map;
     int p_master_drive, p_volume, p_vel_depth, p_comp;
-    float crush_master[SC808_CRUSH_STATE];
+    float crush_master[W8_CRUSH_STATE];
 
     /* The two send buses and the bus glue. Heap-allocated with the engine —
      * the delay line alone is 352 KB, which is why w8_create callocs
@@ -291,14 +268,11 @@
     ClaveCircuit    clc;
     MaracasCircuit  mac;
     /*
-     * The ONE w8 voice still in the audio path. Two circuit rim shots
-     * were built from the schematic and both lost to this algorithm on
-     * hardware, so the transcription is the rim. Every other lane is the
-     * circuit; w8_voices.h keeps the rest of the transcription and the
-     * null test still verifies all of it, straight from the header —
-     * src/tools/nullref.cpp includes it without going through this engine.
+     * The one lane that is not a circuit model. Two circuit rim shots were
+     * built from the schematic and both lost to this oscillator rim on
+     * hardware, so this is the rim. See w8_rim.h.
      */
-    RimClave  rs;
+    RimLane  rs;
 };
 
 /*
@@ -323,14 +297,14 @@
     if(rh != e->fx_rev_hpf)
     {
         e->verb.hpf_hz = rh;
-        e->verb.hp.setHiPass(rh, kSC808_SendRQ, e->sample_rate);
+        e->verb.hp.setHiPass(rh, kW8_SendRQ, e->sample_rate);
         e->fx_rev_hpf = rh;
     }
     const float dh = e->potv[e->p_dly_hpf];
     if(dh != e->fx_dly_hpf)
     {
         e->dly.hpf_hz = dh;
-        e->dly.hp.setHiPass(dh, kSC808_SendRQ, e->sample_rate);
+        e->dly.hp.setHiPass(dh, kW8_SendRQ, e->sample_rate);
         e->fx_dly_hpf = dh;
     }
 
@@ -346,7 +320,7 @@
 
 const char *w8_voice_id(int voice)
 {
-    return (voice >= 0 && voice < SC808_NUM_VOICES) ? kVoiceIds[voice] : "";
+    return (voice >= 0 && voice < W8_NUM_VOICES) ? kVoiceIds[voice] : "";
 }
 
 w8_engine_t *w8_create(float sample_rate)
@@ -372,18 +346,18 @@
     if(!e) return NULL;
     e->sample_rate = sample_rate > 0.0f ? sample_rate : 44100.0f;
 
-    for(int i = 0; i < SC808_NUM_POTS; ++i)
+    for(int i = 0; i < W8_NUM_POTS; ++i)
     {
         e->pot[i]  = g_w8_pots[i].def;
         e->potv[i] = pot_value(i, e->pot[i]);
     }
-    for(int i = 0; i < SC808_NUM_ENUMS; ++i)
+    for(int i = 0; i < W8_NUM_ENUMS; ++i)
         e->env[i] = g_w8_enums[i].def;
 
     /* Resolve every key once. A miss here is a generator/engine mismatch and
      * the loadtest asserts on it rather than letting it degrade silently. */
     char key[64];
-    for(int v = 0; v < SC808_NUM_VOICES; ++v)
+    for(int v = 0; v < W8_NUM_VOICES; ++v)
     {
         const char *id = kVoiceIds[v];
         snprintf(key, sizeof(key), "%s_tune",      id); e->slot[v].tune  = find_pot(key);
@@ -453,8 +427,8 @@
 
 void w8_set_mutes(w8_engine_t *e, unsigned mask)
 {
-    e->mutes = mask & ((1u << SC808_NUM_VOICES) - 1u);
-    for(int v = 0; v < SC808_NUM_VOICES; ++v)
+    e->mutes = mask & ((1u << W8_NUM_VOICES) - 1u);
+    for(int v = 0; v < W8_NUM_VOICES; ++v)
     {
         if(e->mutes & (1u << v))
         {
@@ -489,7 +463,7 @@
 
 void w8_trigger(w8_engine_t *e, int voice, int velocity)
 {
-    if(voice < 0 || voice >= SC808_NUM_VOICES) return;
+    if(voice < 0 || voice >= W8_NUM_VOICES) return;
     if(velocity <= 0) return;                       /* note-off: one-shots */
     if(e->mutes & (1u << voice)) return;
 
@@ -514,7 +488,7 @@
      * reference points land exactly where they always did.
      */
     const int   vi    = velocity > 127 ? 127 : velocity;
-    const float vgain = SC808_FULL_VELOCITY_GAIN
+    const float vgain = W8_FULL_VELOCITY_GAIN
                       * (1.0f - e->potv[e->p_vel_depth]
                                 * (1.0f - (float)vi * (1.0f / 127.0f)));
     /* the strike, in the volts the hardware's VR3 delivers */
@@ -538,12 +512,12 @@
      * on the hardware it shares the same oscillators, but nobody wants a
      * hi-hat swallowing their crash. */
     const int choke = e->env[e->e_choke];
-    if(voice == SC808_CH && choke >= 1) { choke_voice(e, SC808_OH); e->ohc.choke(); }
-    if(voice == SC808_OH && choke == 2) { choke_voice(e, SC808_CH); e->chc.choke(); }
+    if(voice == W8_CH && choke >= 1) { choke_voice(e, W8_OH); e->ohc.choke(); }
+    if(voice == W8_OH && choke == 2) { choke_voice(e, W8_CH); e->chc.choke(); }
 
     switch(voice)
     {
-    case SC808_BD:
+    case W8_BD:
     {
         /*
          * The circuit kick. Its three pots are read as raw POT POSITIONS
@@ -566,7 +540,7 @@
         e->rt[voice].hit_gain = softGain;
         break;
     }
-    case SC808_SD:
+    case W8_SD:
     {
         /* The circuit snare, read as raw POT POSITIONS: Snappy is a divider
          * on the trigger rather than a mix, and Tune is a ratio on both
@@ -590,10 +564,10 @@
      * changes, so the lane's own gain only carries the range below the
      * voltage floor.
      */
-    case SC808_LT: case SC808_MT: case SC808_HT:
-    case SC808_LC: case SC808_MC: case SC808_HC:
-    {
-        const int i = voice - SC808_LT;
+    case W8_LT: case W8_MT: case W8_HT:
+    case W8_LC: case W8_MC: case W8_HC:
+    {
+        const int i = voice - W8_LT;
         TomCircuit *const tc[6] = { &e->ltc, &e->mtc, &e->htc,
                                     &e->lcc, &e->mcc, &e->hcc };
         /* Decay is RING TIME IN SECONDS — the circuit solves the loop gain
@@ -603,15 +577,15 @@
         e->rt[voice].hit_gain = softGain;   /* the rest is in the trigger volts */
         break;
     }
-    case SC808_RS:
+    case W8_RS:
         /*
-         * ONE VOICE, no switch: the w8 rim, which beat two circuit
+         * ONE VOICE, no switch: the Engine A rim, which beat two circuit
          * builds on hardware. Decay arrives as SECONDS of audible ring and
-         * is converted here — w8's envelope runs at curve -42, so the
+         * is converted here — Engine A's envelope runs at curve -42, so the
          * part above 1% of peak is ln(100)/42 = 11% of the declared
          * duration.
          *
-         * The band filters follow Tune. w8 pins them at 63 and 118,
+         * The band filters follow Tune. Engine A pins them at 63 and 118,
          * which is fine while the note never moves and sounds broken once
          * it does.
          */
@@ -619,7 +593,7 @@
                       (double)midicps(63.0f + tune),
                       (double)midicps(118.0f + tune));
         break;
-    case SC808_CL:
+    case W8_CL:
     {
         e->clc.trigger(powf(2.0f, tune / 12.0f),
                        (float)e->pot[e->slot[voice].decay] / 127.0f,
@@ -627,21 +601,21 @@
         e->rt[voice].hit_gain = softGain;
         break;
     }
-    case SC808_MA:
+    case W8_MA:
     {
         e->mac.trigger(powf(2.0f, tune / 12.0f), decay,
                        (float)e->pot[e->ma_attack] / 127.0f, volts);
         e->rt[voice].hit_gain = softGain;
         break;
     }
-    case SC808_CP:
+    case W8_CP:
         /* Burst spacing and tail mix are the hardware's, fixed — the panel
          * has Tune and Decay, like the machine had Level alone. Decay
          * scales the two derived tails together; the burst spacing stays
          * the hardware's 10 ms. */
         e->cpc.trigger(powf(2.0f, tune / 12.0f), decay, 0.010f, 0.0f);
         break;
-    case SC808_CB:
+    case W8_CB:
     {
         e->mbank.setRatio((double)tune);
         e->cbc.setRatio((double)tune);
@@ -649,7 +623,7 @@
         e->rt[voice].hit_gain = softGain;
         break;
     }
-    case SC808_CH:
+    case W8_CH:
         /*
          * Velocity stays a GAIN on the hats, not a trigger voltage. A
          * hotter envelope into the diode gate makes the note LONGER and puts
@@ -661,12 +635,12 @@
         e->mbank.setRatio((double)tune);
         e->chc.trigger(decay, 4.0f);   /* the envelope that matches the references */
         break;
-    case SC808_OH:
+    case W8_OH:
         /* velocity as gain, same reasoning as the closed hat */
         e->mbank.setRatio((double)tune);
         e->ohc.trigger(decay, 4.0f);
         break;
-    case SC808_CY:
+    case W8_CY:
         /* velocity as gain, like the hats; no Tone — the crash balance is
          * the reference's, fixed in the voice */
         e->mbank.setRatio((double)tune);
@@ -725,7 +699,7 @@
 /* A silent lane must not cost a process() call — the cymbal alone runs
  * eighteen biquads. Guarding on the gate gain also means a muted lane keeps
  * rendering until its fade completes, then disappears. */
-#define SC808_LANE(vid, obj) \
+#define W8_LANE(vid, obj) \
     do { if(e->rt[vid].choke_gain > 0.0f && e->obj.active()) \
              w8_add(e, vid, e->obj.process(), &mix, &send_r, &send_d); \
        } while(0)
@@ -741,18 +715,18 @@
     for(int i = 0; i < frames; ++i)
     {
         float mix = 0.0f, send_r = 0.0f, send_d = 0.0f;
-        SC808_LANE(SC808_BD, bdc);
-        SC808_LANE(SC808_SD, sdc);
-        SC808_LANE(SC808_LT, ltc);
-        SC808_LANE(SC808_MT, mtc);
-        SC808_LANE(SC808_HT, htc);
-        SC808_LANE(SC808_LC, lcc);
-        SC808_LANE(SC808_MC, mcc);
-        SC808_LANE(SC808_HC, hcc);
-        SC808_LANE(SC808_RS, rs);           /* the w8 rim, see the trigger */
-        SC808_LANE(SC808_CL, clc);
-        SC808_LANE(SC808_MA, mac);
-        SC808_LANE(SC808_CP, cpc);
+        W8_LANE(W8_BD, bdc);
+        W8_LANE(W8_SD, sdc);
+        W8_LANE(W8_LT, ltc);
+        W8_LANE(W8_MT, mtc);
+        W8_LANE(W8_HT, htc);
+        W8_LANE(W8_LC, lcc);
+        W8_LANE(W8_MC, mcc);
+        W8_LANE(W8_HC, hcc);
+        W8_LANE(W8_RS, rs);           /* the Engine A rim, see the trigger */
+        W8_LANE(W8_CL, clc);
+        W8_LANE(W8_MA, mac);
+        W8_LANE(W8_CP, cpc);
         /*
          * The shared bank ticks ONCE per sample, always — free-running is
          * only true if the oscillators advance while nothing is sounding,
@@ -760,16 +734,16 @@
          * same bus, as the hardware's one HD14584 does.
          */
         const double mbus = e->mbank.tick();
-        SC808_LANE(SC808_CB, cbc);
+        W8_LANE(W8_CB, cbc);
         /* The three lanes that read the shared bus. */
-#define SC808_METAL(vid, circ) \
+#define W8_METAL(vid, circ) \
         do { if(e->rt[vid].choke_gain > 0.0f && e->circ.active()) \
                  w8_add(e, vid, e->circ.process(mbus), \
                            &mix, &send_r, &send_d); } while(0)
-        SC808_METAL(SC808_CH, chc);
-        SC808_METAL(SC808_OH, ohc);
-        SC808_METAL(SC808_CY, cyc);
-#undef SC808_METAL
+        W8_METAL(W8_CH, chc);
+        W8_METAL(W8_OH, ohc);
+        W8_METAL(W8_CY, cyc);
+#undef W8_METAL
 
         /*
          * The wet returns join the bus BEFORE the master stages, so master
@@ -811,9 +785,9 @@
         return 1;
     }
 
-    /* "default" resets a control to its w8 default. Those defaults are not
-     * pot centre — they are the arguments the SynthDefs declare — so a UI
-     * that wants a reset gesture must not guess 64; it asks. */
+    /* "default" resets a control to its default. Those defaults are not pot
+     * centre — they are the voices' reference settings — so a UI that wants a
+     * reset gesture must not guess 64; it asks. */
     const int reset = !strcmp(val, "default");
     const int slot = find_pot(key);
     if(slot >= 0)
@@ -850,11 +824,11 @@
 
 int w8_serialize(const w8_engine_t *e, char *buf, int len)
 {
-    int n = snprintf(buf, len, "{\"v\":%d,\"pots\":[", SC808_STATE_VERSION);
-    for(int i = 0; i < SC808_NUM_POTS && n < len; ++i)
+    int n = snprintf(buf, len, "{\"v\":%d,\"pots\":[", W8_STATE_VERSION);
+    for(int i = 0; i < W8_NUM_POTS && n < len; ++i)
         n += snprintf(buf + n, len - n, i ? ",%d" : "%d", e->pot[i]);
     if(n < len) n += snprintf(buf + n, len - n, "],\"enums\":[");
-    for(int i = 0; i < SC808_NUM_ENUMS && n < len; ++i)
+    for(int i = 0; i < W8_NUM_ENUMS && n < len; ++i)
         n += snprintf(buf + n, len - n, i ? ",%d" : "%d", e->env[i]);
     if(n < len) n += snprintf(buf + n, len - n, "],\"mutes\":%u}", e->mutes);
     return n;
@@ -952,15 +926,15 @@
     const bool remapDist = version < 3;
 
     const char *p = strstr(json, "\"pots\"");
-    enum { kScratch = (SC808_NUM_POTS > SC808_NUM_ENUMS ? SC808_NUM_POTS
-                                                        : SC808_NUM_ENUMS) };
+    enum { kScratch = (W8_NUM_POTS > W8_NUM_ENUMS ? W8_NUM_POTS
+                                                        : W8_NUM_ENUMS) };
     const int kV1Pots  = (int)(sizeof kV1PotKeys  / sizeof kV1PotKeys[0]);
     const int kV1Enums = (int)(sizeof kV1EnumKeys / sizeof kV1EnumKeys[0]);
     int vals[kScratch > kV1Pots ? (kScratch > kV1Enums ? kScratch : kV1Enums)
                                 : (kV1Pots > kV1Enums ? kV1Pots : kV1Enums)];
     int got = 0;
 
-    p = scan_ints(p, vals, potsByName ? kV1Pots : SC808_NUM_POTS, &got);
+    p = scan_ints(p, vals, potsByName ? kV1Pots : W8_NUM_POTS, &got);
     for(int i = 0; i < got; ++i)
     {
         const int v = vals[i] < 0 ? 0 : (vals[i] > 127 ? 127 : vals[i]);
@@ -968,17 +942,17 @@
          * then, which may now live somewhere else or nowhere at all —
          * "accent" is exactly that, and it simply has nowhere to land. */
         const int slot = potsByName ? (i < kV1Pots ? find_pot(kV1PotKeys[i]) : -1) : i;
-        if(slot < 0 || slot >= SC808_NUM_POTS) continue;
+        if(slot < 0 || slot >= W8_NUM_POTS) continue;
         e->pot[slot]  = v;
         e->potv[slot] = pot_value(slot, v);
     }
 
     const char *q = strstr(json, "\"enums\"");
-    scan_ints(q, vals, enumsByName ? kV1Enums : SC808_NUM_ENUMS, &got);
+    scan_ints(q, vals, enumsByName ? kV1Enums : W8_NUM_ENUMS, &got);
     for(int i = 0; i < got; ++i)
     {
         const int slot = enumsByName ? (i < kV1Enums ? find_enum(kV1EnumKeys[i]) : -1) : i;
-        if(slot < 0 || slot >= SC808_NUM_ENUMS) continue;
+        if(slot < 0 || slot >= W8_NUM_ENUMS) continue;
         int v = vals[i] < 0 ? 0 : vals[i];
         if(remapDist)
         {
@@ -996,7 +970,7 @@
 
     const char *mp = strstr(json, "\"mutes\"");
     if(mp) { mp = strchr(mp, ':'); if(mp) e->mutes = (unsigned)strtoul(mp + 1, NULL, 10)
-                                                     & ((1u << SC808_NUM_VOICES) - 1u); }
+                                                     & ((1u << W8_NUM_VOICES) - 1u); }
 }
 
 /* Read-only view of the kit balance, for tools/kit_check. Not part of the
--- sc808_engine.h
+++ w8_engine.h
@@ -1,9 +1,10 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_engine.h — 8W8 voice orchestration.
  *
- * The DSP itself is w8_voices.h, which is a verified transcription of
- * w8.scd and is left alone. This layer is everything a drum machine needs
- * that a set of SynthDefs does not have: pot mapping, per-voice drive and
+ * The voices themselves are the w8_*_circuit.h models and the rim in
+ * w8_rim.h. This layer is everything a drum machine needs around them: pot
+ * mapping, per-voice drive and
  * distortion, velocity, hi-hat choke, per-lane mutes and the master stage.
  *
  * Realtime contract: every entry point here runs on the SPI callback. Nothing
@@ -11,8 +12,8 @@
  *
  * GPL-3.0.
  */
-#ifndef SC808_ENGINE_H
-#define SC808_ENGINE_H
+#ifndef W8_ENGINE_H
+#define W8_ENGINE_H
 
 #include <stddef.h>
 
@@ -30,9 +31,9 @@
  * The rim shot and the claves have a pad each. They share one CHANNEL on the
  * hardware — the 808's panel has a single RS/CL selector and a pattern cannot
  * contain both — and 8W8 ran them as one lane with a mode switch for exactly
- * that reason. Splitting them is a deliberate departure: w8 ships them as
- * two SynthDefs, a pattern that wants both is a normal thing to want, and it
- * makes the roster sixteen, which is what the pad block holds.
+ * that reason. Splitting them is a deliberate departure: a pattern that wants
+ * both is a normal thing to want, and it makes the roster sixteen, which is
+ * what the pad block holds.
  *
  * The cost is the Master pad. With sixteen drums there is no sixteenth pad
  * left for it, so Master is reached by the jog — click for the section list,
@@ -40,11 +41,11 @@
  * this one cannot.
  */
 typedef enum {
-    SC808_BD = 0, SC808_SD, SC808_LT, SC808_MT,
-    SC808_HT, SC808_LC, SC808_MC, SC808_HC,
-    SC808_RS, SC808_CL, SC808_MA, SC808_CP,
-    SC808_CB, SC808_CH, SC808_OH, SC808_CY,
-    SC808_NUM_VOICES
+    W8_BD = 0, W8_SD, W8_LT, W8_MT,
+    W8_HT, W8_LC, W8_MC, W8_HC,
+    W8_RS, W8_CL, W8_MA, W8_CP,
+    W8_CB, W8_CH, W8_OH, W8_CY,
+    W8_NUM_VOICES
 } w8_voice_t;
 
 /* Velocity has no threshold: level follows it all the way up, reaching the
@@ -83,4 +84,4 @@
 #ifdef __cplusplus
 }
 #endif
-#endif /* SC808_ENGINE_H */
+#endif /* W8_ENGINE_H */
--- sc808_params.h
+++ w8_params.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /* Generated by scripts/gen_params.py — DO NOT EDIT BY HAND.
  *
  * module.json is capped at 8 KB by Schwung's loader, so chain_params and the
@@ -6,10 +7,10 @@
  * The pot and enum tables below define storage order for the state blob.
  * Appending is safe; reordering breaks every saved patch.
  */
-#ifndef SC808_PARAMS_H
-#define SC808_PARAMS_H
-
-typedef enum { SC808_LIN = 0, SC808_EXP = 1 } w8_curve_t;
+#ifndef W8_PARAMS_H
+#define W8_PARAMS_H
+
+typedef enum { W8_LIN = 0, W8_EXP = 1 } w8_curve_t;
 
 typedef struct {
     const char    *key;
@@ -25,123 +26,123 @@
     int         def;
 } w8_enum_t;
 
-#define SC808_NUM_POTS  110
-#define SC808_NUM_ENUMS 20
-
-static const w8_pot_t g_w8_pots[SC808_NUM_POTS] = {
-    { "bd_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "bd_attack",     0.0000f,     1.0000f, SC808_LIN,  24 },
-    { "bd_decay",     0.1000f,     8.0000f, SC808_EXP,  87 },
-    { "bd_tone",     0.0000f,     6.0000f, SC808_LIN,  42 },
-    { "bd_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "bd_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "sd_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "sd_decay",     0.1000f,     8.0000f, SC808_EXP, 108 },
-    { "sd_snappy",     0.0000f,     1.0000f, SC808_LIN,  89 },
-    { "sd_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "sd_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "lt_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "lt_decay",     0.0600f,     2.0000f, SC808_EXP,  68 },
-    { "lt_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "lt_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "mt_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "mt_decay",     0.0600f,     2.0000f, SC808_EXP,  56 },
-    { "mt_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "mt_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "ht_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "ht_decay",     0.0600f,     2.0000f, SC808_EXP,  53 },
-    { "ht_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "ht_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "lc_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "lc_decay",     0.0600f,     2.0000f, SC808_EXP,  58 },
-    { "lc_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "lc_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "mc_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "mc_decay",     0.0600f,     2.0000f, SC808_EXP,  36 },
-    { "mc_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "mc_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "hc_tune",    -2.0000f,     2.0000f, SC808_LIN,  64 },
-    { "hc_decay",     0.0600f,     2.0000f, SC808_EXP,  34 },
-    { "hc_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "hc_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "rs_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "rs_decay",     0.0050f,     0.2000f, SC808_EXP,  40 },
-    { "rs_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "rs_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "cl_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "cl_decay",     0.0000f,     1.0000f, SC808_LIN,  64 },
-    { "cl_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "cl_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "ma_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "ma_attack",     0.0000f,     1.0000f, SC808_LIN,  32 },
-    { "ma_decay",     0.0100f,     0.3000f, SC808_EXP,  51 },
-    { "ma_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "ma_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "cp_tune",   -12.0000f,    12.0000f, SC808_LIN,  64 },
-    { "cp_decay",     0.3500f,     2.8000f, SC808_EXP,  64 },
-    { "cp_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "cp_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "cb_tune",     0.5000f,     2.0000f, SC808_EXP,  64 },
-    { "cb_decay",     0.1000f,     2.0000f, SC808_EXP,  62 },
-    { "cb_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "cb_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "ch_tune",     0.5000f,     2.0000f, SC808_EXP,  64 },
-    { "ch_decay",     0.0300f,     0.5000f, SC808_EXP,  47 },
-    { "ch_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "ch_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "oh_tune",     0.5000f,     2.0000f, SC808_EXP,  64 },
-    { "oh_decay",     0.0800f,     1.5000f, SC808_EXP,  74 },
-    { "oh_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "oh_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "cy_tune",     0.5000f,     2.0000f, SC808_EXP,  64 },
-    { "cy_decay",     0.4000f,     6.0000f, SC808_EXP,  70 },
-    { "cy_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "cy_level",     0.0000f,     2.0000f, SC808_LIN,  64 },
-    { "master_drive",     0.0000f,    10.0000f, SC808_LIN,   0 },
-    { "comp",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "volume",     0.0000f,     1.0000f, SC808_LIN, 100 },
-    { "vel_depth",     0.0000f,     1.0000f, SC808_LIN, 127 },
-    { "sd_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "sd_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "lt_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "lt_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "mt_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "mt_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ht_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ht_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "lc_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "lc_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "mc_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "mc_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "hc_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "hc_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "rs_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "rs_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cl_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cl_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ma_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ma_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cp_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cp_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cb_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cb_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ch_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "ch_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "oh_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "oh_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cy_rev",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "cy_dly",     0.0000f,     1.0000f, SC808_LIN,   0 },
-    { "rev_decay",     0.2000f,     0.9300f, SC808_LIN,  73 },
-    { "rev_tone",     0.0000f,     1.0000f, SC808_LIN,  57 },
-    { "rev_hpf",    30.0000f,   800.0000f, SC808_EXP,  62 },
-    { "rev_level",     0.0000f,     1.2000f, SC808_LIN,  85 },
-    { "dly_fdbk",     0.0000f,     0.8500f, SC808_LIN,  52 },
-    { "dly_tone",     0.0000f,     1.0000f, SC808_LIN,  51 },
-    { "dly_hpf",    30.0000f,   800.0000f, SC808_EXP,  62 },
-    { "dly_level",     0.0000f,     1.2000f, SC808_LIN,  85 },
+#define W8_NUM_POTS  110
+#define W8_NUM_ENUMS 20
+
+static const w8_pot_t g_w8_pots[W8_NUM_POTS] = {
+    { "bd_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "bd_attack",     0.0000f,     1.0000f, W8_LIN,  24 },
+    { "bd_decay",     0.1000f,     8.0000f, W8_EXP,  87 },
+    { "bd_tone",     0.0000f,     6.0000f, W8_LIN,  42 },
+    { "bd_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "bd_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "sd_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "sd_decay",     0.1000f,     8.0000f, W8_EXP, 108 },
+    { "sd_snappy",     0.0000f,     1.0000f, W8_LIN,  89 },
+    { "sd_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "sd_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "lt_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "lt_decay",     0.0600f,     2.0000f, W8_EXP,  68 },
+    { "lt_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "lt_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "mt_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "mt_decay",     0.0600f,     2.0000f, W8_EXP,  56 },
+    { "mt_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "mt_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "ht_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "ht_decay",     0.0600f,     2.0000f, W8_EXP,  53 },
+    { "ht_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "ht_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "lc_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "lc_decay",     0.0600f,     2.0000f, W8_EXP,  58 },
+    { "lc_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "lc_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "mc_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "mc_decay",     0.0600f,     2.0000f, W8_EXP,  36 },
+    { "mc_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "mc_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "hc_tune",    -2.0000f,     2.0000f, W8_LIN,  64 },
+    { "hc_decay",     0.0600f,     2.0000f, W8_EXP,  34 },
+    { "hc_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "hc_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "rs_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "rs_decay",     0.0050f,     0.2000f, W8_EXP,  40 },
+    { "rs_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "rs_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "cl_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "cl_decay",     0.0000f,     1.0000f, W8_LIN,  64 },
+    { "cl_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "cl_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "ma_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "ma_attack",     0.0000f,     1.0000f, W8_LIN,  32 },
+    { "ma_decay",     0.0100f,     0.3000f, W8_EXP,  51 },
+    { "ma_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "ma_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "cp_tune",   -12.0000f,    12.0000f, W8_LIN,  64 },
+    { "cp_decay",     0.3500f,     2.8000f, W8_EXP,  64 },
+    { "cp_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "cp_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "cb_tune",     0.5000f,     2.0000f, W8_EXP,  64 },
+    { "cb_decay",     0.1000f,     2.0000f, W8_EXP,  62 },
+    { "cb_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "cb_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "ch_tune",     0.5000f,     2.0000f, W8_EXP,  64 },
+    { "ch_decay",     0.0300f,     0.5000f, W8_EXP,  47 },
+    { "ch_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "ch_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "oh_tune",     0.5000f,     2.0000f, W8_EXP,  64 },
+    { "oh_decay",     0.0800f,     1.5000f, W8_EXP,  74 },
+    { "oh_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "oh_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "cy_tune",     0.5000f,     2.0000f, W8_EXP,  64 },
+    { "cy_decay",     0.4000f,     6.0000f, W8_EXP,  70 },
+    { "cy_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "cy_level",     0.0000f,     2.0000f, W8_LIN,  64 },
+    { "master_drive",     0.0000f,    10.0000f, W8_LIN,   0 },
+    { "comp",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "volume",     0.0000f,     1.0000f, W8_LIN, 100 },
+    { "vel_depth",     0.0000f,     1.0000f, W8_LIN, 127 },
+    { "sd_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "sd_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "lt_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "lt_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "mt_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "mt_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ht_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ht_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "lc_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "lc_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "mc_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "mc_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "hc_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "hc_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "rs_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "rs_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cl_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cl_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ma_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ma_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cp_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cp_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cb_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cb_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ch_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "ch_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "oh_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "oh_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cy_rev",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "cy_dly",     0.0000f,     1.0000f, W8_LIN,   0 },
+    { "rev_decay",     0.2000f,     0.9300f, W8_LIN,  73 },
+    { "rev_tone",     0.0000f,     1.0000f, W8_LIN,  57 },
+    { "rev_hpf",    30.0000f,   800.0000f, W8_EXP,  62 },
+    { "rev_level",     0.0000f,     1.2000f, W8_LIN,  85 },
+    { "dly_fdbk",     0.0000f,     0.8500f, W8_LIN,  52 },
+    { "dly_tone",     0.0000f,     1.0000f, W8_LIN,  51 },
+    { "dly_hpf",    30.0000f,   800.0000f, W8_EXP,  62 },
+    { "dly_level",     0.0000f,     1.2000f, W8_LIN,  85 },
 };
 
-static const w8_enum_t g_w8_enums[SC808_NUM_ENUMS] = {
+static const w8_enum_t g_w8_enums[W8_NUM_ENUMS] = {
     { "bd_dist_type",  7,  0 },
     { "sd_dist_type",  7,  0 },
     { "lt_dist_type",  7,  0 },
@@ -164,7 +165,7 @@
     { "dly_time", 13,  7 },
 };
 
-#define SC808_CHAIN_PARAMS_LEN 11949
+#define W8_CHAIN_PARAMS_LEN 11949
 static const char w8_chain_params_json[] =
     "[{\"key\":\"bd_tune\",\"name\":\"BD Tune\",\"type\":\"int\",\"min\":0,\"max\":127,\"default\":64},{\"key\":\"bd_attack\",\""
     "name\":\"BD Attack\",\"type\":\"int\",\"min\":0,\"max\":127,\"default\":24,\"viz\":false},{\"key\":\"bd_decay\",\"name\":"
@@ -287,7 +288,7 @@
     "ey\":\"ui_focus\",\"name\":\"Focus\",\"type\":\"int\",\"min\":0,\"max\":16,\"default\":0},{\"key\":\"mutes\",\"name\":\"Mute"
     "s\",\"type\":\"int\",\"min\":0,\"max\":65535,\"default\":0}]";
 
-#define SC808_UI_PAGES_LEN 7924
+#define W8_UI_PAGES_LEN 7924
 static const char w8_ui_pages_json[] =
     "{\"pad_layout\":\"drums\",\"focus_param\":\"ui_focus_level\",\"levels\":{\"bd\":{\"name\":\"Bass Drum\",\"knobs\":[\"bd"
     "_tune\",\"bd_attack\",\"bd_decay\",\"bd_tone\",\"bd_drive\",\"bd_dist_type\",\"bd_level\"],\"params\":[{\"key\":\"bd_t"
@@ -373,7 +374,7 @@
 /* The same hierarchy carrying the General MIDI note map, for note_map == 1.
  * The map is switchable at runtime, so one static answer would be wrong half
  * the time; get_param picks. */
-#define SC808_UI_PAGES_GM_LEN 7924
+#define W8_UI_PAGES_GM_LEN 7924
 static const char w8_ui_pages_gm_json[] =
     "{\"pad_layout\":\"drums\",\"focus_param\":\"ui_focus_level\",\"levels\":{\"bd\":{\"name\":\"Bass Drum\",\"knobs\":[\"bd"
     "_tune\",\"bd_attack\",\"bd_decay\",\"bd_tone\",\"bd_drive\",\"bd_dist_type\",\"bd_level\"],\"params\":[{\"key\":\"bd_t"
@@ -456,4 +457,4 @@
     "Volume\"},{\"key\":\"vel_depth\",\"label\":\"Velocity\"},{\"key\":\"hh_choke\",\"label\":\"Choke\"},{\"key\":\"note_map\""
     ",\"label\":\"Note Map\"}]}}}";
 
-#endif /* SC808_PARAMS_H */
+#endif /* W8_PARAMS_H */
--- sc808_plugin.cpp
+++ w8_plugin.cpp
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * w8_plugin.cpp — Schwung plugin_api_v2 wrapper for the 8W8 engine.
  *
@@ -5,9 +6,8 @@
  * therefore does no allocation, no file I/O and takes no locks — all of that
  * happens in create_instance.
  *
- * Structure follows 6W6's sd606_plugin.cpp, which follows 9W9's
- * 9w9_plugin.c, on purpose: the three kits should feel identical under the
- * hands, so the pad map, silent-select window, per-lane mutes and pad-follow
+ * Structure follows the 6W6 and 9W9 plugins on purpose: the three kits
+ * should feel identical under the hands, so the pad map, silent-select window, per-lane mutes and pad-follow
  * are the same mechanisms with an 808 roster. GPL-3.0.
  */
 #include <stdint.h>
@@ -30,7 +30,7 @@
  * voices silently never followed the pad — for months, with no error anywhere.
  * tools/voices_check.py resolves all sixteen of these against the declared
  * levels rather than leaving it to the eye. */
-static const char *const kLevelOf[SC808_NUM_VOICES] = {
+static const char *const kLevelOf[W8_NUM_VOICES] = {
     "bd", "sd", "lt", "mt", "ht", "lc", "mc", "hc",
     "rs", "cl", "ma", "cp", "cb", "ch", "oh", "cy"
 };
@@ -79,8 +79,8 @@
  *                  drum block
  *
  * The kit lives in the LEFT block so it reads like a drum machine instead of
- * being smeared across the grid. SIXTEEN voices fill it exactly, one per
- * w8 SynthDef — 6W6 used half this block and 9W9 eleven pads.
+ * being smeared across the grid. SIXTEEN voices fill it exactly — 6W6 used
+ * half this block and 9W9 eleven pads.
  *
  * Bottom row is the 808's front-panel order (BD SD LT MT); the toms continue
  * into the congas above them, then the percussion, then the metal on top.
@@ -94,17 +94,17 @@
     switch(_note)
     {
     /* row 0 */
-    case 68: return SC808_BD;   case 69: return SC808_SD;
-    case 70: return SC808_LT;   case 71: return SC808_MT;
+    case 68: return W8_BD;   case 69: return W8_SD;
+    case 70: return W8_LT;   case 71: return W8_MT;
     /* row 1 */
-    case 76: return SC808_HT;   case 77: return SC808_LC;
-    case 78: return SC808_MC;   case 79: return SC808_HC;
+    case 76: return W8_HT;   case 77: return W8_LC;
+    case 78: return W8_MC;   case 79: return W8_HC;
     /* row 2 */
-    case 84: return SC808_RS;   case 85: return SC808_CL;
-    case 86: return SC808_MA;   case 87: return SC808_CP;
+    case 84: return W8_RS;   case 85: return W8_CL;
+    case 86: return W8_MA;   case 87: return W8_CP;
     /* row 3 */
-    case 92: return SC808_CB;   case 93: return SC808_CH;
-    case 94: return SC808_OH;   case 95: return SC808_CY;
+    case 92: return W8_CB;   case 93: return W8_CH;
+    case 94: return W8_OH;   case 95: return W8_CY;
     default: return -1;
     }
 }
@@ -134,22 +134,22 @@
 
     switch(_note)   /* GM drum map */
     {
-    case 35: case 36: return SC808_BD;
-    case 38: case 40: return SC808_SD;
-    case 41: case 43: return SC808_LT;
-    case 45: case 47: return SC808_MT;
-    case 48: case 50: return SC808_HT;
-    case 64:          return SC808_LC;   /* low conga        */
-    case 62:          return SC808_MC;   /* mute high conga  */
-    case 63:          return SC808_HC;   /* open high conga  */
-    case 37:          return SC808_RS;   /* side stick */
-    case 75:          return SC808_CL;   /* claves — their own lane now */
-    case 70:          return SC808_MA;
-    case 39:          return SC808_CP;
-    case 56:          return SC808_CB;
-    case 42: case 44: return SC808_CH;
-    case 46:          return SC808_OH;
-    case 49: case 51: case 57: case 59: return SC808_CY;
+    case 35: case 36: return W8_BD;
+    case 38: case 40: return W8_SD;
+    case 41: case 43: return W8_LT;
+    case 45: case 47: return W8_MT;
+    case 48: case 50: return W8_HT;
+    case 64:          return W8_LC;   /* low conga        */
+    case 62:          return W8_MC;   /* mute high conga  */
+    case 63:          return W8_HC;   /* open high conga  */
+    case 37:          return W8_RS;   /* side stick */
+    case 75:          return W8_CL;   /* claves — their own lane now */
+    case 70:          return W8_MA;
+    case 39:          return W8_CP;
+    case 56:          return W8_CB;
+    case 42: case 44: return W8_CH;
+    case 46:          return W8_OH;
+    case 49: case 51: case 57: case 59: return W8_CY;
     default:          return -1;
     }
 }
@@ -287,9 +287,9 @@
      * parameter surface is larger than that — 92 params over 16 pages. */
     if(!strcmp(_key, "chain_params"))
     {
-        if(_len <= SC808_CHAIN_PARAMS_LEN) return -1;   /* refuse to truncate */
-        memcpy(_buf, w8_chain_params_json, SC808_CHAIN_PARAMS_LEN + 1);
-        return SC808_CHAIN_PARAMS_LEN;
+        if(_len <= W8_CHAIN_PARAMS_LEN) return -1;   /* refuse to truncate */
+        memcpy(_buf, w8_chain_params_json, W8_CHAIN_PARAMS_LEN + 1);
+        return W8_CHAIN_PARAMS_LEN;
     }
     /* ui_hierarchy is deliberately served EMPTY, not refused.
      * enterComponentEdit prefers a module's hierarchy and only falls back to
@@ -323,7 +323,7 @@
     {
         const int gm = (g_note_map != 0);
         const char *const j = gm ? w8_ui_pages_gm_json : w8_ui_pages_json;
-        const int n = gm ? SC808_UI_PAGES_GM_LEN : SC808_UI_PAGES_LEN;
+        const int n = gm ? W8_UI_PAGES_GM_LEN : W8_UI_PAGES_LEN;
         if(_len <= n) return -1;
         memcpy(_buf, j, (size_t)n + 1);
         return n;
@@ -338,7 +338,7 @@
     if(!strcmp(_key, "ui_focus_level"))
     {
         const int v = inst->focus_voice;
-        if(inst->focus_count == 0 || v < 0 || v >= SC808_NUM_VOICES) return -1;
+        if(inst->focus_count == 0 || v < 0 || v >= W8_NUM_VOICES) return -1;
         return snprintf(_buf, (size_t)_len, "%u:%s", inst->focus_count, kLevelOf[v]);
     }
```

## Appendix: matched rim class unified diff

Offsets above translate these class-relative hunk coordinates to real file lines.

```diff
--- 
+++ 
@@ -1,10 +1,10 @@
-class RimClave : public VoiceBase {
+class RimLane : public VoiceBase {
 public:
     void init(const double _sr)
     {
         sr_ = _sr;
         rng_.seed(0x8081DDAu);
-        peak_.setPeakEQ(464.0, 0.44, 8.0, sr_);
+        tock_.setPeakEQ(464.0, 0.44, 8.0, sr_);
     }
 
     /* mode 0 = rim shot, 1 = claves. */
@@ -17,11 +17,11 @@
             const EnvSeg a[2] = { { 1.0f, 0.00272f, -42.0f },
                                   { 0.0f, _decay,   -42.0f } };
             env_.set(1.0f, a, 2, sr_);
-            triHz_ = _freqHz * 1.1;
+            toneHz_ = _freqHz * 1.1;
             /* detune = -22 semitones on the pulse, then the same 1.1 lift. */
-            pulHz_ = _freqHz * powf(2.0f, -22.0f / 12.0f) * 1.1;
-            tri_.reset(1.0);
-            pul_.reset(0.0);
+            pulseHz_ = _freqHz * powf(2.0f, -22.0f / 12.0f) * 1.1;
+            tone_.reset(1.0);
+            pulse_.reset(0.0);
             hp_.set(_hpfHz, sr_);
             lp_.set(_lpfHz, sr_);
         }
@@ -30,12 +30,12 @@
             const EnvSeg a[2] = { { 1.0f, 0.0f,   -20.0f },
                                   { 0.0f, _decay, -20.0f } };
             env_.set(1.0f, a, 2, sr_);
-            /* Claves sit far above the rim shot: w8 puts them at note 99
-             * against the rim's 92, so the lane's Tune pot moves both and the
-             * offset is applied here. */
-            clvHz_ = _freqHz * powf(2.0f, 7.0f / 12.0f);
-            sin_.set(clvHz_, sr_);
-            sin_.reset(kPi * 0.5);
+            /* Claves sit far above the rim shot (note 99 against the rim's
+             * 92), so the lane's Tune pot moves both and the offset is
+             * applied here. */
+            claveHz_ = _freqHz * powf(2.0f, 7.0f / 12.0f);
+            clave_.set(claveHz_, sr_);
+            clave_.reset(kPi * 0.5);
         }
         arm();
     }
@@ -48,15 +48,15 @@
 
         if(mode_ == 0)
         {
-            const float tri1  = tri_.process(triHz_, sr_) * env;
-            const float tri2  = pul_.process(pulHz_, 0.8, sr_) * env;
+            const float tone  = tone_.process(toneHz_, sr_) * env;
+            const float body  = pulse_.process(pulseHz_, 0.8, sr_) * env;
             const float punch = rng_.white() * env * 0.46f;
-            sig = peak_.process(tri1 + tri2 + punch);
+            sig = tock_.process(tone + body + punch);
             sig = lp_.process(hp_.process(sig));
         }
         else
         {
-            sig = sin_.process() * env;
+            sig = clave_.process() * env;
         }
 
         if(env_.done()) gate(sig);
@@ -66,12 +66,12 @@
 private:
     double sr_ = 44100.0;
     int    mode_ = 0;
-    double triHz_ = 1827.0, pulHz_ = 512.0, clvHz_ = 2489.0;
+    double toneHz_ = 1827.0, pulseHz_ = 512.0, claveHz_ = 2489.0;
     Env    env_;
-    TriOsc  tri_;
-    PulseOsc pul_;
-    SineOsc sin_;
-    Biquad peak_;
+    TriOsc  tone_;
+    PulseOsc pulse_;
+    SineOsc clave_;
+    Biquad tock_;
     HPF    hp_;
     LPF    lp_;
     NoiseGen   rng_;
```

## Appendix: replacement DSP primitive dependency diff

This supplementary diff also normalizes uppercase SC808→W8 and sc_ugens.h→w8_dsp.h in addition to the requested names; it intentionally leaves SC_ENV/MAX and namespace declarations visible.

```diff
--- 
+++ 
@@ -1,41 +1,24 @@
-/*
- * w8_dsp.h — the SuperCollider UGens w8 uses, as C++.
- *
- * 8W8's Engine A is a transcription of Sonic Pi's w8.scd. A transcription
- * is only worth anything if it is EXACT, so nothing in this file is written
- * from memory or from a textbook: every formula below was read out of
- * SuperCollider's own server plugin sources and the provenance is named at
- * each one. The intent is that rendering a voice here and rendering it in
- * scsynth produces the same samples, and test/nulltest.sh proves it.
- *
- * Where a UGen is a wavetable in SC and a function here (SineOsc), that is
- * called out. Where SC runs something at control rate and we must match its
- * blockiness to null (EnvGen.kr), we reproduce the blockiness rather than
- * "improving" it — an improvement that breaks the null test costs more than
- * it is worth.
- *
- * References, all from supercollider/server/plugins @ develop:
- *   FilterUGens.cpp — LPF, HPF, BPF, the B-suite, Limiter
- *   LFUGens.cpp     — PulseOsc, TriOsc, EnvGen curve segments
- *   NoiseUGens.cpp  — WhiteNoise
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
+/*
+ * w8_dsp.h — the small DSP building blocks the 8W8 voices share: noise,
+ * a segmented envelope, three filters, a biquad and three oscillators.
  *
  * GPL-3.0.
  */
-#ifndef SC_UGENS_H
-#define SC_UGENS_H
+#ifndef W8_DSP_H
+#define W8_DSP_H
 
 #include <math.h>
 #include <stdint.h>
 
-namespace sc {
+namespace dsp {
 
 static const double kPi    = 3.14159265358979323846;
 static const double kTwoPi = 6.28318530717958647693;
 static const double kSqrt2 = 1.41421356237309504880;
 
-/* SC's default control block. EnvGen.kr ticks once per block and the binary
- * op that multiplies it into an audio signal ramps across the block, so this
- * number is audible in the output and has to match scsynth's -z. */
+/* Envelopes step once per control block and are ramped across it, so the
+ * block size is audible: a zero-length segment still lasts one block. */
 static const int kBlockSize = 64;
 
 inline float midicps(const float _note)
@@ -51,16 +34,14 @@
 /* ---- noise ------------------------------------------------------------ */
 
 /*
- * SC's taus88 combined Tausworthe generator, and its white — uniform on
- * [-1, 1). Reproduced exactly rather than substituting an LCG so that a
- * seed-matched null test of the noise voices is possible at all; with a
- * different generator only a spectral comparison is available.
+ * Combined Tausworthe generator (taus88), and white() — uniform on [-1, 1).
+ * Seeded per voice, so a voice's noise is the same every time it is built.
  */
 class NoiseGen {
 public:
     void seed(uint32_t s)
     {
-        /* SC requires s1 > 1, s2 > 7, s3 > 15. */
+        /* Each component needs a minimum seed: s1 > 1, s2 > 7, s3 > 15. */
         s1_ = 1243598713u ^ s; if(s1_ <  2u) s1_ = 1243598713u;
         s2_ = 3093459404u ^ s; if(s2_ <  8u) s2_ = 3093459404u;
         s3_ = 1821928721u ^ s; if(s3_ < 16u) s3_ = 1821928721u;
@@ -74,7 +55,7 @@
         return s1_ ^ s2_ ^ s3_;
     }
 
-    /* white: bit-stuff the mantissa of a float in [2,4), then subtract 3. */
+    /* Put 23 random bits in the mantissa of a float in [2,4), subtract 3. */
     float white()
     {
         union { uint32_t u; float f; } u;
@@ -86,44 +67,14 @@
     uint32_t s1_ = 1243598713u, s2_ = 3093459404u, s3_ = 1821928721u;
 };
 
-/* ---- SC's per-note construction step ----------------------------------
- *
- * SC builds a new synth for every note, and every UGen Ctor finishes by
- * calling its own next(unit, 1): one sample runs through the entire graph
- * with all inputs at their construction-time values before the first real
- * block. Filters see their first input sample twice; oscillators take one
- * extra phase step; a swept oscillator takes that step at the envelope's
- * initial value.
- *
- * 8W8's voices are persistent — constructed once, retriggered thereafter —
- * so reproducing this in the engine would be a BUG, not fidelity: it would
- * repeat a sample and jump a phase on every hit. It exists only so the null
- * test compares like with like, and it is compiled out of the module: the
- * flag and its branch live behind W8_NULLTEST, which only the test tools
- * define.
- *
- * The harness raises the flag, renders exactly one sample, lowers it, and
- * discards that sample.
- */
-#ifdef W8_NULLTEST
-extern bool g_scPriming;
-#endif
-
 /* ---- envelopes -------------------------------------------------------- */
 
 /*
- * One segment of an SC Env with a "curve" shape.
- *
- * From LFUGens.cpp, Env_next / shape_Curve:
- *     a1   = (endLevel - level) / (1 - exp(curve))
- *     a2   = level + a1
- *     b1   = a1
- *     grow = exp(curve / durationInSamples)
- *     per step:  b1 *= grow;  out = a2 - b1
- *
- * which is level + (endLevel-level)(1 - e^{curve·t})/(1 - e^{curve}), t∈[0,1].
- * |curve| < 0.001 degenerates to linear, exactly as SC does — w8 never
- * relies on that, but a pot sweeping a curve through zero would.
+ * One envelope segment: a target level, a time and a curve.
+ *
+ * The curve shape is
+ *     level + (end - level) * (1 - e^{curve·t}) / (1 - e^{curve}),  t in [0,1]
+ * computed incrementally (a1, a2, b1, grow below). |curve| < 0.001 is linear.
  */
 struct EnvSeg {
     float endLevel;
@@ -131,36 +82,25 @@
     float curve;
 };
 
-#define SC_ENV_MAX_SEGS 4
-
-/*
- * EnvGen.kr, and it really is .kr — every envelope in w8 is.
- *
- * That is not a detail to smooth away. An EnvGen at control rate steps once
- * per 64-sample block, so its "sample rate" is 689 Hz at 44.1k, and a segment
- * of ZERO duration still occupies one whole control period — 1.45 ms. w8
- * leans on this: every voice's amplitude envelope is
- *
- *     Env.new([click, 1, 0], [0, decay], curve)
- *
- * whose first segment has duration 0. Run that at audio rate and `click`
- * lasts one sample and does nothing; run it at control rate, as SC does, and
- * `click` is a 1.45 ms pre-level at the front of the note — an audible part
- * of the attack of every drum in the kit. Getting this wrong makes the whole
- * kit sound soft and makes the `click` parameter look vestigial.
- *
- * The value is then linearly ramped across the block by whatever consumes it:
- * a BinaryOpUGen multiplying an audio signal, or SineOsc's frequency input.
- * Both use CALCSLOPE, so one interpolation rule covers both.
+#define W8_ENV_MAX_SEGS 4
+
+/*
+ * A segmented envelope that runs at control rate — one step per 64-sample
+ * block — and is ramped linearly across each block.
+ *
+ * The control rate matters. A segment of zero duration still lasts one whole
+ * control period (1.45 ms at 44.1k), so an envelope that starts at a `click`
+ * level and jumps to 1 in "zero" time holds that click level for 1.45 ms. That
+ * pre-level is an audible part of the attack; at audio rate it would vanish.
  */
 class Env {
 public:
-    /* levels[0] is the start level; segs[i] carries the target and time. */
+    /* _startLevel is the level at t=0; segs[i] carries each target and time. */
     void set(const float _startLevel, const EnvSeg *_segs, const int _count,
              const double _sampleRate)
     {
         start_ = _startLevel;
-        count_ = _count > SC_ENV_MAX_SEGS ? SC_ENV_MAX_SEGS : _count;
+        count_ = _count > W8_ENV_MAX_SEGS ? W8_ENV_MAX_SEGS : _count;
         ctrlRate_ = _sampleRate / (double)kBlockSize;
         for(int i = 0; i < count_; ++i) seg_[i] = _segs[i];
         rewind();
@@ -179,104 +119,27 @@
 
     bool done() const { return done_; }
 
-    /* One audio-rate sample: step the control-rate generator on block
-     * boundaries, ramp linearly in between. */
+    /* One audio-rate sample: step on block boundaries, ramp in between. */
     float next()
     {
-#ifdef W8_NULLTEST
-        /* During the construction sample an EnvGen reports its initial level
-         * and does not step: EnvGen_Ctor's next(unit,1) runs with
-         * m_stage == ENVGEN_NOT_STARTED. */
-        if(g_scPriming) return start_;
-#endif
         if(phase_ >= kBlockSize)
         {
             phase_ = 0;
             prev_  = cur_;
             cur_   = stepControl();
         }
-        /* n/64 for n = 0..63, so the FIRST sample of a block is exactly the
-         * previous control value and the block never reaches `cur` — that is
-         * what CALCSLOPE does (`out = a * b; b += slope;`, b starting at the
-         * old value). Taking t after the increment instead puts the whole
-         * envelope one sample early, which is worth ~35 dB of null. */
+        /* t = n/64 for n = 0..63: the first sample of a block is exactly the
+         * previous control value, and the ramp never quite reaches `cur`. */
         const float t = (float)phase_ / (float)kBlockSize;
         ++phase_;
         return prev_ + (cur_ - prev_) * t;
     }
 
+private:
     /*
-     * The block's control value, HELD constant for all 64 samples.
-     *
-     * This is what a UGen with a control-rate FREQUENCY input sees, and it is
-     * not the same as next(). From Osc_ikk_perform in OscUGens.cpp:
-     *
-     *     int32 freq = (int32)(unit->m_cpstoinc * freqin);
-     *     int32 phaseinc = freq + (int32)(CALCSLOPE(phasein, ...) * ...);
-     *     LOOP1(n, ZXP(out) = lookupi1(...phase...); phase += phaseinc;);
-     *
-     * `freqin` is read ONCE per block and the increment is constant across
-     * it. Only the PHASE input is ramped. So an envelope driving a frequency
-     * is a staircase, while the same envelope multiplying an audio signal is
-     * a ramp — the two consumers see genuinely different signals.
-     *
-     * Interpolating the frequency instead is smoother and wrong: over a fast
-     * pitch sweep the difference integrates into a permanent phase error,
-     * which is what left the bass drum sitting at a flat -19 dB residual for
-     * its whole tail while its envelope and filters were already exact.
-     */
-    float nextHeld()
-    {
-#ifdef W8_NULLTEST
-        if(g_scPriming) return start_;
-#endif
-        if(phase_ >= kBlockSize)
-        {
-            phase_ = 0;
-            prev_  = cur_;
-            cur_   = stepControl();
-        }
-        ++phase_;
-        return cur_;
-    }
-
-    /* The raw control-rate level, for analysis. */
-    float peek() const { return (float)level_; }
-
-private:
-    /*
-     * EnvGen_initSegment, LFUGens.cpp @ Version-3.11.2:
-     *
-     *     counter = (int32)(dur * SAMPLERATE);
-     *     counter = sc_max(1, counter);
-     *     if (counter == 1) unit->m_shape = shape_Linear;
-     *     ...
-     *     unit->m_grow = exp(curve / counter);
-     *
-     * Three things, and each one was worth measurable dB:
-     *
-     *   - the segment runs an INTEGER number of control periods, TRUNCATED.
-     *     Treating the duration as fractional and comparing position against
-     *     it holds every segment one period too long — an envelope sitting at
-     *     full level for an extra 1.45 ms, and 30 dB of null.
-     *   - `counter` — the integer — is also what the curve rate is computed
-     *     over. Using the fractional duration instead put the bass drum's
-     *     pitch envelope at 334.26 Hz where SC has 332.80, which sounds like
-     *     nothing and drifts into a total phase decorrelation over a second.
-     *   - a one-period segment is forced to Linear regardless of curve. This
-     *     is what actually renders w8's zero-duration `click` segments:
-     *     one linear control period straight to the node.
-     *
-     * SAMPLERATE here is the CONTROL rate, because these are all EnvGen.kr.
-     *
-     * VERSION NOTE. SuperCollider's development branch has since reworked
-     * this: it keeps a fractional duration for the curve, floors separately
-     * for the step count, and carries the remainder into the next segment so
-     * a chain of segments does not drift late. That is a better envelope and
-     * it is NOT what is implemented here, because the reference renders come
-     * from 3.11.2 and a transcription that cannot be verified is worth less
-     * than one that can. If the reference is ever moved to a newer SC, this
-     * is the function to change and test/nulltest.sh will say so loudly.
+     * A segment runs a whole number of control periods, truncated, and at
+     * least one. The curve rate is computed over that same integer count,
+     * and a one-period segment is always linear.
      */
     void armSegment()
     {
@@ -312,16 +175,15 @@
 
         if(--counter_ <= 0)
         {
-            /* No snap to the node level: SC only does that for shape_Hold,
-             * and carrying the small shortfall into the next segment is part
-             * of what we are reproducing. */
+            /* No snap to the target: the small shortfall carries into the
+             * next segment. */
             ++segIndex_;
             armSegment();
         }
         return (float)level_;
     }
 
-    EnvSeg seg_[SC_ENV_MAX_SEGS] = {};
+    EnvSeg seg_[W8_ENV_MAX_SEGS] = {};
     int    count_ = 0;
     int    segIndex_ = 0;
     int    counter_ = 1;
@@ -334,23 +196,14 @@
     int    phase_ = kBlockSize;
 };
 
-/* Env.perc(attack, release, level, curve) — levels [0, level, 0], the same
- * curve on both segments. SC's default curve is -4; w8 always names one. */
-inline void envPerc(Env &_e, const float _attack, const float _release,
-                    const float _level, const float _curve, const double _sr)
-{
-    const EnvSeg s[2] = { { _level, _attack, _curve }, { 0.0f, _release, _curve } };
-    _e.set(0.0f, s, 2, _sr);
-}
-
-/* ---- SC's own filters -------------------------------------------------
- *
- * LPF/HPF/BPF are 2nd order and share a Direct Form II core:
+/* ---- filters ----------------------------------------------------------
+ *
+ * LPF and HPF are 2nd-order Butterworth (bilinear transform) sharing a
+ * Direct Form II core:
  *     y0  = in + b1·y1 + b2·y2
- *     out = a0·(numerator over y0,y1,y2)
- * The numerator is what distinguishes them, so it is spelled out per class
- * rather than hidden behind a generic biquad — the sign of the "2·y1" term
- * is the whole difference between a lowpass and a highpass here.
+ *     out = a0·(numerator over y0, y1, y2)
+ * Only the numerator differs — the sign of the "2·y1" term is the whole
+ * difference between the lowpass and the highpass.
  */
 
 class LPF {
@@ -401,49 +254,16 @@
     double y1_ = 0.0, y2_ = 0.0, a0_ = 0.0, b1_ = 0.0, b2_ = 0.0;
 };
 
-/* BPF's second argument is rq (reciprocal of Q), NOT bandwidth in Hz.
- * w8 passes 1 or leaves it at the default 1. */
-class BPF {
-public:
-    void set(const double _freq, const double _rq, const double _sr)
-    {
-        const double pfreq = _freq * (kTwoPi / _sr);
-        const double pbw   = _rq * pfreq * 0.5;
-        const double C = 1.0 / tan(pbw);
-        const double D = 2.0 * cos(pfreq);
-        a0_ = 1.0 / (1.0 + C);
-        b1_ = C * D * a0_;
-        b2_ = (1.0 - C) * a0_;
-    }
+/*
+ * RBJ-cookbook biquad with a highpass and a peaking-EQ setter. b1 and b2 are
+ * stored already negated, so the recursion adds them. Gains in dB convert to
+ * amplitude as 10^(db/40).
+ */
+class Biquad {
+public:
     void reset() { y1_ = y2_ = 0.0; }
-    float process(const float _in)
-    {
-        const double y0 = (double)_in + b1_ * y1_ + b2_ * y2_;
-        const double out = a0_ * (y0 - y2_);
-        y2_ = y1_; y1_ = y0;
-        return (float)out;
-    }
-private:
-    double y1_ = 0.0, y2_ = 0.0, a0_ = 0.0, b1_ = 0.0, b2_ = 0.0;
-};
-
-/* ---- the "B" suite: RBJ cookbook biquads ------------------------------
- *
- * BPeakEQ, BLowShelf, BHiShelf, BHiPass, BBandPass all share one difference
- * equation (BPerformFilterLoop in FilterUGens.cpp) and differ only in how the
- * five coefficients are computed, so here they are one class with named
- * setters. Note SC's sign convention: b1 and b2 are stored ALREADY NEGATED
- * relative to the textbook, so the recursion adds them.
- *
- * The dB-to-linear conversion is pow(10, db/40) — SC writes it as
- * pow(10., db * 0.025), which is the same thing and is why an "amplitude"
- * of a shelf is the square root of its power gain.
- */
-class Biquad {
-public:
-    void reset() { y1_ = y2_ = 0.0; }
-
-    /* BHiPass(freq, rq) */
+
+    /* Highpass. _rq is the RECIPROCAL of Q: Butterworth is sqrt(2). */
     void setHiPass(const double _freq, const double _rq, const double _sr)
     {
         const double w0 = kTwoPi * _freq / _sr;
@@ -458,22 +278,7 @@
         b2_ = (1.0 - alpha) * -rz;
     }
 
-    /* BBandPass(freq, bw) — bw in OCTAVES, unlike BPF's rq.
-     * 0.34657359027997 is ln(2)/2, from the cookbook's bandwidth form. */
-    void setBandPass(const double _freq, const double _bw, const double _sr)
-    {
-        const double w0 = kTwoPi * _freq / _sr;
-        const double sinw0 = sin(w0);
-        const double alpha = sinw0 * sinh((0.34657359027997 * _bw * w0) / sinw0);
-        const double rz = 1.0 / (1.0 + alpha);
-        a0_ = alpha * rz;
-        a1_ = 0.0;
-        a2_ = -a0_;
-        b1_ = cos(w0) * 2.0 * rz;
-        b2_ = (1.0 - alpha) * -rz;
-    }
-
-    /* BPeakEQ(freq, rq, db) */
+    /* Peaking EQ. _rq is the reciprocal of Q, _db the boost or cut. */
     void setPeakEQ(const double _freq, const double _rq, const double _db,
                    const double _sr)
     {
@@ -488,44 +293,6 @@
         b2_ = (1.0 - (alpha / a)) * -rz;
     }
 
-    /* BLowShelf(freq, rs, db) */
-    void setLowShelf(const double _freq, const double _rs, const double _db,
-                     const double _sr)
-    {
-        const double a = pow(10.0, _db * 0.025);
-        const double w0 = kTwoPi * _freq / _sr;
-        const double cosw0 = cos(w0);
-        const double alpha = sin(w0) * 0.5 * sqrt((a + (1.0 / a)) * (_rs - 1.0) + 2.0);
-        const double i = (a + 1.0) * cosw0;
-        const double j = (a - 1.0) * cosw0;
-        const double k = 2.0 * sqrt(a) * alpha;
-        const double rz = 1.0 / ((a + 1.0) + j + k);
-        a0_ = a * ((a + 1.0) - j + k) * rz;
-        a1_ = 2.0 * a * ((a - 1.0) - i) * rz;
-        a2_ = a * ((a + 1.0) - j - k) * rz;
-        b1_ = 2.0 * ((a - 1.0) + i) * rz;
-        b2_ = ((a + 1.0) + j - k) * -rz;
-    }
-
-    /* BHiShelf(freq, rs, db) */
-    void setHiShelf(const double _freq, const double _rs, const double _db,
-                    const double _sr)
-    {
-        const double a = pow(10.0, _db * 0.025);
-        const double w0 = kTwoPi * _freq / _sr;
-        const double cosw0 = cos(w0);
-        const double alpha = sin(w0) * 0.5 * sqrt((a + (1.0 / a)) * (_rs - 1.0) + 2.0);
-        const double i = (a + 1.0) * cosw0;
-        const double j = (a - 1.0) * cosw0;
-        const double k = 2.0 * sqrt(a) * alpha;
-        const double rz = 1.0 / ((a + 1.0) - j + k);
-        a0_ = a * ((a + 1.0) + j + k) * rz;
-        a1_ = -2.0 * a * ((a - 1.0) + i) * rz;
-        a2_ = a * ((a + 1.0) + j - k) * rz;
-        b1_ = -2.0 * ((a - 1.0) - i) * rz;
-        b2_ = ((a + 1.0) - j - k) * -rz;
-    }
-
     float process(const float _in)
     {
         const double y0 = (double)_in + b1_ * y1_ + b2_ * y2_;
@@ -539,58 +306,12 @@
     double a0_ = 1.0, a1_ = 0.0, a2_ = 0.0, b1_ = 0.0, b2_ = 0.0;
 };
 
-/*
- * BHiPass4 is two BHiPass in series — but NOT with the rq you asked for.
- * SCClassLibrary/Common/Audio/BEQSuite.sc:
- *
- *     BHiPass4 { *ar { arg in, freq = 1200.0, rq = 1.0, mul = 1.0, add = 0.0;
- *         rq = sqrt(rq);
- *         coefs = BHiPass.sc(nil, freq, rq);
- *         ^SOS.ar(SOS.ar(in, *coefs), *coefs ++ [mul, add]); } }
- *
- * The sqrt is the whole point: cascading two sections each of rq gives an
- * overall rq of rq^2, so each stage takes the square root to land on the rq
- * that was requested. Passing rq straight through to both stages costs about
- * 2 dB of resonance and was worth 20 dB of null on the open hat.
- */
-class HiPass4 {
-public:
-    void set(const double _freq, const double _rq, const double _sr)
-    {
-        const double rq = sqrt(_rq);
-        a_.setHiPass(_freq, rq, _sr);
-        b_.setHiPass(_freq, rq, _sr);
-    }
-    void reset() { a_.reset(); b_.reset(); }
-    float process(const float _in) { return b_.process(a_.process(_in)); }
-private:
-    Biquad a_, b_;
-};
-
 /* ---- oscillators ------------------------------------------------------
  *
- * All three emit the current sample and THEN advance, which is what SC's
- * per-sample loops do (`ZXP(out) = ...; phase += phaseinc;`).
- *
- * A rendered scsynth file looks at first like it disagrees: the first sample
- * of SineOsc.ar(440, pi/2) is cos(w) = 0.99804, not sin(pi/2) = 1.0. That is
- * not the loop — it is SC constructing a fresh synth per note and running one
- * extra sample through the whole graph at construction. Every UGen Ctor ends
- * by calling its own next(unit, 1). See SC_CTOR_PRIME below.
- *
- * The distinction is invisible at a constant frequency and very visible at a
- * swept one, because the construction step uses the envelope's INITIAL value
- * (470 Hz on the bass drum) while the first real block uses the value after
- * one control step (334 Hz).
- */
-
-/*
- * SineOsc. SC reads an 8192-point sine wavetable with linear interpolation;
- * this is sinf(). The difference is the table's interpolation error, about
- * -100 dBFS at this table size — below anything the null test cares about,
- * and below the 16-bit floor the Move outputs at. Named here so nobody has
- * to rediscover it when the null lands at -95 dB instead of -140.
- */
+ * All three emit the current sample and THEN advance the phase.
+ */
+
+/* Sine. */
 class SineOsc {
 public:
     void set(const double _freq, const double _sr) { inc_ = kTwoPi * _freq / _sr; }
@@ -602,7 +323,7 @@
         if(phase_ > kTwoPi) phase_ -= kTwoPi;
         return v;
     }
-    /* Frequency-modulated form, for the voices whose pitch is an envelope. */
+    /* Frequency-modulated form, for voices whose pitch is an envelope. */
     float process(const double _freq, const double _sr)
     {
         inc_ = kTwoPi * _freq / _sr;
@@ -613,12 +334,8 @@
 };
 
 /*
- * TriOsc. From LFUGens.cpp:
- *     freqMul = 4 / sampleRate;  phase starts wrapped into [0, 4)
- *     z = phase > 1 ? 2 - phase : phase;  phase += freq;
- *     if (phase >= 3) phase -= 4;
- * Output is bipolar -1..1 and NOT band-limited — the corners alias, in SC
- * too. Reproduced, not fixed.
+ * Triangle, bipolar -1..1, phase in [0, 4). Not band-limited: the corners
+ * alias.
  */
 class TriOsc {
 public:
@@ -639,19 +356,10 @@
 };
 
 /*
- * PulseOsc. From LFUGens.cpp:
- *     if (phase >= 1) { phase -= 1; z = duty <= 0.5 ? 1 : 0; }
- *     else            { z = phase < duty ? 1 : 0; }
- *     phase += freq/sampleRate;
- *
- * Two things that matter and are easy to get wrong:
- *   1. the output is UNIPOLAR, 0..1. w8's hi-hats sum six of these, so the
- *      sum carries a large DC term that the filter chain removes. Making it
- *      bipolar "because a square wave is bipolar" changes every hat.
- *   2. the wrap branch forces one sample of the opposite polarity, which is
- *      what stops a duty of exactly 0 or 1 from producing silence.
- * Naive and aliasing, as in SC. On the 808's real hats the oscillators are
- * Schmitt-trigger squares whose harmonics run past Nyquist anyway.
+ * Pulse, UNIPOLAR 0..1 — callers that sum several remove the DC themselves.
+ * The wrap sample forces the opposite polarity, so a duty of exactly 0 or 1
+ * still makes a tick rather than silence. Naive, like the Schmitt-trigger
+ * squares it stands in for.
  */
 class PulseOsc {
 public:
@@ -668,84 +376,6 @@
     double phase_ = 0.0;
 };
 
-/* ---- Limiter ----------------------------------------------------------
- *
- * SC's Limiter is a lookahead limiter over three rotating buffers of
- * `dur` seconds each (default 0.01 s). Two consequences that a naive
- * "just clip it" port would silently lose, and that w8's bass drum
- * depends on:
- *
- *   - it delays the signal by TWO buffers, ~20 ms at the default. The
- *     w8 kick therefore arrives ~20 ms late relative to every other
- *     voice. That is a property of w8, not a bug in this port; the
- *     engine compensates for it explicitly and says so where it does.
- *   - it emits silence until two buffers have filled, so the very start
- *     of the first note is gone.
- *
- * Gain is recomputed once per buffer from max(prev, current) peak and
- * ramped across the next buffer, so it never modulates within a buffer.
- */
-#define SC_LIMITER_MAX 2048
-
-class Limiter {
-public:
-    void set(const float _dur, const double _sr)
-    {
-        long n = (long)ceil((double)_dur * _sr);
-        if(n < 1) n = 1;
-        if(n > SC_LIMITER_MAX) n = SC_LIMITER_MAX;
-        bufsize_ = n;
-        slopeFactor_ = 1.0f / (float)bufsize_;
-        reset();
-    }
-
-    void reset()
-    {
-        for(int i = 0; i < 3; ++i)
-            for(long j = 0; j < bufsize_; ++j) buf_[i][j] = 0.0f;
-        in_ = 0; mid_ = 1; out_ = 2;
-        pos_ = 0; flips_ = 0;
-        slope_ = 0.0f; level_ = 1.0f;
-        prevMax_ = curMax_ = 0.0f;
-    }
-
-    /* Latency in samples, for callers that want to line the kick up with the
-     * rest of the kit rather than inherit SC's 20 ms. */
-    long latency() const { return bufsize_ * 2; }
-
-    float process(const float _in, const float _amp)
-    {
-        buf_[in_][pos_] = _in;
-        const float y = flips_ >= 2 ? level_ * buf_[out_][pos_] : 0.0f;
-        level_ += slope_;
-
-        const float a = fabsf(_in);
-        if(a > curMax_) curMax_ = a;
-
-        if(++pos_ >= bufsize_)
-        {
-            pos_ = 0;
-            const float max2 = prevMax_ > curMax_ ? prevMax_ : curMax_;
-            prevMax_ = curMax_;
-            curMax_  = 0.0f;
-
-            const float next = max2 > _amp ? _amp / max2 : 1.0f;
-            slope_ = (next - level_) * slopeFactor_;
-
-            const int t = out_; out_ = mid_; mid_ = in_; in_ = t;
-            ++flips_;
-        }
-        return y;
-    }
-
-private:
-    float buf_[3][SC_LIMITER_MAX] = {};
-    long  bufsize_ = 441, pos_ = 0;
-    int   in_ = 0, mid_ = 1, out_ = 2, flips_ = 0;
-    float slope_ = 0.0f, level_ = 1.0f, slopeFactor_ = 1.0f / 441.0f;
-    float prevMax_ = 0.0f, curMax_ = 0.0f;
-};
-
-} /* namespace sc */
-
-#endif /* SC_UGENS_H */
+} /* namespace dsp */
+
+#endif /* W8_DSP_H */
```

---
### out-helpers.md

# Shared helper hunk review

## Upstream file -> our file mapping

All our paths below are relative to `/workspace/opt-909/repo/firmware/src/dsp/`. `NONE` means there is no equivalent helper/API in the inspected port, not that the complete firmware lacks every related feature.

| Upstream file / helper | Our file and line | Mapping |
|---|---|---|
| 8W8 `sc_ugens.h` -> `w8_dsp.h`: NoiseGen | drum808.c:209,216; drum808.h:50 | rng_seed/rng_frand2, same taus88 and mantissa construction |
| Env / EnvSeg | drum808.c:346,369,386,719; drum808.h:52 | env_arm/env_step/env_next; inline rim trigger setup; two float segments rather than four double segments |
| LPF / HPF | drum808.c:284,275,266; drum808.h:47 | sc_lpf_set/sc_hpf_set + bq_run, float delta-form transposed DF-II |
| Biquad highpass / peak | drum808.c:294,307,266,1628 | sk_hp_set (q=1/rq, gain parameter), sc_peak_set (linear A supplied), bq_run |
| SineOsc | NONE; related fastmath.h:181,198 | No generic SineOsc object in drum808.c; math evaluators alone do not reproduce its phase accumulator |
| TriOsc / PulseOsc | drum808.c:734,733,748,752; drum808.h:101 | Rim triangle float phase; pulse uint32 phase plus explicit delayed wrap flag |
| Removed BPF / octave-bandpass / shelves / HiPass4 / Limiter / envPerc / nextHeld / peek / NULLTEST | NONE | No corresponding named APIs in drum808.c; its circuit bandpasses, metal filters and master processing are different algorithms |
| 9W9 `webaudio.h`: AudioParam | drum909_dsp.h:79,91,102,126,140 | d9_env_t/set/exp/tick/anchor: exponential and constant only |
| webaudio.h: biquad | drum909_dsp.h:151,158,183,185 | d9_biquad_t/set/reset/tick; LP/HP/BP only |
| webaudio.h: triangle / noise | drum909_dsp.h:196,199,321; drum909.c:997 | uint32 triangle + xorshift noise/span |
| webaudio.h: WaveShaper / saw / linear ramp / target ramp | NONE | d9_shape is circuit distortion, not wa_shaper; no generic saw/linear/target equivalents |
| `er99_circuit.h` -> `9w9_circuit.h` | drum909_dsp.h:208,234,311; drum909.c:14,32,404,525,708,762; drum909.h:28 | d9_tanh/shape and BD/SD state, trigger, split block renderers |
| `er99_perc909.h` -> `9w9_perc909.h` | drum909.c:298,304,472,483,621,637,859,891; drum909.h:56,66 | rim/clap retune, init, trigger, block render |
| `er99_tom909.h` -> `9w9_tom909.h` | drum909.c:462,562,803; drum909.h:46 | three partials, own envelopes and stick path |

## Scope and provenance

Reviewed 8W8 base `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` -> v1.5.1 `1dbef27e13a6c63cb1fc078dd9b4d86172237c51`; 9W9 v2.7.0 `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` -> rename `8e9aebde0c40a4c017f640e5c30a4f70936bb595` -> v2.7.1 `25e03ddc33c2a4dfff702927061381a6e8bd6a3d` -> v2.7.2 `a9e030d9bad46e3e53aafce92c7104b8e7bba47f`. Unrelated histories were compared as extracted file trees, never as a common-ancestor diff. All 18 relevant extracted files matched the commit blob IDs using `git rev-parse SHA:path` and `git hash-object /tmp/cmp/...` inside the respective upstream clone.
Our inspected checkout HEAD is `263a055879f48919881f50812183aee3f4f2dc1e`, with an existing modification to `tests/host/run_drum909_emit_equivalence.sh`. That modification was left untouched. Our line references describe the on-disk source inspected, not an assumed checkout of the proposed scratch base `a18561f17977be8592cef5a3d3cbebf70e4dbf1b`. No patch qualified, so no scratch checkout was made.
Diff method: three context lines (`diff -u` equivalent, Python unified_diff), normalizing `er99->9w9`, `sc808->w8`, `sc::->dsp::`, `RGen->NoiseGen`, `frand2->white`, `trand->next32`, `SinOsc->SineOsc`, `LFTri->TriOsc`, `LFPulse->PulseOsc`. The namespace declaration change `namespace sc` -> `namespace dsp` remains explicit. Case-sensitive residual `ER99_*->W9_*`, `er99_*->w9_*` and `SC_*->W8_*` appear explicitly in the rows. Normalized text is comparison material, not compilable source (e.g. an intermediate identifier can begin `9w9`).
Every row corresponds to exactly one unified-diff hunk; its upstream location is the new-side hunk start, including context. Removed functions use the insertion boundary on the new side and the old range in Notes. `Applies Y` in the later body-comparison table means a shared algorithm exists, not permission or a recommendation to replace it. In the change-hunk tables all changes have Applies N because no new runtime improvement needs porting. `ALREADY` refers only to the stated property, never whole-port PCM identity.

## 8W8 base -> v1.5.1: all 16 hunks

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| w8_dsp.h:1 (H1, +1,24) | drum808.c:1,344,386 | cosmetic | N | differs: C port provenance; ALREADY: 64-sample control cadence | Old -1,41. Header prose/copyright/guard and namespace sc->dsp rename; block commentary shortened. kBlockSize remains 64; no arithmetic change. |
| w8_dsp.h:34 (H2, +34,14) | drum808.c:209 | cosmetic | N | ALREADY: all three XOR seeds and minimum checks | Old -51,16. Noise description and seed comment only. Constants 1243598713/3093459404/1821928721 unchanged. |
| w8_dsp.h:55 (H3, +55,7) | drum808.c:216 | cosmetic | N | ALREADY: bits 0x40000000 OR (taus88 >> 9), minus 3 | Old -74,7. Only white() description changes. No replacement random formula. |
| w8_dsp.h:67 (H4, +67,14) | drum808.c:346; NONE for NULLTEST | structure | N | ALREADY: no g_scPriming branch | Old -86,44. Deletes old 89-111 test-only declaration/prose; rewrites envelope comment. Production had the flag compiled out already; no firmware branch removal speedup. |
| w8_dsp.h:82 (H5, +82,25) | drum808.h:52; drum808.c:719 | structure | N | differs: two-segment port, same start/rewind role | Old -131,36. SC_ENV_MAX_SEGS->W8_ENV_MAX_SEGS stays 4 upstream. Count clamp, copy, control-rate division unchanged. Our two slots suit rim caller, not generic four-segment API. |
| w8_dsp.h:119 (H6, +119,27) | drum808.c:346,369,386; NONE for nextHeld/peek | structure | N | ALREADY: ramped n/64; differs: float state | Old -179,104. Deletes test-only early return plus nextHeld and peek. Removes held-frequency API, not a faster implementation of next(). armSegment executable body untouched; truncation, >=1 count and one-period linear rule remain. |
| w8_dsp.h:175 (H7, +175,15) | drum808.c:369; drum808.h:52 | cosmetic | N | ALREADY: no snap to target; differs: float state | Old -312,16. Comment and segment-array macro name only, still four upstream slots; decrement/advance/arm ordering unchanged. |
| w8_dsp.h:196 (H8, +196,14) | NONE for envPerc; drum808.c:284,275 for filters | structure | N | ALREADY: no envPerc wrapper; differs: filter realization | Old -334,23. Removes wrapper that sets two segments, and shortens filter prose. No runtime speedup for a port that already sets segments directly. |
| w8_dsp.h:254 (H9, +254,16) | NONE for removed BPF; drum808.c:294,266 | structure | N | differs: circuit BP is not SC BPF; highpass has C implementation | Old -401,49. Deletes old BPF 404-430 and rewrites Biquad introduction. Retained highpass equations untouched. Deleted BPF uses tan(rq*w/2), not our constant-peak RBJ bp_set; substituting one for the other changes response. |
| w8_dsp.h:278 (H10, +278,7) | NONE for setBandPass; drum808.c:307 | structure | N | ALREADY: no octave-bandwidth setter | Old -458,22. Removes old 461-477 sinh bandwidth setter. Peak-EQ comment only; alpha/rz, pow exponent and statement order unchanged. |
| w8_dsp.h:293 (H11, +293,6) | NONE for shelf setters; drum808.c:266 | structure | N | ALREADY: no SC shelf APIs | Old -488,44. Deletes old 491-529 low/high shelf setters; common Biquad process unchanged. Removing unused setters does not remove work from bq_run. |
| w8_dsp.h:306 (H12, +306,12) | NONE for HiPass4/SineOsc object; fastmath.h:181 | structure | N | ALREADY: no SC HiPass4 wrapper | Old -539,58. Deletes cascade helper old 542-570, including sqrt(rq), and cuts oscillator provenance comments. Retained SineOsc still uses sinf, double phase and same wrap comparison; no LUT introduced. |
| w8_dsp.h:323 (H13, +323,7) | NONE; related fastmath.h:181 | cosmetic | N | differs: no generic sine accumulator | Old -602,7. Only frequency-modulated overload comment. inc=kTwoPi*freq/sr and call to process unchanged. |
| w8_dsp.h:334 (H14, +334,8) | drum808.c:734,748 | cosmetic | N | differs: float rim phase and precomputed increment | Old -613,12. Triangle description only. reset fmod and process output-before-increment unchanged. New comment phase [0,4) describes reset, not steady state (wrap permits negative phase). |
| w8_dsp.h:356 (H15, +356,10) | drum808.c:733,752 | cosmetic | N | ALREADY: unipolar output and forced wrap polarity; differs: fixed point | Old -639,19. Pulse prose only; branch, comparison, increment order unchanged. |
| w8_dsp.h:376 (H16, +376,6) | NONE | structure | N | ALREADY: no SC lookahead limiter | Old -668,84. Deletes old Limiter 671-747 and renames final guard. No removal from an active signal chain shown by this header diff; port lacks this API. Adding/removing lookahead processing changes latency and PCM and cannot be a bit-identical transplant. |

## 9W9 v2.7.0 -> rename: all 35 hunks

### webaudio.h (6 hunks)

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| webaudio.h:1 (H1, +1,19) | drum909_dsp.h:1 | cosmetic | N | differs: port documentation and pragma once | Old -1,21. Comment and guard only. All automation, filter, shaper, oscillator and noise bodies retained. |
| webaudio.h:23 (H2, +23,15) | drum909_dsp.h:50,79 | cosmetic | N | ALREADY: minimum exponential endpoint 1e-9 | Old -25,15. Endpoint and automation overview comments only; no clamp removal. |
| webaudio.h:163 (H3, +163,8) | drum909_dsp.h:158 | cosmetic | N | ALREADY: linear Q convention; differs: trigonometric evaluator | Old -165,8. Filter provenance/comment wording only, coefficients unchanged upstream. |
| webaudio.h:262 (H4, +262,7) | NONE | cosmetic | N | differs: no wa_shaper equivalent | Old -264,7. Curve description only; same 256 entries, two 2x-rate biquads, lookup interpolation and clamps. |
| webaudio.h:364 (H5, +364,7) | drum909_dsp.h:321 | cosmetic | N | ALREADY: same xorshift32 shifts | Old -366,7. White-noise heading only; double-to-float power-of-two scaling retained upstream. |
| webaudio.h:383 (H6, +383,4) | drum909_dsp.h:23 | cosmetic | N | differs: pragma once | Old -385,4. Closing guard comment only. |

### 9w9_circuit.h (14 hunks)

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_circuit.h:1 (H1, +1,8) | drum909.c:708 | cosmetic | N | differs: split block-render port | Old -1,8. Opening historical comparison rewritten, no model changes. |
| 9w9_circuit.h:11 (H2, +11,19) | drum909_dsp.h:1 | cosmetic | N | differs: port header | Old -11,19. Beater/source prose and guard names only. |
| 9w9_circuit.h:93 (H3, +93,7) | drum909.h:28 | cosmetic | N | differs: d9_bt_t specialized float/integer state | Old -93,7. Type rename er99_bt_t->w9_bt_t only; upstream layout unchanged. |
| 9w9_circuit.h:102 (H4, +102,7) | drum909.c:32,33; drum909_dsp.h:208 | cosmetic | N | differs: LUT tanh and fixed inverse gains | Old -102,7. Function rename only; upstream still tanhf(k*x)/tanhf(k), k floor 0.01 unchanged. |
| 9w9_circuit.h:138 (H5, +138,23) | drum909.c:14,23 | cosmetic | N | ALREADY: 1.95 snappy and 0.0002 VCA constants | Old -138,23. Macro prefixes only, including override spelling. 1/(1-VT) unchanged; no changed threshold. |
| 9w9_circuit.h:175 (H6, +175,7) | drum909_dsp.h:234,311 | cosmetic | N | differs: prepared integer type and shape constants | Old -175,7. Shape function name only. Seven modes, branch structure and arithmetic retained. |
| 9w9_circuit.h:246 (H7, +246,32) | drum909.c:24,25,26; drum909_dsp.h:311 | cosmetic | N | ALREADY: 49 Hz, 4.6 Hz/ms, 16 ms hold; differs: prepared shape | Old -246,32. Renames shape calls, wrapper, BD macros and init. Numeric literals and default branches unchanged. |
| 9w9_circuit.h:298 (H8, +298,14) | drum909.c:404,525 | cosmetic | N | differs: no runtime sample-rate member, integer mute | Old -298,14. Init macro reference and trigger names only. pitch_mod default 1 stays. |
| 9w9_circuit.h:329 (H9, +329,10) | drum909.c:536 | cosmetic | N | ALREADY: same base/sweep/hold constants; differs: exp evaluator | Old -329,10. Only macro prefixes in bd_base/bd_df/bd_phold. expf multiplier and tau/m clamps unchanged upstream. |
| 9w9_circuit.h:370 (H10, +370,7) | drum909.c:708,762 | cosmetic | N | differs: separate per-block BD and SD loops | Old -370,7. Render function rename only; no hoisting or branch deletion upstream. |
| 9w9_circuit.h:394 (H11, +394,7) | drum909.c:18,774 | cosmetic | N | ALREADY: threshold subtraction and normalization | Old -394,7. VCA macro prefixes only. |
| 9w9_circuit.h:408 (H12, +408,7) | drum909.c:32,737 | cosmetic | N | ALREADY: diode1.6 + 0.05*o*o + DC blocker topology; differs: tanh table | Old -408,7. Diode function rename only. Evaluation order upstream unchanged. |
| 9w9_circuit.h:421 (H13, +421,13) | drum909.c:768,778 | cosmetic | N | ALREADY: independently rounded shells; differs: prepared gains/inverse mix | Old -421,13. Diode/shape names only. Ratio conditional, division by 1+mix, and body multiplication unchanged upstream. |
| 9w9_circuit.h:454 (H14, +454,15) | drum909.c:789,793 | cosmetic | N | ALREADY: gated noise, HP then LP, VCA and mix 1.95 | Old -454,15. Macro prefixes and ending guard only. No filter bypass, new constants or removed load upstream. |

### 9w9_perc909.h (10 hunks)

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_perc909.h:1 (H1, +1,5) | drum909.c:859,891 | cosmetic | N | differs: block renderers | Old -1,13. Deletes introductory historical prose only, no rim/clap fix in this transition. |
| 9w9_perc909.h:18 (H2, +18,11) | drum909.c:1 | cosmetic | N | differs: direct includes in port | Old -26,11. Guard and include filename/function-reference names only. |
| 9w9_perc909.h:42 (H3, +42,9) | drum909.h:56; drum909.c:472 | cosmetic | N | differs: integer mute and prepared shape | Old -50,9. Rim type and init function renamed; coefficient/state init unchanged upstream. |
| 9w9_perc909.h:62 (H4, +62,13) | drum909.c:298,621 | cosmetic | N | ALREADY: retune both resonances; differs: C helpers | Old -70,13. Retune and trigger identifiers only. Q multiplier 1.1 unchanged. |
| 9w9_perc909.h:78 (H5, +78,7) | drum909.c:859 | cosmetic | N | ALREADY: two-sample excitation followed by ringing | Old -86,7. Render function rename only. No continuous-noise removal in this diff: the impulse-only behavior is already in base. |
| 9w9_perc909.h:105 (H6, +105,7) | drum909.c:875 | cosmetic | N | ALREADY: 60/32 resonance gains and HP after shape; differs: ring specialization | Old -113,7. Shape call rename only. |
| 9w9_perc909.h:130 (H7, +130,9) | drum909.h:66; drum909.c:483 | cosmetic | N | differs: float next_pulse/integer mute | Old -138,9. Clap type and init function rename only. |
| 9w9_perc909.h:145 (H8, +145,12) | drum909.c:304,637 | cosmetic | N | ALREADY: clap retune and four-event trigger | Old -153,12. Retune/trigger names only. |
| 9w9_perc909.h:164 (H9, +164,7) | drum909.c:891 | cosmetic | N | ALREADY: four events with tail armed on fourth | Old -172,7. Render name only; no change to scheduling or amplitude array. |
| 9w9_perc909.h:197 (H10, +197,9) | drum909.c:920 | cosmetic | N | ALREADY: BP*env -> shape -> HP -> level*accent; differs: prepared shape | Old -205,9. Shape name and guard only. No gain/evaluation-order change upstream. |

### 9w9_tom909.h (5 hunks)

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_tom909.h:41 (H1, +41,16) | drum909.c:820,823,827 | cosmetic | N | ALREADY: 1 / 1.50 / 2.75 partial ratios | Old -41,16. Guard and ratio macro prefixes only; values unchanged. |
| 9w9_tom909.h:64 (H2, +64,9) | drum909.h:46; drum909.c:462 | cosmetic | N | differs: fixed phases and integer mute | Old -64,9. Tom type and init function rename only. Existing explicit noise/DC filter resets retained upstream. |
| 9w9_tom909.h:93 (H3, +93,7) | drum909.c:562 | cosmetic | N | ALREADY: same trigger/envelope structure | Old -93,7. Trigger and parameter type names only. |
| 9w9_tom909.h:131 (H4, +131,7) | drum909.c:803 | cosmetic | N | differs: block renderer with partial skipping | Old -131,7. Render and parameter type names only. |
| 9w9_tom909.h:142 (H5, +142,20) | drum909.c:820 | cosmetic | N | ALREADY: partial gains, stick 1.5, output 0.55; differs: LUT and skips | Old -142,20. Diode calls, ratio macros, shape call and guard names only. Upstream still computes all three partials every sample, with same order. |

## 9W9 rename -> v2.7.1

Zero hunks in each of webaudio.h, 9w9_circuit.h, 9w9_perc909.h and 9w9_tom909.h. Their raw Git blob IDs are identical across these commits; no cross-history ancestry claim is needed.

## 9W9 v2.7.1 -> v2.7.2: all 4 hunks

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| webaudio.h:1 (H1, +1,4) | NONE | cosmetic | N | differs: existing SPDX/provenance headers | Old -1,3. Adds one copyright/additional-terms NOTICE comment at line 1. No executable change; classification is about runtime effects, not legal interpretation. |
| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_circuit.h:1 (H1, +1,4) | NONE | cosmetic | N | differs: existing SPDX/provenance headers | Old -1,3. Adds one copyright/additional-terms NOTICE comment at line 1. No executable change; classification is about runtime effects, not legal interpretation. |
| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_perc909.h:1 (H1, +1,4) | NONE | cosmetic | N | differs: existing SPDX/provenance headers | Old -1,3. Adds one copyright/additional-terms NOTICE comment at line 1. No executable change; classification is about runtime effects, not legal interpretation. |
| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_tom909.h:1 (H1, +1,4) | NONE | cosmetic | N | differs: existing SPDX/provenance headers | Old -1,3. Adds one copyright/additional-terms NOTICE comment at line 1. No executable change; classification is about runtime effects, not legal interpretation. |

## Retained helper bodies: line-by-line numerical comparison

This section covers code with no change hunk, so an unchanged helper is not silently omitted. Upstream 8W8 line numbers refer to v1.5.1 w8_dsp.h; upstream 9W9 line numbers refer to v2.7.2 (one greater than rename/v2.7.1 after the new leading comment). Ranges partition each meaningful body; comments/private field declarations are included where they affect state precision. Source comparisons establish formulas, not measured speed or whole-engine identity.
Mechanical check: removing comments/whitespace and normalizing the requested identifiers gave identical retained NoiseGen, LPF, HPF, SineOsc, TriOsc and PulseOsc class text across the 8W8 revisions. Env also matched after deleting nextHeld/peek and the NULLTEST block; Biquad matched after deleting the three removed setters. Thus there is no hidden retained arithmetic rewrite in those classes.

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| w8_dsp.h:16-18 | drum808.c:28; fastmath.h:38 | risk | Y | differs: float PI/rate constants | Upstream kPi/kTwoPi/kSqrt2 are double. Ours uses rounded PI_SR/TWO_PI_SR and 1.41421356f; moving divisions into these constants changes rounding. |
| w8_dsp.h:24-27 | drum808.c:174,1662; fastmath.h:134 | risk | Y | differs: d8_midicps uses custom exp2 | Upstream 440f*powf(2f,(note-69f)/12f); ours 440f*d8_exp2_cr((note-69f)/12f), used by lane_hz. The argument expression is the same; the exponentiation implementation differs and does not prove all-input libm identity. |
| w8_dsp.h:29-32 | drum808.c:203; fastmath.h:48 | structure | Y | ALREADY: same nested ordered ternary clamp | Both use x<lo ? lo : x>hi ? hi : x; NaN passes through rather than being converted to a bound. No removed clamps. |
| w8_dsp.h:42-48 | drum808.c:209 | structure | Y | ALREADY: same seed XOR and minimum guards | uint32 arithmetic/constants match. Upstream default field initializers also initialize an unseeded object; C rng storage relies on init/seed callers. |
| w8_dsp.h:50-56 | drum808.c:219 | structure | Y | ALREADY: same masks, shifts, XOR and update order | s1 mask FFFFFFFE, shifts 12/13/19; s2 FFFFFFF8, 4/2/25; s3 FFFFFFF0, 17/3/11. Each component depends only on itself; final XOR same. No different Tausworthe recurrence. |
| w8_dsp.h:59-64 | drum808.c:216 | structure | Y | ALREADY: same mantissa construction and subtract | 0x40000000 OR (xor >> 9) yields [2,4), then minus 3. Float lattice is exactly [-1,1-2^-22], not uint32-to-float scaling. Ours combines next32/white inline, not a new upstream speedup. |
| w8_dsp.h:67 | drum808.h:50 | structure | Y | differs: no in-struct C default initialization | Same three uint32 state words; explicit rng_seed is required in C. No upstream change to default seeds. |
| w8_dsp.h:79-85 | drum808.h:52 | structure | Y | differs: 4 generic upstream segments vs 2 in our rim envelope | endLevel/durSeconds/curve are float in both; working level/grow/a2/b1 are double upstream and float here. Generic 4-segment functionality is not exposed by our type. |
| w8_dsp.h:99-107 | drum808.c:719 | risk | Y | differs: specialized trigger fills two arrays directly | Upstream clips count only above 4, computes sr/64 in double, copies segments, rewinds. Our fixed CTRL_RATE=689.0625 is exact at 44100; no variable sample rate. Negative-count behavior is not a useful correctness improvement in either API. |
| w8_dsp.h:109-120 | drum808.c:726 | structure | Y | ALREADY: segment zero, start level, arm, prev=cur=start, phase=64 | done initialization is specialized to known nonzero count. First sample forces control step but returns previous start level. |
| w8_dsp.h:123-136 | drum808.c:386 | risk | Y | ALREADY: phase test and n/64 ramp; differs: control values | Our multiply by exact 1/64 replaces division by 64 (power of two). Increment and lerp order same. Upstream casts step result from double; ours computes it in float, so matching ramp expression does not imply matching samples. |
| w8_dsp.h:144-150 | drum808.c:346,193 | risk | Y | ALREADY: floor/truncate positive duration product, minimum one | Upstream counter=(int)((double)dur*ctrlRate). Our floor_mul uses float-float residual to avoid rounded-up positive product at integer boundaries. Negative durations: floor differs from C truncation but both are then clamped to 1. Exceptional/overflow duration equivalence UNKNOWN. |
| w8_dsp.h:152-157 | drum808.c:357 | risk | Y | ALREADY: one-period or abs(curve)<0.001 forces linear; differs: precision | Upstream fabs(double curve)<double .001 vs fm_fabsf(curve)<float .001f. At curve exactly the float literal .001f, both comparisons are false; neighboring representable float thresholds do not introduce a known discrepancy. Subtraction/division round in float here. |
| w8_dsp.h:158-166 | drum808.c:361 | risk | Y | differs: exp(double) vs d8_exp(float) | Same a1=(end-level)/(1-exp(curve)), a2=level+a1, b1=a1, grow=exp(curve/count). Float cancellation/rounding and fast exponential differ. Upstream double can reduce accumulated numerical error; no proof it is faster, no bit-identical replacement. |
| w8_dsp.h:169-197 | drum808.c:369; drum808.h:52 | risk | Y | ALREADY: done early return, linear/exponential step, decrement and arm; differs: float recurrence | No final snap in either. Upstream casts double level to float at return. State layouts cannot be memcmp-identical across these different representations. |
| w8_dsp.h:211-220 | drum808.c:284 | risk | Y | differs: float coefficients in delta form | Upstream LPF: pfreq=freq*(2pi/sr)*.5, C=1/tan(pfreq), C2=C*C, s2C=C*sqrt2; a0=1/(1+s2C+C2), b1=-2*(1-C2)*a0, b2=-(1-s2C+C2)*a0. Ours uses PI_SR*hz and fm_tanf; numerators n0=a0,n1=2a0,n2=a0; denominator e1=2*a0*(2+s2C), e2=-2*s2C*a0. Algebraic transform, different floating evaluation. |
| w8_dsp.h:221-231 | drum808.c:266 | risk | Y | differs: DF-II double states vs TDF-II float delta states | Upstream y0=in+b1*y1+b2*y2; out=a0*(y0+2*y1+y2), then shift. Our y=n0*x+z1; z1=(n1*x+(y+y+z2))-e1*y; z2=(n2*x-y)-e2*y. State/output rounding differs despite equivalent ideal transfer function. No upstream branch/load optimization. |
| w8_dsp.h:235-244 | drum808.c:275 | risk | Y | differs: float HPF delta coefficients | Upstream C=tan(pfreq), b1=2*(1-C2)*a0; other coefficients same form as LPF. Ours e1=2*a0*(s2C+2*C2), e2=-2*s2C*a0, n1=-2*a0. It avoids rounding a coefficient near -2 but does not reproduce double DF-II state. |
| w8_dsp.h:245-255 | drum808.c:266 | risk | Y | differs: same realization differences as LPF | Upstream numerator y0-2*y1+y2, double accumulation; our generic n1=-2a0 TDF-II. Reset upstream clears only two states; our z1/z2 need separate zeroing by callers. |
| w8_dsp.h:267-279 | drum808.c:294 | risk | Y | differs: sk_hp_set takes Q and gain rather than reciprocal Q | Upstream alpha=sin(w0)*.5*rq, rz=1/(1+alpha), coefficients explicitly negated recursion. Ours alpha=s/(2*q), onepc=2-2*sin(w/2)^2, e1=(2alpha+4sh²)*inv, e2=-2alpha*inv; gain multiplies numerator. rq vs Q must be converted. Double cos/sin vs float polynomial; rearranging multiply/divide cannot be assumed bit-identical. |
| w8_dsp.h:282-294 | drum808.c:307,1628 | risk | Y | differs: upstream takes dB, ours takes amplitude A | Upstream A=pow(10,db*.025), b1=2*rz*cos(w), a1=-b1. Ours supplies A=1.584893192f for rim (8 dB) and computes n1=-2*rz*(1-2sh²), delta e1/e2. Same ideal peak formula, changed setup precision and expression tree. |
| w8_dsp.h:296-307 | drum808.c:266 | risk | Y | differs: double DF-II vs float delta TDF-II | Upstream out=a0*y0+a1*y1+a2*y2. Our bq_run uses already-normalized n coefficients, transposed states and delta feedback. No common byte state representation. |
| w8_dsp.h:315-334 | NONE; related fastmath.h:181,198 | risk | N | differs: no equivalent SineOsc object | set increment 2pi*freq/sr, reset exact supplied double phase, output sinf((float)phase), then add double inc and subtract 2pi once only if >. fm_sin_turns is a float polynomial evaluator without state. Upstream is not an added LUT and not a general high/negative-frequency wrapping fix. |
| w8_dsp.h:340-356 | drum808.c:734,748 | risk | Y | differs: float phase/inc in rim, double upstream | Both emit phase>1?2-phase:phase before adding increment, then if >=3 subtract4. Upstream reset fmod(iphase,4) and negative correction; ours known trigger phase=1 needs no general reset. Inc is hoisted at trigger here; upstream still accepts per-sample freq/sr. Fixed-point 909 triangle is a different phase convention and is not this helper. |
| w8_dsp.h:364-377 | drum808.c:733,752 | risk | Y | ALREADY: opposite polarity on wrap; differs: fixed-point arithmetic | PulseOsc class begins line 364; reset/process at 366/367. Upstream if phase>=1 subtract1 and force duty<=.5?1:0, else compare phase<duty; then add freq/sr. Ours duty .8 forces 0 on delayed wrap; increment and wrap use uint32. Near wrap/quantization thresholds outputs can diverge over time; no whole-range identity proof. |

### Removed 8W8 APIs: each deletion examined

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| sc_ugens.h:89-111 -> w8_dsp.h:67 | NONE | structure | N | ALREADY: no NULLTEST declaration or branch | Old flag externally defined only with SC808_NULLTEST. Deletion does not improve normal production path. |
| sc_ugens.h:222-257 -> w8_dsp.h:142 | NONE | risk | N | ALREADY: no nextHeld or peek | nextHeld returns current control step instead of interpolated previous-to-current ramp. Replacing a held-frequency consumer with next changes pitch integration; this is deletion of an API, not a bit-identical optimization. Our circuit bass drum does not use this old oscillator-envelope graph. |
| sc_ugens.h:337-344 -> w8_dsp.h:198 | NONE | structure | N | ALREADY: no envPerc wrapper | Builds [0,level,0] from two equal-curve segments; deleting a wrapper gives no source-level sample-loop saving. |
| sc_ugens.h:404-430 -> w8_dsp.h:256 | NONE | risk | N | differs: bp_set in drum808.c:242 is RBJ constant-peak circuit BP | Deleted BPF has C=1/tan(rq*w/2), D=2cos(w), a0=1/(1+C), b1=C*D*a0, b2=(1-C)*a0, output a0*(y0-y2). rq convention and bandwidth response differ from Q-based RBJ. No retained replacement formula to import. |
| sc_ugens.h:461-477 -> w8_dsp.h:281 | NONE | risk | N | ALREADY: no octave-bandpass setter | Deleted alpha=sin(w)*sinh((ln2/2*bw*w)/sin(w)); numerator +/-alpha/(1+alpha). Not equivalent to simple reciprocal-Q BPF or constant linear-Q biquad. |
| sc_ugens.h:491-509 -> w8_dsp.h:296 | NONE | structure | N | ALREADY: no SC low-shelf setter | Deleted A=10^(db/40), alpha=.5*sin(w)*sqrt((A+1/A)*(rs-1)+2), sqrt(A) normalization and shelf signs. Our pulse shaper is not this general RBJ shelf. |
| sc_ugens.h:511-529 -> w8_dsp.h:296 | NONE | structure | N | ALREADY: no SC high-shelf setter | Same A/alpha family but high-shelf numerator/denominator signs. No revised arithmetic replaces it. |
| sc_ugens.h:542-570 -> w8_dsp.h:309 | NONE | risk | N | ALREADY: no HiPass4 SC wrapper | Deleted two serial Biquads each rq=sqrt(requested rq). The separate metal circuit highpasses in our port are not evidence of this wrapper. Do not delete our filters or change Q based on this removal. |
| sc_ugens.h:671-747 -> w8_dsp.h:379 | NONE | risk | N | ALREADY: no SC Limiter object in drum808.c | Deleted triple buffer, ceil(dur*sr) clamped 1..2048, two-buffer latency, peak comparison, gain ramp and rotating-buffer loop. Any audible removed limiter work belongs to callers outside this helper scope; header deletion alone proves no active-path speedup. |

### 9W9 Web Audio helpers

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| webaudio.h:48-74 | drum909_dsp.h:79,91 | structure | Y | differs: specialized exponential/constant state | Upstream stores enum, float endpoints/value, double times and time_const. init memset+constant, set_value sets type/value/endpoints; ours set assigns v/cur/v1/left. No generic linear/target states. |
| webaudio.h:78-86,106-114 | NONE | structure | N | differs: no linear ramp or target API | Upstream duration/timeconstant floor to 1 and stores current value as v0; ours has no corresponding methods. Removing unused methods is not a changed runtime optimization here. |
| webaudio.h:91-102 | drum909_dsp.h:102 | risk | Y | ALREADY: endpoint clamp to 1e-9; differs: float duration and recurrence preparation | Upstream clamps p->value in place, stores double t1=max(duration,1 if nonpositive). Ours stores clamped e->cur as local v0, computes ceil(t1)-1, log2 and ratio. For fractional t1 output endpoint begins at k=ceil(t1)-1 in both by source ordering. Interior samples are not identical by construction. |
| webaudio.h:117-143 | drum909_dsp.h:126,140 | risk | Y | differs: powf every sample vs multiply and block anchor | Upstream exp branch computes v0*powf(v1/v0,(float)(t0/t1)), then increments time and overwrites output with v1 when time reaches duration. Ours returns current v before updating next v, with prearranged endpoint tick. Re-anchoring uses fm_exp2f, not powf. Same intended timing does not prove PCM/state equality; no upstream new fix. |
| webaudio.h:174-186 | drum909_dsp.h:158 | risk | Y | ALREADY: normalized cutoff .0001/.9999 and q floor .0001; differs: cos evaluator | Upstream cosf(w), sinf(w); ours cos=1-2*fm_sinf(w/2)^2, sin=fm_sinf(w). Same cutoff and Q clamps retained. cos cancellation error and approximation differ, particularly near DC. |
| webaudio.h:189-246 | drum909_dsp.h:170 | risk | Y | ALREADY: LP/HP/BP RBJ coefficient signs and normalize by reciprocal a0 | Ours hoists common denominator expressions out of three-way numerator selection; only LP/HP/BP supported. Upstream also has notch/peaking and powf gain conversion. Same ideal form, not identical coefficients because trig differs. No upstream hoist added in reviewed transitions. |
| webaudio.h:248-259 | drum909_dsp.h:183,185 | structure | Y | ALREADY: reset four states; same DF-I tick expression and state shift | b0*in+b1*x1+b2*x2-a1*y1-a2*y2 in same order. Given equal input/coefficient/state and matching FP contraction settings, same expression; actual coefficients are computed differently, so whole filters not proved identical. |
| webaudio.h:274-300 | NONE | structure | N | differs: no wa_shaper_t | 256 generated rational-curve values, both up/down biquads at 2*sr, fc=.45*sr, Q=.7071, reset and prev=0. d9_shape/d9_tanh uses another curve and no this oversampling pipeline. Upstream shaper LUT predates both changes; cannot transplant while preserving circuit distortion PCM. |
| webaudio.h:303-325 | NONE | risk | N | differs: no matching 2x shaper pipeline | Clamps x to [-1,1], index scale N-1, caps j to i at last entry, lerps; midpoint then input pass through up filter, each shaped, both down-filtered keeping second. Generation uses i*2/N while lookup uses N-1: asymmetrical endpoint grid is retained, not a new correction. Correctness against external W3C requirements UNKNOWN from repository-only evidence. |
| webaudio.h:332-357 | drum909_dsp.h:196,199; drum909.c:736,776,820 | risk | Y | ALREADY: 909 advances phase before triangle; differs: uint32 modulo | Upstream double phase += floatfreq/doublesr; while>=1 subtract1; triangle double p<.5?4p-1:3-4p, then float cast. Ours quantizes hz*(2^32/sr), integer adds, branchless XOR mirror and float scaling. Boundary mirror has a one-phase-LSB asymmetry from bitwise complement; exact upstream waveform equality not claimed. No upstream new loop change. |
| webaudio.h:360-365 | NONE | structure | N | differs: no saw oscillator helper | Same double increment and while wrap then 2p-1; no replacement or speedup. |
| webaudio.h:371-385 | drum909_dsp.h:321; drum909.c:431 | structure | Y | ALREADY: xorshift32 13/17/5, nonzero engine seed | Upstream noise_init replaces zero with 0x12345678. Ours engine chooses 0xC0FFEE; raw d9_noise has no seed guard (zero remains zero), same recurrence for given nonzero state. Upstream converts ((double)x/2^31) to float then subtracts 1; ours (float)x * 2^-31 then subtracts 1. Under binary32 round-to-nearest, no excess precision/fast-math anomalies, scaling by exact power-of-two commutes with rounding for all uint32 values (no subnormal/overflow here), so conversion results match. This is already our simplification, not an upstream change. Values near UINT32_MAX round to 2 before subtracting, so both can output +1 despite [-1,1) comment; no fix in these versions. |

### 9W9 circuit/percussion/tom helper bodies

| Upstream file:line (new side; hunk) | Our counterpart file:line or NONE | Verdict | Applies to us Y/N | Our code ALREADY has the same / differs | Notes |
|---|---|---|---|---|---|
| 9w9_circuit.h:106-110 | drum909.c:27,32,33; drum909_dsp.h:208 | risk | Y | differs: LUT tanh and reciprocal multiplication | Upstream evaluates two tanhf plus division per diode call. Our fixed 1.6/2 diode helpers use precomputed inverses; general shape prep computes inverse once. Approximation and reciprocal multiply are numerical differences, not proof of equality. No new upstream precomputation. |
| 9w9_circuit.h:142-158 | drum909.c:14-26 | structure | Y | ALREADY: snappy1.95, VT .0002 and norm | All constants already present in base; comments describing past fitting are not changes introduced by rename or v2.7.2. |
| 9w9_circuit.h:179-202 | drum909_dsp.h:234,311 | risk | Y | ALREADY: type1 asymmetry constants and type5 three folds; differs: prepared gain | Upstream t=int(type+.5), k=max(drive,.01); ours type resolved before samples. Type1 uses bias .35, subtract .33638, gain .958; type5 three repeated >1/<-1 branches and .62 folded+.38 clipped body. Upstream conditional division by k vs ours precomputed reciprocal multiplication alters rounding. |
| 9w9_circuit.h:204-217 | drum909_dsp.h:250,280 | risk | Y | ALREADY: quantize/decimate formula; differs: optional state API | steps=1.5+9/k, floor(x*steps+.5)/steps upstream vs multiply by prepared 1/steps. hold=1 below unity else1+(k-1)*1.7; phase+1, single subtract and replace held sample. Upstream accepts NULL state and quantizes only; ours render supplies state and always uses it. No new upstream speedup. |
| 9w9_circuit.h:218-237 | drum909_dsp.h:243,244,286 | risk | Y | ALREADY: SAT/BFZ constants; differs: hoisted constants and gain rounding | SAT x*k+.08*k*x*x, rational wet, m=min(k,1), .65 and1.35 mix. BFZ k*2.5 bias.22 subtract.18033 gain1.05, m=clamp((k-.85)/1.15). Ours c1/c2 hold invariant products and clamped m; upstream recalculates them per call. Arithmetic association and division-vs-reciprocal differ already. |
| 9w9_circuit.h:238-251 | drum909_dsp.h:299 | risk | Y | differs: PDIST division by3 vs multiply by1/3 | Same .12 bias, [-1,1] clamp, y0=.12-.12³/3, factor1.5/1.479. Upstream u-u*u*u/3; ours u-u*u*u*(1/3). Reciprocal is rounded: cannot assert bit-identical cubic. Default diode route uses LUT here. Retained upstream expression, not a new correction. |
| 9w9_circuit.h:254-257 | NONE | structure | N | differs: no stateless shape wrapper API | Wrapper only supplies NULL crush state; ours uses d9_shape with real state. Omitting optional NULL branch is already port-specific. |
| 9w9_circuit.h:261-306 | drum909.c:24-26,404,420 | risk | Y | ALREADY: constants/init values; differs: whole-engine zero contract | Upstream init explicitly resets noise/click/DC filters, envs, oscillators and counters; ours zeroes every byte in drum909_init before specialized initialization. d9_bt_init alone is static and not a standalone public initializer. No uninitialized-state bug established in actual init call chain. |
| 9w9_circuit.h:309-372 | drum909.c:525 | risk | Y | ALREADY: phase .25, tau6..32, m0..1, base/sweep/hold, envelopes and impulse2 | Our bd flag replaces runtime bd_sweep, no hit_tune drift source in inspected code; exp_small vs expf, float durations and ceil-to-integer mute differ. The upstream `(void)(noise_decay>0?... )` has no side effects and ours omits it; no real active-path saving beyond compiler dead-code elimination. |
| 9w9_circuit.h:374-398 | drum909.c:708,762 | structure | Y | ALREADY: active mute, pitch read-before-decay, amp hold and soft VCA | Ours caps block m=min(n,mute), decrements once, splits BD/SD loops, and hoists invariants; upstream tests/decrements double mute every sample. Upstream contains no new source hoist to bring over. |
| 9w9_circuit.h:405-431 | drum909.c:736-782 | risk | Y | ALREADY: advance/round shells independently; differs: inverse mix and LUT | BD diode1.6 asymmetry .05 and DC blocker; SD each diode2 then second mix .40 and divide1+mix. Ours mix ratio/gain loaded once and o*=inv_mix; upstream o/=(1+mix). No bit identity inferred from algebraic similarity. |
| 9w9_circuit.h:434-466 | drum909.c:742,783 | risk | Y | differs: click tail skipping/re-warm and SD click specialization | Upstream continues click filter and floor envelope until mute. Ours skips after env ends, tracks stale/warm state, and SD click attack is pinned0. Snare noise plateau24ms, .7 decay, VCA gate, HP->LP and1.95 already same topology. Exact whole-engine difference outside new upstream hunks; not a candidate upstream bugfix. |
| 9w9_perc909.h:48-79 | drum909.c:298,472,621 | structure | Y | ALREADY: resonances/Q, HP120, trigger impulse2 and env | Upstream fallback tune220/Q12/upper480/Q11 only when unset; our explicit defaults210/Q10/upper480, second Q*1.1. Defaults are engine policy, no newly changed constants in this diff. Mute double countdown vs integer ceil here. |
| 9w9_perc909.h:82-111 | drum909.c:850,859 | structure | Y | ALREADY: transient-only noise and 60/32 gain; differs: zero-input tail specialization | Our warm loop flushes impulse plus two x history samples, then d9_rim_ring uses +0-a1*y1-a2*y2 and state shifts, hoists shape/level/accent/noise mix. Upstream uses full biquad every sample. This upstream baseline is not faster by source operation count; actual timing UNKNOWN without matched measurement. |
| 9w9_perc909.h:136-165 | drum909.c:304,483,637 | structure | Y | ALREADY: BP, HP400 and four-event trigger; differs: explicit engine defaults | pulse_index=0,next_pulse=0,tail=0, mute=(tail+spread*4+40)*ms. Our timer float and mute int vs double upstream. Existing shape state maintained across hits in both call structures; no reset fix introduced. |
| 9w9_perc909.h:168-203 | drum909.c:891 | risk | Y | ALREADY: amp[4]={1.22,1.35,1.68,1}; fourth event starts room tail | Upstream ms loaded per sample/helper call, burst/tail/spread durations computed in event branch, next_pulse double. Ours duration values, tail level, shape, gain hoisted per block; float timer. Decrement-before-next-event cadence and expression order shown; no newly hoisted upstream loads or removed branches. |
| 9w9_tom909.h:52-89 | drum909.c:462; drum909.h:46 | structure | Y | ALREADY: ratios and zeroed filter states at public init | Upstream explicit reset of noise_bp/dc_block fixes standalone uninitialized stack probes historically, but those resets already exist in v2.7.0. Our public drum909_init zeroes whole drum struct before coefficient setup; no missing public-init fix applicable. |
| 9w9_tom909.h:97-132 | drum909.c:562 | risk | Y | ALREADY: phases .25, DC reset, pitch and partial envelopes | 1,.55,.45 starts; main variable decay,110/70ms upper partials,8ms stick; same gains. Our exponential implementation/float times differ, and integer ceil mute replaces double countdown; upstream has no changed trigger formula. |
| 9w9_tom909.h:135-159 | drum909.c:803 | risk | Y | ALREADY: order of three partial additions, *.55, stick*1.5, shape then DC block | Ours skips upper partials/stick once envelopes end and tracks stale stick filter; upstream computes all to mute. Our stick_g=noise_level*1.5 is precomputed vs upstream left-associative filter*env*noise_level*1.5; evaluation order differs. Restoring upstream grouping is not a new bugfix or demonstrated faster change, and could change PCM. |

## Decision, tests, benchmark and limits

No newly introduced upstream speedup or bugfix applies to this helper area. The 8W8 retained arithmetic is unchanged after renames; removed classes/methods are not improvements to our existing render loops. The 9W9 files change names/prose only up to v2.7.1 and gain one leading comment at v2.7.2. Numerical differences from our C port were already present, and are recorded above rather than mislabeled as new upstream fixes.
Therefore the user’s conditional scratch-patch criterion was not met. No worktree, branch, source edit, commit, push, post, network git operation or computer/device access was performed. No host tests, PCM/state memcmp or benchmarks were run: there is no candidate binary pair to validate. Bit identity of a new patch: NOT_APPLICABLE. Existing-port bit identity to upstream: NOT_PROVED; several differences above make a blanket claim wrong.
Read `/workspace/dnbox/optimizer/909-bench/run_interleaved.sh`: it runs existing `base`, `tip`, `tipm1` executables from fixed `/workspace/opt/wt-909b-*` paths, rotates ordering over N rounds, pins CPU2 and collects the final worstcase JSON line. It does not automatically build or select a new scratch candidate. Running it unchanged here would benchmark unrelated existing binaries and cannot substantiate this review. Peak/mean/median/min/noise: NOT_RUN; within-noise judgment: UNKNOWN. All runtime speed comparisons above are source observations or explicitly unknown, not timing claims.
Coverage is the named helper files and their C counterparts, including unchanged numerical bodies. External SuperCollider/W3C conformance and target CPU cycle counts were not independently re-established. Comments claiming previous acoustic fits or null-test results are historical source commentary, not tests performed in this review. No legal conclusion is drawn from added NOTICE wording.

## Blob verification evidence

| Tree | File | Verified Git blob |
|---|---|---|
| 9pin | src/dsp/webaudio.h | `71a62f373633bb14fc5e95e499737e181bd07f60` |
| 9pin | src/dsp/er99_circuit.h | `93569f10f0ca0be4bce12a42d3e9ddbbfae2b856` |
| 9pin | src/dsp/er99_perc909.h | `b0ad749fe54494ecea61d53002eff3aadd159bd2` |
| 9pin | src/dsp/er99_tom909.h | `194df7923078c961110abc26f7c8b774cbf9cdb9` |
| 9ren | src/dsp/webaudio.h | `94d5810617d4ddab8804d0337731f4a0b14e4ae3` |
| 9ren | src/dsp/9w9_circuit.h | `b0a866253e937995b053df20f26d892ab3f33236` |
| 9ren | src/dsp/9w9_perc909.h | `319128a457c8b5d57e2453c4f72e29f2d36af5a9` |
| 9ren | src/dsp/9w9_tom909.h | `f20bebfad2e63ac585a35e8db201a4cc90d8ab95` |
| 9v271 | src/dsp/webaudio.h | `94d5810617d4ddab8804d0337731f4a0b14e4ae3` |
| 9v271 | src/dsp/9w9_circuit.h | `b0a866253e937995b053df20f26d892ab3f33236` |
| 9v271 | src/dsp/9w9_perc909.h | `319128a457c8b5d57e2453c4f72e29f2d36af5a9` |
| 9v271 | src/dsp/9w9_tom909.h | `f20bebfad2e63ac585a35e8db201a4cc90d8ab95` |
| 9v272 | src/dsp/webaudio.h | `010930584ec583e40162abcd156411e77ea43edd` |
| 9v272 | src/dsp/9w9_circuit.h | `0daa3cbc1d7aff2387d57b914e5b6d6957b0bfb1` |
| 9v272 | src/dsp/9w9_perc909.h | `f8a7c8c275ac1095433eb9bfb2ae6cab6d6d0d4d` |
| 9v272 | src/dsp/9w9_tom909.h | `31518e9afedde95c7f04c0b80387f2a990ba5e96` |
| 8pin | src/dsp/sc_ugens.h | `f59dae20520b7212ab0fa7ae4491bde81760d14d` |
| 8v151 | src/dsp/w8_dsp.h | `3a6b712f4714930a43b946248be5881a8e1ec264` |

## Complete normalized three-context-line diffs

These preserve every changed line so the hunk tables can be checked directly. Empty transitions are stated above. Only the requested textual substitutions were applied; no arithmetic or numeric-literal normalization was performed.

### 8base-151 / w8_dsp.h

```diff
--- /tmp/cmp/8pin/src/dsp/sc_ugens.h
+++ /tmp/cmp/8v151/src/dsp/w8_dsp.h
@@ -1,41 +1,24 @@
-/*
- * sc_ugens.h — the SuperCollider UGens w8 uses, as C++.
- *
- * 8W8's Engine A is a transcription of Sonic Pi's w8.scd. A transcription
- * is only worth anything if it is EXACT, so nothing in this file is written
- * from memory or from a textbook: every formula below was read out of
- * SuperCollider's own server plugin sources and the provenance is named at
- * each one. The intent is that rendering a voice here and rendering it in
- * scsynth produces the same samples, and test/nulltest.sh proves it.
- *
- * Where a UGen is a wavetable in SC and a function here (SineOsc), that is
- * called out. Where SC runs something at control rate and we must match its
- * blockiness to null (EnvGen.kr), we reproduce the blockiness rather than
- * "improving" it — an improvement that breaks the null test costs more than
- * it is worth.
- *
- * References, all from supercollider/server/plugins @ develop:
- *   FilterUGens.cpp — LPF, HPF, BPF, the B-suite, Limiter
- *   LFUGens.cpp     — PulseOsc, TriOsc, EnvGen curve segments
- *   NoiseUGens.cpp  — WhiteNoise
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
+/*
+ * w8_dsp.h — the small DSP building blocks the 8W8 voices share: noise,
+ * a segmented envelope, three filters, a biquad and three oscillators.
  *
  * GPL-3.0.
  */
-#ifndef SC_UGENS_H
-#define SC_UGENS_H
+#ifndef W8_DSP_H
+#define W8_DSP_H
 
 #include <math.h>
 #include <stdint.h>
 
-namespace sc {
+namespace dsp {
 
 static const double kPi    = 3.14159265358979323846;
 static const double kTwoPi = 6.28318530717958647693;
 static const double kSqrt2 = 1.41421356237309504880;
 
-/* SC's default control block. EnvGen.kr ticks once per block and the binary
- * op that multiplies it into an audio signal ramps across the block, so this
- * number is audible in the output and has to match scsynth's -z. */
+/* Envelopes step once per control block and are ramped across it, so the
+ * block size is audible: a zero-length segment still lasts one block. */
 static const int kBlockSize = 64;
 
 inline float midicps(const float _note)
@@ -51,16 +34,14 @@
 /* ---- noise ------------------------------------------------------------ */
 
 /*
- * SC's taus88 combined Tausworthe generator, and its white — uniform on
- * [-1, 1). Reproduced exactly rather than substituting an LCG so that a
- * seed-matched null test of the noise voices is possible at all; with a
- * different generator only a spectral comparison is available.
+ * Combined Tausworthe generator (taus88), and white() — uniform on [-1, 1).
+ * Seeded per voice, so a voice's noise is the same every time it is built.
  */
 class NoiseGen {
 public:
     void seed(uint32_t s)
     {
-        /* SC requires s1 > 1, s2 > 7, s3 > 15. */
+        /* Each component needs a minimum seed: s1 > 1, s2 > 7, s3 > 15. */
         s1_ = 1243598713u ^ s; if(s1_ <  2u) s1_ = 1243598713u;
         s2_ = 3093459404u ^ s; if(s2_ <  8u) s2_ = 3093459404u;
         s3_ = 1821928721u ^ s; if(s3_ < 16u) s3_ = 1821928721u;
@@ -74,7 +55,7 @@
         return s1_ ^ s2_ ^ s3_;
     }
 
-    /* white: bit-stuff the mantissa of a float in [2,4), then subtract 3. */
+    /* Put 23 random bits in the mantissa of a float in [2,4), subtract 3. */
     float white()
     {
         union { uint32_t u; float f; } u;
@@ -86,44 +67,14 @@
     uint32_t s1_ = 1243598713u, s2_ = 3093459404u, s3_ = 1821928721u;
 };
 
-/* ---- SC's per-note construction step ----------------------------------
- *
- * SC builds a new synth for every note, and every UGen Ctor finishes by
- * calling its own next(unit, 1): one sample runs through the entire graph
- * with all inputs at their construction-time values before the first real
- * block. Filters see their first input sample twice; oscillators take one
- * extra phase step; a swept oscillator takes that step at the envelope's
- * initial value.
- *
- * 8W8's voices are persistent — constructed once, retriggered thereafter —
- * so reproducing this in the engine would be a BUG, not fidelity: it would
- * repeat a sample and jump a phase on every hit. It exists only so the null
- * test compares like with like, and it is compiled out of the module: the
- * flag and its branch live behind SC808_NULLTEST, which only the test tools
- * define.
- *
- * The harness raises the flag, renders exactly one sample, lowers it, and
- * discards that sample.
- */
-#ifdef SC808_NULLTEST
-extern bool g_scPriming;
-#endif
-
 /* ---- envelopes -------------------------------------------------------- */
 
 /*
- * One segment of an SC Env with a "curve" shape.
- *
- * From LFUGens.cpp, Env_next / shape_Curve:
- *     a1   = (endLevel - level) / (1 - exp(curve))
- *     a2   = level + a1
- *     b1   = a1
- *     grow = exp(curve / durationInSamples)
- *     per step:  b1 *= grow;  out = a2 - b1
- *
- * which is level + (endLevel-level)(1 - e^{curve·t})/(1 - e^{curve}), t∈[0,1].
- * |curve| < 0.001 degenerates to linear, exactly as SC does — w8 never
- * relies on that, but a pot sweeping a curve through zero would.
+ * One envelope segment: a target level, a time and a curve.
+ *
+ * The curve shape is
+ *     level + (end - level) * (1 - e^{curve·t}) / (1 - e^{curve}),  t in [0,1]
+ * computed incrementally (a1, a2, b1, grow below). |curve| < 0.001 is linear.
  */
 struct EnvSeg {
     float endLevel;
@@ -131,36 +82,25 @@
     float curve;
 };
 
-#define SC_ENV_MAX_SEGS 4
-
-/*
- * EnvGen.kr, and it really is .kr — every envelope in w8 is.
- *
- * That is not a detail to smooth away. An EnvGen at control rate steps once
- * per 64-sample block, so its "sample rate" is 689 Hz at 44.1k, and a segment
- * of ZERO duration still occupies one whole control period — 1.45 ms. w8
- * leans on this: every voice's amplitude envelope is
- *
- *     Env.new([click, 1, 0], [0, decay], curve)
- *
- * whose first segment has duration 0. Run that at audio rate and `click`
- * lasts one sample and does nothing; run it at control rate, as SC does, and
- * `click` is a 1.45 ms pre-level at the front of the note — an audible part
- * of the attack of every drum in the kit. Getting this wrong makes the whole
- * kit sound soft and makes the `click` parameter look vestigial.
- *
- * The value is then linearly ramped across the block by whatever consumes it:
- * a BinaryOpUGen multiplying an audio signal, or SineOsc's frequency input.
- * Both use CALCSLOPE, so one interpolation rule covers both.
+#define W8_ENV_MAX_SEGS 4
+
+/*
+ * A segmented envelope that runs at control rate — one step per 64-sample
+ * block — and is ramped linearly across each block.
+ *
+ * The control rate matters. A segment of zero duration still lasts one whole
+ * control period (1.45 ms at 44.1k), so an envelope that starts at a `click`
+ * level and jumps to 1 in "zero" time holds that click level for 1.45 ms. That
+ * pre-level is an audible part of the attack; at audio rate it would vanish.
  */
 class Env {
 public:
-    /* levels[0] is the start level; segs[i] carries the target and time. */
+    /* _startLevel is the level at t=0; segs[i] carries each target and time. */
     void set(const float _startLevel, const EnvSeg *_segs, const int _count,
              const double _sampleRate)
     {
         start_ = _startLevel;
-        count_ = _count > SC_ENV_MAX_SEGS ? SC_ENV_MAX_SEGS : _count;
+        count_ = _count > W8_ENV_MAX_SEGS ? W8_ENV_MAX_SEGS : _count;
         ctrlRate_ = _sampleRate / (double)kBlockSize;
         for(int i = 0; i < count_; ++i) seg_[i] = _segs[i];
         rewind();
@@ -179,104 +119,27 @@
 
     bool done() const { return done_; }
 
-    /* One audio-rate sample: step the control-rate generator on block
-     * boundaries, ramp linearly in between. */
+    /* One audio-rate sample: step on block boundaries, ramp in between. */
     float next()
     {
-#ifdef SC808_NULLTEST
-        /* During the construction sample an EnvGen reports its initial level
-         * and does not step: EnvGen_Ctor's next(unit,1) runs with
-         * m_stage == ENVGEN_NOT_STARTED. */
-        if(g_scPriming) return start_;
-#endif
         if(phase_ >= kBlockSize)
         {
             phase_ = 0;
             prev_  = cur_;
             cur_   = stepControl();
         }
-        /* n/64 for n = 0..63, so the FIRST sample of a block is exactly the
-         * previous control value and the block never reaches `cur` — that is
-         * what CALCSLOPE does (`out = a * b; b += slope;`, b starting at the
-         * old value). Taking t after the increment instead puts the whole
-         * envelope one sample early, which is worth ~35 dB of null. */
+        /* t = n/64 for n = 0..63: the first sample of a block is exactly the
+         * previous control value, and the ramp never quite reaches `cur`. */
         const float t = (float)phase_ / (float)kBlockSize;
         ++phase_;
         return prev_ + (cur_ - prev_) * t;
     }
 
+private:
     /*
-     * The block's control value, HELD constant for all 64 samples.
-     *
-     * This is what a UGen with a control-rate FREQUENCY input sees, and it is
-     * not the same as next(). From Osc_ikk_perform in OscUGens.cpp:
-     *
-     *     int32 freq = (int32)(unit->m_cpstoinc * freqin);
-     *     int32 phaseinc = freq + (int32)(CALCSLOPE(phasein, ...) * ...);
-     *     LOOP1(n, ZXP(out) = lookupi1(...phase...); phase += phaseinc;);
-     *
-     * `freqin` is read ONCE per block and the increment is constant across
-     * it. Only the PHASE input is ramped. So an envelope driving a frequency
-     * is a staircase, while the same envelope multiplying an audio signal is
-     * a ramp — the two consumers see genuinely different signals.
-     *
-     * Interpolating the frequency instead is smoother and wrong: over a fast
-     * pitch sweep the difference integrates into a permanent phase error,
-     * which is what left the bass drum sitting at a flat -19 dB residual for
-     * its whole tail while its envelope and filters were already exact.
-     */
-    float nextHeld()
-    {
-#ifdef SC808_NULLTEST
-        if(g_scPriming) return start_;
-#endif
-        if(phase_ >= kBlockSize)
-        {
-            phase_ = 0;
-            prev_  = cur_;
-            cur_   = stepControl();
-        }
-        ++phase_;
-        return cur_;
-    }
-
-    /* The raw control-rate level, for analysis. */
-    float peek() const { return (float)level_; }
-
-private:
-    /*
-     * EnvGen_initSegment, LFUGens.cpp @ Version-3.11.2:
-     *
-     *     counter = (int32)(dur * SAMPLERATE);
-     *     counter = sc_max(1, counter);
-     *     if (counter == 1) unit->m_shape = shape_Linear;
-     *     ...
-     *     unit->m_grow = exp(curve / counter);
-     *
-     * Three things, and each one was worth measurable dB:
-     *
-     *   - the segment runs an INTEGER number of control periods, TRUNCATED.
-     *     Treating the duration as fractional and comparing position against
-     *     it holds every segment one period too long — an envelope sitting at
-     *     full level for an extra 1.45 ms, and 30 dB of null.
-     *   - `counter` — the integer — is also what the curve rate is computed
-     *     over. Using the fractional duration instead put the bass drum's
-     *     pitch envelope at 334.26 Hz where SC has 332.80, which sounds like
-     *     nothing and drifts into a total phase decorrelation over a second.
-     *   - a one-period segment is forced to Linear regardless of curve. This
-     *     is what actually renders w8's zero-duration `click` segments:
-     *     one linear control period straight to the node.
-     *
-     * SAMPLERATE here is the CONTROL rate, because these are all EnvGen.kr.
-     *
-     * VERSION NOTE. SuperCollider's development branch has since reworked
-     * this: it keeps a fractional duration for the curve, floors separately
-     * for the step count, and carries the remainder into the next segment so
-     * a chain of segments does not drift late. That is a better envelope and
-     * it is NOT what is implemented here, because the reference renders come
-     * from 3.11.2 and a transcription that cannot be verified is worth less
-     * than one that can. If the reference is ever moved to a newer SC, this
-     * is the function to change and test/nulltest.sh will say so loudly.
+     * A segment runs a whole number of control periods, truncated, and at
+     * least one. The curve rate is computed over that same integer count,
+     * and a one-period segment is always linear.
      */
     void armSegment()
     {
@@ -312,16 +175,15 @@
 
         if(--counter_ <= 0)
         {
-            /* No snap to the node level: SC only does that for shape_Hold,
-             * and carrying the small shortfall into the next segment is part
-             * of what we are reproducing. */
+            /* No snap to the target: the small shortfall carries into the
+             * next segment. */
             ++segIndex_;
             armSegment();
         }
         return (float)level_;
     }
 
-    EnvSeg seg_[SC_ENV_MAX_SEGS] = {};
+    EnvSeg seg_[W8_ENV_MAX_SEGS] = {};
     int    count_ = 0;
     int    segIndex_ = 0;
     int    counter_ = 1;
@@ -334,23 +196,14 @@
     int    phase_ = kBlockSize;
 };
 
-/* Env.perc(attack, release, level, curve) — levels [0, level, 0], the same
- * curve on both segments. SC's default curve is -4; w8 always names one. */
-inline void envPerc(Env &_e, const float _attack, const float _release,
-                    const float _level, const float _curve, const double _sr)
-{
-    const EnvSeg s[2] = { { _level, _attack, _curve }, { 0.0f, _release, _curve } };
-    _e.set(0.0f, s, 2, _sr);
-}
-
-/* ---- SC's own filters -------------------------------------------------
- *
- * LPF/HPF/BPF are 2nd order and share a Direct Form II core:
+/* ---- filters ----------------------------------------------------------
+ *
+ * LPF and HPF are 2nd-order Butterworth (bilinear transform) sharing a
+ * Direct Form II core:
  *     y0  = in + b1·y1 + b2·y2
- *     out = a0·(numerator over y0,y1,y2)
- * The numerator is what distinguishes them, so it is spelled out per class
- * rather than hidden behind a generic biquad — the sign of the "2·y1" term
- * is the whole difference between a lowpass and a highpass here.
+ *     out = a0·(numerator over y0, y1, y2)
+ * Only the numerator differs — the sign of the "2·y1" term is the whole
+ * difference between the lowpass and the highpass.
  */
 
 class LPF {
@@ -401,49 +254,16 @@
     double y1_ = 0.0, y2_ = 0.0, a0_ = 0.0, b1_ = 0.0, b2_ = 0.0;
 };
 
-/* BPF's second argument is rq (reciprocal of Q), NOT bandwidth in Hz.
- * w8 passes 1 or leaves it at the default 1. */
-class BPF {
-public:
-    void set(const double _freq, const double _rq, const double _sr)
-    {
-        const double pfreq = _freq * (kTwoPi / _sr);
-        const double pbw   = _rq * pfreq * 0.5;
-        const double C = 1.0 / tan(pbw);
-        const double D = 2.0 * cos(pfreq);
-        a0_ = 1.0 / (1.0 + C);
-        b1_ = C * D * a0_;
-        b2_ = (1.0 - C) * a0_;
-    }
+/*
+ * RBJ-cookbook biquad with a highpass and a peaking-EQ setter. b1 and b2 are
+ * stored already negated, so the recursion adds them. Gains in dB convert to
+ * amplitude as 10^(db/40).
+ */
+class Biquad {
+public:
     void reset() { y1_ = y2_ = 0.0; }
-    float process(const float _in)
-    {
-        const double y0 = (double)_in + b1_ * y1_ + b2_ * y2_;
-        const double out = a0_ * (y0 - y2_);
-        y2_ = y1_; y1_ = y0;
-        return (float)out;
-    }
-private:
-    double y1_ = 0.0, y2_ = 0.0, a0_ = 0.0, b1_ = 0.0, b2_ = 0.0;
-};
-
-/* ---- the "B" suite: RBJ cookbook biquads ------------------------------
- *
- * BPeakEQ, BLowShelf, BHiShelf, BHiPass, BBandPass all share one difference
- * equation (BPerformFilterLoop in FilterUGens.cpp) and differ only in how the
- * five coefficients are computed, so here they are one class with named
- * setters. Note SC's sign convention: b1 and b2 are stored ALREADY NEGATED
- * relative to the textbook, so the recursion adds them.
- *
- * The dB-to-linear conversion is pow(10, db/40) — SC writes it as
- * pow(10., db * 0.025), which is the same thing and is why an "amplitude"
- * of a shelf is the square root of its power gain.
- */
-class Biquad {
-public:
-    void reset() { y1_ = y2_ = 0.0; }
-
-    /* BHiPass(freq, rq) */
+
+    /* Highpass. _rq is the RECIPROCAL of Q: Butterworth is sqrt(2). */
     void setHiPass(const double _freq, const double _rq, const double _sr)
     {
         const double w0 = kTwoPi * _freq / _sr;
@@ -458,22 +278,7 @@
         b2_ = (1.0 - alpha) * -rz;
     }
 
-    /* BBandPass(freq, bw) — bw in OCTAVES, unlike BPF's rq.
-     * 0.34657359027997 is ln(2)/2, from the cookbook's bandwidth form. */
-    void setBandPass(const double _freq, const double _bw, const double _sr)
-    {
-        const double w0 = kTwoPi * _freq / _sr;
-        const double sinw0 = sin(w0);
-        const double alpha = sinw0 * sinh((0.34657359027997 * _bw * w0) / sinw0);
-        const double rz = 1.0 / (1.0 + alpha);
-        a0_ = alpha * rz;
-        a1_ = 0.0;
-        a2_ = -a0_;
-        b1_ = cos(w0) * 2.0 * rz;
-        b2_ = (1.0 - alpha) * -rz;
-    }
-
-    /* BPeakEQ(freq, rq, db) */
+    /* Peaking EQ. _rq is the reciprocal of Q, _db the boost or cut. */
     void setPeakEQ(const double _freq, const double _rq, const double _db,
                    const double _sr)
     {
@@ -488,44 +293,6 @@
         b2_ = (1.0 - (alpha / a)) * -rz;
     }
 
-    /* BLowShelf(freq, rs, db) */
-    void setLowShelf(const double _freq, const double _rs, const double _db,
-                     const double _sr)
-    {
-        const double a = pow(10.0, _db * 0.025);
-        const double w0 = kTwoPi * _freq / _sr;
-        const double cosw0 = cos(w0);
-        const double alpha = sin(w0) * 0.5 * sqrt((a + (1.0 / a)) * (_rs - 1.0) + 2.0);
-        const double i = (a + 1.0) * cosw0;
-        const double j = (a - 1.0) * cosw0;
-        const double k = 2.0 * sqrt(a) * alpha;
-        const double rz = 1.0 / ((a + 1.0) + j + k);
-        a0_ = a * ((a + 1.0) - j + k) * rz;
-        a1_ = 2.0 * a * ((a - 1.0) - i) * rz;
-        a2_ = a * ((a + 1.0) - j - k) * rz;
-        b1_ = 2.0 * ((a - 1.0) + i) * rz;
-        b2_ = ((a + 1.0) + j - k) * -rz;
-    }
-
-    /* BHiShelf(freq, rs, db) */
-    void setHiShelf(const double _freq, const double _rs, const double _db,
-                    const double _sr)
-    {
-        const double a = pow(10.0, _db * 0.025);
-        const double w0 = kTwoPi * _freq / _sr;
-        const double cosw0 = cos(w0);
-        const double alpha = sin(w0) * 0.5 * sqrt((a + (1.0 / a)) * (_rs - 1.0) + 2.0);
-        const double i = (a + 1.0) * cosw0;
-        const double j = (a - 1.0) * cosw0;
-        const double k = 2.0 * sqrt(a) * alpha;
-        const double rz = 1.0 / ((a + 1.0) - j + k);
-        a0_ = a * ((a + 1.0) + j + k) * rz;
-        a1_ = -2.0 * a * ((a - 1.0) + i) * rz;
-        a2_ = a * ((a + 1.0) + j - k) * rz;
-        b1_ = -2.0 * ((a - 1.0) - i) * rz;
-        b2_ = ((a + 1.0) - j - k) * -rz;
-    }
-
     float process(const float _in)
     {
         const double y0 = (double)_in + b1_ * y1_ + b2_ * y2_;
@@ -539,58 +306,12 @@
     double a0_ = 1.0, a1_ = 0.0, a2_ = 0.0, b1_ = 0.0, b2_ = 0.0;
 };
 
-/*
- * BHiPass4 is two BHiPass in series — but NOT with the rq you asked for.
- * SCClassLibrary/Common/Audio/BEQSuite.sc:
- *
- *     BHiPass4 { *ar { arg in, freq = 1200.0, rq = 1.0, mul = 1.0, add = 0.0;
- *         rq = sqrt(rq);
- *         coefs = BHiPass.sc(nil, freq, rq);
- *         ^SOS.ar(SOS.ar(in, *coefs), *coefs ++ [mul, add]); } }
- *
- * The sqrt is the whole point: cascading two sections each of rq gives an
- * overall rq of rq^2, so each stage takes the square root to land on the rq
- * that was requested. Passing rq straight through to both stages costs about
- * 2 dB of resonance and was worth 20 dB of null on the open hat.
- */
-class HiPass4 {
-public:
-    void set(const double _freq, const double _rq, const double _sr)
-    {
-        const double rq = sqrt(_rq);
-        a_.setHiPass(_freq, rq, _sr);
-        b_.setHiPass(_freq, rq, _sr);
-    }
-    void reset() { a_.reset(); b_.reset(); }
-    float process(const float _in) { return b_.process(a_.process(_in)); }
-private:
-    Biquad a_, b_;
-};
-
 /* ---- oscillators ------------------------------------------------------
  *
- * All three emit the current sample and THEN advance, which is what SC's
- * per-sample loops do (`ZXP(out) = ...; phase += phaseinc;`).
- *
- * A rendered scsynth file looks at first like it disagrees: the first sample
- * of SineOsc.ar(440, pi/2) is cos(w) = 0.99804, not sin(pi/2) = 1.0. That is
- * not the loop — it is SC constructing a fresh synth per note and running one
- * extra sample through the whole graph at construction. Every UGen Ctor ends
- * by calling its own next(unit, 1). See SC_CTOR_PRIME below.
- *
- * The distinction is invisible at a constant frequency and very visible at a
- * swept one, because the construction step uses the envelope's INITIAL value
- * (470 Hz on the bass drum) while the first real block uses the value after
- * one control step (334 Hz).
- */
-
-/*
- * SineOsc. SC reads an 8192-point sine wavetable with linear interpolation;
- * this is sinf(). The difference is the table's interpolation error, about
- * -100 dBFS at this table size — below anything the null test cares about,
- * and below the 16-bit floor the Move outputs at. Named here so nobody has
- * to rediscover it when the null lands at -95 dB instead of -140.
- */
+ * All three emit the current sample and THEN advance the phase.
+ */
+
+/* Sine. */
 class SineOsc {
 public:
     void set(const double _freq, const double _sr) { inc_ = kTwoPi * _freq / _sr; }
@@ -602,7 +323,7 @@
         if(phase_ > kTwoPi) phase_ -= kTwoPi;
         return v;
     }
-    /* Frequency-modulated form, for the voices whose pitch is an envelope. */
+    /* Frequency-modulated form, for voices whose pitch is an envelope. */
     float process(const double _freq, const double _sr)
     {
         inc_ = kTwoPi * _freq / _sr;
@@ -613,12 +334,8 @@
 };
 
 /*
- * TriOsc. From LFUGens.cpp:
- *     freqMul = 4 / sampleRate;  phase starts wrapped into [0, 4)
- *     z = phase > 1 ? 2 - phase : phase;  phase += freq;
- *     if (phase >= 3) phase -= 4;
- * Output is bipolar -1..1 and NOT band-limited — the corners alias, in SC
- * too. Reproduced, not fixed.
+ * Triangle, bipolar -1..1, phase in [0, 4). Not band-limited: the corners
+ * alias.
  */
 class TriOsc {
 public:
@@ -639,19 +356,10 @@
 };
 
 /*
- * PulseOsc. From LFUGens.cpp:
- *     if (phase >= 1) { phase -= 1; z = duty <= 0.5 ? 1 : 0; }
- *     else            { z = phase < duty ? 1 : 0; }
- *     phase += freq/sampleRate;
- *
- * Two things that matter and are easy to get wrong:
- *   1. the output is UNIPOLAR, 0..1. w8's hi-hats sum six of these, so the
- *      sum carries a large DC term that the filter chain removes. Making it
- *      bipolar "because a square wave is bipolar" changes every hat.
- *   2. the wrap branch forces one sample of the opposite polarity, which is
- *      what stops a duty of exactly 0 or 1 from producing silence.
- * Naive and aliasing, as in SC. On the 808's real hats the oscillators are
- * Schmitt-trigger squares whose harmonics run past Nyquist anyway.
+ * Pulse, UNIPOLAR 0..1 — callers that sum several remove the DC themselves.
+ * The wrap sample forces the opposite polarity, so a duty of exactly 0 or 1
+ * still makes a tick rather than silence. Naive, like the Schmitt-trigger
+ * squares it stands in for.
  */
 class PulseOsc {
 public:
@@ -668,84 +376,6 @@
     double phase_ = 0.0;
 };
 
-/* ---- Limiter ----------------------------------------------------------
- *
- * SC's Limiter is a lookahead limiter over three rotating buffers of
- * `dur` seconds each (default 0.01 s). Two consequences that a naive
- * "just clip it" port would silently lose, and that w8's bass drum
- * depends on:
- *
- *   - it delays the signal by TWO buffers, ~20 ms at the default. The
- *     w8 kick therefore arrives ~20 ms late relative to every other
- *     voice. That is a property of w8, not a bug in this port; the
- *     engine compensates for it explicitly and says so where it does.
- *   - it emits silence until two buffers have filled, so the very start
- *     of the first note is gone.
- *
- * Gain is recomputed once per buffer from max(prev, current) peak and
- * ramped across the next buffer, so it never modulates within a buffer.
- */
-#define SC_LIMITER_MAX 2048
-
-class Limiter {
-public:
-    void set(const float _dur, const double _sr)
-    {
-        long n = (long)ceil((double)_dur * _sr);
-        if(n < 1) n = 1;
-        if(n > SC_LIMITER_MAX) n = SC_LIMITER_MAX;
-        bufsize_ = n;
-        slopeFactor_ = 1.0f / (float)bufsize_;
-        reset();
-    }
-
-    void reset()
-    {
-        for(int i = 0; i < 3; ++i)
-            for(long j = 0; j < bufsize_; ++j) buf_[i][j] = 0.0f;
-        in_ = 0; mid_ = 1; out_ = 2;
-        pos_ = 0; flips_ = 0;
-        slope_ = 0.0f; level_ = 1.0f;
-        prevMax_ = curMax_ = 0.0f;
-    }
-
-    /* Latency in samples, for callers that want to line the kick up with the
-     * rest of the kit rather than inherit SC's 20 ms. */
-    long latency() const { return bufsize_ * 2; }
-
-    float process(const float _in, const float _amp)
-    {
-        buf_[in_][pos_] = _in;
-        const float y = flips_ >= 2 ? level_ * buf_[out_][pos_] : 0.0f;
-        level_ += slope_;
-
-        const float a = fabsf(_in);
-        if(a > curMax_) curMax_ = a;
-
-        if(++pos_ >= bufsize_)
-        {
-            pos_ = 0;
-            const float max2 = prevMax_ > curMax_ ? prevMax_ : curMax_;
-            prevMax_ = curMax_;
-            curMax_  = 0.0f;
-
-            const float next = max2 > _amp ? _amp / max2 : 1.0f;
-            slope_ = (next - level_) * slopeFactor_;
-
-            const int t = out_; out_ = mid_; mid_ = in_; in_ = t;
-            ++flips_;
-        }
-        return y;
-    }
-
-private:
-    float buf_[3][SC_LIMITER_MAX] = {};
-    long  bufsize_ = 441, pos_ = 0;
-    int   in_ = 0, mid_ = 1, out_ = 2, flips_ = 0;
-    float slope_ = 0.0f, level_ = 1.0f, slopeFactor_ = 1.0f / 441.0f;
-    float prevMax_ = 0.0f, curMax_ = 0.0f;
-};
-
-} /* namespace sc */
-
-#endif /* SC_UGENS_H */
+} /* namespace dsp */
+
+#endif /* W8_DSP_H */
```

### 9pin-9ren / webaudio.h

```diff
--- /tmp/cmp/9pin/src/dsp/webaudio.h
+++ /tmp/cmp/9ren/src/dsp/webaudio.h
@@ -1,21 +1,19 @@
 /*
- * webaudio.h — the handful of Web Audio API nodes er-99 uses, in C.
+ * webaudio.h — a handful of W3C Web Audio API nodes, in C.
  *
- * er-99's DSP is a pure Web Audio node graph (no AudioWorklet), so every
- * behaviour it relies on is defined mathematically by the W3C Web Audio spec.
- * This header implements exactly those definitions, which is what makes the
- * port verifiable against reference renders instead of tuned by ear.
+ * Each node implements the behaviour the W3C Web Audio spec defines
+ * mathematically, so results are verifiable rather than tuned by ear.
  *
  * Spec references:
  *   BiquadFilterNode  https://www.w3.org/TR/webaudio/#filters-characteristics
  *   AudioParam ramps  https://www.w3.org/TR/webaudio/#dom-audioparam-*
  *   WaveShaperNode    https://www.w3.org/TR/webaudio/#WaveShaperNode
  *
- * GPL-3.0 (er-99 is GPL-3.0).
+ * GPL-3.0.
  */
 
-#ifndef ER99_WEBAUDIO_H
-#define ER99_WEBAUDIO_H
+#ifndef W9_WEBAUDIO_H
+#define W9_WEBAUDIO_H
 
 #include <math.h>
 #include <stdint.h>
@@ -25,15 +23,15 @@
 #define M_PI 3.14159265358979323846
 #endif
 
-/* Web Audio clamps exponential ramp endpoints away from zero; er-99 relies on
- * this by ramping to 0.00001 rather than 0. */
+/* Web Audio clamps exponential ramp endpoints away from zero; envelopes rely
+ * on this by ramping to 0.00001 rather than 0. */
 #define WA_MIN_EXP 1.0e-9f
 
 /* ===================================================================== */
 /* AudioParam — the automation timeline                                   */
 /* ===================================================================== */
 /*
- * er-99 uses four automation methods. Rather than model a full event list we
+ * The voices use four automation methods. Rather than model a full event list we
  * keep the single active segment, which is sufficient because every voice
  * re-arms its envelopes on trigger (the one exception, the clap's 5 scheduled
  * taps, is handled by the clap voice itself).
@@ -165,8 +163,8 @@
  * Coefficients exactly as the Web Audio spec defines them (which is the RBJ
  * cookbook). Note the spec's BANDPASS is the constant-0dB-peak-gain variant,
  * and Q is interpreted in dB for lowpass/highpass but linearly for
- * bandpass/notch/peaking — er-99 leaves Q at default (1) for LP/HP/notch and
- * sets it explicitly only on bandpass, so we take Q linearly throughout.
+ * bandpass/notch/peaking — the voices leave Q at default (1) for LP/HP/notch
+ * and set it explicitly only on bandpass, so we take Q linearly throughout.
  */
 static inline void wa_biquad_set(wa_biquad_t *f, const wa_filter_type_t _type,
                                  const float _freq, const float _q,
@@ -264,7 +262,7 @@
 /* WaveShaperNode                                                         */
 /* ===================================================================== */
 /*
- * er-99's curve, verbatim from index.ts:
+ * Saturation curve:
  *   curve[i] = (PI + amount) * x / (PI + amount * |x|),  x = i*2/n - 1
  *
  * The node runs at oversample='2x'. We reproduce that: upsample x2 (linear),
@@ -366,7 +364,7 @@
 }
 
 /* ===================================================================== */
-/* White noise — er-99 uses lib/noise.js: Math.random() * 2 - 1           */
+/* White noise, uniform in [-1, 1)                                        */
 /* ===================================================================== */
 
 typedef struct { uint32_t state; } wa_noise_t;
@@ -385,4 +383,4 @@
     return (float)((double)n->state / 2147483648.0) - 1.0f;
 }
 
-#endif /* ER99_WEBAUDIO_H */
+#endif /* W9_WEBAUDIO_H */
```

### 9pin-9ren / 9w9_circuit.h

```diff
--- /tmp/cmp/9pin/src/dsp/er99_circuit.h
+++ /tmp/cmp/9ren/src/dsp/9w9_circuit.h
@@ -1,8 +1,8 @@
 /*
  * 9w9_circuit.h — circuit-informed TR-909 voice models (BD / SD / toms).
  *
- * er-99's oscillator voices are "triangle + VCA envelope", which is why its
- * toms sound thin and its kick lacks attack. The real 909 analog voice is:
+ * A plain "triangle + VCA envelope" voice gives thin toms and a kick with no
+ * attack. The real 909 analog voice is:
  *
  *   - an oscillator producing a TRIANGLE that is rounded toward a SINE by
  *     back-to-back diodes conducting ~0.5-0.6 V either side of ground,
@@ -11,19 +11,19 @@
  *   - an AMPLITUDE envelope (Decay),
  *   - and a separate CLICK path — a short impulse plus lowpass-filtered noise,
  *     summed and given their own fast envelope (the Attack control). This is
- *     the beater contact, and it is what er-99 is missing entirely.
+ *     the beater contact.
  *
  * The snare adds a second tuned oscillator and a noise path with its own
  * envelope and highpass (Snappy).
  *
- * Sources: Network-909 circuit analysis (cited by er-99 itself), Roland
+ * Sources: Network-909 circuit analysis, Roland
  * TR-909 service documentation, and the sound-mod analysis at firstpr.com.au.
  *
  * GPL-3.0.
  */
 
-#ifndef ER99_CIRCUIT_H
-#define ER99_CIRCUIT_H
+#ifndef W9_CIRCUIT_H
+#define W9_CIRCUIT_H
 
 #include "webaudio.h"
 
@@ -93,7 +93,7 @@
     int         impulse;        /* samples of initial impulse left */
     double      mute_countdown;
     float       sample_rate;
-} 9w9_bt_t;
+} w9_bt_t;
 
 /*
  * Back-to-back diode rounding. A triangle wave's sharp peaks are what make it
@@ -102,7 +102,7 @@
  * model of that pair. drive ~2 reproduces the 909's rounded triangle; higher
  * values push it back toward a harder, more aggressive tone.
  */
-static inline float 9w9_diode_round(const float _x, const float _drive)
+static inline float w9_diode_round(const float _x, const float _drive)
 {
     const float k = _drive > 0.01f ? _drive : 0.01f;
     return tanhf(k * _x) / tanhf(k);
@@ -138,23 +138,23 @@
  * preset is quieter still on the noise leg (0.12 against his hardware's 0.18
  * at 5 ms) — its Snappy was evidently set below noon, so the hardware stays
  * the reference. */
-#define ER99_SNAPPY_MIX 1.95f
+#define W9_SNAPPY_MIX 1.95f
 
 /* Turn-on voltage of the single-transistor VCAs. Fitted, not assumed: at 0.027
  * the snare's shells were cut off entirely by 100 ms where the real drum still
  * has them at -30 dB and audible to 150. Fitted again once the shell decay was
  * measured directly (tau 28 ms, not 36): at 0.005 the tail still gated off
  * before 150 ms where the references are at -39 dB and running to -46 at
- * 200 ms. Overridable so the fitting rig can sweep it (-DER99_VCA_VT=...).
+ * 200 ms. Overridable so the fitting rig can sweep it (-DW9_VCA_VT=...).
  *
  * 0.0002 after fitting the shells against Roland's own TR-909 with Snappy at
  * zero — the cleanest shell reference there is. At 0.001 the tail still cut
  * short of theirs (0.005 against 0.015 at 150 ms, both around -40 dB, so this
  * is tidiness rather than anything audible). */
-#ifndef ER99_VCA_VT
-#define ER99_VCA_VT    0.0002f
+#ifndef W9_VCA_VT
+#define W9_VCA_VT    0.0002f
 #endif
-#define ER99_VCA_NORM  (1.0f / (1.0f - ER99_VCA_VT))
+#define W9_VCA_NORM  (1.0f / (1.0f - W9_VCA_VT))
 
 /*
  * The voice/master distortion stage. Seven characters:
@@ -175,7 +175,7 @@
  * All types are level-normalised below unity drive, so Drive at minimum is
  * transparent for every one of them.
  */
-static inline float 9w9_shape_st(const float _x, const float _drive,
+static inline float w9_shape_st(const float _x, const float _drive,
                                   const float _type, float *_st)
 {
     const int t = (int)(_type + 0.5f);
@@ -246,32 +246,32 @@
     }
     case 0:
     default:
-        return 9w9_diode_round(_x, k);
-    }
-}
-
-static inline float 9w9_shape(const float _x, const float _drive, const float _type)
-{
-    return 9w9_shape_st(_x, _drive, _type, (float*)0);
+        return w9_diode_round(_x, k);
+    }
+}
+
+static inline float w9_shape(const float _x, const float _drive, const float _type)
+{
+    return w9_shape_st(_x, _drive, _type, (float*)0);
 }
 
 /* The measured resting pitch of the stock kick — every Tune position on the
  * reference machine settles here. */
-#define ER99_BD_BASE_HZ 49.0f
+#define W9_BD_BASE_HZ 49.0f
 /* Fitted against all four lettered Tune positions at once (24 windowed pitch
  * readings, total squared error 26 — about 1 Hz per point): the sweep rises
  * 4.6 Hz per ms of its time constant, and HOLDS for ~16 ms before falling —
  * C9's charge time, Fraser's "fixed attack time of the sweep". Without the
  * hold no exponential matches the recordings; the early readings sit far
  * above any curve that starts falling at t=0. */
-#ifndef ER99_BD_DF_PER_MS
-#define ER99_BD_DF_PER_MS 4.6f     /* sweep height per ms of tau */
+#ifndef W9_BD_DF_PER_MS
+#define W9_BD_DF_PER_MS 4.6f     /* sweep height per ms of tau */
 #endif
-#ifndef ER99_BD_PHOLD_MS
-#define ER99_BD_PHOLD_MS  16.0f    /* sweep hold before the fall */
+#ifndef W9_BD_PHOLD_MS
+#define W9_BD_PHOLD_MS  16.0f    /* sweep hold before the fall */
 #endif
 
-static inline void 9w9_bt_init(9w9_bt_t *v, const float _sr)
+static inline void w9_bt_init(w9_bt_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_osc_init(&v->osc,  _sr);
@@ -298,14 +298,14 @@
     v->noise_gated = 0;
     v->amp_hold_left = 0;
     v->bd_sweep = 0; v->bd_df = 0.0f; v->bd_mult = 0.0f; v->bd_phold = 0;
-    v->bd_base = ER99_BD_BASE_HZ;
+    v->bd_base = W9_BD_BASE_HZ;
     if(v->pitch_mod <= 0.0f) v->pitch_mod = 1.0f;
     v->out_gain = 0.0f;
     v->impulse = 0;
     v->mute_countdown = 0.0;
 }
 
-static inline void 9w9_bt_trigger(9w9_bt_t *v, const float _accent)
+static inline void w9_bt_trigger(w9_bt_t *v, const float _accent)
 {
     const float sr = v->sample_rate;
     const float ms = 0.001f * sr;
@@ -329,10 +329,10 @@
         const float m  = v->sweep_depth < 0.0f ? 0.0f
                        : (v->sweep_depth > 1.0f ? 1.0f : v->sweep_depth);
         const float pm = v->pitch_mod > 0.0f ? v->pitch_mod : 1.0f;
-        v->bd_base  = ER99_BD_BASE_HZ * (1.0f + m * (pm - 1.0f));
-        v->bd_df    = ER99_BD_DF_PER_MS * tau * (1.0f + 1.2f * m);
+        v->bd_base  = W9_BD_BASE_HZ * (1.0f + m * (pm - 1.0f));
+        v->bd_df    = W9_BD_DF_PER_MS * tau * (1.0f + 1.2f * m);
         v->bd_mult  = expf(-1.0f / (tau * ms));
-        v->bd_phold = (int)(ER99_BD_PHOLD_MS * ms);
+        v->bd_phold = (int)(W9_BD_PHOLD_MS * ms);
         wa_set_value(&v->pitch, v->bd_base);
     }
     else
@@ -370,7 +370,7 @@
     v->mute_countdown = (longest + 20.0f) * ms;
 }
 
-static inline float 9w9_bt_render(9w9_bt_t *v, const float _noise)
+static inline float w9_bt_render(w9_bt_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -394,7 +394,7 @@
      * continuously, which is what the junction does. Zeroing gain outright
      * (what this did first) cuts the shell mid-cycle and clicks. */
     if(v->tune2 > 0.0f && v->osc2_mix > 0.0f)
-        amp = amp > ER99_VCA_VT ? (amp - ER99_VCA_VT) * ER99_VCA_NORM : 0.0f;
+        amp = amp > W9_VCA_VT ? (amp - W9_VCA_VT) * W9_VCA_NORM : 0.0f;
 
     /* Oscillator: triangle, rounded toward sine by the diode pair. The pair
      * is ALWAYS in circuit (Whittle: conduction at ~0.5-0.6 V at the stock
@@ -408,7 +408,7 @@
          * and H2/H1 2-4% — the pair conducts slightly asymmetrically, and a
          * perfectly symmetric shaper produces none. The square term's DC goes
          * through the blocker below. */
-        o = 9w9_diode_round(o, 1.6f);
+        o = w9_diode_round(o, 1.6f);
         o += 0.05f * o * o;
         o = wa_biquad_tick(&v->dc_block, o);
     }
@@ -421,13 +421,13 @@
          * grow a ~120 Hz difference tone, a phantom pitch no 909 makes. The
          * user Drive stage after this still distorts the mix on purpose;
          * at low Drive it stays clean. */
-        o = 9w9_diode_round(o, 2.0f);
+        o = w9_diode_round(o, 2.0f);
         const float ratio = v->tune > 1.0f ? v->tune2 / v->tune : 1.0f;
-        o += 9w9_diode_round(wa_osc_triangle(&v->osc2, f * ratio), 2.0f)
+        o += w9_diode_round(wa_osc_triangle(&v->osc2, f * ratio), 2.0f)
            * v->osc2_mix;
         o /= (1.0f + v->osc2_mix);
     }
-    float body = 9w9_shape_st(o, v->drive, v->dist_type, v->crush_st) * amp;
+    float body = w9_shape_st(o, v->drive, v->dist_type, v->crush_st) * amp;
 
     /* Click: impulse + lowpass-filtered noise, own envelope (Attack). */
     float click = 0.0f;
@@ -454,15 +454,15 @@
         {
             /* Same junction, same soft cutoff — and once it is shut, stay shut
              * (the envelope only falls further). */
-            env = env > ER99_VCA_VT ? (env - ER99_VCA_VT) * ER99_VCA_NORM : 0.0f;
+            env = env > W9_VCA_VT ? (env - W9_VCA_VT) * W9_VCA_NORM : 0.0f;
             if(env <= 0.0f) v->noise_gated = 1;
         }
         snare = wa_biquad_tick(&v->noise_lpf,
                     wa_biquad_tick(&v->noise_hpf, _noise))
-              * env * v->snappy * ER99_SNAPPY_MIX;
+              * env * v->snappy * W9_SNAPPY_MIX;
     }
 
     return (body + click + snare) * v->out_gain;
 }
 
-#endif /* ER99_CIRCUIT_H */
+#endif /* W9_CIRCUIT_H */
```

### 9pin-9ren / 9w9_perc909.h

```diff
--- /tmp/cmp/9pin/src/dsp/er99_perc909.h
+++ /tmp/cmp/9ren/src/dsp/9w9_perc909.h
@@ -1,13 +1,5 @@
 /*
  * 9w9_perc909.h — circuit-informed TR-909 RIM SHOT and HAND CLAP.
- *
- * Replaces er-99's versions, which are not the target any more:
- *
- *  - er-99's rim is a single 220 Hz bandpass (its "parallel" topology only ever
- *    feeds filter 0 — the other two are wired to the output but nothing drives
- *    them), so it comes out dull and one-dimensional.
- *  - er-99's clap approximates the burst with five scheduled gain taps and no
- *    real delay line.
  *
  * The hardware:
  *
@@ -26,11 +18,11 @@
  * GPL-3.0.
  */
 
-#ifndef ER99_PERC909_H
-#define ER99_PERC909_H
+#ifndef W9_PERC909_H
+#define W9_PERC909_H
 
 #include "webaudio.h"
-#include "9w9_circuit.h"   /* 9w9_shape() */
+#include "9w9_circuit.h"   /* w9_shape() */
 
 /* ===================== RIM SHOT ===================== */
 typedef struct {
@@ -50,9 +42,9 @@
     double      mute_countdown;
     float       sample_rate;
     float       accent_gain_;
-} 9w9_rim909_t;
-
-static inline void 9w9_rim909_init(9w9_rim909_t *v, const float _sr)
+} w9_rim909_t;
+
+static inline void w9_rim909_init(w9_rim909_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune  > 0 ? v->tune  : 220.0f,
@@ -70,13 +62,13 @@
     v->accent_gain_ = 1.0f;
 }
 
-static inline void 9w9_rim909_retune(9w9_rim909_t *v)
+static inline void w9_rim909_retune(w9_rim909_t *v)
 {
     wa_biquad_set(&v->bp1, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
     wa_biquad_set(&v->bp2, WA_BANDPASS, v->tune2, v->res * 1.1f, 0.0f, v->sample_rate);
 }
 
-static inline void 9w9_rim909_trigger(9w9_rim909_t *v, const float _accent)
+static inline void w9_rim909_trigger(w9_rim909_t *v, const float _accent)
 {
     const float ms = 0.001f * v->sample_rate;
     wa_set_value(&v->amp, 1.0f);
@@ -86,7 +78,7 @@
     v->mute_countdown = (v->decay + 20.0f) * ms;
 }
 
-static inline float 9w9_rim909_render(9w9_rim909_t *v, const float _noise)
+static inline float w9_rim909_render(w9_rim909_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -113,7 +105,7 @@
      * had been living off that hiss, and lost most of its level when it went. */
     float y = wa_biquad_tick(&v->bp1, exc) * 60.0f
             + wa_biquad_tick(&v->bp2, exc) * 32.0f;
-    y = 9w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);
+    y = w9_shape_st(y * a, v->drive, v->dist_type, v->crush_st);
     y = wa_biquad_tick(&v->hp, y);
     return y * v->level * v->accent_gain_;
 }
@@ -138,9 +130,9 @@
     double      mute_countdown;
     float       sample_rate;
     float       accent_gain_;
-} 9w9_clap909_t;
-
-static inline void 9w9_clap909_init(9w9_clap909_t *v, const float _sr)
+} w9_clap909_t;
+
+static inline void w9_clap909_init(w9_clap909_t *v, const float _sr)
 {
     v->sample_rate = _sr;
     wa_biquad_set(&v->bp, WA_BANDPASS, v->tune > 0 ? v->tune : 1000.0f,
@@ -153,12 +145,12 @@
     v->accent_gain_ = 1.0f;
 }
 
-static inline void 9w9_clap909_retune(9w9_clap909_t *v)
+static inline void w9_clap909_retune(w9_clap909_t *v)
 {
     wa_biquad_set(&v->bp, WA_BANDPASS, v->tune, v->res, 0.0f, v->sample_rate);
 }
 
-static inline void 9w9_clap909_trigger(9w9_clap909_t *v, const float _accent)
+static inline void w9_clap909_trigger(w9_clap909_t *v, const float _accent)
 {
     const float ms = 0.001f * v->sample_rate;
     v->pulse_index = 0;
@@ -172,7 +164,7 @@
     v->mute_countdown = (v->tail_decay + v->spread * 4.0f + 40.0f) * ms;
 }
 
-static inline float 9w9_clap909_render(9w9_clap909_t *v, const float _noise)
+static inline float w9_clap909_render(w9_clap909_t *v, const float _noise)
 {
     if(v->mute_countdown <= 0.0) return 0.0f;
     v->mute_countdown -= 1.0;
@@ -205,9 +197,9 @@
 
     const float env = wa_param_tick(&v->burst) + wa_param_tick(&v->tail);
     float y = wa_biquad_tick(&v->bp, _noise) * env;
-    y = 9w9_shape_st(y, v->drive, v->dist_type, v->crush_st);
+    y = w9_shape_st(y, v->drive, v->dist_type, v->crush_st);
     y = wa_biquad_tick(&v->hp, y);
     return y * v->level * v->accent_gain_;
 }
 
-#endif /* ER99_PERC909_H */
+#endif /* W9_PERC909_H */
```

### 9pin-9ren / 9w9_tom909.h

```diff
--- /tmp/cmp/9pin/src/dsp/er99_tom909.h
+++ /tmp/cmp/9ren/src/dsp/9w9_tom909.h
@@ -41,16 +41,16 @@
  *
  * GPL-3.0.
  */
-#ifndef ER99_TOM909_H
-#define ER99_TOM909_H
+#ifndef W9_TOM909_H
+#define W9_TOM909_H
 
 #include "webaudio.h"
 #include "9w9_circuit.h"
 
 /* VCO2 : VCO1 : VCO3 from the timing capacitors above. */
-#define ER99_TOM_R1  1.00f
-#define ER99_TOM_R2  1.50f
-#define ER99_TOM_R3  2.75f
+#define W9_TOM_R1  1.00f
+#define W9_TOM_R2  1.50f
+#define W9_TOM_R3  2.75f
 
 typedef struct {
     wa_osc_t    osc[3];
@@ -64,9 +64,9 @@
     float       crush_st[2];
     double      mute_countdown;
     float       sample_rate;
-} 9w9_tom_t;
+} w9_tom_t;
 
-static inline void 9w9_tom909_init(9w9_tom_t *t, const float _sr)
+static inline void w9_tom909_init(w9_tom_t *t, const float _sr)
 {
     t->sample_rate = _sr;
     for(int i = 0; i < 3; ++i)
@@ -93,7 +93,7 @@
  * same Tune / Decay / Attack / Drive / Dist / Level the UI already has and no
  * new parameter has to exist for it.
  */
-static inline void 9w9_tom909_trigger(9w9_tom_t *t, const 9w9_bt_t *p,
+static inline void w9_tom909_trigger(w9_tom_t *t, const w9_bt_t *p,
                                        const float _accent)
 {
     const float sr = t->sample_rate;
@@ -131,7 +131,7 @@
     t->mute_countdown = (d1 + 60.0f) * ms;
 }
 
-static inline float 9w9_tom909_render(9w9_tom_t *t, const 9w9_bt_t *p,
+static inline float w9_tom909_render(w9_tom_t *t, const w9_bt_t *p,
                                        const float _noise)
 {
     if(t->mute_countdown <= 0.0) return 0.0f;
@@ -142,20 +142,20 @@
     /* Three oscillators at the board's ratios, each rounded by its own diode
      * pair and gated by its own envelope. */
     float o = 0.0f;
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[0], f * ER99_TOM_R1), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[0], f * W9_TOM_R1), 2.0f)
        * wa_param_tick(&t->env[0]);
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[1], f * ER99_TOM_R2), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[1], f * W9_TOM_R2), 2.0f)
        * wa_param_tick(&t->env[1]);
-    o += 9w9_diode_round(wa_osc_triangle(&t->osc[2], f * ER99_TOM_R3), 2.0f)
+    o += w9_diode_round(wa_osc_triangle(&t->osc[2], f * W9_TOM_R3), 2.0f)
        * wa_param_tick(&t->env[2]);
     o *= 0.55f;                       /* three partials summed, keep headroom */
 
     const float stick = wa_biquad_tick(&t->noise_bp, _noise)
                       * wa_param_tick(&t->noise_env) * t->noise_level * 1.5f;
 
-    float y = 9w9_shape_st(o + stick, p->drive, p->dist_type, t->crush_st);
+    float y = w9_shape_st(o + stick, p->drive, p->dist_type, t->crush_st);
     y = wa_biquad_tick(&t->dc_block, y);
     return y * t->out_gain;
 }
 
-#endif /* ER99_TOM909_H */
+#endif /* W9_TOM909_H */
```

### 9v271-9v272 / webaudio.h

```diff
--- /tmp/cmp/9v271/src/dsp/webaudio.h
+++ /tmp/cmp/9v272/src/dsp/webaudio.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * webaudio.h — a handful of W3C Web Audio API nodes, in C.
  *
```

### 9v271-9v272 / 9w9_circuit.h

```diff
--- /tmp/cmp/9v271/src/dsp/9w9_circuit.h
+++ /tmp/cmp/9v272/src/dsp/9w9_circuit.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_circuit.h — circuit-informed TR-909 voice models (BD / SD / toms).
  *
```

### 9v271-9v272 / 9w9_perc909.h

```diff
--- /tmp/cmp/9v271/src/dsp/9w9_perc909.h
+++ /tmp/cmp/9v272/src/dsp/9w9_perc909.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_perc909.h — circuit-informed TR-909 RIM SHOT and HAND CLAP.
  *
```

### 9v271-9v272 / 9w9_tom909.h

```diff
--- /tmp/cmp/9v271/src/dsp/9w9_tom909.h
+++ /tmp/cmp/9v272/src/dsp/9w9_tom909.h
@@ -1,3 +1,4 @@
+/* Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms: see NOTICE. */
 /*
  * 9w9_tom909.h — the TR-909's tom voice, from the voicing-board schematic.
  *
```
