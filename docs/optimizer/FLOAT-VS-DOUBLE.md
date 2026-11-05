# Float vs double на SMK-37 (DnBOX / X0X)

Дата: 2026-11-06 (Asia/Vladivostok UTC+10).  
База: worktree `/workspace/opt-fp/wt`, ветка `lane/optimizer-fp-20261106` @ `df4d3f0c5cfd9f6ef3d335bd675ffdb38725a08c` (без правок кода — дерево исходников не менялось).  
Метки: **MEASURED** / **ESTIMATE** / **ESTIMATE-TARGET** / **MEASURED-DEVICE** / **NOT_RUN** / **UNVERIFIED**.

## Краткий ответ владельцу

**Да: double дороже float на этом ядре.**  
Причина: FPU pi32v2/`-mfprev1` — **только single precision**; `double` идёт через **программную эмуляцию** (`__adddf3` и т.п. в `libcompiler_rt`), а не через аппаратную инструкцию. Поэтому firmware использует `float` + свой `fastmath.h` (без libm).  
В текущем audio path (drum909/808, fxbus, master, miami/chicago backends) **дорогих double уже нет** — чинить нечего. Device timing **NOT_RUN** (измерения — у владельца).

## 1. Hardware (SMK-37)

| Факт | Значение | Источник |
|---|---|---|
| SoC | JieLi **AC791N** (семья WL82 / «AC79»), тот же чип что FM-1 | `BUILDING.md:413`; публично: https://doc.zh-jieli.com/AC79/zh-cn/master/getting_started/overview/index.html |
| CPU | **pi32v2**, `-mcpu=r3` | `tools/build.py:58`; `tools/target_obj_check.sh:3,19` |
| FPU | **Hardware single-precision** (`-mfprev1`). Double — **software** (`__*df*`) | Repo: `firmware/src/dsp/fastmath.h:4-5`; `tools/build.py:56-58,523-528`. Публично: FAQ JieLi https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/toolchains/faqs/faq_float_point.html §4.1.4.1–2 |
| Циклы float mul/add | **UNVERIFIED** (в FAQ/overview нет таблицы latency) | FAQ выше; overview AC79 |
| Циклы soft-double | **UNVERIFIED** числом; качественно «медленнее и больше кода» | FAQ §4.1.4.1 п.3 |
| Clock | **MEASURED 360 MHz** на устройстве (sys); HSB 180, LSB 60. SDK max table 320 MHz | `firmware/hal/fm1_ble.h:31,42`; `docs/plans/2026-10-04-dual-core-and-clock.md:101-108` |
| SRAM (SDK) | **578 KB** on-chip (публичный overview) | AC79 overview URL выше |
| RAM map firmware | `.data/.bss` **96 KiB** @ `0x01C08000`; pool `0x01C20000` len `0x54000`; XIP app | `firmware/app.ld:16-27` |
| Flash map SMK-37 | app / BLE 64 KiB @ `0x8B000` / store / slots 80+120 KiB — allow-list | `firmware/hal/fm1_flash_map.h:25-55` |
| Cores | 2× pi32v2 | `firmware/hal/fm1_cpu1.h:2-5`; overview «双核DSP» |

**Вывод для audio:** любая `double`-операция в hot path = вызов soft-float из `libcompiler_rt`, не одна FPU-инструкция. Firmware **запрещает** soft-double в образе (`tools/build.py:523-526`; `BUILDING.md:404`).

## 2. Compile flags

### Firmware (device, `tools/build.py`)
- База: `-Os -ffunction-sections -fdata-sections -fno-builtin -Wall` — `tools/build.py:55`
- FPU: `-mcpu=r3 -mfprev1 -ffp-contract=off` — `tools/build.py:58`
- DSP/seq/engine: `-O2` вместо `-Os` (`O2_UNITS`) — `tools/build.py:64-66`
- Original synth: `-fno-fast-math -ffp-contract=off` (+ soft-FMA defines) — `tools/build.py:142-144`
- **Нет** `-lm` / libm в link app: только объекты + `ld --gc-sections` — `tools/build.py:476`
- Gate: soft-double в символах → ошибка сборки — `tools/build.py:523-526`
- Per-file check: `-Wdouble-promotion`, forbid `__*df*`, libm — `tools/target_obj_check.sh:19-38`

### Host tests
- Типично: `-O2 -ffp-contract=off` (+ часто `-Wdouble-promotion -Werror`) — `tests/run_tests.sh:294`; `host/build_host.sh:5,53`
- Original/host synth: `-fno-fast-math -ffp-contract=off` — `tests/host/original_build.sh`
- **Нет** повсеместного `-fsingle-precision-constant` в device flags (**не найдено** в `tools/build.py`).

## 3. Где задокументирован выбор float

| Место | Суть |
|---|---|
| `firmware/src/dsp/fastmath.h:2-8` | FPU float-only, no libm; host с `-ffp-contract=off` = те же сэмплы |
| `tools/build.py:56-58,523-528` | «AC79 single-precision FPU»; soft-double = баг |
| `BUILDING.md:404` | Сборка падает при soft-double |
| `firmware/src/dsp/drum909_dsp.h:3-4,12-14` | float only, no libm, no double; phase/pos вместо double 9W9 |
| `firmware/src/dsp/drum808.c:6-17` | 8W8 был double; порт устроен под float FPU |
| `docs/perf/engine-load-20261031.md:45-46` | нет `__adddf3`; FPU single |
| `BEHAVIOR.md` | bit-identical для других оптимизаций (не про float/double FPU) |

## 4. Аудит audio path на double (стоимость времени)

Метод на базе `df4d3f0c`:
1. `gcc -O2 -ffp-contract=off -fno-fast-math -fno-builtin -Wdouble-promotion -Wfloat-conversion` по `firmware/src/dsp/{drum909,drum808,fxbus,master,breaks,x0x_param,original_*}.c` и hot `experiments/original_synth/*` — **0 warning**.
2. `nm -u` host `.o` — **нет** `tanh`/`exp`/`sin`/… double libm; только табличные `x0x_tanh_*` у fxbus.
3. Target (JieLi clang 4.0.1, публичный toolchain): те же файлы `-target pi32v2 -mcpu=r3 -mfprev1 -O2 -fno-builtin -ffp-contract=off -Wdouble-promotion` — **0 warning**; `nm -u` — **нет** `__*df*` / libm.

| Класс | Находки в hot path | Горячий? |
|---|---|---|
| (a) double literals без `f` | Не найдены как promotions (warnings пусты) | — |
| (b) libm double (`tanh`/`exp`/…) | Нет; math = `fm_*` / таблицы | — |
| (c) `double` переменные/поля | В device с `ORIGINAL_USE_SOFT_FMA` MasterTune идёт integer-RNE (`chicago_tuning.c:53-54` + `master_tune_rne.h`), не FP double. `#else` ветка с `double` — **не device** | control, не sample |
| (d) printf/varargs promotion | В DSP hot path printf нет | — |

**Итог:** списка «hits с file:line для фикса» **нет**. Fix-коммиты **не делались**.

## 5. Fixes / bit-identical

Не применимо: нечего править.  
Если бы появился bare `1.0` в float-выражении: кандидат на `1.0f` → классификация **BIT-IDENTICAL** только после memcmp PCM+state на harness с `-O2 -ffp-contract=off -fno-fast-math -fno-builtin`. Сейчас **не доказано / не требуется**.

## 6. Ветка / bundle / bench

| | |
|---|---|
| Branch | `lane/optimizer-fp-20261106` |
| Tip SHA | `df4d3f0c5cfd9f6ef3d335bd675ffdb38725a08c` (= база; **0 fix commits**) |
| Bundle | `/workspace/dnbox/optimizer/push/fp.bundle` |
| tar.gz | `/workspace/fp-bundle.tar.gz` |
| Verify | `git bundle verify /workspace/dnbox/optimizer/push/fp.bundle` → ref tip выше, complete history |
| Host worstcase Δ | **NOT_RUN** для fp-lane (нет дельты кода). Базовый interleaved harness — см. `909-struct.md` / `WORSTCASE.md` |
| Micro host float vs double | **MEASURED-HOST** x86_64, `gcc -O2 -fno-builtin -ffp-contract=off -fno-fast-math`, N=40e6, taskset-c0 (3 прогона): tanhf≈20.2–21.2 ns, tanh≈39.1–40.1 ns, **ratio ≈1.9**; expf≈exp ≈12–13 ns; mulss/mulsd **невалидны** (цикл схлопнулся в 0 ns). **Не переносится на pi32v2**; device **NOT_RUN** |

## 7. Смысл для audio path

- Держать **только float** + `fm_*` / offline-таблицы — это не «микрооптимизация», а соответствие железу.
- Появление double в hot path = soft-libcall + раздувание кода; сборка должна упасть на soft-double gate.
- Дальнейший выигрыш — не «float вместо double» (уже сделано), а структурные lane’ы (tanh pair, idle skips, …) — см. `TARGET-ESTIMATE.md`.
