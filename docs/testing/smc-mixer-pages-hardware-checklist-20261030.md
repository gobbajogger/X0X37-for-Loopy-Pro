# SMC mixer page follow and lock (PR #717): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Stop at the first failure and record what you saw.

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/717
(`wip/smc-mixer-integration`). This checklist follows SMC-11. While UNLOCKED, the
mixer follows the keyboard page. While LOCKED, the mixer keeps its own page and
captured fader/knob targets. Keyboard page changes cannot move the locked mixer,
but the mixer's own page/mapping UI remains live and moves `lock_page`. The
fader-button LEDs follow the mixer page. Lock saves `lock_part`, `lock_view`,
`lock_page`, `lock_bank`, and `lock_kit` per peer and part.

Connection, guarded boot, and one-peer behavior belong to the separate PR #700
checklist. Do not use this checklist to qualify PR #700.

## 1. What you need

- One SMK-37 with a qualified DEV candidate that contains PR #717, installed by
  the authorized integrator.
- The exact candidate identity and the last-known-good package for rollback.
- One SMC mixer in the owner's captured DAW preset. A second SMC is required only
  for the per-peer repeat in Task 2.
- A view with at least three distinct pages or mappings whose fader/knob labels
  and fader-button LEDs are easy to tell apart.

## 2. Start states

- **S-FOLLOW:** Connect the SMC. Select 303A on the keyboard and open a sound view
  with at least two pages. Make sure SMC target lock is OFF. Put the keyboard on
  page 1. Touch one SMC fader and encoder so the temporary SMC screen shows their
  current destinations.
- **S-LOCK:** Do S-FOLLOW. Note the current part, view, page, drum bank/kit if
  applicable, and the fader/knob destinations. Press the captured Pause control
  once and release it to engage SMC target lock.

## 3. Tasks

Task 1. UNLOCKED mixer follows the keyboard page
- Start: S-FOLLOW.
- LEDs to watch: the SMC fader-button LEDs for the active mixer page/mapping.
1. On the keyboard, change from page 1 to page 2 without touching SMC page controls.
2. Touch SMC fader 1 and encoder 1 to show their destinations.
3. Change the keyboard to another valid page or mapping in the same view and repeat.
- Expected: the mixer moves with every keyboard page change. Its temporary screen,
  fader/knob destinations, and fader-button LEDs all match the keyboard's current
  page or mapping.
- Stop if: the mixer stays on its old page, a fader or knob still targets the old
  page, or the fader-button LEDs do not move with the keyboard page.

Task 2. LOCKED mixer owns its page while keeping captured targets
- Start: S-LOCK.
- LEDs to watch: the SMC fader-button LEDs for the mixer page/mapping. They follow
  the mixer, not later keyboard page changes.
1. Change the keyboard to a different page and then to a different part. Do not
   use SMC navigation yet.
2. Touch SMC fader 1 and encoder 1. Confirm that both still act on the targets
   captured at S-LOCK.
3. Use the SMC's own page/mapping UI to move to another mixer page or mapping.
   Confirm that the mixer page label and fader-button LEDs move to that selection.
4. Touch SMC fader 1 and encoder 1 again. They must still edit the captured
   targets even though the mixer page/mapping display moved. This mixer navigation
   updates `lock_page`; it must not release or retarget the lock.
5. While still LOCKED, switch the keyboard VIEW (not only its page or part) to
   a view with fewer pages, or no pages, than the captured mixer view. Verify
   that the SMC keeps its own page, page label and fader-button LED mapping.
   Open the SMC's own page/mapping popup and change its page/mapping again:
   its page controls and labels must still work, moving `lock_page` despite
   the keyboard's different view. Touch fader 1 and encoder 1: both must still
   edit the original captured targets.
6. If a second SMC is available, repeat S-LOCK on the other peer with a different
   part/page/bank. Change pages from each mixer in turn. Each peer must keep its
   own locked context and must not move the other peer's page or captured targets.
7. Unlock the first mixer with the normal long-Pause gesture while the keyboard
   is still on the different view. It must return to following that live keyboard
   view/page. Return the keyboard to a multi-page view and change its page; the
   unlocked mixer must follow with matching labels, targets and fader-button LEDs.
- Expected: keyboard page, part and VIEW changes cannot move a locked mixer.
  Mixer navigation remains available, even when the keyboard view has fewer or
  no pages. The mixer keeps its own page labels and fader-button LEDs, and the
  SMC's own popup changes its page/mapping while the captured fader/knob targets
  stay fixed. Unlock resumes following the keyboard's live view and page. The
  saved lock context is scoped per peer and part: `lock_part`, `lock_view`,
  `lock_page`, `lock_bank`, and `lock_kit` do not leak between peers or parts.
- Stop if: keyboard page, part or VIEW navigation moves or clamps the locked
  mixer page or label; the SMC popup/page controls stop working on a smaller or
  no-page keyboard view; its fader-button LEDs follow the keyboard instead of
  the mixer; a page change retargets a captured fader/knob; one peer changes
  another peer's lock context; or unlock fails to resume following the live
  keyboard view and page.

## 4. Rollback

1. Record the task number, step, candidate identity, page/part, LEDs, and target
   that failed.
2. Stop playback and switch off the keyboard.
3. Ask the authorized integrator to reinstall the last-known-good package.
4. Confirm the old version in ABOUT X0X.
5. Record the result in PR #717 as FAIL. Do not mark the candidate hardware-tested.
