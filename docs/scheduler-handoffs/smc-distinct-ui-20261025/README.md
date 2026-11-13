# Distinct SMC controls and quiet page navigation

Reason: populated mixer knobs duplicated faders, while unchanged absolute fader
packets could reopen the temporary screen after SMK page navigation. The musician
needs useful independent controls and a clear indication of the touched destination.

Purpose/design: preserve every fader, channel, acceleration and button/transport
mapping. HOME/MIX PARTS knobs add part pan, reverb decay, delay feedback and filter
resonance; drum MIX/SEQ knobs find the matching voice Pan descriptor in the active
bank. GLOBAL/PERFORM K3 and MIX/MASTER K4 use resonance. Other ordinary rows already
owned by any fader park with ON FADER n feedback instead of a substituted parameter.
Specialized clip editors retain their existing field ownership.

The focused destination/value now occupies the main display, with real page or
kit/bank context, explicit pickup direction/target versus ACTIVE, and a smaller
quiet overview. It uses existing fonts/rendering and bounded stack buffers, with
no graphics allocation and the same 1,200 ms inactivity timeout.

Production-path reproduction: panel navigation and smc_frame alone are silent.
After navigation, an unchanged absolute packet previously reached unconditional
smc_show/smc_ui_begin even without changing a parameter. Pickup history is reset
on target changes, so it cannot establish fresh physical activity. One physical
history bit per strip now survives pickup refresh; repeated samples still seed
normal pickup crossing history but cannot open/extend the screen. Nonzero relative
turns and real changed fader samples retain feedback. Hardware echo is a hypothesis,
not an observed cause of a particular physical-device packet.

Expected benefit: independent mix dimensions, readable touched-control feedback,
and no overlay flash from the reproduced unchanged-sample navigation sequence.
Host/frame/build results are recorded alongside this file; device behavior needs
separate owner testing. No hardware, raw capture, publication or protected merge
is performed by this source/build stage.

## Inputs and boundaries

Accepted base NIGHTLY: 93449136baae02c9715ba20edc5c43176786b635, refreshed before
editing. PR150 remains at 025fba5a777b74012bf0cc1746fda207482423bc; its unbound Pause
lock is not integrated or activated. Future integration must snapshot
smc_knob_target independently from smc_target and preserve full pref_t identity,
kit/bank/sequence/song context and the physical-history/presentation distinction.
Do not copy fader identities into knob slots. Re-run exact locked/unlocked maps,
parked-row and navigation-provenance checks on the combined source. Physical Pause
input and LED binding require their separately owned evidence.

PR122 b037edac22f914bd14c6ca7aaa2988a530abd13d routing changes remain separate.
Current direct BLE SMC topology is unchanged; no draft multi-peer proposal is
imported. No controller/ABI/build/package-source changes are intended.

Workflow: reviewed trial e2482a5039527195e346925c751ac3e3a333ada9,
docs/engineering-delivery/SKILL.md SHA256
2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3.
Native method/input evidence is reused from task17; unchanged matching controller
SHA256 956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a
is reused after exact closure checks, then fresh applications are composed and
checked per model. CTL_NO_CACHE=1 remains set (installer cache protection, not a
compiler-cache flag). This candidate is BUILD_ONLY until the relevant later gates.

## Verified source, tests and BUILD_ONLY artifacts

Built and independently reviewed source: `4e9d06f542058413a34472ff0e84d88c1360af6c`;
root `2466b26661a93f763abf14b9df3941567f4476b2`;
firmware tree `3ea8fd24d5d65cfdc119cb3f3083a92f2e6bc60d`.
Later evidence/documentation commits leave all compiler inputs unchanged.

[Independent source review](source-review.json): ACCEPT_SCOPED_SOURCE_AND_HOST_QA.
All 17 affected suites passed in the independent isolated checkout; 4,810 UI
assertions passed under ASan without diagnostics. Five runtime-negative mutants
were detected. Drum controls/pads/audio passed 124,992 checks with zero failures.
These are host observations, not physical LCD or control-surface proof.

[Independent package review](artifact-review.json) and [input reuse](input-reuse.json)
verify the exact model/chip/app/loader/controller payloads, nested/header/file CRCs,
flash bounds and erased padding, deterministic recomposition, and wrong-model,
controller-pin and controller-size rejection. Both fresh builds exited zero in
pinned offline Linux/amd64 image
`sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.
All 640 toolchain manifest entries and 16 pinned SDK/tool files match task17;
controller source, shared ABI, build/package scripts and assets are unchanged.
No fresh controller compilation was necessary. Its accepted ELF/map hashes remain
verified in the private input-reuse record.

Both apps are 550,012 B; each has 2,660 B code free, 412 B static RAM free,
and 8,324 B pool free (8,192 B required). The reused 65,492 B controller has
44 B flash free and 544 B RAM free, ABI 2.

| Model | Bundled bytes | SHA256 | Whole-file CRC32 |
| --- | ---: | --- | --- |
| Elite | 651396 | `448dbfecfc558e34528467c2db5b9cada9bff5f5e4b8dcc9bd9ccff9e234b571` | `d74116ee` |
| Pro | 651392 | `5c6a28a76b7f62e9dd1fe1a9490b1b127830d52bfe4f52c14a812dad9eab0e9e` | `4d92f8a9` |

Private native inputs and packages remain at
`/Users/mac/Documents/Codex/2026-10-25/task-41/native-source/`.
The exact paths for app, loader, plain package and controller-joined package are
in artifact-review.json. No binary, vendor SDK/toolchain or raw capture is committed.

[Frame manifest](frame-manifest.json) names actual production 240×240 PNGs.
[Prior pickup](frames/00-prior-pickup.png), [focused pickup](frames/01-main-pickup.png),
[part pan](frames/09-part-pan.png), [808 SEQ bank 2](frames/10-808-seq-bank2-pan.png),
and [parked knob](frames/12-parked-knob.png) were inspected headlessly.
The prior frame is reused only from the identical accepted SMC source hash;
new frames come from this tested source. Frame inspection supports layout claims,
not physical LCD legibility measurements or hardware performance claims.

## Corrections retained in the evidence

The baseline production-path test reproduced the unchanged-packet reopen after
panel navigation and after drum-target/pickup refresh. An initial independent
review also found confirmation parking clearing physical history; that regression
was reproduced and repaired before final tests/builds. Existing test fixtures that
expected shared level targets were updated to assert independent pan and unchanged
fader levels. The extracted clip fixture needed declarations for new routing
helpers; full production clip tests also passed.

The first build at 5effa44 failed because firmware has no strlen implementation.
That call was replaced with bounded end-pointer trimming. Its failed log remains
private under `task-41/failed-build-5effa44/`. A subsequent shared-output build was
stopped to isolate generated files from host tests; it is not qualification evidence.
Only the final frozen-checkout, exit-zero build is accepted here. No failed run is
reported as passing. No source/build permission denial occurred.

STANDARD/MEDIUM was requested for bounded agents; the delegation tool did not
expose a service-tier selector. Effective tier is not verified by these receipts.
No fast mode, GUI, hardware, Actions execution or release/full gate was requested.

## Next hardware handoff

Next owner: the parent/integrator and separately authorized native hardware owner.
First bind the proven task17/task19 native-prep adapter to these exact new source,
model/loader/plain-logical/bundled/controller hashes and reviews. Preserve singleton
model/port checks, staged loader validation, fresh atomic backup, slot map,
interruption handling, readback and restore verification. A record adaptation alone
does not require rebuilding unchanged accepted inputs. This is BUILD_ONLY, not
DEV CHECKED, installable qualification, hardware PASS or publication approval.

After separate authorization and exact-candidate prep, test each affected model:
HOME/MIX knob independence; all 11 voice pans in both kits and both banks; parked
list rows; pickup direction/acquisition; real movement/encoder/action feedback;
idle/rapid SMK page navigation; active-screen cancellation; repeated/no-change
packets; clip fields and 1.2-second return. Preserve data and retain the prior
accepted task17 model package/controller pair for rollback. If a new physical packet
sequence still opens the overlay unexpectedly, identify that sequence in a separately
authorized native investigation; generic echo suppression is not proven here.
PR150 follow-on integration and physical Pause/LED proof remain separate above.

## Later base observation

During final PR readback, NIGHTLY advanced to `fbc23a4a7f0c9486a84f013ec54cf0aec4d22dfe`
(PR138 workflow docs/tests and PR111 read-only wire test/evidence). The exact
firmware/assets/ble/tools/host/build.sh comparison against the reviewed base
exited zero; a read-only merge-tree check found no conflicts. No application
rebuild follows from those unchanged compiler inputs. This branch remains based
on accepted 93449136 to preserve the exact reviewed/built candidate. The normal
integrator still owns fresh combined-tree/protection checks; no protected merge
is performed here. See late-base-observation.json.
