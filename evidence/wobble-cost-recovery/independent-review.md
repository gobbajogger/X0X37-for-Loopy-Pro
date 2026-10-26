# Independent native cost-recovery review

Reviewed candidate engine SHA256:
`02eabd6e48fc59bc5623cac6826b77a13d0fb49aac19db7042ce0c3346e688ad`.
Independent scratch clone only; reviewer made no product edits.

Scope: shared `mix_targets` and `glide_set` calls via `noinline`, and existing
`X0X_COLD`/Clang `minsize` for eight setup/project/loop/UI helpers. All float
expressions, update order, sample processing, parameter definitions, buffer
layouts and lifetime remain unchanged. Inspected the newly marked helpers'
callsites: main-loop/UI/editor/project/setup paths. Existing reserves, flash
regions, DSP optimization flags, WBL1/schema1 bank, native303 and project ABI
were not changed. No missing behavior was removed to make the build fit.

PASS on the exact copied candidate:

- Backend lifecycle/bank/stereo/mono normal and ASan+UBSan suite, including
  default/release preprocessing exclusion probes.
- Native303 adapter and firmware registry suite.
- Independent existing exhaustive303 adapter review plus actual whole-app
  comparison with frozen foundation `6f717e216390ffca0d20be9e2204ab401ad09792`.
  WAV 1267756 bytes is identical; SHA256
  `a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`.
- State codec: 70509 checks, raw project ABI35052 bytes and envelope56 bytes.
- Actual candidate engine in independent dedicated-array/pre-render reference:
  stereo with nonzero side and reverb/delay sends is byte-identical; WAV1267756
  bytes SHA256
  `db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99`.
- New independent direct-DSP oracle:394752 samples across six parameter banks,
  changing stereo block lengths1..256 and reverse mono lengths256..1, buffer
  guards, exact context progression, transport panic retaining the bank and
  switching back to303. Normal and ASan+UBSan pass. Context remains144 bytes.

No actionable host behavior defect found. Commands, exact source hashes, exits
and log hashes are in `qa-receipt.json`; every command exited0. Private output
WAVs are not a publication or audio-fidelity qualification.

Unchanged non-engine files verified against predecessor receipt:

- synth_engine.h93820335e3d433cfc3a71b78d7f6b868231d72449c9a712d23dde93cd3d4a326
- wobble_backend.h90904505c8ed8b3a0781af684596649f8d877f91ea9606f83ebc10f6c95c76b9
- wobble_config.hfa7bc6659e60a8b221af9029f094f5f0cc68c3598360eb4ef0b717d3d4098b48
- build.py1bebd5267efcd130a1519bfad3a60e0be57a843ab91ad125b92d3c60fe1482ac
- wobble.c19dac9935a9524407418bb8bd7cc662d7acf36ff2fad31d1f85e5e69d3529e43

Native link/postchecks remain a separate acceptance observation. Added calls
affect static call depth, so the final target compiler frames must be recorded.
Host CPU observations do not prove native cycles, target deadline, interrupt
nesting or total stack high-water. No hardware/install/publication qualification.
Fidelity failures and refined-candidate import remain outside this cost-only
review; this result makes no new fidelity claim.

## Final-candidate refresh

Final engine SHA256
`f94821ae58692e0e8b6c71edbabf11ab33accf71ffd832bca4611189df856e5b`.
The only delta after the prior reviewed candidate is `noinline` on `render_sub`.
Copied and froze that exact file into the independent clone. Both affected
whole-app tests were rerun: native303 default output remains identical at
`a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`;
stereo/send reference remains identical at
`db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99`.
Both WAVs are1267756 bytes. The completed check group exited0 and both logs
contain their explicit PASS assertions. The exact changed `engine.c` was compiled
inside each check. Inspected final diff: all arithmetic and call order unchanged.

Unchanged backend/header/DSP/state/build inputs preserve their exact hashes above;
normal/sanitized backend, registry, state and new independent oracle proof are
reused only for that unchanged closure. The final receipt records both revisions,
final log hashes and the reuse mapping. No final test was inferred solely from
a commit identifier.

## Final linked frame review

Inspected actual final Elite linked `x0x.dis`. Stack prologues show:
engine_render88 bytes (52 saved+36 local), render_sub92 (52+40),
mix_targets20, glide_set0. Unchanged wobble_render76, waveform20, wrap4,
and mono callback100. The known direct chain through these functions is280
bytes and the corresponding compatibility-mono chain380. Each is60 bytes
larger than the inherited known chain (220/320 respectively), because the
outlined render adds a call frame. This is a measured static cost, not a
whole-ISR bound. Final linked disassembly SHA256 and prologue evidence are in
`native-frames-review.json`. No claim of target high-water or timing acceptance.

The native build owner reports Elite actual full-link and postchecks passing
at552556 bytes (116 bytes XIP free), RAM98084/98304 and pool335740/344064.
This independently reviewed code remains eligible only for scoped host and
application-fit acceptance. Full stack/cycles/device and package/controller
pairing remain separate gates. Pro results require their own observed output.

## Canonical barrier correction: final source closure

Final engine remains
`f94821ae58692e0e8b6c71edbabf11ab33accf71ffd832bca4611189df856e5b`.
Final synth_engine.h is now
`fd64dedab31737ef215ff478462efaf7f896f1851f205886a8be10c253851f61`.
Its only change is the empty memory fence spelling from `__asm__ __volatile__`
to `__asm__ volatile`. Both are the same compiler barrier. Inspected the exact
pre-existing MMIO scanner exception: it permits only empty volatile asm with a
memory clobber. The build scanner and all check rules remain byte-unchanged;
no check was weakened. Independent `mmio_check()` now returns an empty error
list. Negative regex controls still reject nonempty asm or a different clobber.
An independent Apple clang21 -O2 -std=c99 probe produced identical assembly for
both spellings; input/output proof is retained in `barrier-review.json`.

Reran on this exact final engine/header closure: backend normal+ASan/UBSan,
native303 lifecycle/registry, exhaustive303 review, actual default whole-app
WAV equivalence, stereo/send dedicated-array reference, and new independent
394752-sample fragment oracle normal+ASan/UBSan. All eight recorded commands,
including both compilation commands, passed (see `barrier-final-qa-receipt.json`).
Default WAV remains1267756 bytes SHA256
`a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`;
stereo/send WAV remains1267756 bytes SHA256
`db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99`.
Only unchanged state/layout70509-check evidence is reused from the prior review.

Final host/source disposition: PASS_SCOPED. No actionable host behavior defect.
Native owner must retain final target recompilation/equivalence and each model's
actual linked checks. All stack/cycles/hardware/package/controller limits above
remain; the canonical barrier spelling makes no new acceptance claim for them.
