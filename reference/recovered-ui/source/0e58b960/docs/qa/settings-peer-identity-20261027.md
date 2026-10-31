# Connected mixer display and DnBOX identity

Bounded Settings continuation atop26f923. Reuses existing two-controller central transport, does not enable a third link. `plat_ble_peers` snapshots state/role and at most two20-byte names within one existing IRQ guard; no pointers escape. UI joins all connected slots, including duplicate names, comma-separated. Non-central/off/disconnected states display no stale peer name. Local advertised Bluetooth identity remains untouched.

The visible NAME row uses a clipped,24px/s endpoint-paused marquee only on overflow. It resets when content/count/state/role/font travel changes or the row leaves view. Read-only NAME remains selectable. The existing frame/rectangle-hash path updates actual pixels; no timer or hardware task added.

About uses exact DnBOX and authentic X0X_VERSION, preserving metrics only when full identity fits. DEV is not an allocated numeric build. One existing-font lowercase n is retained in XS/S/B; all other producer glyphs unchanged. No global font/style replacement.

Affected proof commands:
- `python tests/host/ble_peer_snapshot_test.py`: actual snapshot body with synthetic transport/IRQ assertions, not hardware.
- `sh tests/host/ui_peer_names_test.sh`: actual UI/engine host fixtures, names/state/marquee/About/bounds.
- Same host test under ASan+UBSan (`ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer unsupported under current ptrace).
- Existing grouped settings navigation proof; no unrelated all-glyph or DSP re-review.

Private exact receipts and actual native240 frames retained outside public source. Native Elite/Pro BUILD_ONLY and matching controller package checks are separate from source/host review. Real devices and publication remain unrun. Existing remote source-delivery denial is not bypassed by this descendant.
