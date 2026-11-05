# 909 optimizer — worst-case bench (MEASURED, host x86_64)

Дата: 2026-11-06 (Asia/Vladivostok UTC+10). Всё ниже — **MEASURED на host x86_64** (Intel Xeon, 8 CPU, box). **Device (AC79/SMK-37) ms — NOT_RUN** (нет device toolchain). Ничего не пушилось.

## Деревья (git worktree, detached, ветки не тронуты)

| Метка | Commit | Worktree |
|---|---|---|
| base | `a18561f17977be8592cef5a3d3cbebf70e4dbf1b` | `/workspace/opt/wt-909b-base` |
| tip~1 | `daf1e281` (BD hoist, без SMP skip-lerp) | `/workspace/opt/wt-909b-tipm1` |
| tip | `263a055879f48919881f50812183aee3f4f2dc1e` | `/workspace/opt/wt-909b-tip` |

Харнесс `tests/perf/` (worstcase_half_bench.c + run_worstcase_half_bench.py) скопирован без изменений из `/workspace/opt/wt-base-e9` (untracked).

## Компилятор и флаги

- `cc (Debian 14.2.0-19) 14.2.0`
- `-O2 -ffp-contract=off -fno-fast-math -std=c99 -DX0X_HOST` + `ORIGINAL_FLAGS`/`ORIGINAL_SOURCES` из `tests/host/original_build.sh` + **`-fno-builtin`** (через `X0X_HOST_DEFS`). Тот же -O2, что в WORSTCASE.md (там -fno-builtin не было).
- Сценарий: как в WORSTCASE.md, 128 halves/run (512 кадров/half, бюджет 11.6 ms), fixture `assets/factory-breaks/909GR.wav` (одинаковый абсолютный путь для всех трёх). Запуск `taskset -c 2`.
- 42 прогона на дерево, **чередуются** (порядок ротируется base/tip/tip~1).

## Аудио identity

`audio.i32` SHA-256 одинаков во всех 126 прогонах всех трёх деревьев: `2536d3a06a73…` (совпадает с base-e9/combined из WORSTCASE.md).

## Worst-case half (MEASURED host, N=42 каждое)

| Метрика | base | tip~1 | tip | Δ tip~1 vs base | Δ tip vs base | Δ tip vs tip~1 |
|---|---:|---:|---:|---:|---:|---:|
| Peak/half median, ms | 0.872 | 0.884 | 0.903 | +1.39% | **+3.57%** | +2.16% |
| Peak/half min, ms | 0.708 | 0.697 | 0.697 | −1.60% | −1.57% | +0.03% |
| Peak/half IQR (Q1–Q3), ms | 0.738–0.957 | 0.768–0.967 | 0.790–0.998 | | | |
| Peak/half max, ms | 3.185 | 2.110 | 2.492 | (выбросы планировщика) | | |
| Mean/half median, ms | 0.661 | 0.664 | 0.664 | +0.42% | +0.49% | +0.07% |
| Mean/half min, ms | 0.617 | 0.619 | 0.628 | +0.42% | +1.89% | +1.46% |
| Mean/half max, ms | 0.926 | 0.815 | 0.846 | | | |
| 909 stage (median avg ticks/half) | 291184 | 290679 | 292793 | −0.17% | +0.55% | +0.73% |
| Peak vs 11.6 ms бюджет (median) | 7.51% | 7.62% | 7.78% | | | |
| Deadline misses | 0 | 0 | 0 | | | |

Первая половина (N=21) отдельно: tip vs base peak median +3.93%, min −2.93%; mean median +1.96%; tip vs tip~1 peak median +3.35%.

**Вывод:** разброс peak (IQR ≈ 0.22 ms ≈ 25%) много больше любых Δ. В этом worst-case 909 ветка **не даёт измеримого улучшения** peak/mean half; медианы слегка хуже (+0.5% mean, +3.6% peak), min peak ≈ −1.6% — всё в пределах шума. Ожидание −0.8% (ESTIMATE из 909.md) не подтверждено и не опровергнуто на уровне шума.

## Integral sampler fractional path (263a0558)

Microbench из лейна: `tests/host/drum909_integral_test.c` (bench(), 9 чередующихся замеров, median ns/block 32 кадра), собран `-O2 -ffp-contract=off -fno-fast-math -fno-builtin`, `taskset -c 2`. Candidate = tip; ref = base или tip~1 (копия `run_drum909_integral.sh` с другим `BASE=`). Identity PASS 41600 calls во всех запусках. 3 запуска на ref. Лог: `909-bench/integral.log`.

| Путь | tip vs base (3 запуска) | tip vs tip~1 (3 запуска) — эффект только 263a0558 |
|---|---|---|
| samplers fractional | +5.90%, +5.33%, +6.00% | −12.18% (шумный запуск, ref 1484 ns), +4.14%, +1.61% |
| samplers integral | −6.72%, −7.53%, −11.58% | −5.63%, −9.30%, −8.20% |
| dense integral | +0.32%, −0.40%, +1.26% | −4.72%, −0.36%, +0.06% |
| dense fractional | +1.52%, +0.97%, −1.79% | −0.92%, −0.87%, +0.07% |

Fractional sampler path: против base стабильно **≈ +5–6%**; изолированно 263a0558 (vs tip~1) — **+1.6…+4.1%** в двух чистых запусках (третий зашумлён). Соответствует ранее заявленным ≈ +3%.

Эффект 263a0558 в полном worst-case (tip vs tip~1): peak median +2.16%, min +0.03%; mean median +0.07%; 909 stage +0.73% — **в пределах шума**, измеримого эффекта нет.

## Воспроизведение

```sh
cd /workspace/opt-909/repo
for p in base:a18561f1 tip:263a0558 tipm1:daf1e281; do n=${p%%:*}; c=${p#*:}
  git worktree add --detach /workspace/opt/wt-909b-$n $c
  cp -r /workspace/opt/wt-base-e9/tests/perf /workspace/opt/wt-909b-$n/tests/
  (cd /workspace/opt/wt-909b-$n && X0X_HOST_DEFS=-fno-builtin python3 tests/perf/run_worstcase_half_bench.py \
     --output /workspace/dnbox/optimizer/909-bench/$n --fixture /workspace/opt/wt-909b-base/assets/factory-breaks/909GR.wav --blocks 128 --rounds 1)
done
cd /workspace/dnbox/optimizer/909-bench
S=0 N=21 ./run_interleaved.sh && S=21 N=21 ./run_interleaved.sh && python3 stats.py
# fractional microbench (из /workspace/opt/wt-909b-tip):
OPT_LEVELS=O2 CC="cc -fno-builtin" taskset -c 2 sh tests/host/zz_integ_base.sh    # ref = a18561f1
OPT_LEVELS=O2 CC="cc -fno-builtin" taskset -c 2 sh tests/host/zz_integ_tipm1.sh   # ref = daf1e281
```

Артефакты: `/workspace/dnbox/optimizer/909-bench/{base,tip,tipm1}/r*/row.json`, `stats.txt`, `integral.log`, `run_interleaved.sh`, `stats.py`.
