# Current one-build UI integration acceptance plan

## Scope and pins

Reference recovery base: `094bb1a55c159aefb0e2bfa39790a1b4b74cee40` (`wip/one-build`). Historical UI references and individual origins are in MANIFEST.json. This is a plan for the existing one-build/UI producer and retained independent reviewer, not a new implementation claim. Freeze and record the actual integrated candidate SHA, changed paths, dependency closure and reference manifest hash before testing. Keep reviewed source fixed during review; only affected proof changes require rechecking.

Preserve Chicago/Miami original-engine behavior, project revision13-only loading, current themes, current parameter semantics and later functional fixes. Do not import the old project schema, legacy engine implementations or historical build flags. No private-header retrieval, SDK redistribution, device action or publication is authorized by this plan.

## Required positive and negative cases

| Area | Required acceptance and evidence |
|---|---|
| Settings | Six top-level sections: MIDI + CLOCK, BLUETOOTH, AUDIO, KEYS + SCREEN, PROJECT, SYSTEM. Preserve all 28 original action identities and current applicable additions; map every historical action to its current destination. Enter/back preserves selected section and scroll; HOME/GLOBAL transitions are deterministic. Navigation does not mutate the project. Held entry/repeat cannot change a value accidentally. Reset confirmation cancel preserves state; test long enums, empty/disconnected names and long peer names. |
| Semantic glyphs | Check each affected real caller at minimum/midpoint/maximum, selected/unselected and unchanged-value redraw. Validate meaning rather than only image equality: filter, drive, loop, waveform and continuous controls must reflect the actual current parameter. Reused/cache glyph state must not leak across pages, parts, banks or theme changes. Preserve current engine-specific ranges and labels. |
| Bars and effects | Actual value bars and clipping/selection states must remain visible at endpoints. Reverb TONE/Decay retain current semantics; do not implement withdrawn TONE redesign. Master/filter and send-effect controls must remain mapped to their real current DSP meanings. |
| TAPE | Delay input and wet-tail signal cause the intended activity, including stopped transport; silent running transport must not animate as fake activity. Idle freezes the phase; resuming does not catch up elapsed idle time. Mute, zero return, DIGI mode, reset/project hold and late silent segments cannot leave stale motion. Reuse historical cases only after mapping to current telemetry rather than copying old engine code. |
| Headers and Bluetooth | Header/action capitalization, part identity and current themes survive navigation. Connected peer names use an immutable bounded snapshot; duplicate names and long names remain attributable to separate peers, while disconnected cached names disappear. A host render is not physical radio/LED acceptance. |
| Current feature preservation | Exercise current Chicago/Miami pages, pads, envelopes and performance display affected by integration. Keep revision13-only negative loading checks and current source-selection contract. No historical reference test may silently switch to legacy engines or old schemas. |

## Verified current command and its limitations

The following runner was read directly at the pinned base (no execution in this recovery):

```sh
sh tests/host/original_release_ui_test.sh build/host/original-release-ui
```

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/tests/host/original_release_ui_test.sh

The script generates host assets; selects original units/backends from tools/build.py; explicitly compiles with `X0X_ORIGINAL_SYNTH=1`, `X0X_DEV_WOBBLE=0`, `X0X_WOBBLE=0`; runs normal and ASan/UBSan variants; then compares logs and corresponding PNGs. It accepts an output-directory argument. Command syntax is VERIFIED_FROM_SOURCE; execution here is NOT_RUN. The selected release-screen suite is not full approved-mock parity. Its normal/sanitized equality only compares those executions with each other, not with the approved reference designs.

The archived tests under source/0e58b960 are historical specifications. Their old runner flags and dependencies are NOT_VERIFIED for current one-build execution. The producer must map applicable cases to current supported harnesses and record exact commands before running; do not blindly execute or claim those archives passed against the new candidate. Missing private dependencies are a precise pending prerequisite, never a reason to fetch or publish restricted files.

## Actual candidate renders and review

Capture actual candidate output for all six Settings sections, back/confirmation/scroll and long-name states; affected Home/Mix/control/footer glyph contexts; TAPE active/idle transitions; and affected current themes. Label synthetic host output as host output, with candidate SHA, exact render command, image hashes and state inputs. Compare each affected output to the specific approved design and explain intentional current-feature differences. Do not substitute the archived mock PNGs as candidate renders. Retained independent review must inspect semantic/readability/edge-state differences and the new exact-input evidence, without resetting review budgets.

## Status and downstream gates

| Check | Current status |
|---|---|
| Pinned release-runner command syntax | VERIFIED_FROM_SOURCE |
| Current candidate host runner execution | NOT_RUN |
| Current candidate approved-mock parity | NOT_RUN |
| Current candidate independent UI acceptance | PENDING |
| Full owner-Mac SDK V1.2.1 build, install and performance commands | NOT_VERIFIED in this reference-only recovery; existing build owner must verify from current source |
| Owner-Mac build / golden / performance results | NOT_RUN |
| Firmware/controller/device/hardware acceptance | NOT_RUN |

Deliver exact commands/exits, candidate pins, changed-state renders, failures and independent findings to the existing one-build integrator and QA. A posted pointer is not consumer action. A source reference PR is neither an installed package nor acceptance of the current combined build.
