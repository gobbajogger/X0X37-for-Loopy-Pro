# SMC Transport HCI completed-packets regression — producer evidence
Consumed **gobbajogger/X0X37-for-Loopy-Pro** own PR #50 HEAD **5f61392253bd5d7bd0dd2586df5d0f6659b05ddd**, full Git root `9f1ef910872e043001cbb19537098f2ac86c8b38`, compiled `firmware/` subtree `d3c8b2db8ad898daa7cfcf3ac554248162cda584`, BLE source Git blob `a5f6a96ac1b6c0f111cadd67a2da1d3225a5850a`.

**New defect:** HCI event 0x13 Number Of Completed Packets is encoded as 4-byte [connection handle, completed count] pairs, not separate arrays. Single-handle tests do not reveal this. Multiple handles can miscredit or leave a connected BLE MIDI sender starved.

**Native source-fragment experiment** (Linux x86_64): GCC and Clang with plain, UBSan and ASan+UBSan (`-std=c99 -O1 -g -Wall -Wextra -Werror`) each compiled successfully (6/6 per variant, all compile exits 0). Before the patch, every run failed **4/15 assertions** and exited 1. With the corrected production event-handler fragment, every run passed **15/15**, exit 0. Includes first/middle/last-target handle positions, misleading unrelated packet counts, zero handles, truncation, and credit saturation. Original regression file SHA256 `47f317d2cb5d9379885340cc2c19ff89fbc5cf278ba1fa74de0722d4b47e42e4`.

**Explicit limitation:** the native reproducer compiled a source-matched *evt_in HCI 0x13 branch projection*, not the complete Git checkout or target firmware. The new repository test links `firmware/src/ble/ble_midi_host.c` and was added to `tests/run_tests.sh ble-hci-completed`, but this exact integration test was **NOT run** locally. The installed Classic Build skill catalog was unavailable on this invocation; actual native tools and live Git were inspected.

This adds only SMC transport source and regression on its existing owned isolated PR branch; #89 / #94 remain separate unmerged dependencies. New PR HEAD is producer SOURCE_CHECKED_FRAGMENT_ONLY / **full-source producer checking incomplete**, not full SOURCE_COMPLETE readiness, independent QA PASS, BUILD_ONLY PASS, integrated NIGHTLY, publishable device-test bytes, or hardware evidence.

**Next:** Hourly Build may compile this new HEAD Elite/Pro/ctl BUILD_ONLY immediately; Independent QA validates exact linked C source; sole Safe Merge reconciles #50 + #89 + #94. Do not merge or publish until all required gates are satisfied.
