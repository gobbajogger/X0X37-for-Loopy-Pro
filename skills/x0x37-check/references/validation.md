# Execute checks and use evidence

Add regression tests for meaningful defects.

Use results again in other workflow stages when the checked source, configuration, tools, dependencies, and check scope are the same.
A new stage or commit ID alone does not require another test or build.
Rerun invalidated checks and failed checks after repair. Do not rerun unrelated passing checks.

Record scope, commands, inputs, result, and log path once. Link that evidence in handoffs.
State material coverage gaps. Do not list every unrelated omitted check. Run new required coverage.

Use `X0X_TEST_TIME_SCALE` only where supported. Formal gates use fixed real time.
Run tests in scratch folders and repositories. Never use live queues, results, or status files as test fixtures.

Preserve required code, static RAM, and pool margins. Do not remove features or reduce sample quality for a PASS.
