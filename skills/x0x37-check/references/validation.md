# Execute checks and use evidence

Add regression tests for meaningful defects.

Use results again in other workflow stages when the checked source, configuration, tools, dependencies, and check scope are the same.
A new stage or commit ID alone does not require another test or build.
Rerun invalidated checks and failed checks after repair. Do not rerun unrelated passing checks.

A fresh application build with a retained controller is not a fresh controller build.
For controller reuse, verify its exact hash, provenance, model, and application/controller
ABI and package compatibility. Label it retained. A fresh controller claim needs a
successful build from the identified controller source, toolchain, SDK, and configuration.
Changed controller inputs or incompatible ABI invalidate reuse; do not relabel old output.

Record scope, commands, inputs, result, and log path once. Link that evidence in handoffs.
Label prior results as reused or historical, never as rerun. Captures and simulated sends
are not fresh hardware observations. A cloud validation result is not device evidence.
State material coverage gaps. Do not list every unrelated omitted check. Run new required coverage.

Use `X0X_TEST_TIME_SCALE` only where supported. Formal gates use fixed real time.
Run tests in scratch folders and repositories. Never use live queues, results, or status files as test fixtures.

Preserve required code, static RAM, and pool margins. Do not remove features or reduce sample quality for a PASS.
