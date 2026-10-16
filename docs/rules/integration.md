# Rules v3: integration and gate

Read when: before you move commits onto integration (git push), request a gate or run the gate script.

Terms: A gate is one run of the gate script. Each gate writes one row in /workspace/x0x37-results/index.md.

### Rule 11 [v2: 6, and the merge part of v2: 5]
Step: P5 integration merge, P6 gate.
Rule owner: Merge Integrator.

Merge Integrator owns the branch order and moves the lane commits onto integration. At the end of the batch, it requests one gate, and Build and Test Runner runs the gate script. Split or bisect only if that gate fails.

### Integration push [new, Orchestrator 2026-10-17 00:04, owner OK]
Step: P5 integration merge.
Rule owner: Merge Integrator.

- Builders commit only on lane branches, never directly on integration.
- Merge Integrator moves integration only with git push to the box repository. Do not use git update-ref.
- The server hook of the box repository runs the strict quick check on each push to integration.
- Skip the hook only with `git push -o skip-quickcheck="<why>"`. The hook logs each skip.

### Gate run [new, from Build and Test Runner, Orchestrator 2026-10-17 00:05, owner OK]
Step: P6 gate.
Rule owner: Build and Test Runner.

- Merge Integrator sends this request line to Build and Test Runner: `GATE REQUEST batch <N>: integration <full hash>`.
- Build and Test Runner runs the gate with the normal HOME. Never override HOME. The command is `/workspace/x0x37-refs/gate/x0x37_gate.sh <full 40-character integration commit>`.
- Use these options only on request:
  - `--force "<reason>"` runs the gate again after a PASS. The reason goes in the index row.
  - `--trial` runs a commit that is not on integration. A TRIAL PASS never counts for a release.
- The gate refuses a commit that is not on integration. It holds gate.lock (exclusive) and uses time scale 1.
- The gate writes /workspace/x0x37-results/<tree>-<time>.md and one row in /workspace/x0x37-results/index.md.
- If the toolchain or the SDK is missing, the result is VOID, and the gate writes no row.
- The last output line is `PASS|FAIL <hash> (time scale 1)`.

### Free pool [owner 2026-10-16 21:53]
Step: P4 lane commit, P6 gate.
Rule owner: Merge Integrator.

build.sh and the gate require 8192 B of free pool or more. The 8192 B limit replaces the 4096 B limit.
