# Web-site lane status

Owner: web-site. Branch: codex/web-site. Base: e888d046bd16d7f9f64e9c824ca4027570bb38fe. Scope: web/, tools/make_pages.py and matching web checks. Local commits only; no push, publication or connected-device access.

## Evidence and progress

- Read ../feature-audit/FEATURE-MAP.md and original descriptions for all 14 assigned items.
- Checked firmware PR #34 at 1d7561c6b3c6ed3b042e8d8b4eaa727b8a9beee4 and #66 at 923ad3c0824b41f50ec48706aa60e2e87fee9ed1, including review findings. Their single-writer/reconnect and strict archive successors are retained in the base; no whole-PR replay.
- Checked site PR #90 at 9df2dfb7afe172f75d1ca8866edffc72657beb12. It records merged workspace repairs and byte-bound version assignments, and explicitly retains EC035 (cross-client write race). It is evidence, not proof that this firmware worktree contains that separate site.
- Other lane scopes are taken from the current owner instruction and git worktree inventory. No project-chat reader is available. No other lane is edited.
- Ordinary local workflow selected; no PR or merge mode requested.
- Design proposal in preparation. Next: independent review, implementation, Node/headless tests and requested host/build validation.

## Feature acceptance

- 75 partial: unified install/data workspace pending.
- 77 partial: DnBOX shared Cyrillic/Latin font and styling pending.
- 78 partial: actual manual/news exist; shared navigation/history pending.
- 79 partial: retained scan serial and write-lock guards; visible stages/states pending.
- 80 partial: retained local-file serial/model/size checks; prominent chooser and adjacent errors pending.
- 81 partial: early unsupported state pending.
- 82 partial: responsive/focus/live status/reduced-motion validation pending.
- 84 partial: preserve catalog/package retention; honest history pending. No invented package numbers or publication claims.
- 85 partial: retained OTA and slot migration guards; fresh-target and snapshot checks pending. Atomic cross-client snapshot-to-OTA needs firmware support.
- 87 partial: sample preparation under install/recovery pending.
- 89 partial: existing speed-up/storage-rate algorithm retained; integration and memory disclosure pending.
- 51 partial: strict archive checks retained; complete fresh snapshot and target checks pending.
- 53 partial: projects/sample connection epochs pending.
- 55 blocked for personal-data acceptance: no owner backup selected and device access is forbidden. Synthetic completeness checks remain authorized.

## Validation

Not yet run on a changed candidate. Firmware is unchanged (Chicago/Miami only; project revision 13 retained). Requested X0X_ORIGINAL_SYNTH=1 Docker build and tests/run_tests.sh will be recorded with actual flash/RAM margins. No flash or device test is authorized.

## Commits

None yet.

Design proposal: web/workspace-proposal.html. Palette: #eaf0f5 ground, #192c40 ink, #465d73 muted, #ffffff paper, #234ea0 action, #b5c5d6 boundaries. The shared Cyrillic/Latin face is Golos Text (OFL). Actual ordered connection steps supply the visual hierarchy; the proposal avoids decorative device states. It preserves existing safety paths and makes unsupported/offline behavior explicit.

Independent design review identified narrow-screen file-input overflow and small header link targets. The proposal now constrains file inputs and gives header links 44px targets; these become actual headless acceptance checks.

## Implementation checkpoint

The unified DnBOX workspace, shared self-hosted Golos Text font (OFL), current Manual/What's New and catalog History are implemented. Sample preparation remains below install/recovery and transfers exact processed PCM/ADPCM to Samples. Same-version catalog updates retain older models; missing or changed historical package bytes refuse generation.

Initial validation: make_pages_test.py PASS; test_breaks.mjs PASS; test_projects.mjs 34/34 PASS; test_projects_page.mjs 7/7 PASS. Required independent generated-browser review PASS at 320/390/1440 with MIDI disabled (build/web-site/review-browser.json). Mocked sample page initially failed its old success-caption expectation; actual source now reports complete versus partial restore. The test expectation was updated and remains under rerun.

Independent correctness review found two real issues: CRC-only comparison did not justify a byte-for-byte restore claim (fixed to compare bytes), and reused MIDI port objects could bypass reconnect detection (a latched session watcher is being added and regression-tested). These findings are not marked PASS yet. Firmware build is still running in x0x37-web-site-build-20261030. No device was accessed.
