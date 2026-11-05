# Оценка стоимости на цели SMK-37 (pi32v2) — PRIMARY

Дата: 2026-11-06 (Asia/Vladivostok UTC+10).  
Цель: **оптимизировать под DEVICE**, не под host PC.  
Метки: **ESTIMATE-TARGET** (арифметика из источников) · **UNVERIFIED** (нет цифры в источнике) · **MEASURED-HOST** (вторично) · **MEASURED-DEVICE** · **NOT_RUN**.

Toolchain: публичный JieLi clang 4.0.1 (`tools/get_toolchain.sh` → `https://pkgman.jieliapp.com/s/linux-toolchain`), установлен в `/workspace/toolchain`. Быстрая проверка: hot `.o` собраны с device flags; insn counts ниже — из `objdump` целевого объекта (это **число инструкций**, не доказанные циклы).

## A. Архитектура: стоимость операций

Источники: JieLi float FAQ https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/toolchains/faqs/faq_float_point.html ; AC79 overview https://doc.zh-jieli.com/AC79/zh-cn/master/getting_started/overview/index.html ; repo `firmware/src/dsp/fastmath.h:4-5`, `tools/build.py:56-58`, `docs/plans/2026-10-04-dual-core-and-clock.md:101-108`, `firmware/app.ld`, `firmware/hal/fm1_perf.h`.

| Операция | Что говорит источник | Метка |
|---|---|---|
| float +/−/× (r3 + `-mfprev1`) | «одна аппаратная инструкция» (FAQ §4.1.4.1 п.2) | качественно **ESTIMATE-TARGET**: 1 insn/op; **циклы/insn UNVERIFIED** |
| float ÷, sqrt | В overview есть «математический ускоритель»; **latency UNVERIFIED**. В firmware hot path ÷/sqrt — через `fm_*` (много insn), не libm | **UNVERIFIED** cycles; см. insn `fm_expf` ниже |
| double +/−/×/÷ | Software (`__adddf3` и др. в libcompiler_rt); «медленнее и больше кода» (FAQ §4.1.4.1 п.3) | **ESTIMATE-TARGET**: ≫ 1 HW float insn; **кратность UNVERIFIED** |
| libm `tanh`/`exp`/… (float или double) | Firmware **не линкует libm** (`tools/build.py` link без `-lm`; `target_obj_check.sh` forbid). Math = `fastmath.h` / таблицы | вызов libm в DSP = **запрещён / не используется** |
| Load из Flash (XIP) vs RAM | Код в XIP (`app.ld:4-5,20`); hot `.ram_text` только flash-driver. I$/D$ есть (overview). **Wait states / miss penalty UNVERIFIED** | **UNVERIFIED** |
| Clock | **MEASURED** 360 MHz sys на устройстве | `fm1_ble.h:42`; dual-core-and-clock.md:105 |
| Бюджет сэмпла | ESTIMATE: `360e6/44100 ≈ 8163` циклов/сэмпл @360 MHz (арифметика); план также даёт 7256@320 / 5442@240 | `docs/plans/2026-10-04-performance.md:59` + clock 360 |

**Правило ранжирования:** сначала target-cost (пропуск работы / уменьшение insn в hot loop / уход от soft-double). Host ns — только вторичный сигнал (**MEASURED-HOST**), сам по себе **не засчитывается**.

## B. Целевая сборка hot-функций (insn count)

Флаги: `-target pi32v2 -mcpu=r3 -mfprev1 -O2 -fno-builtin -ffp-contract=off -Wdouble-promotion` @ `df4d3f0c`.  
Метка: **ESTIMATE-TARGET (insn из целевого asm)**; циклы = insn × CPI с **CPI UNVERIFIED**.

| Символ | Insn | Примечание |
|---|---:|---|
| `d9_tanh` | 153 | packed table + run search (пара значений — суть `df4d3f0c`) |
| `d9_render` | 2468 | главный per-block 909 |
| `d9_emit` | 68 | |
| `d9_diode20` | 8 | |
| `fm_expf` | 75 | soft float math, не libm |
| `d9_env_exp` | 226 | |
| soft-double / libm в UNDEF | **0** | `nm -u` drum909/808/fxbus/master/breaks |

`x0x_tanh_value` / `d9_tanh_pair` как отдельные символы **не** вынесены (логика внутри `d9_tanh` / inline).

## C. Rank: float-vs-double + существующие lane’ы

Сортировка: **target-cost first**. Host — в скобках, вторично.

| # | Идея / lane | Tip / SHA | Target-cost аргумент | Host (вторично) | Вердикт target |
|---|---|---|---|---|---|
| 0 | **Запрет double / уже float-only audio** | политика @ tip; gate `build.py:523` | Любой double в hot path → soft-libcall ≫ HW float (**ESTIMATE-TARGET**, кратность **UNVERIFIED**). Сейчас hits = 0 | MEASURED-HOST tanh/tanhf ≈1.9× на x86 — **не переносится** | **Уже сделано**; fp-fix commits = 0 |
| 1 | **`df4d3f0c` d9_tanh_pair** | `df4d3f0c5cfd9f6ef3d335bd675ffdb38725a08c` `lane/optimizer-909-struct-20261106` (**на remote**) | ~10× `d9_tanh`/sample/voice; было 2× run-search+unpack на вызов → 1 search + 1×8 B load. **ESTIMATE-TARGET:** −(стоимость одного run-scan ≈ O(9) сравнений + лишний unpack) × ~10/sample. Insn body 153 (asm). I-cache: меньше кода на два lookup | MEASURED-HOST worstcase 42+42: peak median −8%, 909 stage −24% (`909-struct.md`) | **Высокий приоритет device** (горячий 909) |
| 2 | **`daf1e281` BD pitch-hold hoist** | `daf1e281a93c581fe564c749d42e3aeac0fdcf2f` | Убрать повторный recalc inc на hold-сегменте BD. **ESTIMATE-TARGET:** −N float/int ops × samples_hold/block только когда pitch hold | MEASURED-HOST BD hold −7.6% ns (`909.md`) | Средний; узкий путь |
| 3 | **Idle / silence skips** | `lane/optimizer-idle-presets-20261106` tip remote `73e8f517…` / local `ccb368f0…`; idle `b186be6c` | Пропуск settled-zero master/FX/Chicago inserts / BREAK clear. **ESTIMATE-TARGET:** ≈0 работы на тишине vs полный DSP; выигрыш ∝ доле тишины (на worstcase dense → мал) | MEASURED-HOST combined peak −8.9% vs e9e6643e (`WORSTCASE.md`) — смесь lane’ов | Высокий на реальной игре; низкий на dense PERF TEST |
| 4 | **Engines / fxbus unity skip** | remote `lane/optimizer-engines-20261106` @ `758c8e0d…` | Пропуск mono unity-volume pass. **ESTIMATE-TARGET:** −один buffer walk/block когда volume=1 | host bit-id prior | Средний |
| 5 | **`8081c00d` latency clip raw notes** | `8081c00d2552f0d5ba42e4e27485d54ff25f9066` `lane/optimizer-latency-20261106` | Не audio DSP: пропуск pitch stages на clip keys. **ESTIMATE-TARGET:** −control path, не half-render | host bit-id | Низкий для CPU half; полезен для latency UX |
| 6 | **`263a0558` SMP skip-lerp** | `263a055879f48919881f50812183aee3f4f2dc1e` `lane/optimizer-909-20261106` | Меньше работы при frac=0; fractional path может **дороже**. **ESTIMATE-TARGET:** знак зависит от доли integer vs frac hits | MEASURED-HOST frac **+3…+6%**; worstcase в шуме (`909-bench.md`) | **Осторожно** — возможен регресс на цели |
| 7 | **IMA LUT / chicago coef / storage** | port bundles в `REPORT.md` | Реже decode/coef rewrite / LZ scan. **ESTIMATE-TARGET:** вторично к 909/808 peak | host PASS | Средний/низкий для worst half |
| 8 | **Новый float←double fix в audio** | — | Hits = 0 @ `df4d3f0c` | — | **N/A** |
| 3b | **`d0d311f3` zl-silence-v2** | `d0d311f3a6013c4380bc003a883c0d1e36aafe3a` `lane/optimizer-zl-silence-v2-20261106` (есть в `/workspace/opt-zl-silence-v2`, parent `b186be6c`) | `chi_fx2_insert_advance_zero`: порог 128→64 + word-scan/finite guards. **Compiled** insn `measure_advance_zero` 373→573; compact `advance_zero` 465→683 (больше код skip-path; выигрыш = пропуск полного insert process на тишине). CPI **UNVERIFIED** | MEASURED-HOST microbench в `zl-silence.md` | Высокий на тишине; на dense — мал |

### Арифметика примера (tanh pair) — ESTIMATE-TARGET

Пусть на dense 909: ~10 вызовов `d9_tanh` / sample / активный голос (профиль host, `909-struct.md`).  
До: 2× `x0x_tanh_value` ≈ 2×(линейный поиск ≤9 run + unpack).  
После `df4d3f0c`: 1× поиск + одно 8-байтное чтение двух entries.  
Экономия ≈ **1 поиск/run-scan на вызов** × 10 / sample.  
Доля 909 в worst half ~46% (host stage share, `WORSTCASE.md`) → ожидаемый вклад в half **заметный**, но **CPI и cache UNVERIFIED** → диапазон для device: ориентир **−5…−25% стадии PR_909** (широко из-за UNVERIFIED CPI); peak half уже, если 909 доминирует.  
Host −24% 909 stage — **MEASURED-HOST**, не proof device.

### Soft-double vs float (если бы double вернули)

FAQ: float = 1 HW insn; double = soft routine.  
**ESTIMATE-TARGET:** даже одна soft-double add на sample × 512 × 2 renders/half ≫ бюджет, если soft ≫ HW (кратность **UNVERIFIED**, поэтому gate «ноль soft-double в образе»).

## D. Что не засчитывать

- Любой **MEASURED-HOST** speedup без device / без target insn-аргумента.
- Microbench mulss/mulsd на x86 (цикл оптимизирован в 0 ns) — мусор.
- Cycle counts, которых нет в datasheet/ISA — только **UNVERIFIED**.

## E. Toolchain

- Есть: публичный JieLi **clang 4.0.1** target `pi32v2` (`get_toolchain.sh`). Не gcc upstream.
- Альтернатива community: `pi32v2-isa` / LLVM fork (wiki) — не ставили глубоко; для этого отчёта достаточно JieLi.
- Device wall-time: **NOT_RUN** здесь; см. `OWNER-MEASURE-KIT.md`.

## F. Compiled vs estimate (insn, 2026-11-06 VLAT)

**Метод счёта:** `objdump -d --section=.text.SYM` (JieLi `common/bin/objdump`), число строк вида `^\s+[0-9a-f]+:` = одна инструкция на адресную строку (включая `<unknown instruction>`). Только тела функций; без `.rela`.  
**Флаги:** как `tools/build.py` DSP/`ORIGINAL_SYNTH_O2_UNITS`: `-target pi32v2 -mcpu=r3 -mfprev1 -O2 -fno-builtin -ffp-contract=off -ffunction-sections` + `-idirafter …/pi32v2/include`.  
**Toolchain:** `https://pkgman.jieliapp.com/s/linux-toolchain` → пакет `jieli-linux-toolchains-20260730.1`; `clang --version` → **clang version 4.0.1 () ()** (InstalledDir `…/pi32v2/bin`).  
**CPI / циклы: UNVERIFIED** — ниже только insn, не изобретать cycles.

### Расхождения ESTIMATE-TARGET (секция B) ↔ compiled

| Символ | ESTIMATE-TARGET | Compiled | Δ |
|---|---:|---:|---:|
| `d9_render` | 2468 | **2367** | −101 |
| `d9_env_exp` | 226 | **217** | −9 |
| `fm_expf` (в `drum909.o`) | 75 | **66** | −9 |

Совпало (не дельта): `d9_tanh` 153, `d9_emit` 68, `d9_diode20` 8, soft-double/libm UNDEF **0**.

### Дополнительные compiled counts (не было численного ESTIMATE в B)

| Предмет | Before → After | Примечание |
|---|---|---|
| `d9_tanh` (`daf1e281` → `df4d3f0c`) | **98 → 153** | `d9_tanh_pair` static inline внутрь `d9_tanh`; отдельного `.text.d9_tanh_pair` нет |
| `d9_tanh` в `fxbus.o` | **93 → 146** | своя weak/gnu_inline копия |
| `x0x_tanh_value` call sites (исходники) | 2× в `d9_tanh` → 2× только в fallback `d9_tanh_pair` | символ inline, в obj нет; hot path после pair — один run-search + 8 B load |
| Голоса 909 (outlined `-fno-inline-functions`, полные тела; в production `-O2` встроены в `d9_render`, кроме clap) | BD **172**, SD **152**, tom **167**, RS(rim) **162**, CP **124**, smp(OH/RD/CR/CH) **138** | heuristic back-edge ≈ per-sample: BD 159 / SD 146 / tom 158 / rim 130 / clap 75 / smp 94 |
| `d9_render_clap` production | **348** | отдельная секция |
| zl-silence `measure_advance_zero` (`b186be6c` → `d0d311f3`) | **373 → 573** | forced noinline wrapper над `chi_fx2_insert_advance_zero` |
| zl-silence compact `.text.advance_zero` | **465 → 683** | |

### Soft-double / soft-libm в hot `.o`

`nm -u` + поиск `__*df*` / libm-имён: **пусто** в `drum909.o`, `drum808.o`, `breaks.o`, `master.o`, `fxbus.o`, `chicago_fx2_compact.o` (d0d311f3). Вызовов `__adddf3`/…/`tanh`/`expf` (libm) нет.
