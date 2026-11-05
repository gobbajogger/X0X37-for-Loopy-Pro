# Маппинг upstream-репо → наши движки (добавлено 2026-11-06 ~02:15 VLAT)

Наше дерево: `/workspace/opt-909/repo` @ `263a055879f48919881f50812183aee3f4f2dc1e` (lane/optimizer-909, база v2). Upstream-клоны: `/workspace/upstream/*`.

| Repo | Что это | Наш движок | Доказательства |
|---|---|---|---|
| schwung-9W9 (канонический владелец по нашим README/LICENSING — athousanddetails; michaelmenesescs — копия с тем же пином) | 909 drum module (er99_*/9w9_* circuit, perc909) | **909** (`dsp/drum909*`, `dsp/fxbus*`) | LICENSING.md: «909 (`dsp/drum909*`, `dsp/fxbus*`) ported from 9W9 … after ER-99»; README «909 … Based on 9W9»; `x0x_param.h:5` «exactly like 9W9's panel» |
| schwung-8W8 (athousanddetails; копии michaelmenesescs, charlesvestal) | 808 circuit module (`sc808_*_circuit.h`, `SchmittBank`) | **808** (`dsp/drum808*`) | LICENSING.md «808 (`dsp/drum808*`) ported from 8W8»; `drum808.c:17` «8W8: double», `:820` «sc808_mt_circuit.h SchmittBank» |
| schwung-303 (charlesvestal; копия michaelmenesescs) | Open303 (Robin Schmidt, rosic_*) + Devilfish + drive | **никакой сейчас** (исторически — удалённый `bass303`) | README: «303 A and B — Open303 … Based on schwung-303» (устаревшая строка); BEHAVIOR.md BAS-07: «The 303 parts use only the Chicago and Miami engines. The bass303 engine … removed», остались только legacy ID в `dsp/bass_param_ids.h`; `rg rosic/Open303` в `firmware/src` — только комментарии (`chicago_env.h:7` «Open303 default», `seq/sequencer.h:25`). Chicago = `original_chicago_backend.c`, комментарии ссылаются на «Chicago Acid AU 3.1.6»; Miami = `original_miami_backend.c`, MIAMI-OPT.md «original Miami 3.1.6 Audio Unit». Ни один из них не содержит кода Open303/schwung-303 — по найденному. Происхождение Chicago/Miami (вендор AU) в дереве явно не указано → **UNKNOWN** кроме «реплика AU 3.1.6» |
| schwung-tb3po (charlesvestal; форк michaelmenesescs) | JS-модуль + `src/dsp/tb3po.c`: **генератор MIDI-паттернов** (порт Phazerville `TB_3PO`), звука не делает | **никакой из DSP-движков**; соответствует нашему **секвенсору** `firmware/src/seq/tb3po.{c,h}` | LICENSING.md «TB-3PO (`seq/tb3po*`) ported from schwung-tb3po»; `seq/tb3po.h:2` «A port of schwung-tb3po's generator», `tb3po.c:18` «xorshift32, as schwung-tb3po»; upstream README: «TB-3PO emits MIDI only; the squelch comes from your synth» |
| charlesvestal/schwung | хост-платформа (shim, UI, chain) | **никакой** (наша платформа — Felucca) | README «X0X is a fork of Felucca»; DSP-кода голосов в schwung нет |
| FX | — | наш `fxbus*` из 9W9; Chicago-вставки (`app/chicago_*.h`) — от Chicago AU | LICENSING.md (fxbus ← 9W9). Upstream FX-источник Chicago: UNKNOWN |

**schwung-303 vs schwung-tb3po:** разные проекты одного автора (Charles Vestal). 303 — синтезатор (C++ Open303 в `src/dsp/open303/`), tb3po — MIDI-генератор (`src/dsp/tb3po.c`, без rosic/Open303). Файлы не пересекаются, общей истории нет (merge-base не находится). tb3po README рекомендует ставить его *в пару* со schwung-303.

**tb3po == Miami? НЕТ.** tb3po — генератор нот → наш `seq/tb3po.c`. Miami — синтезаторный движок (`dsp/original_miami_backend.c`, wobble MG1, `miami_selected_voice.c`), реплика «Miami 3.1.6 AU»; кода schwung-tb3po или schwung-303 в нём нет. Значит, upstream-источника перф-оптимизаций для Miami/Chicago среди этих репо нет вообще.

---

# UPSTREAM-SEARCH — внешняя DSP-перф-работа (read-only)

Дата: 2026-11-06 ~02:20 VLAT (UTC+10). Метод: `git clone --filter=blob:none` всех веток в `/workspace/upstream/*`, локальный `git log --all` grep по subject+body (perf|optimi*|LUT|lookup|denormal|ftz|fixed-point|Q15/Q31|sinf/expf/powf|hoist|invariant|cache|coefficient|idle|silent|precompute|faster|cpu), просмотр diff веток вне main, WebFetch страниц PR/issues/forks. `gh` не залогинен; REST API unauth упёрся в rate limit после 3 запросов (forks частично через WebFetch). Ничего не пушили, PR/комментариев нет.

## Главный вывод
**NOT FOUND** по всем темам (909, 808, Miami, Chicago, FX). Ни в одном репо нет коммитов/PR/issues с перф-оптимизацией DSP голосов. Upstream-репо 9W9/8W8 — это sound-design/circuit-model работа; наш `drum909.c`/`drum808.c` — порт этих моделей (см. `x0x_param.h`, `tools/quickcheck.sh` ссылки на `schwung-9W9`/`schwung-8W8`). Всё, что есть у нас в 909.md / INVENTORY.md, — наше и новее upstream.

## По репозиториям
| Repo | Ветки | Коммитов | Пин / после пина | Verdict |
|---|---|---|---|---|
| michaelmenesescs/schwung-9W9 | main, feat/step-tap-vs-hold, voice-rebuild, mac-mini-backup | 107 | пин `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` (2026-09-19, v2.7.0) существует, на main. После пина в `src/`: только `8e9aebde0c40a4c017f640e5c30a4f70936bb595` (2026-10-11, rename er99→9w9; нормализованный code-diff: circuit.h 0, perc909.h 0, engine.c 5, engine.h 2, params.h 4 строки — переименование/state tag, не перф). `mac-mini-backup` `f240b842d507063c49330c77cfae50a026e57c94` = только vst/*.svg/skin. PR: 0. Forks: 0 | **NOT FOUND** |
| michaelmenesescs/schwung-8W8 | main, circuit-toms-clap-and-16-pads, mac-mini-backup | 68 | пин `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` = tip main. После пина `src/` не менялся. `mac-mini-backup` `86ffe3967f25fcc4d210901030503ef4ce399805` = vst/skin only. Issues: 0 | **NOT FOUND** |
| charlesvestal/schwung-8W8 | main | 67 | tip = `94aa2716…` (тот же пин). PR: 0, forks: 0 | **NOT FOUND** |
| charlesvestal/schwung-303, michaelmenesescs/schwung-303 | main (+1 у mm) | 22 / 41 | перф-коммитов нет (Open303 порт, drive) | **NOT FOUND** |
| charlesvestal/schwung-tb3po, michaelmenesescs/schwung-tb3po (fork, pushed 2026-10-18) | main, feature/dual-slot, mac-mini-backup | 52 / 99 | только UI/sequencer/release | **NOT FOUND** |
| charlesvestal/schwung (host) | 291 | 11090 | перф только host-уровня: `d3e50203f0d130c498f85640c5c155c5f8998854` (scenes: skip param lookup for engaged morph), PR #605 (`40292bb6895efe15e7dd043561b72482ffad2227` load modules off SPI callback), PR #140 (MIDI drain), `a0b36c0636d18625e88e7f59db8a0a257d6fd300` (resample bridge zero block). Не DSP голосов, к SMK-37 не применимо | **NOT FOUND (DSP)** |
| michaelmenesescs/schwung | — | — | clone FAIL (нет/приватный) | UNREACHABLE |

## Справочно (не перф-коммиты, а дизайн, уже у нас)
- 8W8 `5d526253a419ba29fda6961134e053ba179cb711`: один общий Schmitt metal bank тикает раз на сэмпл для всех metal-голосов ("cheaper than what it replaces"). **ALREADY IN**: `drum808.c` ~1706/1965 (общий bank, работает только пока звучит metal-голос — у нас ещё и idle-skip).
- 8W8 `c0191314edf19e5b44bb0b3716e4371f5f37815e`: control-rate частота sample-and-hold на блок (SC-семантика). Это поведение модели; у нас 909 hoist'ы (`daf1e281` и др.) — наш аналог. ALREADY IN по сути; bit-identity к нам не относится.
- 8W8 `0363f48c2aab7a5d2258b184f3eade170b50b841`: `pot/127.0f` вместо double — ради совпадения с engine, не перф. UNKNOWN relevance.

## Темы
- 909: NOT FOUND · 808: NOT FOUND · Miami: NOT FOUND (нет в upstream) · Chicago: NOT FOUND (нет в upstream) · FX: NOT FOUND (8W8/9W9 send FX без перф-коммитов).

## Floors
Нечего портировать → RAM/flash cost 0; host bit-identity тест не строился.

## Ограничения
REST API rate-limited (unauth): форки schwung-8W8(mm), schwung-303 через API не получены; WebFetch: forks 9W9 = 0, charlesvestal/8W8 = 0. Форки charlesvestal/schwung (host) не перебирались (не DSP голосов).
