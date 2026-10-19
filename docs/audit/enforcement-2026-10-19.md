# Rule enforcement audit — 2026-10-19

Scope: rule scripts, hooks, affected tests and BUILDING.md in the isolated rules
lane. No firmware, SDK, reference pins, budget limits or hardware state changed.

| File | Cause | Repair and resulting behavior |
| --- | --- | --- |
| tools/prefs_lint.py | Every dated old preference entry had to match the current rule wording. Superseded procedures could block a valid rewrite. | Stop at the explicit historical record divider. Current entries still need valid rule pointers and coverage. Preserve old entries as evidence. |
| tools/quickcheck.sh | Rule lanes and the old strict marker forced grammar heuristics to block all work. Native macOS time arithmetic used GNU date. | Grammar is advisory unless X0X_STE_STRICT=1 requests strict review. Use portable elapsed seconds. Keep current rule lint and Trigger/Check commit evidence. |
| tools/ste_check.py | Owner-message checks banned useful hashes, paths, concise fragments and all install questions. | Permit required evidence and questions. Reject empty messages. Human review applies the current plain English and concise style rules. |
| tools/quickcheck.sh | A docs-only folder check created the shared cache. Missing flock could continue without a lock. Commit mode waited without a limit. | Folder checks use their own build folder. Commit checks require flock before cache mutation, reject setup errors and use a bounded lock wait. |
| tools/quickcheck.sh | Defaults assumed the box repository and installed hook. Explicit DSP reference variables were overwritten. | Discover the source repository and use the adjacent checked-in hook. Preserve caller reference paths. Explicit environment overrides remain supported. |
| tools/gate.sh, tools/quickcheck.sh | Local clone hardlinks failed on the Mac filesystem mounted into the Linux container. | Use git clone --no-local for private validation clones. Preserve Git objects and commit hashes. Keep clone push URLs disabled. The mounted-workspace gate regression now passes. |
| tools/gate.sh | Box-only language hid a valid Linux container path. Native macOS could fail deep in Linux signal setup. | Document Linux native/container support. Native macOS returns a clear setup VOID before the signal wrapper. Full gate validation remains intact. |
| tools/pre-push.hook | The runner, skip log and approved roots used fixed host paths. The failure text required an old role handoff. | Accept configured local runner, log and root paths. Report the missing path and its settings. Still block outside destinations, protected references, shallow history and absent QUICK PASS evidence. |
| tools/install_gate.sh | The new portable adjacent-hook default needed the hook in installed gate folders too. | Install pre-push.hook with the gate tools. A regression compares its exact bytes and executable mode. |
| tools/release_tree_check.sh | The default assumed the box bare repository. | Discover the current Git directory and accept explicit local repository paths. Preserve protected tree and ancestry checks. |
| tools/quickcheck.sh | A source-proven device bug still demanded a newly named INLOG file or owner exception. | Bug evidence: tests/path accepts a changed regression file. Require a fixed-tree PASS and broken-parent FAIL. Keep the owner exception explicit and device results separate. |
| BUILDING.md | No complete local setup example tied paths, hook settings, status and install evidence together. | Give concrete commands, reference pins, compiler settings, local state paths and evidence boundaries. Old roster status flags remain diagnostic. |
| docs/rules/SCRIPT-CHECKS.md | Old numbered proposals and box-specific rollout notes looked current. | Preserve exact old bytes in docs/archive/rules-2026-10-17/rules/SCRIPT-CHECKS.md. Link current checks and their actual scope. |

## Checks

Checks ran on the dirty rules lane based on 280eb4d. No full firmware release gate
or device test was claimed.

- Native Mac: current preference lint 4/4; owner-message cases 12/12; shell syntax,
  Python compilation and diff whitespace checks passed.
- Native Mac: folder docs quickcheck passed with X0X_QC_WORK=/proc/x0x37-unused.
  This proves that folder mode does not touch the shared cache.
- Local Linux image x0x37-validation:local: portable rule checks 5/5; rules
  quickcheck 10/10; grammar quickcheck 13/13; device regression evidence 9/9;
  install evidence guard 21/21; gate signal/process checks 5/5.
- The gate signal fixture first exposed mounted-filesystem ownership and hardlink
  failures. The ownership setting applied only inside the ephemeral test container.
  The clone fix removed the hardlink failure. The final test passed on the mounted
  Mac workspace, with the real private-clone path.
- Final portability and gate process log: build/rules-enforcement-checks.log.

A device bug without a runnable host regression can finish independent local work
and report the validation limit. The device-bug quickcheck cannot certify an
unrun regression. A selected test PASS does not authorize install or release.
Use X0X_RESULTS_DIR with the same results path as gate X0X_RESULTS when checking
install evidence. Preserve model checks, flash and RAM budgets, slot backups,
readback and actual device evidence at their respective steps.
