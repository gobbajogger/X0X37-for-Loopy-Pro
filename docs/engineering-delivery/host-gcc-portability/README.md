# Strict GCC host portability repair

This isolated branch starts from43d078dd; it does not edit the combined checkout being tested. Author source pin4e20d3fa69a08c77bc36a741b0ae4b37e3df8cbb/tree d917d2172f872161553819e7d5d229537daf12f9. Independent review is required.

## Cause and resulting behavior

Actual GCC12.2 controls reproduce all four assigned compile failures on both current combined43d and BASEd633. The fastmath/UBSan/sustain failures reject weak extern-inline definitions under existing-Werror; editor dispatch rejects an uninitialized command read in its unchanged OTA wire mock. Source/blob and extracted-dispatch identity are recorded, not inferred from older proof.

- Keep native Clang FM_ONE/D9_ONE weak sharing exactly unchanged. Other compilers use local static-inline bodies, avoiding GCC's invalid weak-inline/static-helper linkage combination. D9 always-inline/noinline and weak outlined-body behavior remain otherwise unchanged.
- GCC's supported optimize("Os") preserves cold-size intent where clip-key/Chicago shared helpers previously used unsupported minsize. Clang expands to the exact original attributes. The existing x0x_cold.h already has a proper compiler guard and is unchanged.
- Split three ensemble chained-if lines into separate statements. No expression, branch or arithmetic changes.
- OTA test mock checks outer frame length and decoded-buffer capacity, then requires three decoded bytes before command access. No zero-initialization hides malformed packets. Actual malformed/oversized mock inputs fail cleanly; valid identity handling remains.

No default compiler flag, warning/error policy, production OTA/parser rule, or reserved-data save guard is weakened.

## Executed proof

Seven scoped strict-GCC selectors pass after the attribute/mock repair: fastmath, fastmath-ubsan, bass303-sustain, editor-ota-dispatch, project-303-stream, wave-motion-migrate, project-protocol. The follow-up ensemble statement formatting then passes project-stage and chicago-ens. Product export compiles with original-Werror, actual WBL1 stage/file validation both return0, schema6/sound10 payload36396+header32, and the current frontend accepts the exact36428bytes (SHA256c169f42d...). Existing test flags retained.

Actual pinned JieLi Clang comparison of parent43d vs4e20 yields exact preprocessed function bytes for all eight native weak inline helpers, identical normalized preprocessing for four affected production translation units, and **byte-identical four target object files**. Raw whole preprocessor output differs only in whitespace; that difference is explicitly retained. Comparison flags and compiler hash are recorded. This is native-path identity evidence, not a fresh complete package/controller/DEV/hardware qualification.

Newly visible project-save-preflight runtime failure remains separate: its old fixture randomizes invalid reserved raw flags/snapshot targets and now reaches unsupported -9 before capacity -7, while writing zero objects. The existing recorder fixture owner owns the correction; this branch does not change the guard or relabel the failed test. Other full-registry failures are also separate. Before logs are preserved, including intermediate failures.

Receipt names all exact source blobs, commands from before controls, logs and SHA256. No binaries, generated raw export, proprietary SDK content, raw owner captures or credentials are committed. No hardware, push, merge or publication performed.
