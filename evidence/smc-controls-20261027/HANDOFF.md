# Reviewed SMC candidate for task01a17891

Sole source owner: this SMC lane. Sole merger: task01a17891. No hardware, native build, publication or competing merge was performed.

Branch: `lane/dk-smc-lock-clock-cue-20261027`. Base: `504edff4ae49481b2d7c54e3c914d0b305e81358`.

The earlier reviewed lifecycle commit is `feff1034ca547e6397d080f9e5979957965d61cb`. The later commit containing this handoff has final production blob `182eea98bbeb2f81d22055caa436eb08cf35d244`. Verify the source blob at the branch head before integration. Tests and generated host-input hashes are in `navigation/receipt.json`; exact final host run is `navigation/host-final.log`.

| Owner acceptance | Actual result |
| --- | --- |
| Rec8/Solo8/Select8 repeated paging | PASS within entered pattern/sequence/song, sound/generator and MIX workspaces; repeated held positives dispatch once; advance/wrap tested |
| Direct drum-entry strip8 role | RETAINED drum Record/Solo/Select, including bank2 inactivity; an always-global mode-key override is PENDING owner choice and is not claimed |
| Up/Down knob-bank only | PASS; visible view/page and clip stage unchanged; opposite direction, all drum pan/send banks and label/edit mapping tested; menus retain row ownership |
| Actual physical FX paths | PASS all 32 DAW row actions, release, state mutation, captions and wire LEDs; hosted/User decode and source latch/main-MMC tap behavior tested |
| Home+SELECT/LeftRight | PASS; actual3e7→504 assignment regression reproduced; FX Home tap is deferred while held, consumed modifiers stay FX, plain tap navigates once; Help dismissal retains priority |
| Lock intentional navigation | PASS FX, source/drum/page and bank gestures without redirecting captured faders; explicit bank changes rebind knobs only; direct drum8 preserved |
| Overlay lifetime | PASS long and overlapping holds, 1200 ms after last release, real and pickup-pending movement renewals, duplicate/clock silence, loss/disconnect/reset cleanup and timestamp wrap |
| 600 ms unlock | PASS fresh deliberate hold, not initial capture press or short tap; dropped+accepted cross-route release ordering covered |
| BPM feedback | PASS at 60/120/180 BPM, playing sample phase, external MIDI beat phase and steady lock; physical lamp identity is not a hardware PASS |
| Pending pattern cues | PASS accumulated forward/back, reversal cancellation, bounds and current/captured-part isolation |
| Earlier exact working Pro pin | NOT ESTABLISHED by retained install evidence; Elite install receipt and owner-reported older source are attributed without claiming behavioral PASS |

Final frozen-source proof: 18 registered scoped host selectors PASS. FX binding 446 checks, UI 6548 checks, lifecycle 240 checks and combined navigation/lifetime 122 checks. Old-source and rendered-pixel negative controls reach runtime assertions. Development mutation runs rejected 14 UI and 9 interaction mutants at their recorded source pin; they are not represented as final-source reruns.

Sol source review: gpt-6.1-sol APPROVE on the final production blob, three rounds for navigation, overlay and FX; N1 documentation and N2 rendered drum ownership resolved. Earlier lifecycle review approved after resolving the deferred unlock/loss ordering finding. Review is source evidence, not independent native execution QA or device validation.

Actual compact physical mapping, comparison with source 3e7 and 504, screenshots, known limitations and installation-history evidence: `navigation/FX-MAPPING.md`, `navigation/function-history.json`, `navigation/screenshots/`.

Next stage: task01a17891 integrates these source/test changes through its normal controlled branch order and validates the exact combined Pro candidate. Latest observed firmware nightly was `ebf11813bcbba6bae2c650415063acf145dae2e1`; its Bluetooth role-switch change touches ui.c while smc_mixer.c/main_fm1.c remain unchanged from 504. Bass/UI PR629 qualification remains separate. Carry the explicit direct-drum strip8 limitation into the owner checklist; do not label all strip8 contexts globally bound.

## Final bounded Record-feedback consumer disposition

Production navigation/FX/overlay commit: `30c49688aa58e28bcaeaca72c0481fd9dc2ce459`; production blob remains `182eea98bbeb2f81d22055caa436eb08cf35d244`. The final tip adds only the specific Record-feedback fixture, host linkage and evidence cleanup; it does not change firmware.

The original fixture was replayed against PR629 `39cff954042ff9717acd8f56b0d0d66f58f23494` and failed at its reported line93 because it expected stopped step entry from EDIT. The fixture now enters visible SEQ through actual panel input and retains destination, gate, write position, other-bass and emitted Record assertions. The producer default passes4 writes/7 projections; the isolated feature-on PR629 plus frozen SMC source replay passes4 writes/11 projections and separate stopped EDIT/no-hidden-step checks for both bass parts and both drum kits. Those EDIT checks intentionally reject pre-split source through a runtime pattern comparison.

Required direct consumer command after integrating the PR629 split:

```sh
X0X_HOST_DEFS='-D_POSIX_C_SOURCE=200809L -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0' \
  sh tests/host/smc_edit_no_write_test.sh
```

This is an explicit consumer gate, not a base504 registered selector or a runtime heuristic that could hide a regression. The ordinary `smc-record-feedback` selector continues to check actual visible-SEQ writes and lamps. Feature-on host linkage includes the available factory-bank object. The owner-reported Mac `RUSAGE_INFO_V4` compilation problem is separate; Linux reached and verified the assertions. See `record-consumer/receipt.json` and logs.

The fixture-only follow-up received one GPT Sol APPROVE with no findings. The owner requested wrap-up at the nearest safe checkpoint: no new implementation/review loop, hardware action, competing merge or broader qualification follows this handoff. PR629 is not final qualified. Existing sole merger task01a17891 owns the next integration decision and combined native gates.
