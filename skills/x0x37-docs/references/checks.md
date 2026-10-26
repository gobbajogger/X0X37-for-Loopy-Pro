# Check project instruction changes

For a skill or routing edit, validate each changed `SKILL.md` with the installed skill-creator `quick_validate.py`, resolve its relative Markdown links, and run `git diff --check`. Read the current repository `AGENTS.md` and owner instructions for conflicting authority. Use one independent review of the final behavior text when the permission or delivery contract changes.

For changes to executable guards, validation tools, or installer evidence, select the matching tests from the paths actually present in this checkout. `tests/host/test_install_gate_guard.sh`, `tests/host/test_validation_inputs.py`, `tests/host/test_dev_validate.py`, `tests/host/test_gate_reason.py`, and `tests/host/test_gate_trap.sh` cover their named contracts. Follow [scoped checks](../../x0x37-check/SKILL.md) and [local proof](../../../docs/rules/local-proof.md); do not run those tests for a prose-only clarification.

Record commands, exact revision or dirty tree, results, and missing dependencies. A documentation check is not a firmware build, release gate, or device PASS.
