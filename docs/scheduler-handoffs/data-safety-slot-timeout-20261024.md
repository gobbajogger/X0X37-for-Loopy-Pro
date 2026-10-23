# Data Safety — fail closed on unanswered sample layout

**QA finding consumed:** PR #96 comment 6371153853 at source bce12276fdaa1fecd16b665b0151276ab2f6c4bd; root tree `878a96694ba8532f9eb152316b1e5bbcc1936f5e`; compiled `firmware/` tree `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`.

The CLI installer previously converted a lost or unanswered `SMP_INFO` to `None` and then skipped sample-preservation planning, even if flash contained samples. A stock-looking handshake identity cannot prove that flash is empty. Missing parser layouts must refuse too. The source now raises `SlotError` in both ambiguous cases; existing `run()` refuses **before** the updater writes. Valid known empty layout still proceeds; valid risky layout still makes backup. `--no-slots` remains an explicit hazardous operator opt-out only after samples are independently confirmed unnecessary.

Producer native source-window regression: 14 cases; old source 3 FAIL / exit 1; corrected windows 14/14 PASS / exit 0; Python py_compile exit 0. Exact executed commands, test log digests and negative assertions are recorded in `data-safety-slot-timeout-20261024.json`. Source windows matched the previously verified original GitHub functions but this run did **not** execute a complete checkout, firmware target builds, browser tests or a real device. Added effective full-source regression tests to `tests/x0x_slots_info_preflight_test.py` for Independent QA to run.

**Disposition:** source checked (host scoped); independent QA pending; BUILD_ONLY immediately permitted, no QA/merge/host permission needed for compiling. **Public NIGHTLY CLI installation NO-GO** until independent full-root backup/restore/OTA/model/CRC/source/license/immutable HTTP evidence. Publisher should not select an unsafe live candidate. No device flash or direct protected-ref writes.

**Handoff:** Independent QA → verify exact immutable HEAD; Hourly Build → exact source Elite/Pro/controller BUILD_ONLY; sole firmware Safe Merge → reconcile reviewed #81 → #91 → #96 and integrate into NIGHTLY. Production main stays hardware-proven and separately approved.
