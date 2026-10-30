# Data Safety — PR #81 linkless filesystem backup recovery (2026-10-24)

**Source producer status:** SOURCE-COMPLETE for the scoped Python change, native host checked; independent full-root QA, stack integration, complete installer/OTA negatives and public qualification remain pending. This is NOT hardware, target-build, or NIGHTLY-installable evidence.

## Consumed QA finding

Independent QA reproduced that firmware PR #81 HEAD `057009814e63dbb34b8f7f3de03aa9391bd00306` could not save a *new* sample archive when `os.link` raised `EOPNOTSUPP`, as on a filesystem without hard links: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/81#issuecomment-6368640414 .

## Correction

Preserve the staged, fsynced and byte-verified private archive. Prefer existing atomic hard-link publication, but on a genuinely **unsupported hard-link syscall only**, attempt a native exclusive rename (`renameat2(RENAME_NOREPLACE)` on Linux; `renameatx_np(RENAME_EXCL)` on macOS). Neither mode replaces an existing destination. If no safe exclusive rename exists, refuse with a useful `SlotError`; **never** use check-then-rename, copying to the final name or direct write. Fail permission-denied, race and other I/O errors without bypassing them. Keep atomic `--force` replacement unchanged.

Linux execution genuinely exercised the `renameat2` code path with `os.link` patched to `EOPNOTSUPP`. macOS native behavior was **not** exercised. Simulated link rejection does not certify a real exFAT volume. Directory fsync/power-loss persistence, full-tree install recovery, on-device slots, and host-specific file permissions remain open QA checks.

## Reproducible test evidence

- Fixture: locally reconstructed original PR #81 backup function window from actual Git patch, tied to source blob `fea90b39c93283bdeb31f238e3a9af5dc1e5f280`. Its full reconstructed module blob is **not** claimed identical to the complete remote source file.
- BEFORE: `X0X_SLOTS_SOURCE=pr81_x0x_slots.py python3 -m unittest test_portable_backup -v`: **exit 1**, 9 tests, 6 errors (including missing helper and EOPNOTSUPP). Real `os.link` path positive controls succeeded. Baseline log local SHA256 `8190cd941ad5d1b38388eac8eea9a99ddec920ebb9d26a9ce2cfa2c7dfa2177a`.
- AFTER: `X0X_SLOTS_SOURCE=candidate_pr81_x0x_slots.py python3 -m unittest test_portable_backup -v`: **exit 0**, 9/9 PASS. Committed test Git blob `390db7782e2b262d51a7fd4dbb0a3491ca374818`, passing log Git blob `0189166dd90bb8afc62aaf49bbf298da87e35edb`, log SHA256 `26112e68ef88fa82e2ddb602d86c64fae10866396ae964f0c154bdf3b0a64b6f`.
- `python3 -m py_compile candidate_pr81_x0x_slots.py test_portable_backup.py`: **exit 0**.
- New remote `tools/x0x_slots.py` full blob `c51c948394cf97c83a6237a467764a7822f098a0`, changed only the backup tool. Full exact-root checkout and complete tests **NOT RUN**: shell `git ls-remote ... refs/heads/nightly` failed DNS resolution, exit 128; connected GitHub blob write/readback is separate.
- Test path checked in as `tests/x0x_slots_backup_portable_test.py`. Its byte-identical local regression Git blob matches `390db7782e2b262d51a7fd4dbb0a3491ca374818`.

## Handoff and unresolved gates

Independent QA: rerun original 11 atomic tests, new 9 tests and full slot/installer/OTA negatives on exact **full repository** commit; verify Darwin native and unavailable exclusive rename failure, and inspect stacked #91 / #96 compatibility. Safe Merge: preserve #81 → #91 → #96 dependency while reconciling source. Hourly Build: BUILD_ONLY Elite/Pro/ctl from this immutable producer candidate immediately; build must not wait for QA or PR merge. **No public downloads, package/OTA/controller integrity or physical-device PASS claimed.**