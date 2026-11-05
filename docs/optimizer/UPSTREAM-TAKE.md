# UPSTREAM-TAKE — что взять из новых релизов 9W9 / 8W8 без изменения звука

Дата: 2026-11-06 ~02:55 VLAT (UTC+10). Read-only, ничего не портировано. Деревья: `/tmp/cmp/{9pin,9ren,9v271,9v272,8pin,8v151}` (git archive из `/workspace/upstream/*`).

## Сводка
- Релизы/теги новее наших баз: **только** athousanddetails 9W9 `v2.7.1` = `25e03ddc33c2a4dfff702927061381a6e8bd6a3d`, `v2.7.2` = `a9e030d9bad46e3e53aafce92c7104b8e7bba47f`; 8W8 `v1.5.1` = `1dbef27e13a6c63cb1fc078dd9b4d86172237c51`; плюс michaelmenesescs 9W9 `8e9aebde…` (rename) и skin-бэкапы. Теги michaelmenesescs: 9W9 v0.6.0…`v2.7.0` (`1d9c73f7db9c0e32fb96eef11ff85f1c070d20f5`, предок пина), 8W8 v1.0.0…`v1.5.0` (= пин `94aa2716…`) — новее баз нет. charlesvestal/schwung-8W8: тегов нет.
- **Аудио:** upstream golden render, база vs новейший релиз — **бит-в-бит** для обоих: 9W9 `src/tools/render.c` (gcc -O2, 11 голосов `c_*.wav` + pattern, `cmp` SAME 12/12); 8W8 `tools/golden_check --write` (g++ -O2, 34 строки, `cmp` identical).
- **Функционально новые возможности: НЕТ.** Ни новых параметров, режимов, пресетов, MIDI, UI-фич, оптимизаций. Изменения = переименования, рефактор (уход от SC-кода), 1 фикс загрузки state, лицензия/копирайт, удаление тулов.
- **TAKE: 1 (только документация, атрибуция), OWNER DECIDES: 1 (строка в ABOUT), SKIP: 12.**
- **Лицензия:** v2.7.2/v1.5.1 добавили NOTICE с GPL-3.0 §7(b) атрибуцией («9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9», то же для 8W8) в source, docs и about/credits + §7(c) пометка «изменено, не оригинал». Наши базы (до 2026-10-27) — чистый GPL-3.0 без NOTICE: юридически эти условия на наш текущий порт не распространяются, пока мы не берём код новых релизов; наши LICENSING.md/README/MANUAL уже указывают 9W9/8W8 + athousanddetails + ссылки, но не дословную строку и не «A Thousand Details».

## TAKE (по ценности/риску)
1. **Атрибуция дословно (docs)** — добавить в LICENSING.md и docs/MANUAL.md строки `9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9` и `8W8 by A Thousand Details - https://github.com/athousanddetails/schwung-8W8` + «modified for X0X/DnBOX, not endorsed by A Thousand Details». 0 B прошивки, риск нулевой, делает нас совместимыми, если позже возьмём что-либо из новых релизов.

## Таблица изменений (база → новые релизы)
| # | Repo / tag | Full SHA | Файлы | Что | Наш рендер бит-идентичен | RAM/flash | Есть в нашем дереве | §7 | Вердикт |
|---|---|---|---|---|---|---|---|---|---|
| 1 | mm/9W9 main | `8e9aebde0c40a4c017f640e5c30a4f70936bb595` | src/dsp/er99_*→9w9_*, CMakeLists, scripts/*, README, docs/porting | rename er99→w9, state tag `9w9v3;` (+`er99v3;` грузится), удалён `tools/render_reference.mjs` | YES (не трогаем; upstream рендер SAME) | 0 | нет (у нас `drum909_*`, свой формат) | нет (до NOTICE) | SKIP — косметика |
| 2 | ad/9W9 v2.7.1 | `25e03ddc33c2a4dfff702927061381a6e8bd6a3d` | src/dsp/9w9_engine.c/.h (`w9_state_tag_ok`), module.json, loadtest.c | принимает любой короткий `*v3;` тег без `=` | YES | ~+100 B text, 0 RAM (в upstream) | нет: наш project.c не читает 9W9 state-блобы | нет NOTICE в 2.7.1 | SKIP — неприменимо |
| 3 | ad/9W9 v2.7.2 | `a9e030d9bad46e3e53aafce92c7104b8e7bba47f` | NOTICE (новый), копирайт-строка в каждый .c/.h/.js/.html, help.json «About», README, release.yml | лицензия/атрибуция | YES | 0 (комментарии) | частично (наш LICENSING/MANUAL) | **да**: вводит §7(b)(c) | TAKE (только docs, п.1 выше) |
| 4 | ad/9W9 v2.7.2 | `a9e030d9…` | src/dsp/webaudio.h, src/web_ui.html | убраны упоминания ER-99/Matthew Cieplak из комментариев/credits | YES | 0 | нет | — | SKIP — мы **сохраняем** кредит ER-99 (апстрим ER-99 GPL-3.0) |
| 5 | ad/9W9 v2.7.2 | `a9e030d9…` | scripts/gen_params.py, ui_chain.js, render.c, loadtest.c, tests/test_screen_reader_ui.mjs | только шапки/имена | YES | 0 | нет | — | SKIP |
| 6 | ad/8W8 v1.5.1 | `1dbef27e13a6c63cb1fc078dd9b4d86172237c51` | src/dsp/sc808_*→w8_*; `sc_ugens.h`→`w8_dsp.h` (RGen→NoiseGen, frand2→white, та же taus88-формула) | рефактор/ребрендинг ухода от SuperCollider-кода | YES (golden identical) | 0 (те же поля) | частично: наш `drum808.c` уже свой C-порт | да (NOTICE) | SKIP — нет выигрыша |
| 7 | ad/8W8 v1.5.1 | `1dbef27e…` | удалён `sc808_voices.h` (SC-транскрипции, A/B «Engine» путь), rim → `w8_rim.h` (`RimLane`, `VoiceBase` silence gate 200 сэмплов < 3.2e-5) | удаление мёртвого кода | YES (golden) | upstream −flash | у нас нет sc808-движка; silence gate уже есть (`drum808.c` ~1783/1831 tail floor) | да | SKIP — ALREADY IN по сути |
| 8 | ad/8W8 v1.5.1 | `1dbef27e…` | tools/*, test/*, src/tools/{nullref,probes}.cpp удалены, `THIRD_PARTY.md`, docs/DESIGN.md, PUNCHLIST.md, scripts/Dockerfile.sc, src/vendor удалены | чистка инфраструктуры SC null-test | n/a | 0 | нет | — | SKIP |
| 9 | ad/8W8 v1.5.1 | `1dbef27e…` | module.json author, web_ui.html, help.json, gen_params.py, ui_chain.js | credits: убраны sc808 (MIT, Horiuchi/Aaron) и Werner/Abel/Smith, «About» с NOTICE | YES | 0 | нет | да | SKIP — мы **сохраняем** кредит sc808 (MIT требует notice, наш rim после sc808) |
| 10 | mm/9W9 mac-mini-backup | `f240b842d507063c49330c77cfae50a026e57c94` | vst/*.svg, skin.css, vst.json | skin | n/a | n/a | нет | — | SKIP |
| 11 | mm/8W8 mac-mini-backup | `86ffe3967f25fcc4d210901030503ef4ce399805` | vst/* | skin | n/a | n/a | нет | — | SKIP |
| 12 | mm/9W9 ветки feat/step-tap-vs-hold `e6c43b211c857f5bd1dad6d16d692c7120e7d795`, voice-rebuild `fbad6a668f22a131ddba1e76330644f9486579f3`; mm/8W8 circuit-toms-clap-and-16-pads `1d09d66b8d5e25aa4e6b8e83e9fb059b6b9bdc18` | — | — | предки баз (уже влиты) | — | — | — | — | SKIP |
| 13 | charlesvestal/schwung-8W8 | tip = база `94aa2716…` | — | нет новых коммитов | — | — | — | — | SKIP |

Floors: ни один пункт не добавляет полей в `drum909_t` (7648) / drum808 state или таблиц → pool 8200/8192, BLE RAM 26336/26368, BLE flash 61552/65536 не затронуты.

## New features
| Что добавляет | Польза | RAM/flash | SOUND-CHANGING | §7 | В нашем дереве | Вердикт |
|---|---|---|---|---|---|---|
| (новых фич/параметров/режимов/пресетов/MIDI в 9W9 v2.7.1–2.7.2 и 8W8 v1.5.1 **не найдено**) | — | — | — | — | — | — |
| 9W9/8W8 «About»-страница в help.json с атрибуцией (`a9e030d9…`, `1dbef27e…`) | видимые кредиты; аналог нашего `ACT_ABOUT` («ABOUT DnBOX», `app/ui.c` ~2491) | ~60–120 B app flash на строки (оценка, UNKNOWN точно); BLE не затронут; RAM 0 | NO | выполняет §7(b) «about screen», если возьмём код новых релизов | да (ABOUT экран) | OWNER DECIDES — только при переходе на новые релизы; app-flash headroom не измерен |

## Доступность (без API)
- athousanddetails/schwung-9W9: clone OK, refs `main`, теги `v2.7.1`, `v2.7.2`; forks page — форков нет.
- athousanddetails/schwung-8W8: clone OK, `main`, `v1.5.1`; форков нет.
- charlesvestal/schwung-303: clone OK, теги v0.1.0…v0.3.3; форки: только michaelmenesescs/schwung-303.
- michaelmenesescs/schwung-8W8: clone OK, 3 ветки, теги v1.0.0…v1.5.0; форков нет.
- michaelmenesescs/schwung: **недоступен**: `git ls-remote https://github.com/michaelmenesescs/schwung` → `fatal: could not read Username for 'https://github.com': terminal prompts disabled` (приватный или не существует).
