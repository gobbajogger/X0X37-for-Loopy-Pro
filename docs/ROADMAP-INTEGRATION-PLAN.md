# ROADMAP Integration Plan

**Proposed path:** `docs/ROADMAP-INTEGRATION-PLAN.md`  
**Local box path:** `/workspace/m7a-out/plan1/ROADMAP-INTEGRATION-PLAN.md`  
**Docs-only.** No merges, pushes, approvals, or owner decisions are made here.  
**Written:** 2026-11-05 ~19:40 Asia/Vladivostok (UTC+10) by DnBOX Dev (executor).  
**Repo:** `gobbajogger/X0X37-for-Loopy-Pro` (public). Tracking: issue **#832**. Sole integrator: **PR #698**.

Every factual claim cites a source (`file@sha`, `#issue`/`#PR`, or `#832` comment id). Anything not directly stated is marked **INFERRED**. PASS / FAIL / NOT_RUN are kept distinct. This plan does **not** approve, accept, merge, or publish anything on the owner's behalf.

Related drafts (cite/reconcile, do not copy as authority): `/workspace/dnbox/master-plan.md` (also #832 c6653788083), `release-plan.md`, `publication-blockers.md`, `workflow-master-plan.md` (#832 c6653684011).

---

## 1. Scope and pins

| Pin | Value | Source |
|---|---|---|
| ROADMAP | `docs/ROADMAP.md` @ `5f578fc0642a1200a5163802bcd69277c511bfc7` (blob `a204a54c989d6f323d619e70f799427603756541`) | PR #812 head; local `/workspace/m7a-out/xport5/ROADMAP-5f578fc0.md`; gate-report.json |
| ROADMAP rows (gate parser) | 203 tracker+Done rows; Master tracker 182; historical Done 21; +6 selected fixes → **209 gate rows** | gate-report.json `total_roadmap_rows=203`; `rows` len 209; ROADMAP Counts line |
| Issue #832 comments read | **90** comments; through id **`6654589751`** at `2026-11-05T09:34:30Z` (09:34Z = 19:34 Asia/Vladivostok) | cursor-github `list_issue_comments` page1 per_page=100 |
| `main` SHA | `1a6bc3fe0245277b5ae31ab554c73d74b9ab4d83` | cursor-github `get_commit` ref=main; #832 c6653921403 / c6654589751 |
| 24.5 candidate | `8be16486e33f65b31c76710e1d825b6aa701c2c9` tree `f952ab4455723e4a1a68fdb3a8eea22d2da59710` | gate-output.txt; ROADMAP/#832; master-plan §3.1 |
| RC1 | `c32c54ac8ec68aaca53adccf7af66f155590f2dc` tree `ac490f7fae9198748aa54bd914d42f99d45444f9` | get_commit; #832 c6653704790 / c6654544506 |
| #698 head (time of writing) | `02cc7d104254b4427f257c310fb366bfc16a8e02` on `wip/one-build` → base `nightly` (`66073c68…`), draft, mergeable=true | get_pull_request #698 |
| Gate inputs / selection | selection-ref `c51790052aa1e2d6753a4a2357c464e912325435` blob `936581ba…` (branch push pending per task brief / Dev plans) | gate-output.txt; #832 c6653641857 |
| Gate run (xport5) | candidate 8be16486 + ROADMAP 5f578fc0 + selection c5179005 → **PRESENT=2 MISSING=144 QUEUED=63**, `passed=false` | `/workspace/m7a-out/xport5/gate-output.txt`, `gate-breakdown.txt`, `gate-report.json` |
| M7a Option A combined-v3 | `lane/dev-m7a-option-a-combined-v3-20261105` head `490e656c01b21c52b5116b109bde8e94be856016` tree `7cbc23087ca5109159d0c8cce7a216e4fcb63cdd` | #832 c6654544506; Git Verifier PASS c6654589751 |

**Selected release fixes** (selection c5179005 / gate `selected_release_rows`): `FIX-809-REVERT`, `FIX-INSTALLER-BACKUP`, `FIX-SMC-ALIGN`, `FIX-UI-TILES`, `PR-820`, `PR-821` — all **MISSING** proofs in the xport5 gate run.

---

## 2. Full item inventory

Gate row set = historical Done (DONE-001…) + Master tracker (#n / L* / M* / ROW-* / QA* / PR-*) + selected release fixes. Status text is from ROADMAP@5f578fc0 as parsed by `tools/roadmap_completeness.py` into gate-report.json. Gate **result** is PRESENT / MISSING / QUEUED from that run.

**Owner rule:** only owners **named in a cited source**; otherwise `unassigned (no owner named in sources)`.

**Areas / overlap:** when derived from title keywords only → marked INFERRED in the Areas column with `*`.

### 2.1 Summary counts

| Set | Count | Source |
|---|---:|---|
| ROADMAP Master tracker rows | 182 | ROADMAP Counts; gate section Master tracker |
| Historical Done rows (DONE-*) | 21 | gate section historical Done |
| Selected release fixes | 6 | gate-report selected_release_rows |
| Gate rows total | 209 | gate-report rows |
| Gate PRESENT / MISSING / QUEUED | 2 / 144 / 63 | gate-report summary |
| Open PRs surveyed (API page) | 100 | list_pull_requests state=open |

### 2.2 Inventory table (every gate row — 209)

Columns shortened for size. Full titles/status in ROADMAP@5f578fc0 / `gate-inventory.json`. Owner default = unassigned unless listed in §2.3.

| ID | Title | BR/PR | Status→Gate | Areas* | Carry25 |
|---|---|---|---|---|---|
| DONE-001 | Clip mode: APC40 mkII emulation for  | — | done→MISSING | see ROADMAP* | Y |
| DONE-002 | Clip mode: Fixes: the firmware drops | — | done→MISSING | see ROADMAP* | Y |
| DONE-003 | Docs: BEHAVIOR.md (intended behavior | — | done→MISSING | see ROADMAP* | Y |
| DONE-004 | Colours: Colour tool that calculates | — | done→MISSING | see ROADMAP* | Y |
| DONE-005 | Step 1: The installer backs up the l | — | done→MISSING | see ROADMAP* | Y |
| DONE-006 | Step 2: The firmware stores the 909  | — | done→MISSING | see ROADMAP* | Y |
| DONE-007 | Step 3: Factory breaks: 2 bars at 44 | — | done→MISSING | see ROADMAP* | Y |
| DONE-008 | Step 4: New flash layout: one 64 KiB | — | done→MISSING | see ROADMAP* | Y |
| DONE-009 | Step 5: Bluetooth MIDI code: the Jie | — | done→MISSING | see ROADMAP* | Y |
| DONE-010 | Installed: The keyboard ran 81d5f74, | — | done→MISSING | see ROADMAP* | Y |
| DONE-011 | Safety: The installer refuses to add | — | done→MISSING | see ROADMAP* | Y |
| DONE-012 | Safety: Bluetooth start guard: a fai | — | done→MISSING | see ROADMAP* | Y |
| DONE-013 | Bluetooth: Bluetooth starts: STATUS  | — | done→MISSING | see ROADMAP* | Y |
| DONE-014 | Bluetooth: The Mac connects over Blu | — | done→MISSING | see ROADMAP* | Y |
| DONE-015 | Bluetooth: The iPhone connects over  | — | done→MISSING | see ROADMAP* | Y |
| DONE-016 | Clip keys: Knobs 2-8 send CC 48-54 w | — | done→MISSING | see ROADMAP* | Y |
| DONE-017 | Bluetooth: BLE-16: when the Bluetoot | — | done→MISSING | see ROADMAP* | Y |
| DONE-018 | Clip colours: The LCD clip colours a | — | done→MISSING | see ROADMAP* | Y |
| DONE-019 | Pad colours: Pad colour calibration  | — | done→MISSING | see ROADMAP* | Y |
| DONE-020 | Pad colours: The owner calibrated th | — | done→MISSING | see ROADMAP* | Y |
| DONE-021 | Gate: The gate builds and tests the  | — | done→MISSING | see ROADMAP* | Y |
| #1 | Два полноценных выбираемых движка Ch | — | in 0.0.24→PRESENT | see ROADMAP* | Y |
| #2 | Достоверный звук оригинальных движко | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #3 | (R003) Chicago/Miami knob reach, scr | — | verified and agreed by owner→MISSING | see ROADMAP* | Y |
| #4 | Pitch Bend и педаль на своих физичес | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #5 | Все согласованные эффекты без скрыто | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #6 | Заводские пресеты и удобный выбор зв | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #7 | Привязка звука и движка к паттерну | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #8 | Полный sound lock на отдельной ноте | — | missing→QUEUED | see ROADMAP* | Q |
| #9 | Честная замена старых движков | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #10 | Реальные имена пресетов и переключен | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #11 | Ноты длиннее одного шага | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #12 | Запись фактической длительности удер | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #13 | Музыкально корректная вероятность | — | in 0.0.24 (historical probab→MISSING | see ROADMAP* | Y |
| #14 | RANDOM без расхождения с предпросмот | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #15 | Сохранение cutoff locks при навигаци | — | missing→QUEUED | see ROADMAP* | Q |
| #16 | Жест принадлежит исходной партии до  | held-notes-ui | in progress (exact reviewed →QUEUED | ui.c | Q |
| #17 | Безопасный жизненный цикл motion и з | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #18 | Единые внутренние и внешние часы | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #19 | Правильные Start Stop Continue и оче | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #20 | Визуальный метроном от настоящих час | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #21 | Синхронные доли Miami и удобный выбо | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #22 | Legato при записи превращается в sli | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #23 | Сохранить отдельные модели 909 и 808 | — | in 0.0.24→PRESENT | see ROADMAP* | Y |
| #24 | Независимая адаптивная нарезка двух  | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #25 | Понятная клавиатурная навигация по с | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #26 | Правильный MIDI выход BREAKS | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #27 | Pitch Bend для BREAKS | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #28 | Jungle stretch вместо HALFTIME | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #29 | Три выбранных владельцем штатных бре | — | missing→QUEUED | see ROADMAP* | Q |
| #30 | Читаемый восьмиполосный микшер | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #31 | Осознанный выбор полосы и режима | — | in progress→QUEUED | see ROADMAP* | Q |
| #32 | Физический Pause как context lock с  | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #33 | Два независимых SMC по Bluetooth | — | in progress→QUEUED | see ROADMAP* | Q |
| #34 | Безопасные отключение и возврат SMC | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #35 | Разведение SMC и других контроллеров | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #36 | Названия peer и сведения о подключен | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #37 | Страницы восьмой полосой, банки руче | — | in progress→QUEUED | see ROADMAP* | Q |
| #38 | Индивидуальный drum mixer вторым наж | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #39 | Ручки не дублируют фейдеры | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #40 | Контекст Select и исключение User mo | — | in progress→QUEUED | see ROADMAP* | Q |
| #41 | Отдельная страница эффектов на микше | — | missing→QUEUED | see ROADMAP* | Q |
| #42 | Полноценный временный SMC экран вмес | — | in 0.0.24 (historical baseli→MISSING | see ROADMAP* | Y |
| #43 | Тихая смена связанного SMC контекста | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #44 | Накопление и отмена ожидаемой смены  | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #45 | Единый pickup и правильный уровень п | — | in progress→QUEUED | see ROADMAP* | Q |
| #46 | Pickup и lock feedback в музыкальном | — | in progress→QUEUED | see ROADMAP* | Q |
| #47 | Явное надёжное Save Project | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #48 | Явное безопасное Load Project | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #49 | Раздельные Clear Song и Reset Projec | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #50 | Полный переносимый экспорт и импорт  | — | missing→QUEUED | see ROADMAP* | Q |
| #51 | Надёжный backup и restore сэмплов | — | in 0.0.24 (historical baseli→MISSING | see ROADMAP* | Y |
| #52 | Атомарные операции при нескольких кл | — | missing→QUEUED | see ROADMAP* | Q |
| #53 | Не терять данные при смене соединени | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #54 | Явно решить совместимость старого фо | — | missing→QUEUED | see ROADMAP* | Q |
| #55 | Полнота восстановления всех прежних  | — | missing→QUEUED | see ROADMAP* | Q |
| #56 | Довести именно новый согласованный U | — | missing→QUEUED | see ROADMAP* | Q |
| #57 | Обзор реального содержимого всех пат | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #58 | Показывать velocity без потери сильн | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #59 | Больше места музыкальным шагам | — | in progress→QUEUED | see ROADMAP* | Q |
| #60 | Осмысленные waveform и morph glyphs | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #61 | Правдивые глифы огибающих и эффектов | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #62 | Правильные подсказки физических руче | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #63 | Безопасный wrap навигации | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #64 | Структурированные настройки вместо п | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #65 | Анимация только по настоящей активно | — | in 0.0.24 (historical activi→MISSING | ui.c | Y |
| #66 | Полная ADSR кривая при редактировани | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #67 | Экран остаётся пока продолжается вза | — | in progress→QUEUED | see ROADMAP* | Q |
| #68 | Матрица контролов с выделенной актив | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #69 | Раздельные экраны секвенсора и звуко | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #70 | Настраиваемые цветовые темы, включая | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #71 | Единое написание DnBOX и качественны | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #72 | Статическая загрузка без нежелательн | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #73 | Независимый recovery bootloader | — | missing→QUEUED | see ROADMAP* | Q |
| #74 | Диагностика неработающих пэдов SMK37 | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #75 | Одна понятная страница установки и у | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #76 | Импорт и экспорт song через сайт | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #77 | Новый DnBOX сайт и общий шрифт | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #78 | Настоящие Manual What’s New и Histor | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #79 | Понятные шаги и состояния подключени | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #80 | Заметный локальный файл прошивки и о | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #81 | Заранее сообщать о неподдержанном бр | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #82 | Доступность на телефоне и компьютере | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #83 | Правильный Bluetooth controller внут | — | missing→QUEUED | see ROADMAP* | Q |
| #84 | Сохранять устанавливаемые версии и ч | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #85 | Устойчивое обновление и перенос данн | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #86 | Показывать возможности настоящими из | — | missing→QUEUED | see ROADMAP* | Q |
| #87 | Превью и подготовка сэмплов под уста | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #88 | Несколько визуальных вариантов сайта | — | missing→QUEUED | see ROADMAP* | Q |
| #89 | Экономия памяти ускорением сэмплов п | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #90 | Полноценное редактирование паттернов | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #91 | Piano roll для двух синтезаторных па | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #92 | Отдельные редакторы 909 808 и BREAKS | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #93 | Импорт MIDI файлов с явным предпросм | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #94 | Никакой тихой потери музыкальных дан | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #95 | Локальные draft save load и Undo | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #96 | Генераторы New и Mutate | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #97 | Удобный iPhone интерфейс по мотивам  | — | missing→QUEUED | see ROADMAP* | Q |
| #98 | Выделение и групповые операции | — | missing→QUEUED | see ROADMAP* | Q |
| #99 | Полные операции над part и достижимы | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #100 | Redo и предсказуемая история редакти | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #101 | Grid scale snap ghost и touch keyboa | — | missing→QUEUED | see ROADMAP* | Q |
| #102 | Корректные жесты и отмена на телефон | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #103 | Sound controls в редакторе соответст | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #104 | Двусторонний протокол синхронизации  | — | missing→QUEUED | see ROADMAP* | Q |
| #105 | Отдельный используемый TouchOSC прое | — | missing→QUEUED | see ROADMAP* | Q |
| #106 | Реальная нагрузка в idle не выше 20  | — | missing→QUEUED | see ROADMAP* | Q |
| #107 | Надёжность под совместной нагрузкой | — | missing→QUEUED | see ROADMAP* | Q |
| #108 | Не потерять старые полезные исправле | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #109 | Проверки не должны выдавать ложный у | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #110 | Проверка повседневного удобства без  | — | missing→QUEUED | see ROADMAP* | Q |
| #111 | Одна полная обновлённая NIGHTLY верс | — | missing→QUEUED | see ROADMAP* | Q |
| #112 | Настоящие пакеты обеих моделей | — | missing→QUEUED | see ROADMAP* | Q |
| #113 | Доставить через существующий установ | — | missing→QUEUED | see ROADMAP* | Q |
| #114 | Не смешивать состояния готовности | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #115 | Независимая проверка именно итоговых | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #116 | Не забывать ни один запрос | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #117 | Сократить путь от просьбы до доступн | — | missing→QUEUED | see ROADMAP* | Q |
| #118 | Production отдельно от NIGHTLY | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #119 | Библиотека песен сообщества | — | missing→QUEUED | see ROADMAP* | Q |
| #120 | Автоматический inbox предложений и г | — | missing→QUEUED | see ROADMAP* | Q |
| #121 | Польза официальной M VAVE прошивки и | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| #122 | Проверка вклада Cursor и полноты рев | — | missing→QUEUED | see ROADMAP* | Q |
| ROW-43a43b4af32c | FX41-REC (recovery evidence alias of | — | in progress→QUEUED | see ROADMAP* | Q |
| ROW-e7ebadf91a40 | Local source preservation delta (903 | — | done→MISSING | see ROADMAP* | Y |
| ROW-fcf14c2de61c | 122-feature reconciliation sheet | — | done→MISSING | see ROADMAP* | Y |
| PR-790 | PR #790 Q1 sequencer chain atomic | #790@314614c0 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-791 | PR #791 F2 recording reconciliation | #791@b7ae1dd8 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-792 | PR #792 Q3 editor slots | #792@356bba30 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-793 | PR #793 OPT-B gfx | #793@9bc0ac56 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-794 | PR #794 F6a site data | #794@ed5c33ab | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-795 | PR #795 N5 update safety | #795@9e6440f7 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-796 | PR #796 B2 engine peaks | #796@faed57f9 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-797 | PR #797 QB HAL/USB hygiene | #797@c1b09d5c | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-799 | PR #799 LIVE-A safety | #799@9973e7e9 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-801 | PR #801 B3 FREE PLAY first render | #801@f708b8ed | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-802 | PR #802 OPT-A audio path | #802@bfecd02c | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-804 | PR #804 B1 installer retry | #804@982bf98e | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-805 | PR #805 B5 USB audio reader progress | #805@b558c1b0 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-806 | PR #806 nightly version embed + stri | #806@62cf11e9 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-808 | PR #808 F6b web editor | #808@019b8219 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-811 | PR #811 param write sources | #811@f753ee66 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-810 | PR #810 live risks / instant unmute | #810@45a7285e | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-798 | PR #798 F1 Chicago/Miami controls | #798@4e1ae81e | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-800 | PR #800 F4 SMC mixer remainder | #800@e81dc430 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-803 | PR #803 F7 truthful input checks | #803@64326980 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-789 | PR #789 SMC dedicated popup | #789@26e4c7f1 | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-807 | PR #807 F5 waveform cache | #807@6508d90a | in 0.0.24→MISSING | see ROADMAP* | Y |
| PR-809 | PR #809 OPT-C UI frame | #809@3241ece5 | in 0.0.24 (historical integr→MISSING | ui.c | Y |
| L1 | USB SysEx OTA/UBOOT mid-set | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| L2 | Autosave flash erase during tails | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| L3 | LOAD wipes UI state | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| L4 | PLAY plays SONG only on SONG screen | — | waiting on owner→QUEUED | see ROADMAP* | Q |
| L5 | Undo/redo on transport keys | — | waiting on owner→QUEUED | see ROADMAP* | Q |
| L6 | Global PANIC (legacy audit alias of  | #839 alias | approved, planned for 0.0.25→QUEUED | engine/ui/seq; panic_* | 25/gate |
| L7 | Tempo fine nudge / ramp | — | waiting on owner→QUEUED | see ROADMAP* | Q |
| L8 | Muted fader jumps to level | — | done→MISSING | see ROADMAP* | Y |
| L9 | Fader loses pickup under motion | — | missing→QUEUED | see ROADMAP* | Q |
| L10 | Cue at BAR only | — | waiting on owner→QUEUED | see ROADMAP* | Q |
| L11 | BREAK mute late by one step | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| L12 | Loudness between parts/banks | — | in progress→QUEUED | see ROADMAP* | Q |
| L13 | Clip grid lost on host drop | — | missing→QUEUED | see ROADMAP* | Q |
| M1 | Global PANIC: keyboard FX+SEQ; SMC B | #839@69351a26 | approved, planned for 0.0.25→QUEUED | engine/ui/seq; panic_* | 25/gate |
| M2 | muted fader takes its level on unmut | — | done→MISSING | see ROADMAP* | Y |
| M3 | instant tempo | — | done→MISSING | see ROADMAP* | Y |
| M4 | Reset the selected part's saved soun | private | missing (reset-overlay mocku→QUEUED | see ROADMAP* | 25/gate |
| M5 | (unknown item) | — | waiting on owner→QUEUED | see ROADMAP* | 25/gate |
| M6 | SMC FX page effect selection/throw a | #815/#800 | missing (effect mockup appro→QUEUED | see ROADMAP* | 25/gate |
| M7 | Per-part settings ON/OFF toggle (909 | combined-v3@490e656c | missing (mockup proposal onl→QUEUED | project/settings_t/web; M7b=ui.c | 25/gate |
| M8 | PLAY while a question is open: answe | private | approved, planned for 0.0.25→QUEUED | see ROADMAP* | 25/gate |
| M9 | Approved 0.0.25 integration plan, 15 | #812 | approved, planned for 0.0.25→QUEUED | see ROADMAP* | 25/gate |
| M10 | Mandatory measured free pool headroo | gate | approved, mandatory gate for→QUEUED | see ROADMAP* | 25/gate |
| M11 | NIGHTLY publishing automatically fol | workflow | approved, mandatory NIGHTLY →QUEUED | see ROADMAP* | 25/gate |
| M12 | ROADMAP completeness gate for pinned | #822@46d3e952 | implementation pending (orig→QUEUED | docs/build | 25/gate |
| M13 | Straight 1/8 and 1/4 SEQ rates for 9 | #824@a12ea10c | open (timing CORE delivered →QUEUED | sequencer* | 25/gate |
| QA1 | Rate-pad composition host test fails | #834+ | open (pre-existing baseline →QUEUED | see ROADMAP* | 25/gate |
| QA2 | Repair 33 known host-test failures a | — | open, scheduled after the 20→QUEUED | see ROADMAP* | 25/gate |
| ROW-ecaaa1c520af | FX hold + < > remap per button | — | in progress→QUEUED | see ROADMAP* | Q |
| ROW-938cc69314d2 | Instant unmute (muted bus keeps runn | — | in 0.0.24→MISSING | see ROADMAP* | Y |
| ROW-80982d592fa7 | Strict installer model match | — | done→MISSING | see ROADMAP* | Y |
| ROW-63901cf850f6 | Engine coefficient caching (Chicago/ | #813/#814 | in progress (existing Chicag→QUEUED | engine miami/chicago | Q |
| ROW-e3113dfd04ab | CPU idle bug | — | in progress→QUEUED | see ROADMAP* | Q |
| L14 | Screen lag bug (existing row; canoni | #821 | in progress (hardware lag no→QUEUED | ui.c | Q |
| FIX-809-REVERT |  | sel c5179005 | required selected release fi→MISSING | ui.c | Y |
| FIX-INSTALLER-BACKUP |  | sel | required selected release fi→MISSING | see ROADMAP* | Y |
| FIX-SMC-ALIGN |  | sel/#820 | required selected release fi→MISSING | ui.c | Y |
| FIX-UI-TILES |  | sel/#821 | required selected release fi→MISSING | ui.c | Y |
| PR-820 |  | #820@c3d35f80 | required selected release fi→MISSING | ui.c | Y |
| PR-821 |  | #821@179ab384 | required selected release fi→MISSING | ui.c | Y |

\* Areas with trailing intuition from titles or known hotspots; file-level claims for hotspots are sourced in §3. Owners: see §2.3.

### 2.3 Named owners only (else unassigned)

| ID | Owner | Source |
|---|---|---|
| M1 / L6 | Codex stage | master-plan.md §3.2 (#839) |
| M13 | `plan_global_seq_rates` | ROADMAP@5f578fc0 M13 Source |
| (all other rows) | unassigned (no owner named in sources) | — |

Deps / overlap / next-step detail: §3–§5 and ROADMAP Next step column; pipeline board in `roadmap-pipeline.md`.

## 3. Overlap matrix and conflict hot spots

### 3.1 Shared-file overlap matrix

| Shared surface | Items / lanes touching it | Conflict notes | Sources |
|---|---|---|---|
| `settings_t` size/layout (x0x.h) | M7a Option A (stays **40 B**); any future settings fields; PANIC/state if it grows structs | M7a explicitly keeps `settings_t` at 40 B, flash object 44 B all-ON / 48 B any-OFF | #832 c6653641857, c6653704790 |
| `.x0x` word 28 / format | M7a firmware macro + web parser; project save/load; web editor | v2 split branches **mismatched** (QA word-28 NEGATIVE); combined-v3 fixes in **one tree** | #832 c6654007069, c6654544506 |
| `ui.c` ownership | #16 held-notes; #65 visuals; FIX-UI-TILES/#821/L14; FIX-SMC-ALIGN/#820; FIX-809-REVERT/#809; M1 PANIC UI; M7b ON/OFF UI (**not started**); M4/M6/M8 (likely) | Serialize writers; integrator freezes one tree per step | ROADMAP statuses; #839; #832 c6653641857 (M7b NOT started); master-plan §5.3 |
| `project.c` | M7a capture/header; saved-role fences (#789 evidence); #698 gaps (`read_settings_payload`) | #698 still lacks saved-role fences per master-plan §3.2 | #832 c6654544506; master-plan §3.2 / PB §3 |
| `seq.c` / sequencer | #11–#13; M13 (#824); M7a seq/engine gates | M13 deferred behind profiling; schema owner decision | ROADMAP M13; #824 |
| PANIC files / M1 | PR #839 core; private combined `cf9b47bb`; L6 alias | Full chain not public; overlap with M7a/M7b **NOT_RUN** re-check | #839; master-plan §3.5; #832 c6654544506 NOT_RUN list |
| `miami_*` / Chicago coef | #814 Miami, #813 Chicago, ROW-63901cf850f6, cache-retention lane, engine profiling on 8be16486 | Cache lane Chicago **strict FAIL** (unused `cache`); no measured savings admitted | #814; ROADMAP ROW-63901cf850f6; #832 c6653465505 |
| Docs / rules / AGENTS | #812 ROADMAP; #841/#842; Docs lanes v3/v4; M12 gate docs | #842 draft do-not-merge; candidate 0-fail bar vs main | #841/#842; master-plan §3.3; D10 |

### 3.2 Hot spots (detail)

1. **settings_t budget** — M7a Option A claims no growth of `settings_t` (40 B) while encoding part-OFF in `.x0x` word 28 + optional flash object growth (#832 c6653641857). Any other feature adding settings fields must prove layout compatibility before #698 admission (**owner/integrator**, not decided here).
2. **Word 28 / format** — Independent QA `QA-LX016-M7A-V2-CROSS-BRANCH-WORD28-NEGATIVE` (#832 c6654007069): web-v2 embedded firmware macro lacked CRC8 check byte while web JS required it (31/31 reject). Combined-v3 (#832 c6654544506) puts firmware+web+e2e in one tree; **QA exact-tree review still pending** per Dev hand-off.
3. **ui.c ownership** — Multiple 24.5 selected fixes and 0.0.25 UI features contend. M7b (per-part ON/OFF UI) is explicitly **NOT started** and depends on PANIC/ui.c ownership clarity (#832 c6653641857).
4. **PANIC #839 vs M7a/M7b** — #839 is core-only draft @69351a26; full chain private. M7a reports PANIC overlap re-check **NOT_RUN** (#832 c6654544506). Do not assume non-overlap.
5. **Miami #814** — Open PR @dcc9833f onto `build/0.0.24`; master-plan notes missing private GCC14 fix `51e8e211`. ROW-63901cf850f6 still “no measured savings”.

---

## 4. Lanes: parallel vs serialize; agent ownership

### 4.1 Agent roles (as named in sources)

| Role | Identity / note | Source |
|---|---|---|
| Grok Bot coordinator | orchestration only; no formal approval | #832 c6653684011; master-plan §3.6 |
| Dev | `9aca8b39…` — M7a Option A, roadmap gate inputs | #832 c6653684011; c6653641857 |
| QA | `e63e4749…` — advisory exact-tree review | #832 c6653684011; c6653787449 |
| QA 2 | `2ae655aa…` — second advisory | #832 c6653684011; c6653617848 |
| Docs | `452fbdec-f2bd-4b8f-892a-a9d66a968521` — docs/rules writer | #832 c6653684011 |
| Build (Grok Bot) | `76a6e0f4…` — gaps/runbooks/host-only; **not** native_build | #832 c6653684011; c6653962863 |
| Git Verifier | `2835999d…` — push readback | #832 c6653684011; c6653921403; c6654589751 |
| #698 integrator | PR #698; live operator **UNKNOWN** | master-plan §3.6; get_pull_request #698 |
| native_build | role id `6ada68578b488191ac2ce427fd41cff4`; JieLi clang 4.0.1 + AC79 SDK V1.2.1 | #832 c6653962863; master-plan §3.6 |
| independent_qa | `6adafbbf54088191a86715d49e7dc6b0` — formal acceptance | #832 c6654007069 header; master-plan §6.3 |
| Human owner | `gobbajogger` — owner-only decisions | ROADMAP; master-plan §5.5 |

### 4.2 Parallel lanes (no shared writer)

| Lane group | Can run in parallel | Serialize against |
|---|---|---|
| A. Docs/rules (#841/#842, Docs v3/v4) | yes, docs-only trees | production `main` merge (#842 needs owner auth) |
| B. M12 gate inputs (selection+evidence @c5179005) | yes vs firmware feature lanes | must match ROADMAP blob a204a54c before gate PASS |
| C. Host FAIL classification on 8be16486 (Build/QA) | yes | native_build auth |
| D. Fixture/test-only branches (#823,#834–#840, builtin-break, saved-role) | yes among themselves if different files | #698 admission order |
| E. Engine coef / Miami #814 / cache lane | yes vs M7a **if** no shared merge yet | miami_* file ownership at integrate time |
| F. M7a combined-v3 | yes for QA review now | #698 integrate; PANIC/ui.c; M7b |
| G. M1 PANIC (#839 + private chain) | parallel development | **serialize** with M7b/ui.c and any ui.c 24.5 fixes at integrate |
| H. M13 #824 | parallel (PARTIAL) | owner D9; profiling priority (ROADMAP) |

### 4.3 Must serialize

1. Any two writers on **`ui.c`** (24.5 LCD/SMC/OPT-C fixes, PANIC UI, M7b, M4/M6/M8).
2. **Firmware+web format** changes (M7a word 28) — ship as **one** immutable tree (lesson from c6654007069).
3. **#698 integrator steps** — one frozen source tree per step (M9 / master-plan).
4. **Native build → formal independent_qa → catalog_sync → website_publisher** publication chain (publication-blockers.md; nightly-publication-path.md).
5. **M10 pool measurements** at every integration step before calling a step done (ROADMAP M10).

---

## 5. Merge order via #698 integrator

Integrator PR: **#698** `wip/one-build` @ `02cc7d104254…` → `nightly`. Draft. Missing pieces vs 24.5/25 called out in master-plan §3.2 / publication-blockers §3 / executor-status-832 — **not** re-decided here.

**Rule:** each step freezes **one immutable source tree** (commit+tree SHA). Gates between steps must cite that tree. Owner/integrator select actual admission; this is a proposed order with dependency reasons only.

| Step | Admit / freeze | Why this order | Integrator freeze | Gate before next |
|---:|---|---|---|---|
| 0 | Pin bases: `8be16486` (24.5), `c32c54ac` (RC1 lineage), ROADMAP `5f578fc0` | Shared references for all proofs | Record SHAs in #698 / #832 | Git Verifier identity on any new lane tips |
| 1 | Docs/rules lane intended for nightly (#841 / successor) if integrator accepts | Docs-only; unblocks AGENTS 0-fail bar clarity | Freeze docs tree | QA advisory + no firmware behavior change |
| 2 | M12 tooling #822 + **gate inputs** selection/evidence @`c5179005` (or owner-approved successor) pinned to ROADMAP blob `a204a54c` | Gate cannot PASS without matching pins | Freeze tool+json tree | `roadmap_completeness.py` runs; expect QUEUED for unselected; close MISSING proofs |
| 3 | 24.5 selected fixes proofs + test-only closures (PR-820/821, FIX-*, launcher/saved-role/builtin-break as applicable) onto frozen 24.5-derived tree **X245** | Closes publication blockers for NIGHTLY host bar without inventing scope | Freeze X245 = production bytes decision (owner D1) | Host suite receipts; classify FAIL/NOT_RUN separately |
| 4 | Native Elite+Pro on X245 (`native_build`) | RM checklist; M10 precursor | Freeze build receipts | exit 0; RAM/pool reported |
| 5 | Formal independent_qa on X245 + M12 PASS → publication chain | Separates advisory from acceptance | Freeze accepted inputs | #816/#822 HOLD disposition by their owners |
| 6 | Carry-forward matrix 24.5→25; compose **X25** base | H9: every 24.5 delta PRESENT or owner exclusion | Freeze X25 parent tree | Matrix reviewed |
| 7 | M1 PANIC (public #839 + whatever owner authorizes from private chain) | Approved 0.0.25; touches panic/ui — before M7b | Freeze after PANIC | Host+mutants; ui.c ownership note; M10 |
| 8 | M7a combined firmware+web tree (`490e656c` / `7cbc2308` or successor after QA) | Format change; must be one tree; before M7b UI | Freeze M7a tree | QA exact-tree; word-28 e2e; PANIC overlap check no longer NOT_RUN |
| 9 | M8 / M4 / other owner-selected D8 items | Depend on ui/SMC clarity post-PANIC | One item per freeze | Per-item QA + M10 |
| 10 | M7b `ui.c` ON/OFF UI | Explicitly depends on ui.c ownership after PANIC/M7a | Freeze UI tree | Device/UI mockup rules |
| 11 | Engine cache / #814 only if selected (D8) and attribution exists | ROADMAP forbids claiming savings without measurement | Freeze opt tree | Audio identity; CPU evidence |
| 12 | M13 only if owner D9 approves | Not downgrade-safe; deferred | Freeze or skip | Schema decision recorded |
| 13 | X25 native + M10 both models + M12 PASS + formal QA → M11 publish path | End state “ready for owner to publish” | Freeze X25 release tree | Owner checklist §6.2 (master-plan) |

---

## 6. Reaching a NIGHTLY build (per step)

### 6.1 Shared acceptance criteria (checkable by QA and Build)

For each frozen tree `T` (commit + tree SHA):

| Check | Who runs | PASS means | Keep separate |
|---|---|---|---|
| Exact-tree identity | Git Verifier | tip/tree/parent/author match; no unexpected trailers; ref policy | verified=false unknown_key is reported, not hidden (c6654589751) |
| Advisory exact-tree review | QA / QA 2 | stated PASS/FAIL on named tree; not release acceptance | advisory ≠ formal |
| Host suites | Build / Dev / QA as scoped | commands + rc recorded | PASS / FAIL / NOT_RUN / SKIP listed separately |
| Mutants | Dev/QA as scoped | red then green on revert (e.g. M7a word28) | — |
| ASan/UBSan | Dev/QA as scoped | rc=0 on scoped tests | NOT_RUN if env missing |
| Native Elite + Pro | **native_build** only | JieLi **clang 4.0.1** + **AC79 SDK V1.2.1** (`d179b448…`); `X0X_DEV_PADCAL=0`; exit 0 | Native remains **NOT_RUN** until that owner runs it (#832 c6653962863) |
| M10 pool | native_build | ≥8192 B Elite **and** Pro, fresh, not inferred | NOT_RUN blocks |
| M12 completeness | #822 tool + pins | PRESENT for required; QUEUED for unselected; `passed=true` | xport5 today: PASS **not** met (2/144/63) |
| Formal independent_qa | independent_qa role | names T + native receipts | advisory Grok QA does not count |
| Publication | catalog_sync → website_main_merger → website_publisher | live bytes match; owner install path | agents never flash (M11 / ROADMAP) |

### 6.2 Known host baseline (do not collapse)

| Claim | Value | Source |
|---|---|---|
| Catalog / master-plan host on 8be16486 | **366 PASS / 18 FAIL**; DEVICE NOT TESTED | publication-blockers.md; master-plan §3.1; #832 c6653465505 |
| Dev M7a combined-v3 note | full `tests/run_tests.sh` + **19-FAIL baseline comparison** = **NOT_RUN** on that tree | #832 c6654544506 |
| Contradiction | **18 FAIL** vs **19-FAIL** wording | See §8 |

### 6.3 Gate inputs status (xport5 run)

| Class | Count | Notes |
|---|---:|---|
| PRESENT | 2 | #1, #23 |
| MISSING | 144 | includes required Master/Done rows lacking pinned proofs + 6 selected fixes + #65 unrecognized status |
| QUEUED | 63 | explicitly not yet built/accepted (incl. M13 new vs 70b5a033) |

Source: `gate-breakdown.txt`, `gate-output.txt`, `gate-report.json` with selection `c5179005` and ROADMAP `5f578fc0`.

### 6.4 What stays NOT_RUN / HOLD (who decides)

| Item | State | Who decides to clear | Source |
|---|---|---|---|
| Native build | NOT_RUN (runbook ready; auth historically blocked) | native_build owner + owner auth (D7) | #832 c6653962863; publication-blockers §1 |
| Device / hardware | NOT_TESTED on catalog 8be entry | owner install via nightly site | ROADMAP M11; catalog notes |
| #816 | QA HOLD / separate RC1 QA entry | #816 owners / QA process | PR #816; master-plan §3.2 |
| #822 publication | HOLD; scoped source passes ≠ release | #822 reviewer / independent_qa | master-plan §3.2 |
| M7a PANIC overlap | NOT_RUN | Dev/QA after PANIC tree public | #832 c6654544506 |
| M7b ui.c | not started | owner selects; ui.c owner after PANIC | #832 c6653641857 |
| M13 schema | awaiting owner | owner D9 | ROADMAP M13 |
| Formal release acceptance | not granted by this plan | owner + independent_qa | master-plan §6.3 |

### 6.5 Per merge-step NIGHTLY needs (abbrev)

- **Steps 0–2:** docs + gate pins; host optional; native NOT_RUN OK for docs-only.
- **Steps 3–5 (24.5 NIGHTLY):** full host 0 FAIL/0 SKIP/0 NOT_RUN **or** owner-documented waiver (D6 recommends keep bar); native Elite+Pro; M12 PASS; formal QA; publisher — see master-plan §6.1.
- **Steps 6–13 (0.0.25):** carry-forward + M10 every step + M7 firmware**and**web together + disposition QA1/QA2 — master-plan §6.2.

---

## 7. M7a / M7b

### 7.1 Option A status

| Fact | Detail | Source |
|---|---|---|
| Decision | Option A = refuse legacy 67801b9a-format `.x0x` word-28 OFF files | #832 c6653641857; coordinator GS/D13 |
| Combined-v3 branch | `lane/dev-m7a-option-a-combined-v3-20261105` @ `490e656c…` tree `7cbc2308…` | #832 c6654544506 |
| Git Verifier | PASS on tip/tree/identity; 16 linear commits; no PR | #832 c6654589751 |
| Earlier v2 | fw `8c9332fc` / web `5f4b0c82` — superseded; word-28 cross-branch NEGATIVE | #832 c6653921403; c6654007069 |
| Host evidence | e2e PASS; part_off PASS; project_stage PASS; scoped ASan/UBSan; mutants red/green | #832 c6654544506 |
| QA review | **pending** exact-tree review on combined-v3 (Dev hand-off) | #832 c6654544506 |
| Integrator | **UNREVIEWED** / not admitted to #698 | #832 c6654544506 |
| settings_t | stays 40 B | #832 c6653641857 |
| Ships together | **firmware + web** in one release; old firmware and current web refuse OFF files | #832 c6653704790; c6654544506 |

### 7.2 What ships together

- Firmware: `PROJ_FILE_OFF_WORD` CRC8 in `x0x.h` / `project.c`; seq/engine gates for part OFF.
- Web: `web/x0x_projects.js` parser + tests requiring check byte.
- Tests: `part_off_web_e2e` wired into `tests/run_tests.sh` (class G) on combined-v3.
- **Not** included: M7b UI, native build, device, PANIC overlap proof.

### 7.3 M7b `ui.c`

- **Not started** (#832 c6653641857).
- Depends on **`ui.c` ownership** after PANIC (#839 / full chain) and other ui.c admissions (24.5 LCD/SMC fixes).
- Must not begin a second ui.c writer lane until the integrator names the frozen base tree for UI work (**INFERRED** process rule from serialize list; ownership assignment = owner/integrator).

---

## 8. Open questions / risks

Each item labels **who can decide**. Owner decisions are **only the user (`gobbajogger`)**.

| ID | Question / risk | Decider | Source |
|---|---|---|---|
| D1 | Is 24.5 NIGHTLY exact `8be16486` bytes + test closures, or a new candidate? | **Owner** | master-plan §5.5 |
| D2 | Confirm selection rows pinned to 5f578fc0 (6 fixes ± #111–#113) | **Owner** | master-plan §5.5; selection c5179005 |
| D3 | #13 PARA editing remainder required vs queued | **Owner** | ROADMAP #13; master-plan D3 |
| D4 | Who presses publish vs M11 automatic | **Owner** | ROADMAP M11; master-plan D4 |
| D5 | Native build mandatory for 24.5? | **Owner** | ROADMAP checklist; #822 review notes via master-plan |
| D6 | Keep zero FAIL/SKIP/NOT_RUN bar? | **Owner** | ROADMAP pre-publish; AGENTS v3 |
| D7 | Restore native-build auth (401) | **Owner** | publication-blockers §1 |
| D8 | 0.0.25 scope beyond M1/M8/M9/M10–M12 (M7/M4/M6/cache/…) | **Owner** | master-plan D8 |
| D9 | M13 schema/protocol + downgrade | **Owner** | ROADMAP M13 |
| D10 | Canonical rules lane; authorize #842? | **Owner** | #842 draft; master-plan D10 |
| D11 | Waiting-on-owner L4/L5/L7/L10/M5 | **Owner** | ROADMAP |
| D12 | Orphaned producer work ownership after handover | **Owner** | master-plan D12 |
| D13 | Confirm M7 Option A (veto window) | **Owner** | master-plan D13; Dev Option A already implemented pending QA |
| K-ui | ui.c multi-writer collisions | **#698 integrator** schedules; **Owner** if policy | §3–§5 |
| K-panic | PANIC private chain vs public #839 | **Owner** / #839 producer | master-plan §3.5 |
| K-ops | Are native_build / independent_qa / catalog automations still running? | **Owner** / automation operators | master-plan K1 UNKNOWN |
| K-698-op | Live #698 operator UNKNOWN | **Owner** | master-plan §3.6 |

### 8.1 Contradictions found between sources

| Topic | Source A | Source B | Handling in this plan |
|---|---|---|---|
| Host FAIL count on 24.5 | master-plan / publication-blockers: **366 PASS / 18 FAIL** | #832 c6654544506: **19-FAIL baseline** comparison NOT_RUN on M7a tree | Report both; do not pick. Classify FAILs on frozen tree before NIGHTLY claim. |
| Can gate run on 5f578fc0? | master-plan §5.1 (earlier): gate exits 2 because selection/evidence pinned to old blob `4bfb141f` | xport5 run with selection **`c5179005`** against ROADMAP **5f578fc0** produced PRESENT=2 MISSING=144 QUEUED=63 | Prefer **live xport5 evidence**; treat master-plan sentence as stale relative to c5179005. |
| PR-789 / PR-800 “in 0.0.24” | ROADMAP status in 0.0.24 | Both still **draft** open PRs (list_pull_requests / master-plan §5.2) | Flagged inconsistency; owner/#812 clarify (master-plan). |
| M11 auto-publish vs “owner publishes” | ROADMAP M11 | Task/master-plan D4 tension | Owner D4 only. |
| M7 status “missing (mockup only)” vs Option A code | ROADMAP M7 status text | Pushed combined-v3 with host evidence | Status text not yet updated; code exists but **QUEUED/unadmitted**. |

### 8.2 INFERRED items in this document

- Areas columns marked `*` (title-keyword heuristics).
- M13 areas `seq.c` from #824 scope.
- M8/M4 “likely ui.c” touch.
- Process rule “no second ui.c writer until integrator names base” (derived from serialize constraints, not an owner quote).
- Merge-step numbering is a **proposal** for #698, not an integrator commitment.

---

## Appendix A — Key open PRs (survey)

| PR | Head (12) | Base | Draft | Role in plan |
|---|---|---|---|---|
| #698 | 02cc7d104254 | nightly | True | WIP: one-build rev13 integration, UI glyphs, freeplay rate p |
| #812 | 5f578fc0642a | build/0.0.24 | False | docs: master tracker in ROADMAP.md |
| #814 | dcc9833f24b2 | build/0.0.24 | False | perf(miami): cache MG and glide coefficients without sound c |
| #816 | ea47aa52c0d9 | artifacts/nightly-dev | True | Prepare separate 0.0.25-rc1 QA entry |
| #817 | 53849739f1fc | build/0.0.24.1 | True | Keep Miami p-lock edits to one lane |
| #820 | c3d35f80410c | build/0.0.24.1 | True | Reduce SMC popup and header LCD repaint delays |
| #821 | 179ab3840b8d | build/0.0.24.1 | True | Reduce live key lag by bounding LCD transfers |
| #822 | 46d3e95251f1 | build/0.0.24.5 | True | Block nightly publication when ROADMAP source evidence is in |
| #823 | 6f0606c1c5a2 | build/0.0.24.5 | True | test(smc): restore remainder checks for caption-aware symbol |
| #824 | a12ea10c3488 | build/0.0.24.5 | True | feat(seq): add eighth and quarter timing core (M13, partial) |
| #839 | 69351a26a756 | nightly | True | M1: Add panic chord core |
| #840 | ac404b4f52af | chat/025-rc1-source-c32c54ac-20261103 | True | QA2: Repair RC1 UI host fixtures |
| #841 | f14ed88126da | nightly | False | docs: reconcile cloud, Grok and stage handoff rules (nightly |
| #842 | 6f027cf4154b | main | True | docs: reconcile cloud, Grok and stage handoff rules (product |

## Appendix B — Source index

- ROADMAP@`5f578fc0` / blob `a204a54c`
- gate: `/workspace/m7a-out/xport5/gate-report.json`, `gate-output.txt`, `gate-breakdown.txt`, `roadmap-row-diff.txt`
- #832 comments through `6654589751` (incl. 6653465505, 6653641857, 6653684011, 6653704790, 6653787449, 6653788083, 6653921403, 6653962863, 6654000433, 6654007069, 6654544506, 6654589751)
- PRs #698, #812, #814, #816, #822, #839, #840, #841, #842 (get_pull_request / list)
- Drafts: `/workspace/dnbox/master-plan.md`, `release-plan.md`, `publication-blockers.md`, `workflow-master-plan.md`, `executor-status-832-20261105.md`

**End of plan.** No owner decision, merge, or publication is implied.
