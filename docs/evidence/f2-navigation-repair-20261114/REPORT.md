# F2 #15: reviewed source proposal; #16 contract pending

Holding a stopped bass step, editing cutoff, paging EDIT or opening its list,
then releasing now restores the edited live parameters to their native bases.
The saved locks stay intact. This is a source proposal for the incumbent FMB;
it is not an integrated firmware or hardware acceptance result.

## Exact inputs consumed

- Source: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, root tree
  `72d28fbc741244d18fa3d18495da16ab0fcb1d24`. All 5,345 exported Git blobs
  matched the recursive API tree before editing. After execution, only
  `firmware/src/app/ui.c` differs among those original blobs; its new blob is
  `7a7ea2b8f29ed2e1e35169c3cdc286fcfff26696`.
- Preservation commit: `7fe5eca0608294f96229eeb0221a00329707987b`, including
  `docs/evidence/f2-15-16-94197fc8-qualification/REPORT.md`, `NEXT-STAGE.json`,
  original fixtures, logs, receipts and replay. All 25 remote files match the
  recovered Library packet. All entries in its SHA256SUMS.json match.
- Original qualification Library ZIP SHA-256:
  `68853844eb6b31e14ccef4391998dca43b3200490cdd5f61c1aea8efdcd97824`.
  Prior repair intake ZIP SHA-256:
  `6d377966cff776f6ac823d7218de7cd9212763b2d8fc7062cb57b641ca789ecd`.
  The prior diagnosis and independent semantics review were consumed and reused.
- Maintained policy pin: nightly `66073c68dd80d51a7053432c2c161a8b720a1341`.
  Connected account and authorized repository owner both verify as `gobbajogger`.
  Default connected author is used; no identity override.

No unchanged baseline qualifier was rerun. Its recorded 426/12 step failures,
408/20 live-navigation failures and bounded unchanged passes remain historical
input evidence, not new runs. Original-source release identities and later
NOTE LOCK decisions remain unchanged.

## Smallest repair

One 32-bit UI field records parameters actually sent to the bass engine during
the stopped held-step gesture. `bass_step_cancel()` restores those IDs through
the unchanged `ui_restore_base()` using `held_step_part`, then clears the field.
The field covers all 32 sound-cell parameters and accumulates edits across pages.
No current-page enumeration, overlay admission or current-part check can hide
this cleanup. Unedited parameters are not restored. While playing, cancellation
discards the field without overriding sequencer-owned values.

Only three production hunks change: UI gesture storage, stopped `lock_turn()`
publication, and `bass_step_cancel()` cleanup. No engine/DSP, lock storage,
physical release/token routing, UI layout, control mapping or #16 policy changes.
Drum/BREAK cleanup is unchanged. Native base restoration still handles original
factory values; this patch does not replace it with quantized UI bytes.

## Focused execution and review

Linux x86_64, GCC 14.2.0, Python 3.12.14. Original-input verification passed
for all 10 active inputs, manifest
`9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`.
No setup workaround or SDK access was needed for these host checks.

| Check | Result |
|---|---|
| New step-navigation fixture, normal | PASS, exit 0, 718 assertions, zero failures |
| Same fixture, ASan/UBSan | PASS, exit 0, 718 assertions, zero failures; no sanitizer diagnostic |
| Existing held-step release | PASS, exit 0, 144 checks; both kits/voices, four step banks |
| Existing Free Play/release safety | PASS, exit 0, 17,957 checks; cancellation expectations unchanged |
| Independent source and changed-check review | No blocking findings; see INDEPENDENT-REVIEW.md |

The original 36 navigation cases and all their assertions remain, with a portable
include path. All 12 original failing cases now pass. Added controls cover two
edited pages, an intentionally different unedited live parameter, the other
voice, open-list release, SAVE-clear, playback ownership, and immediate cleanup
on helper-level predecessor replacement. Existing physical SEQ successor
controls remain. The helper test is explicitly distinct from UI admission.

During fixture development, an incorrect assumption that EDIT admits a second
step press caused failed supplemental assertions. A revised live-key assumption
also failed. Independent review traced both to existing admission/gesture
semantics. The final helper-level replacement test makes no claim about that
mixed EDIT interaction and changes no product behavior or old expected result.
Final per-case diagnostics reflect actual failures. Compiler warnings remain
visible in the logs, including the inherited qualifier's signedness warning.

Reproduce from the proposed source:

```sh
sh tests/host/step_navigation_restore_test.sh
SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  SMC_UI_OUT=build/host/step-navigation-restore-sanitized \
  sh tests/host/step_navigation_restore_test.sh
sh tests/host/held_step_release_test.sh
X0X_LIVE_TEST_OUT=build/host/freeplay-live-keys-safety \
  sh tests/host/freeplay_live_keys_test.sh
```

These are native-compiled whole-app Linux host checks with production UI/engine
and inherited hardware/clip substitutions, `X0X_DEV_PADCAL=1`. They are not
instruction-set emulator, target SDK, electrical-key timing, audible device or
hardware evidence. Full suite, FMB composition, native Elite/Pro budgets and
device checks are NOT_RUN. No full F2, REC, performance or UX audit was repeated.

## Ownership and incumbent intake

Only `chat/f2-navigation-repair-20261114` is this worker's proposal lane.
FMB retains shared `ui.c` and integration ownership. The current #893 comment
6859626206 identifies that owner; #896 comment 6869443460 limits REC to its
feeder and requires overlapping UI integration by the designated integrator.
Pattern, SMC, REC, flash-shrink, mod-wheel and UX lanes were not changed.

The two-line-context `source.patch` passes `git apply --check` (exit 0) against:

- #893 head `bc54e1b2440759f5f9d1c550b31e8cd57281dca1`, `ui.c` blob
  `73392ba0056ae10aa9c8f683e3d9657f7513e6c7`.
- #896 head `8b0b6d311577fcc303212016423dd4f5ea4e8184`, `ui.c` blob
  `5af70e36fe02e05287760a8e182c80d828d650b9`.

A default three-line-context patch conflicts with the mod-wheel lane solely
because it has `UI_READOUT_MS` where the baseline has `900u`. Preserve the
incumbent timeout; neither this patch nor intake replaces an entire shared file.
The apply checks use isolated copies and are not integrated execution evidence.

The existing FMB-facing #893 conversation is the intake pointer destination.
Actual receiver scoped action/consumption is NOT_OBSERVED. The unavailable
`coordination/merge-queue` ref returned 404, and no standard create_thread or
read_thread tool is exposed. Return this structured completion to creator
`01a1af1b-2207-7017-b4fb-4e7f6c2c90ea`; do not invent successor execution.

## #16 remains pending

The original 20 early-off paths remain an unresolved qualification result.
Recovered PR810 rationale and `owner_switches()` explicitly require cancellation
on admission loss. Current #891 comments 6861909260, 6862057228 and 6870128400
retain PLAN ONLY and interaction-plan approval. They do not settle navigation
retention or authorize a redesign. No tests were rewritten to force acceptance.

FMB and the incumbent UX owner must distinguish HOME list-back from HOME action,
other-part viewing from actual sound/mode replacement, and SMC presentation from
takeover. If retention is approved, preserve press-time voice/token/generation
until release while retaining Stop/PANIC/project/mode safety and stale-release
isolation. Removing cancellation alone is unsafe. This does not block #15 intake.

State: #15 REVIEWED PROPOSAL; #16 CONTRACT PENDING. No merge, publication,
SDK/source-access denial workaround or device action. No owner decision is
needed to preserve this proposal; integration and #16 disposition remain with
the existing incumbents. Remote delivery receipts belong with final handoff.
