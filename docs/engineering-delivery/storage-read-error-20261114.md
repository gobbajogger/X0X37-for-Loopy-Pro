# Storage save-selection read-error repair

An unreadable current copy could be treated as invalid by save selection. With only A committed, an A header/body read failure selected A as the write target; a subsequent program failure could leave no recoverable copy. The saved RAM reproduction established this on the baseline. This repair returns `-2` before buffer staging, erase or programming when selection encounters a read error.

## Source and scope

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Parent/base: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`; tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`.
- Baseline `firmware/src/storage.c` blob: `922a4dc0ee266c03b7865689ca16ce07f881d234`.
- Repaired storage blob: `1edfa6b748dba5b4dd892898316bcc812f19cd1b`.
- New regression C / shell blobs: `baa7a7bef4eebf01f574e9463dbfbc24d6ee866b` / `f86b54fba8180bd22a1977987d9bf773c3ef5dfb`.
- Unchanged dependencies: `crc32_nibble.h` `193ce7bcf580fcb69e59d66e610ce4ea3baaed60`; `app/plat.h` `b57f2737c5090feac762286794c3f4b9fa1b28ae`; existing `tests/storage_test.c` `6f2f00d6365a89251295877a6c34aa370df0f38b`.
- Isolated branch: `chat/storage-read-error-safety-20261114`, targeting `lane/026-combined-20261112` for draft review. Coordinator retains integration ownership.

Pre-edit overlap check: live target remained at the base. PR #900 head `d963c3925a5b05f329e24c1a793a6326118fa466` excludes `storage.c`. Open PR and pattern/fmb branch searches exposed no conflicting pattern-owner storage edit; unpublished edits were not visible. Coordinator designated this lane the sole core-storage writer and reported that the pattern owner retains A/B structure. Current workflow guidance was read at `66073c68dd80d51a7053432c2c161a8b720a1341`.

Only `storage.c`, the two focused regression files and this receipt change. `st_body_ok` now distinguishes read failure (`-2`) from CRC mismatch (`0`) and validity (`1`). `st_pick` propagates header/body I/O errors; `st_save` aborts on that result. Newest-valid order, A-on-tie, CRC fallback and alternate-bank writes are preserved. The older body remains unread when the newest body is fully validated. The layout, owner handling, `st_current` and `st_load` are byte-identical to baseline; format, reserved regions and commit-header-last ordering are unchanged.

## Reused baseline reproduction (not rerun)

This is the complete saved compile/run transcript. Its exit 0 means the destructive failure was reproduced, not that baseline durability passed. The fixture was an 11-byte synthetic settings object in RAM NOR.

```text
SOURCE_BLOB_MATCH firmware/src/storage.c 922a4dc0ee266c03b7865689ca16ce07f881d234
SOURCE_BLOB_MATCH firmware/src/crc32_nibble.h 193ce7bcf580fcb69e59d66e610ce4ea3baaed60
cc (Debian 14.2.0-19) 14.2.0
COMMAND: cc -std=c11 -O0 -Wall -Wextra -Wno-unused-function /workspace/dnbox-source-audit-20261114/repro/storage_read_error.c -o /tmp/dnbox-audit-ktph6gb8/storage_read_error


COMPILE_EXIT=0
case=write-failure-control save_rc=-6 injected_read_failures=0 erases=1 reload_rc=11 prior_idea_recovered=1
case=unreadable-A-header save_rc=-6 injected_read_failures=1 erases=1 reload_rc=-1 prior_idea_recovered=0
case=unreadable-A-body save_rc=-6 injected_read_failures=1 erases=1 reload_rc=-1 prior_idea_recovered=0
REPRODUCED: read-error save selection can erase the sole committed copy; control preserves it.


RUN_EXIT=0
```

## Changed-scope verification

Run from the repository root using Debian GCC 14.2.0-19. The runner compiles actual `storage.c` with the real public object map/CRC helper, using `-std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function` and each `-DBOARD_SMK37=0/1`. The modeled payload is 600 bytes, spanning three body-read windows.

| Command | Result |
| --- | --- |
| `sh tests/host/storage_read_error_test.sh` | Exit 0; both maps passed |
| `CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/storage_read_error_test.sh` | Exit 0; both maps passed without sanitizer diagnostics |
| `cc -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -DBOARD_SMK37=0 tests/storage_test.c -o /tmp/storage-existing-f5tr6kmx/test-0` then that executable | Compile/run exit 0; `storage test passed` |
| Same existing test with `-DBOARD_SMK37=1`, output `/tmp/storage-existing-f5tr6kmx/test-1`, then that executable | Compile/run exit 0; `storage test passed` |

Each focused run printed:

```text
PASS BOARD_SMK37=0 objects=7 fixtures=658 uncertain_reads=441 failed_writes=112 normal=84 lazy_selection=21 checks=29071
PASS BOARD_SMK37=1 objects=7 fixtures=658 uncertain_reads=441 failed_writes=112 normal=84 lazy_selection=21 checks=29071
```

| Acceptance case | Observed host result, for both maps and all seven objects |
| --- | --- |
| Either header unreadable; any visited A/B body window unreadable, including fallback after newer CRC damage | Save returns `-2`; zero erase/program calls; entire flash and shared staged buffer unchanged |
| Remove that fault | Existing load result restored; subsequent save succeeds in the expected alternate bank with correct next sequence |
| Sole valid A/B; either newer bank; equal sequences | Correct current selection; A wins ties |
| Header/body CRC damage; genuine blank storage | Readable corruption falls back or permits replacement; blank remains writable. Load preserves `-1` blank / `-3` damaged distinctions |
| Older body unreadable while newer is fully valid | Older body is not read during selection; no spurious uncertainty; previous-copy load still works when readable |
| Erase failure; partial first/later payload or header program failure | Error returned; prior current copy and every byte outside target sector preserved; retry succeeds |
| Ordinary existing storage regression | Passed: normal save/load, CRC fallback, torn writes, shared-buffer aliases, boot-stage hold and region map checks |

The new harness is standalone (`sh tests/host/storage_read_error_test.sh`); it is not added to the broad test runner. The original audit and PR #896 checks were not repeated. No full-project build, native/cross build, link/size check, physical device, power-cut experiment, SDK/binary upload, firmware publication or merge was performed. These host/RAM results do not prove hardware driver behavior or end-to-end project durability. Separate independent review and any integration gates remain with the coordinator.
