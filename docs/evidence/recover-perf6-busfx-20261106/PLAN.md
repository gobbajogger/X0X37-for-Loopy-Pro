# PERF #6 and bus-FX recovery

Base: `a06e20189141ff5fb8766e2c6152be460f4356c3`.
Lane: `lane/recover-perf6-busfx-20261106`.
Writer and local test/build executor: this Codex task. Independent review uses a read-only worker.

Restore only the missing bounded project-stream copy and bus-FX call invariants from the pinned recovery archive. Keep packed project bytes, object boundaries, capacity checks, flush failures, flash verification, audio output and all sample recurrences unchanged. `recovered-project.patch` contains unrelated font/UI transcript data and supplies no project.c hunk; do not apply it.

1. Restore and characterize the recovered differential tests against the exact base. Add runtime cost checks for the bus-FX invariants.
2. Restore bounded memcpy in st_put and hoist cutoff/tempo work once per nonempty bus-FX call. Register both tests in the host suite.
3. Run focused tests, full host suite, and Elite/Pro development builds with authenticated SDK V1.2.1 and the JieLi compiler here. Fix actual failures; retain FAIL/NOT_RUN states until resolved.
4. Compare base/candidate code, RAM, pool and .ram_text using identical native inputs. Do not claim target cycles without measurement.
5. Complete independent review, push source/tests/sanitized evidence to the named lane through the GitHub API, reuse any existing PR, and verify the remote commit/tree.

Stop at verified branch delivery. No protected branch integration, device operations, SDK/toolchain files, binaries, secrets or raw captures enter the delta.
