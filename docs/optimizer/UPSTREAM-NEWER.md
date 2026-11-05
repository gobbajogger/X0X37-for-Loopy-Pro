# UPSTREAM-NEWER — более новые upstream-версии относительно наших портов

Дата: 2026-11-06 ~02:40 VLAT (UTC+10). Read-only: ничего не пушили. Клоны `--filter=blob:none` в `/workspace/upstream/*` (fetch --all), сравнение деревьев в `/tmp/cmp`. Для 8W8 собран host golden render (g++ 14 поставлен на box).

## Итог
- **9W9 → NOT FOUND** (новых DSP-изменений нет; только rename, совместимость state-тега, копирайт-шапки).
- **8W8 → NOT FOUND** (v1.5.1 — рефактор/переименование sc808→w8, golden render **бит-в-бит** совпал с пином).
- **303 → NOT FOUND для нас** (одна реальная перф-правка есть, но только в форке michaelmenesescs и только для Open303/bass303, который у нас удалён).
- **tb3po → NOT FOUND** (после базы только лицензия/релиз).
- ⚠ **Лицензионный флаг:** athousanddetails опубликовал 9W9 v2.7.2 и 8W8 v1.5.1 под «GPL-3.0 with additional terms (section 7): attribution» (файл NOTICE). Наш порт сделан с пинов ДО этих терминов; если будем брать что-то из новых версий — учесть NOTICE. Решение за владельцем.

## 1. Базы наших портов
Источник: `evidence/smc-mixer-ui-popup-stage4/environment.json` → `ref_heads` (refs, с которыми гоняются наши host-тесты 909/808/303/tb3po; `tools/quickcheck.sh` использует `$REFS/schwung-*`). Плюс LICENSING.md/README (порт из 9W9, 8W8, schwung-303, schwung-tb3po). В нашем git первый коммит, добавивший drum909/drum808/tb3po: `2377537582b8d9a236edde8c2bf4e9ae83f4792c` (2026-10-03, импорт целиком, без upstream SHA в сообщении).

| Upstream | База (full SHA) | Что это | Проверка | Уверенность |
|---|---|---|---|---|
| schwung-9W9 | `10cbe5c1e1c9e65b0818bb34644ab1a13092082c` | 2026-09-19 «chore: update release.json for v2.7.0» | есть в michaelmenesescs/schwung-9W9 main; в athousanddetails (история переписана) объекта нет | высокая (ref_heads); побайтное сравнение drum909↔er99 не делалось (порт C→наш стиль) |
| schwung-8W8 | `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997` | 2026-09-19 «Add Screen Reader support» | tip main у charlesvestal и michaelmenesescs | высокая (ref_heads) |
| schwung-303 | `7680578cdecd91e512cfddb9c9eb07b287fb64f4` | 2026-08-31 «release: v0.3.3» | есть в charlesvestal и michaelmenesescs | средняя: это ref тестов; bass303 у нас удалён (BEHAVIOR.md BAS-07) |
| schwung-tb3po | `f49d81d05d1e29ae37728a08e30f193892be3bc9` | 2026-09-01 «release: v0.3.1» | есть в обоих; `src/dsp` tree `bc53573c…` | высокая (ref_heads + комментарии `seq/tb3po.c`) |

## 2. Что достижимо
- athousanddetails/schwung-9W9 (канон, 2 коммита, только main), athousanddetails/schwung-8W8 (1 коммит) — **история переписана/сквошнута**, пинов в них нет; сравнивал деревьями.
- michaelmenesescs/{9W9,8W8,303,tb3po}, charlesvestal/{8W8,303,tb3po} — все ветки получены.
- Forks (WebFetch): 9W9 (mm) 0, charlesvestal/8W8 0; charlesvestal/tb3po → только michaelmenesescs. Списки форков athousanddetails/*, charlesvestal/schwung-303, mm/8W8 через API — **не получены** (unauth rate limit). michaelmenesescs/schwung — недоступен.
- Ветки, не содержащие базу: 9W9 `feat/step-tap-vs-hold`, `voice-rebuild`; 8W8 `circuit-toms-clap-and-16-pads`; tb3po `feature/dual-slot` — все **предки** базы/main (уже влиты), нового не несут.

## 3. Все коммиты после базы
| Repo | Full SHA | Дата | Что изменено | Вердикт | Бит-идентично нам | Floors |
|---|---|---|---|---|---|---|
| michaelmenesescs/schwung-9W9 main | `8e9aebde0c40a4c017f640e5c30a4f70936bb595` | 2026-10-11 | rename er99→9w9, state tag 9w9v3 (старый грузится), удалён webaudio renderer | irrelevant (rename) | YES для DSP (нормализованный diff: circuit.h/perc909.h 0 строк; engine.c/h, params.h — только тег/имена) | 0 |
| michaelmenesescs/schwung-9W9 mac-mini-backup | `f240b842d507063c49330c77cfae50a026e57c94` | 2026-10-18 | vst/*.svg, skin.css | irrelevant (skin) | n/a | n/a |
| athousanddetails/schwung-9W9 | `25e03ddc33c2a4dfff702927061381a6e8bd6a3d` | 2026-10-27 | v2.7.1: vs `8e9aebde` в src отличаются только `9w9_engine.c/h` (`w9_state_tag_ok`: принимает любой короткий `*v3;` тег), module.json, loadtest | bugfix (state load), не DSP | YES (аудио-путь не тронут) | ~+100 B text, 0 RAM; у нас свой формат проекта → неприменимо |
| athousanddetails/schwung-9W9 | `a9e030d9bad46e3e53aafce92c7104b8e7bba47f` | 2026-10-27 | v2.7.2: копирайт-строка в каждый файл, NOTICE с доп. условиями GPL §7, help.json | irrelevant (лицензия) ⚠ | YES | 0 |
| michaelmenesescs/schwung-8W8 main/mac-mini-backup | `86ffe3967f25fcc4d210901030503ef4ce399805` | 2026-10-18 | vst skin | irrelevant | n/a | n/a |
| athousanddetails/schwung-8W8 | `1dbef27e13a6c63cb1fc078dd9b4d86172237c51` | 2026-10-27 | v1.5.1: sc808_*→w8_*, `sc_ugens.h`→`w8_dsp.h` (RGen→NoiseGen, frand2→white — тот же taus88 и та же бит-формула), `sc808_voices.h` (SC-транскрипция) удалён, rim вынесен в `w8_rim.h`; NOTICE с доп. условиями | irrelevant (рефактор/лицензия) ⚠ | **YES (MEASURED host)**: `tools/golden_check --write` с обеих версий (g++ -O2, x86_64) → 34 строки, `cmp` идентичны; `tools/golden.txt` в обеих тоже идентичны | 0 (нет новых состояний) |
| charlesvestal/schwung-303 main | `1c8e20381c1f23cbe20c618f248ac173b72d913e` | 2026-11-02 | LICENSE/NOTICE/README/build.sh | irrelevant | n/a | n/a |
| michaelmenesescs/schwung-303 main=mac-mini-backup | `f574a78861a9484ab39ffc26c86067d00c1a419e` | 2026-10-18 | skin + **`src/dsp/plugin.cpp`: `setTanhShaperDrive` (перестройка таблицы ~0.7 ms) применяется только при изменении и не чаще раза в 1024 кадра** | **optimization** (только Open303-обёртка) | NO (задержка применения drive до 1024 кадров меняет звук на свипе) | n/a: у нас нет bass303/Open303 (BAS-07) → неприменимо |
| michaelmenesescs/schwung-303 | `ccc2f1fe…`…`6c426a29…` (20 коммитов, см. `git log ^7680578c origin/main`) | 04–08.2026 | переписанная копия той же истории (src tree `ccc2f1fe`=`8a02f2ff`= tree базы) | irrelevant (дубли) | YES (тот же tree) | — |
| charlesvestal/schwung-tb3po main | `c5d6e1df27e66c35d5653ca9a5a9d99a2a16536b` | 2026-11-02 | build.sh, module.json (лицензия в tarball) | irrelevant | n/a | n/a |
| michaelmenesescs/schwung-tb3po mac-mini-backup | `efbc26c011a65012ebac6f1d931024f557d29bd4` | 2026-09-01 | release.json | irrelevant | YES (`src/dsp` tree = базе `bc53573c`) | — |
| michaelmenesescs/schwung-tb3po | 44 переписанных коммита (`474d6784…`…`c946cdaf…`) | 04–09.2026 | дубли истории charlesvestal с другими SHA | irrelevant | YES (`474d6784:src/dsp` = база) | — |

## 4. Действия
Нечего портировать. Единственная перф-правка (`f574a788`, shaper throttle) касается удалённого Open303. Владельцу: учесть новые доп. условия NOTICE у 9W9 v2.7.2 / 8W8 v1.5.1.

## 5. Дополнение 2026-11-06 ~02:55 VLAT: форки/теги без API
- athousanddetails/schwung-9W9: `git fetch --all --tags` → `main`, `v2.7.1` (`25e03ddc33c2a4dfff702927061381a6e8bd6a3d`), `v2.7.2` (`a9e030d9bad46e3e53aafce92c7104b8e7bba47f`); github.com/…/forks — форков нет. Upstream render.c база vs v2.7.2: 12/12 WAV **бит-в-бит** (gcc -O2).
- athousanddetails/schwung-8W8: `main`, `v1.5.1` (`1dbef27e13a6c63cb1fc078dd9b4d86172237c51`); форков нет.
- charlesvestal/schwung-303: теги v0.1.0…v0.3.3 (v0.3.3 = база); форк — только michaelmenesescs/schwung-303 (уже разобран).
- michaelmenesescs/schwung-8W8: теги v1.0.0…v1.5.0, `v1.5.0` = база `94aa2716…`; ветки разобраны; форков нет.
- michaelmenesescs/schwung: недоступен — `fatal: could not read Username for 'https://github.com': terminal prompts disabled`.
- Итог не меняется: все репо NOT FOUND. Детали TAKE/SKIP — `UPSTREAM-TAKE.md`.
