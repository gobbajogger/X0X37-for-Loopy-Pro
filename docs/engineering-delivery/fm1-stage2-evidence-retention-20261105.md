# Proposed FM-1 stage-2 evidence retention disposition

Date: 2026-11-05. Status: current proposal for Dot's source-selection decision.
Build: none; this is a documentation proposal.
Replaces: the two handoffs' local pointers to five omitted historical payloads.
Checked against: immutable public base `c007de94f6afd4e46b22ff3e88dd0ec581cc2674`, its Git tree metadata, and the retained historical manifest.

The current source-only candidate omits the five FM-1 stage-2 receipt payloads
listed below. This proposal keeps that omission and replaces the retained
handoffs' local pointers with exact immutable public references. Its purpose is
to keep the historical record inspectable without copying the old logs into the
current candidate. It changes no firmware behavior and establishes no new
qualification result.

This is a new explicit retention disposition proposed to Dot for adoption. The
original reason for deleting the five payloads is not established. This note does
not invent that reason or claim earlier authorization for the deletion. Acceptance
of this disposition remains Dot's source-selection decision.

The evidence remains in the public base and its existing Git blobs. Deleting a
path from a later tree does not erase Git history, invalidate the historical
evidence, or remove a prior public disclosure. Retention preserves what the old
records can establish on their original inputs; it does not convert them into
proof for current source, target builds, packages, or device behavior. Existing
failed and unrun qualification remains unchanged.

## Exact retained evidence

All five paths are under
`cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/`.
The links pin the public base above. Git tree metadata verifies each path, blob,
mode `100644`, and byte count without reproducing its payload here.

| Historical path suffix at that directory | Existing Git blob | Bytes |
| --- | --- | ---: |
| [checkpoint.json](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c007de94f6afd4e46b22ff3e88dd0ec581cc2674/cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/checkpoint.json) | `2025b240f14e66d94d52cdf437a08f8078730a7b` | 1824 |
| [gate-reason.log](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c007de94f6afd4e46b22ff3e88dd0ec581cc2674/cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/gate-reason.log) | `77ae579cee80ae265b24825c1e86b193b431039d` | 100 |
| [install-slots.log](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c007de94f6afd4e46b22ff3e88dd0ec581cc2674/cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/install-slots.log) | `ab65e28ac413acec3be079a5461d41b772d3e71c` | 1129 |
| [update-contract.log](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c007de94f6afd4e46b22ff3e88dd0ec581cc2674/cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/update-contract.log) | `7b8bcae12c6c237dea42e0b8f8bb94fc6983444c` | 6498 |
| [validation-inputs.log](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c007de94f6afd4e46b22ff3e88dd0ec581cc2674/cloud-handoff/2026-10-20/fm1_reference_stage2_20261020/build/fm1-reference-stage2/validation-inputs.log) | `605436b91137ea79bc45a9be2a80b55a81c1a065` | 102 |

The historical [one-build manifest](../manifests/one-build-b982c1ee.tsv) retains
its original five entries unchanged. Their presence documents historical input
identities; it does not assert that these paths exist in the current candidate.

The updated [start handoff](../../cloud-handoff/2026-10-20/START-HERE.txt) and
[convergence handoff](../../cloud-handoff/2026-10-20/delivery_convergence_20261020/HANDOFF.txt)
retain their original historical meaning. Their old stage instructions do not
become current execution authority through these reference repairs.
