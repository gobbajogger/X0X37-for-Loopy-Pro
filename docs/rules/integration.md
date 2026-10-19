# Integration and gate

Read before integration, a formal gate, or release evidence.

### Rule 11: branch order and formal validation

The current integration task owns the firmware branch order. Lane agents give it tested commits. They do not create a second coordinator or overwrite its work. Local fast forwards or cherry-picks are valid when the assigned integrator controls the checkout. Use Git operations that preserve history. Do not use `git update-ref` to evade checks.

Use one formal gate for a finished batch. Run focused tests during development. Re-run a gate for a changed validation tree, a failed run that needs proof of repair, or an explicit force reason. Do not repeat a successful gate or review without a cause.

A supported Linux x86-64 environment, including the local container, can run the formal gate. The box is not required. Use the maintained `tools/gate.sh` and its documented environment variables. Set `X0X_REPO`, `X0X_INTEGRATION`, `X0X_RESULTS`, `X0X_REFS`, `X0X_GATE_WORK`, `X0X_GATE_LOCK`, `X0X_PREPUSH_HOOK`, `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, and `AC79_SDK` to actual accessible paths as needed. Do not override `HOME` to simulate the box. Use a writable work path visible through the build container's mounts.

Keep the four DSP references at the exact gate pins. Keep both dev and release checks, model guards, test completeness, release tree checks, CRC evidence, and budgets. Static RAM free stays at zero or more. Pool headroom stays at 8192 B or more. SMK-37 Pro and Elite are the maintained targets; keep the documented FM-1 allowance distinct from SMK target requirements.

Only the formal gate writes its result. A TRIAL PASS, selected host test, quick check, setup-only run, VOID, or hand-written row is not a release PASS. The install guard uses the gate's exact validation tree key and artifact CRCs. Record the source commit and configurations too. Docs-only edits do not create new device evidence.

If setup is absent, state the missing path or tool and the checks still completed. Local task completion and a formal gate are separate evidence levels. An unavailable release dependency leaves release pending; it does not make completed local work endless.
