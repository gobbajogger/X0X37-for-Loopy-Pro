# QA2: functional status is not timing acceptance

The existing QA2 qualification owner must retain audio timing FAIL separately
from a passing functional `audio-stress` row. Otherwise an installer candidate
could be described as fully qualified from an insufficient suite summary.
This bounded diagnostic advances failure disposition; it fixes no firmware and
closes none of QA2's 33 historical failures or two NOT_RUN cases.

## Plan and scope

Request: LX-REQ-016, ROADMAP QA2, with LX-REQ-017 host-proof context.
Reproduce the reporter-to-summary boundary with synthetic values, authenticate
all source inputs, and deliver the result for the existing qualification owner.
Stopping condition: reviewed, remotely verified diagnostic source and receipt.
Only this new evidence directory is owned by this task. Existing B2/#796,
host-fixture #823, #812 tracker, #822 gate, integration and performance owners
retain their scopes. No shared ledger or ROADMAP write is claimed.

## Verified inputs

- Firmware candidate: `build/0.0.24.5` at
  `8be16486e33f65b31c76710e1d825b6aa701c2c9`, tree
  `f952ab4455723e4a1a68fdb3a8eea22d2da59710`.
- Maintained workflow/base for this evidence branch: `nightly` at
  `66073c68dd80d51a7053432c2c161a8b720a1341`.
- [Authoritative tracker](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/496c87269349b70f9a900bd95b996d765aa51a80/docs/ROADMAP.md):
  QA2 remains OPEN; full release checks require ZERO FAIL/SKIP/NOT_RUN.
- Coordination snapshot: `4a35a6cdaa0f112db6b3f577542138afd170c20d`.
- [Existing #823 receipt](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/823)
  reports functional wrapper PASS with chorus timing FAIL. Its SMC repair and
  pending UI-feedback ownership query are separate work, not taken over here.
- [Existing B2 documentation](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/8be16486e33f65b31c76710e1d825b6aa701c2c9/docs/perf/b2-stress-023/README.md)
  explicitly separates functional success from timing status and records owner
  closeout. This is not a newly discovered accidental flag omission.

Open PRs, relevant discussions and host branches were inspected read-only.
No old conversation reader was exposed, so prior Work-thread activity could not
be independently inspected. GitHub-linked ownership is retained, not presumed
inactive. Library search yielded older unrelated evidence, which was not reused.

## Executed proof

```sh
python3 docs/engineering-delivery/qa2-audio-stress-exit-20261103/probe.py \
  --repo /workspace/X0X37-for-Loopy-Pro \
  --out docs/engineering-delivery/qa2-audio-stress-exit-20261103/results.json
```

Exit 0: four synthetic reporter-to-summary cases PASS. `results.json` records
the exact Git blobs and extracted invocation. The actual pinned reporter and
summary parser execute; the suite's exit-to-status mapping is modeled, and the
complete shell wrapper/full suite is NOT_RUN. No workload or benchmark executes.

| Synthetic case | Strict flag | Reporter exit | Parsed suite status | Timing |
|---|---|---|---|---|
| Within limits | absent | 0 | ok | PASS |
| Miami 2,397,367 ns, limit 1,740,000 ns | absent | 0 | ok | FAIL |
| Within limits | present | 0 | ok | PASS |
| Same over-limit input | present | 1 | FAIL | FAIL |

The numbers are fixture inputs inspired by the existing #823 report, not a
replayed historical capture or new measurement. PCM is zero-filled synthetic
size-validation input. All four results retain target NOT_RUN. Existing reporter
unit tests are not rerun or presented as new proof.

## Next owner and remaining gate

The existing LX-REQ-016 qualification owner should keep functional and timing
statuses separately bound to the exact candidate. For a release that requires
host timing acceptance, consume each actual plain/chorus/peaking report with
`--require-host-limits`, preserving CSV/PCM hashes and nonzero disposition. Do not
change the global wrapper's documented policy without reconciling that scope
with the existing B2/host owner. No new measurement is requested outside the
existing serialized slot. This diagnostic does not authenticate old raw logs.

Consumer adoption is PENDING. The full host suite, Elite/Pro builds, current
per-model pool measurements, audio parity, native packages, external uploader
adoption and device acceptance are NOT_RUN here. Publication remains HOLD.

## Workflow reconciliation

Current owner instructions specify exact gobbajogger author/committer and fresh
threads at meaningful stages. They supersede bundled default-author-only and
no-fresh-thread-required wording. The candidate's older Classic-only testing
restriction conflicts with the current delegated execution-environment task and
the maintained nightly contract; this task uses the selected cloud environment.
The candidate also permits private headers while the plugin excludes them: no
such bytes are added. No source merge, publication or device authority is inferred.

Fresh subagents perform read-only diagnosis and independent artifact review.
Their task identities are not claimed as newly created top-level Work threads;
no top-level thread-creation tool is available here.
