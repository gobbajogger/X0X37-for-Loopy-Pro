# SMC Transport — reconnect epoch isolation (2026-10-24)

Consumed own #50 8cb39637c60200ff2e4a3339c7ec3731092fb80d; root 60321e7a28b68b1d8c30f1861ada1891c7ce9c30, compiled firmware subtree 6a37e40671e77e75c1c4e1109c80855b4e5c87b3, BLE C blob 4313ac30e4051f28b52c2b8cc6c303e8a1a3487f.

A successful LE connection invokes link_reset() without necessarily seeing a previous disconnect-complete event. The old code reset ATT/inbound state but did not flush outbound MIDI and SysEx. Thus queued Note Off and partial SysEx from before the newly accepted link could replay. This is source-observed only, NOT device-observed.

Source fix: move q_r=q_w and sx_on=sx_len=sx_pos=0 into link_reset(). Preserve failed new connection and wrong-handle disconnect behavior.

Local native source-matched *projection*, not complete source, on Linux x86_64: GCC/Clang × plain/UBSan/ASan+UBSan. Original: six compiles exit 0; six runs exit 1 (each 5 failures / 20 checks). Repaired: six compiles exit 0; six runs exit 0 (20/20 each). Command: sh replay.sh, with replay data available separately to owner. Test Git blob for exact locally executed test must be 2d496297f2767405e5a8a9a07b53c932228865ed. The attached test is registered to run against real firmware/src/ble/ble_midi_host.c; **full real-source runner NOT executed**, target builds NOT executed here.

Remaining OWN source-stack dependency: #89 ca777a056e7de0075aac30927a4f6b158d55c465 (SysEx bounds) -> #94 9083dc35a5e270930323b44dde7bdc5916f826f1 (ACL PB0/PB3). They and #50 are distinct BLE-host trees, NOT a combined candidate. Sole Safe Merge must reconcile on its integration lane; do not flatten or pretend approved.

**Next:** Hourly Build can compile exact new #50 SHA Elite/Pro/ctl BUILD_ONLY immediately; Independent QA verify full-root mailbox/reconnect/ACL/SysEx/NoteOff negatives, source tree and correct model; Safe Merge owns NIGHTLY integration; public Catalog/Publisher must await independent prehardware, backup/restore/OTA, model/controller/integrity, source/license and immutable HTTP gates. Device UNKNOWN. No hosting/flash/production action.
