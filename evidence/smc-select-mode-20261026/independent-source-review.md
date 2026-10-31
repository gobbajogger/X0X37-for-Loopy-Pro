# Independent SMC SELECT source/host review

Verdict: **PASS**, no blocking source finding for candidate `bbb6eba673582bbb9a0b35db961ec64dcbbfc3ee`, tree `130f2695a75599c3b533b54385538a4dc92ecf1f`, against base `d633833ac369f8466bc2563791614206070e87d1`. Production source blob: `5995318c34222d5909db90348552ea58484d6a66`.

The actual FX predicate now requires V_FX, no overlay and no clip-grid mode. Physical presses acquire per-bus release ownership only when accepted as FX. Releasing in another mode clears the original bus without acquiring a new selection action. Leaving the FX context cancels active and delayed main-strip holds; a main tap that navigates out of FX also clears another simultaneous source hold. Reconnect, project busy and lost-release clearing reuse smc_fx_clear and clear the new ownership mask. Existing target locks still park navigation.

Exact semantics: physical notes24–28 in FX operate the configured source-bus effect; this is not universally low-pass. ROOM page's 303B SELECT note27 defaults to low-pass. Notes29–31 in FX retain the existing short-tap navigation versus >=180ms momentary main-bus hold. Outside FX the five source SELECT buttons choose their ordinary part/drum context, and main SELECT buttons use existing view/page mappings. No preset/engine/bass source was changed.

Independent commands:
- `sh tests/host/smc_select_mode_test.sh`: PASS842, real production queue/UI/engine dispatch, not a stub of smc_fx_try.
- `python3 tests/host/smc_buttons_test.py`: PASS, ordinary mapping assertions updated to intended SELECT semantics.
- `sh tests/host/smc_lock_test.sh`: PASS701. Old incorrect HOME-as-FX expectations were replaced with ordinary source selection; explicit V_FX momentary-effect assertions retained.
- Reviewer-only overlay/confirmation/clip transition probe: PASS144, covering all eight SELECT notes, pending and active holds, context exit, and late release after returning to FX.
- Identical new production SELECT test compiled against preserved base smc_mixer.c: expected runtime FAIL at line15 (non-FX SELECT erroneously acquires FX), exit1. This was an assertion failure, not a compile failure.
- `git diff --check d633833a..HEAD`: PASS.

Exact command environments, source/test hashes and log digests are in receipt.json. Logs and reviewer-only synthetic probe are in this directory. No production source or committed test was edited by this reviewer. Native Elite/Pro/controller capacity/qualification and physical device behavior remain separate coordinator gates. This review does not authorize or claim flashing, publishing or readiness.

## Harness-only follow-up review

Verdict: **PASS** for `94061cf46483718bb63397d3c5fca8ca3d7ebb63`, tree `df01941b2bc136ac911632ae0c8de589405ee230`, relative to `bbb6eba`. Only the Solo-overflow and reachability harnesses changed. Production source blob remains `5995318c34222d5909db90348552ea58484d6a66`; the prior842 SELECT/144 boundary/701 lock source evidence remains applicable.

The isolated Solo fixture already substitutes FX entry points and dispatches only Solo notes. Its new no-FX-context helper resolves the extracted production release function's new dependency; it does not replace Solo queue, release generation or overflow handling. All24 prior runtime scenarios and assertions remain.

The full production reachability fixture now asserts actual ordinary SELECT part/view changes outsideFX, then tests the same real input inFX. Physical Note routes require release; stock User MMC routes require a second toggle press, matching their lack of key-up events. The interrupted-SysEx path and Help path retain explicit assertions and now require the intended nonFX selection without a momentary effect. No route or refusal assertion was removed.

Independently executed on the committed follow-up:
- `python3 tests/host/smc_solo_overflow_test.py`: **24/24PASS**, exit0.
- `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE sh tests/host/smc_reachability_test.sh`: **all five routesPASS**, exit0.
- `git diff --check bbb6eba..94061cf`: PASS.

Original implementer before logs were inspected: Solo failed compilation because the helper was absent; reachability failed a stale HOME-as-FX runtime assertion. The repaired coverage does not relabel either failure as a pass. Exact commands, hashes and log digests are in harness-receipt.json. No broader or native qualification claim is made by this extension.
