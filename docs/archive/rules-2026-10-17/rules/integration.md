# Integration and gate

Read when: before you move commits onto integration (git push), request a gate or run the gate script.

### Rule 11
Step: P5 integration merge, P6 gate. Rule owner: Merge Integrator.

Merge Integrator owns the branch order and moves the lane commits onto integration. Move integration only with git push to the box repository. Do not use git update-ref.

The server hook runs the strict quick check on each push to integration. Skip it only with `git push -o skip-quickcheck="<why>"`. At the end of the batch, Merge Integrator requests one gate. Split or bisect only if that gate fails.

build.sh and the gate require 8192 B of free pool or more.

### Gate run
Step: P6 gate. Rule owner: Build and Test Runner.

- Merge Integrator sends this request line to Build and Test Runner: `GATE REQUEST batch <N>: integration <full hash>`.
- Build and Test Runner runs `/workspace/x0x37-refs/gate/x0x37_gate.sh <full 40-character integration commit>` with the normal HOME. Never override HOME.
- Use `--force "<reason>"` (run again after a PASS) and `--trial` (a commit that is not on integration) only on request. A TRIAL PASS never counts for a release.
- The header of x0x37_gate.sh gives the gate outputs, the lock and the VOID result.
