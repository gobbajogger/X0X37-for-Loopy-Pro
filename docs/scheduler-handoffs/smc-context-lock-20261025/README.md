# Prepared SMC context lock — physical binding pending

**The requested Pause feature is incomplete.** The prepared control-ownership
mechanism is independently reviewed and host-tested; no MIDI event activates it,
and the blinking/steady Pause light is not implemented. Existing STOP/PLAY/REC,
their feedback and keyboard PLAY/PAUSE retain their existing behavior.

Reason: navigating the keyboard currently redirects the SMC controls. Capturing
complete destinations lets a musician keep performing on the original context
while using the keyboard elsewhere. Host tests verify that ownership and pickup;
the resulting physical user experience remains unverified and unreachable until
the correct button is identified and bound. The 1.2-second full-screen feedback
continues to expire into the current UI; it is not pinned by the lock.

## Source and explicit scope

Installed base PR144: `3862175d5815d2bd505ab04ca1c8e1868761a2e7`, root
`87819bd7af981f8a7315875233f69ff720bc13ce`.
Reviewed/built source: `89463c4aa83eb19a00a42d1410541323cfbc5253`, root
`f11bf0088d259fd8eeac4e131f6e819daf877aff`, firmware tree
`d5831559c6d59b588b1af15583912bf7634bcc5d`.
Later commits add tests/docs/evidence only. This task owns the isolated
`lane/smc-pause-context-lock-20261025` lane. Separately owned NIGHTLY integration
`93449136baae02c9715ba20edc5c43176786b635` was fetched and verified to have the
same root as the installed base; no source rebase/rebuild was needed for that
identical baseline. No protected ref was changed by this task.

The [scope/design plan](../../plans/2026-10-25-smc-context-lock.md) records precise
fader, knob, row and global actions. The main-loop `smc_lock_button(pressed)`
entry point de-duplicates held press/release messages. Snapshots freeze full
parameter, kit/bank, selected part, pattern-slot and song-bar identity. Changes
to target ownership rearm pickup and clear old crossing/acceleration history.
Held Solo release retains its original target. The display shows a compact
captured context, for example `LOCK 909 BANK 1`.

Deliberate boundaries:

- Entering lock in clip grid/editors is refused because their fields have
  stage-dependent ownership. An ordinary-page lock continues safely if the
  keyboard subsequently navigates into clip mode; unlocking resumes clip fields.
- Song insert/delete/clear/length changes and successful undo invalidate captured
  song-bar knobs until unlock, preventing edits to shifted/recreated bars.
- Target-changing SMC navigation, bank/channel keys, SELECT and row REC wait for
  unlock. Keyboard navigation stays independent. Existing global PLAY/STOP/REC,
  confirmation priority and Help dismissal remain. Rewind/forward cue the
  captured part's current pattern; editor refs stay on their captured pattern.
  Global REC and sound-motion recording retain current sequencer/UI semantics.
- Reconnect retains the captured context and clears pickup/held-button state;
  project reset clears the runtime lock.

## Exact remaining identification

The stock protocol map and earlier USB/BLE captures were searched first.
The 5,394-message USB capture and successful BLE capture have no physical gesture
labels and do not establish Pause. Owner-history note53/CC53 is an unverified
lead. PR18 relabelled the existing STOP `0x5D` without physical provenance.
The current code proves that `0x5D/0x5E/0x5F` are STOP/PLAY/REC, not that any of
them is the requested physical Pause. No speculative interception was added.

Need one labelled physical Pause press/release in the current preset, with raw
status/channel/data bytes and held-repeat behavior. The smallest prepared native
input-only procedure is owner-local
`/Users/mac/Documents/Codex/2026-10-25/task-22/PAUSE-IDENTIFICATION.md`;
its ten-second input-only listener has SHA256
`69d9085d1846d08d40b8d4650cd24b0a2d8ae2b5b9153733848c6256d8217e3c`.
It has not been hardware-run. The parent assigned sole capture ownership to the
fresh native task `01a17193-7d06-74d7-acb2-df34f84fa02b`; this source task opens
no ports. Raw capture bytes must remain private.

LED support needs separate evidence. Existing direct-SMC BLE feedback supports
note velocity 127/0 with success-only caching and reconnect invalidation, but
Pause's address/response and native blink support are unproven. An input capture
alone does not prove the light. Obtain a labelled mapping or separately authorized
single-light on/off validation. After identification, steady-on means locked;
a calm nonblocking 500ms phase can indicate following without clock/playback
coupling or redundant sends. That scheduler is not part of this source stage.

## Independent verification

[Source review](source-review.json): `ACCEPT_PREPARED_UNBOUND_SOURCE`, no remaining
source findings. [Independent tests](independent-tests.json): 393/393 production
integration assertions plain and ASan, four detected runtime mutations, and
25 affected suites with final passing results. Coverage includes both kits and
banks, all valid Solo aliases and original-target release, pattern/song identity,
press/release duplicates, reconnect, pickup, lock→navigate→edit→unlock, overlay
cancellation, exact 1200ms return, existing transport, all-channel note53/CC53
nonbinding and output-cache no-spam behavior. Synthetic framebuffer inspection
confirms the compact context line is readable; it is not physical LCD evidence.

The two initial aggregate test attempts exited 1. Their fixture dependencies,
macOS feature macro and an ineffective mutation were repaired; every final suite
result and all failed logs remain in the receipt. No aggregate exit-0 result is
invented. The absent-operation negative uses old production SMC plus current
UI/engine and a no-op API shim; it is explicitly a hybrid missing-lock
demonstration, not full exact-root baseline QA. Unchanged historical fastmath
negative-shift UBSan evidence is reused; no clean full-engine UBSan claim.

## Private native BUILD_ONLY

[Build receipt](private-build-receipt.json): real Elite DEV, Pro DEV and fresh
cache-disabled matching controller, all ten recorded commands exit 0, exact pinned
SDK/compiler/DSP and clean source. Per model: 1,988 B code free, 316 B static RAM
free, 8,324 B pool free (minimum 8,192). Exact bundled model/controller/header,
nested CRC/layout/slot map, plain+controller composition and ELF→controller
reproduction passed. [Coordinator reconciliation](coordinator-verification.json)
independently checks actual package bytes and source/test/log hashes.

| Private normal bundle | Bytes | SHA256 | CRC32 |
| --- | ---: | --- | --- |
| Elite | 651396 | `fe9358fe64b411c3cde8fb9b70bc630e070d4167e049df91e6855212ba2d469c` | `42b719f4` |
| Pro | 651392 | `6f893df24672c013add4daa553a7c0b154e02d2541c3ab22813f6bd0d18c8011` | `aca79296` |

Controller: 65,492 B, ABI 2, SHA256
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`, 44 B flash
free and 544 B RAM free. All bytes remain owner-local under task22/build-copy;
no package, controller, SDK/toolchain, preset, capture, secret or user data is
committed or published.

The normal linker removes the unreferenced lock entry point, so these normal
packages cannot expose the Pause feature. A [separate diagnostic relink](retained-entry-diagnostic.json)
retains `smc_lock_button` via `-u` using the same objects: 496 B added, 1,492 B
code free, unchanged RAM/pool. It passes image/budget checks but produces no
normal package and does not prove future input/LED routing fits. Normal bundle
hashes were rechecked unchanged afterward.

Stage: `PREPARED_UNBOUND_SOURCE / SCOPED_INDEPENDENT_HOST_QA_PASSED /
BUILD_ONLY / PACKAGE_NOT_PUBLISHED / DEVICE_NOT_TESTED`. No flashing, hardware,
GUI/computer use, protected merge, deployment, access or scheduler change occurred.
No denied, pending or uncertain execution remains within this source/build task;
physical identification and subsequent binding remain external dependencies.
