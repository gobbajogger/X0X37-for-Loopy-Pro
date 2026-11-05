# OWNER-MEASURE-KIT — один прогон на SMK-37

Дата: 2026-11-06 (Asia/Vladivostok UTC+10). Одна страница. Цель: **одно A/B на устройстве** расставляет lane’ы; host не решает.

## Что прошить

Уже на remote (ls-remote), без fp-fix commits:

| Метка | Branch | Full SHA | Роль |
|---|---|---|---|
| **B0 база** | `lane/build-25-m1-port-v2-20261106` (или тот pin, с которого собирает DnBOX Build) | `e9e6643e612b9fb4e968a77d016f3bb24213dc88` | контроль |
| **A1 909-struct** | `lane/optimizer-909-struct-20261106` | `df4d3f0c5cfd9f6ef3d335bd675ffdb38725a08c` | d9_tanh_pair (главный audio-кандидат) |
| **A2 909** | `lane/optimizer-909-20261106` | `263a055879f48919881f50812183aee3f4f2dc1e` | BD hoist + SMP skip-lerp |
| **A3 idle-presets** | `lane/optimizer-idle-presets-20261106` | remote tip `73e8f517a9a57c94ff380f46f078799b82b5a9de` (local tip может быть впереди — сверять Build) | silence skips |
| **A4 latency** | `lane/optimizer-latency-20261106` | `8081c00d2552f0d5ba42e4e27485d54ff25f9066` | не DSP half |
| **A5 engines** | `lane/optimizer-engines-20261106` | `758c8e0d3e0bebad0e8113bc4345dee6b6a01d9e` | fxbus unity skip |

**Комбинированный образ** (все lane’ы сразу): локальный `49410097…` / `lane/optimizer-combined-20261106` **не remote**. Нужен **DnBOX Build**: собрать Elite/Pro `BOARD=smk37` из выбранного merge SHA, выдать `.fwsc` + записать точный SHA в отчёт. Без Build — только одиночные A1…A5 vs B0.

`lane/optimizer-fp-20261106` = тот же `df4d3f0c` (**нет** новых коммитов) — отдельно не прошивать.

## Что читать на устройстве (только существующее)

1. **Экран PERF** + **RUN PERF TEST** (~15 s): `firmware/src/app/ui.c:2182-2226,2475-2481,5976+`; стадии из `eng_prof[]`.
2. Счётчики стадий: `firmware/src/app/engine.h:177-178`  
   `PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ` + `eng_prof_frames`.  
   Накопление: `engine.c:24-28` `PROF_ADD` через `plat_cycles()` (`plat.h:138-140`).
3. Опционально железо: `fm1_perf.h:5-24` `C0_TL_CKCNTL` / `fm1_perf_cycles()` — **«Not verified on hardware»** в комментарии; если `fm1_perf_cycles_run()` ложен, UI уже падает на TIMER4. **Не включать** stall DBG без нужды (`fm1_perf_stalls_enable`).
4. **Не выдумывать** новые счётчики / UART-пробы, которых нет в дереве.

Записать с PERF: % CPU total, % по стадиям (×10 как в UI), факт dropout/overrun если показывает тест.

## Ожидания (ESTIMATE-TARGET ranges)

Циклы/insn **UNVERIFIED** → диапазоны широкие; это гипотезы до вашего замера.

| Сравнение | Ожидание на PERF TEST (dense) | Ожидание на «живой» тишине |
|---|---|---|
| A1 vs B0 | **PR_909 −5…−25%**; peak half заметнее, если 909 доминирует | слабее, если 909 молчит |
| A2 vs B0 / vs A1 | PR_909 малый Δ; возможен **плюс** на frac SMP | ≈0 |
| A3 vs B0 | dense ≈0…−5% peak; | **MASTER/FX/303** сильно вниз на idle |
| A5 vs B0 | малый − на FX при unity | малый |
| A4 vs B0 | half ≈0; смотреть latency UX, не % | — |
| float←double fix | **N/A** (hits=0) | — |

Host ns **не** подставлять как ms устройства.

## Правило решения (один прогон)

1. Прошить **B0**, RUN PERF TEST, записать стадии.  
2. Прошить **A1**, тот же тест, тот же паттерн/громкости.  
3. **Оставить A1**, если PR_909 упал ≥ **10%** относительно B0 **или** total CPU/peak half упал ≥ **5%** без роста другой стадии ≥ того же.  
4. **Отбросить A1**, если Δ внутри шума (|Δ|<3% total) или PR_909 вырос.  
5. Далее A3 (idle) тем же правилом на **idle-сценарии** (не только PERF TEST).  
6. A2 принимать только если не ухудшает A1 на dense.  
7. A4/A5 — принимать по своим метрикам (latency / FX), не по peak half, если half Δ <3%.  
8. Комбинированный билд (Build) — только после выбора победителей; один финальный PERF TEST vs B0.

Метка результата: **MEASURED-DEVICE**. Всё до этого — ESTIMATE-TARGET / MEASURED-HOST.
