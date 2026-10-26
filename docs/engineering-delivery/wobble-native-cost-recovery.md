# Original wobble native flash recovery

The inherited private DEV fixture exceeded its real linked application flash
window by716 bytes on both SMK-37 models. This bounded change makes the original
alternative fit that same window, preserving all existing DSP arithmetic,
project/audio formats, buffers and memory reserves. It does not expose a public
engine selector or qualify an installable instrument.

## Exact source and change

Base PR219 commit `6391d0ec834a983609bdb5e5e1bdd8d91f05dc8d`, tree
`973155edf20824585936f34166f9c153790195e7`; its tested-to-final diff was verified
to contain only documentation/receipt changes. The inherited receipt SHA256 is
`e0d87b7e66c2cbd90dba4c96ad9ea3e34b9f5cf1782d437621b3d5431dee6fd4`.
The canonical engineering contract is pinned at
`e2482a5039527195e346925c751ac3e3a333ada9`, SHA256
`2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.

Final tested code: `fa26655c310a067ade76e5a5d08746f97bf34d1f`, tree
`5617b36e7e0a119b02bbcbe7f28c34d7225d33b3`. Later evidence changes do not alter
compiled source. One integrator edited an isolated clone; predecessor checkouts
and the preset owner's files were not changed.

The engine outlines repeated gain-target setup and its small setter, and keeps
the render stretch as a separate function. Eight setup/project/loop/UI functions
use the existing Clang minsize hint. Engine remains -O2 and DSP remains -Oz;
expressions, state ordering and samples are unchanged. The synth compiler fence
uses canonical `__asm__ volatile` spelling, the same empty memory barrier as its
old `__volatile__` alias. The unchanged source scanner explicitly permits that
canonical empty barrier. Native recompilation produced byte-identical engine
objects for the spelling correction on BOTH models; no check was weakened.

## Measured result

Both actual pi32v2/r3 Elite and Pro final links exit0. Both final linked
soft-double, ROM-reference, RAM-text call and flash/RAM/pool checks pass; the
unchanged source MMIO scanner also passes. Actual app image is552556 bytes,
versus inherited failed-link load553388:832 bytes recovered,116 bytes spare in
the unchanged552672-byte window. No soft-double helpers or ROM references;
RAM-text has1082 instructions and no calls. Linked text548080, ram_text3500,
c1_text608, data368 and bss97700 bytes.

RAM occupancy including alignment is98084/98304:220 bytes spare. Pool335740
of344064 leaves8324 bytes,132 above the required8192 reserve. All linker regions,
stack reservations, buffers, generated break/drum assets and reserve settings are
unchanged. Default/release exclude WBL1, whose16-byte/schema1 retained bank and
144-byte context are unchanged. Native303 remains ID1/schema1 and16 original pots.

Static linked frames: engine_render88, render_sub92, mix_targets20, glide_set0;
unchanged wobble render76, waveform20, wrap4 and mono adapter100. Known direct
WBL chain is280 bytes and compatibility-mono chain380, each60 bytes above the
inherited known chain. These are observed partial chains, NOT total ISR bounds,
stack high-water or target cycle/deadline proof. Target timing and high-water
remain NOT_RUN; no stronger realtime or device claim follows from fit.

Early bounded probes honestly retained their failures: noinline mixer alone
reduced overflow716→396; cold setup reduced it to44; additional metadata hints
had no effect and were removed; shared gain setter reduced it to28; outlining
render_sub closed it with116 spare. The first Pro combined pipeline failed only
the inherited fence-spelling source check after its actual link and linked
checks passed. Final corrected BOTH-model pipelines pass with exact object
identity mapping and actual relinks. The uncertain historical foundation full-link
action remains NOT_ESTABLISHED and was not retried. This is a comparison with the
exact combined716-byte failure, not a claimed foundation full-image delta.

## Host and independent review

Independent QA against the final engine/header closure passes backend normal and
ASan/UBSan lifecycle, transactional bank, fragments1..256, registry/exclusion,
526336 bit-exact303 samples,70509 state/layout checks, default whole-app audio,
and stereo with nonzero side/reverb/delay against dedicated-array reference.
An independent six-bank direct oracle passes394752 samples, normal and sanitized.

Default WAV1267756 bytes retains SHA256
`a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`;
stereo/send reference1267756 bytes retains
`db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99`.
Private audio is not committed. Review and precise closure/reuse receipts are in
`evidence/wobble-cost-recovery/`. No audio equivalence to a proprietary instrument
is claimed. Inherited fidelity/history/alias limitations remain open.

## Build identity and limits

Pinned local Linux/amd64 image
`sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`,
clang SHA256
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`,
SDK revision `d179b4484759423312073f5fbb232501aa491047` tracked clean before/after.
Vendor inputs were read-only; network none. Both DEV_PADCAL and DEV_WOBBLE=1.
Complete commands, timings, tool/generated-input/object/output hashes and actual
linked checks are in the receipt and final-native.json/log. Unchanged actual
objects and generated headers were verified byte-for-byte against predecessor
receipts before reuse; changed engine was compiled for each model. Private maps,
ELFs, app images and assembly remain with the isolated producer checkpoint.
A fresh full reproduction uses tools/build.py --board smk37 --model elite/pro
with those two DEV flags. The retained incremental helper requires its recorded
unchanged object closure; it is not a general validator substitute.

No package was produced or controller pairing qualified. This is
APP_LINKED_POSTCHECKED plus scoped independent host QA, not DEV CHECKED,
installable, full timing/stack/hardware acceptance or a release. dev_validate.py
still strips the alternate flag and cannot certify this fixture. No protected
merge, GUI/hardware write, Actions execution, public binary/audio or licensing
change occurred.

## Deferred scope and ownership release

Frozen refinement `ad206210f4909f31f992475c58d0696fb2a115fb`, remote
`9b94d154643a041586997353d32675a9fd1c4a4f`, tree
`ffd67d7fdc1a18aa5bfbfb084796a89201128ae5`, receipt SHA256
`16c3f1aab0590b131eea30f5470d6e84fbf0f01f99d0a8d4129018efa4d3c0df`
was verified, including every recorded source/dependency hash, and NOT imported.
Only116 flash bytes remain and known stack cost rose60 bytes; consuming sound
changes here would reopen this bounded exact-candidate cost/QA result. Its native
cost remains unknown. A fresh explicit import must preserve the adapter/fastmath/
MIT-helper closure, measure both models and run affected independent QA.

Preset hooks are DEFERRED: no engine_preset_refresh/base/snapshot/bridge hook was
imported and no engine writer lease was granted to the other lane. Proposal
patch digest `68760d89417795ab1ed367a5b06a46f554c85f59af188fed7b096442545bd29b`
is recorded as an interface-only observation. The assigned preset successor
`01a17326-7fdd-7613-8a6a-8ed3cc65a641` retains that separate work. Its96-byte model
plus runtime snapshot/bridge/code and controls-owner hooks need their own combined
resource and behavior proof;220 RAM and116 flash spare cannot establish fit.

This stage releases source ownership to active outcome coordinator
`01a1730a-4ccf-7430-b295-11692ec23090` for assignment. No successor was created:
the assigned coordinator retains continuity and the separate preset successor
already exists. Next source work must start from this exact frozen receipt;
retain all remaining gates instead of labeling the fixture finished.
