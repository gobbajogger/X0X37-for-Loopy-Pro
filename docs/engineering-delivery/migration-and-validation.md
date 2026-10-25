# Reconciliation and adoption

The purpose is one readable contract across environments, so the same stage does
not acquire contradictory owners, evidence labels or approvals. Expected benefits
are fewer repeated builds, duplicate assignments and optional-metadata stalls.
These are design expectations; host-fixture measurements below are a bounded I/O
proxy, with no measured production token or firmware throughput improvement and
no direct firmware or musician-interface change.

## Sources and instruction boundaries

This proposal is based on Cursor's actual branch
`cursor/clarify-classic-build-skill-e96a`, commit
`aa90929547f32eb255ec2cbd88eac919b0257099`, leaving that branch untouched.
Its parent is PR [#149](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/149),
`edf991deed24d49d25c4abcedcd8452c9c58eb3c`. Related lineage includes
[#133](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/133) (full cycle),
[#134](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/134) (optional ledger),
[#137](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/137) (adaptation),
[#146](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/146) (placement), and
[#147](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/147) (nightly policy).
The other reconciled workflow is `coordinate-engineering-delivery`, read on
2026-10-25, plus classic-build contract v0.1.53. Their public project rules inform
this contract; their environment-specific service routes and private notes are not
copied into repository authority.

The source task supplies the owner's later directions on medium effort, agent-run
testing, fresh stages and automatic qualifying source merges. This proposal preserves
those directions without treating another branch's claimed authorization as proof
of new flashing/publication rights. In particular it does not import unattended
installation, arbitrary binary-license waivers, blanket human review or self-QA.
An absent duplicate receipt does not erase genuine available evidence.

The inspected source tree has a `.cursor/skills/loopyx0x37-classic-build/` entrypoint
and no existing root AGENTS, CLAUDE, Copilot or Cursor-rule adapter. This change adds
root AGENTS and thins the existing Cursor skill. It does not invent other providers'
automatic loading behavior. A provider/session must actually consume the contract;
a pointer alone cannot make an external system obey it.

`AGENTS.md` mandates the canonical contract. Current user instructions/permissions
take precedence; `BUILDING.md`, `BEHAVIOR.md`, `LICENSING.md` and actual source keep
their subject-specific role. Cursor runtime/build/package references preserve
technical procedures and link to the same contract for policy. Superseded text stays
in Git history at the source revision, including technical provenance. PR #134's
ledger remains optional evidence transport rather than a metadata prerequisite.

## Documentation walkthroughs

These are contract checks, not build, scheduler or hardware execution receipts.

| Scenario | Expected contract behavior |
| --- | --- |
| App source changes; authentic stored controller bytes and complete controller closure match | Build affected Elite/Pro apps; reuse controller; recompose/check exact packages and pairing; rerun affected current-head checks. `CTL_NO_CACHE` isolates installer-cache writes. Missing controller evidence blocks dependent packaging, not unrelated app work. |
| Fresh build and independent QA stages share the same immutable input | Separate owners/output areas; parent reads durable progress. A missed READY callback does not cause duplicate work. BUILD_ONLY is not package, QA or hardware acceptance. |
| Hardware report only says old firmware works, or only Elite was tested | Preserve its actual scope; no claim of current full applicable hardware proof. Main source merge waits for exact bytes/model/behavior proof and normal protections; publication remains separate. |
| Identified scheduler prompt is authorized; conversation ID/profile/history are unavailable | Already executing maintainer reads current target/schema, changes prompt only with available guards, preserves other settings and reads back. Optional unknown metadata does not block. Known concurrent writer must hand off or serialize. |
| Scheduler write response is lost, or genuinely denied | Read target before repeating uncertain mutation. A real denial blocks the dependent write and is not rerouted through another agent. Independent work continues. |
| Parent needs an external task to perform missing operations | Verify actual admission identity, input consumption and progress reader before claiming handoff success. This does not prohibit prompt-only work already executing with sufficient authority and inputs. |

Link/path checks and an independent contradiction review belong in the PR evidence.
No firmware build is required for this documentation and host-decision-aid diff. No runtime recovery,
device result, scheduler change or measured throughput improvement is claimed.

## Adoption and rollback

Review this isolated source proposal against its pinned Cursor base; it does not
claim Cursor accepted ownership or merge either branch. An authorized integrator
chooses adoption after actual review/protection checks. Pin the resulting canonical
commit and file SHA-256 in future handoffs. A verified offline copy suffices.

The separate browser/status repository needs only a thin AGENTS reference pinned to
the accepted source commit. If unreachable, use a verified supplied copy; do not
create another maintained policy body. Plugin/coordinator/scheduler entrypoints can
be migrated in separately authorized stages. Preserve settings outside the assigned
change and retain useful technical procedures. Scheduler IDs, role splits and
frequencies are not permanent requirements: an authorized value audit may support
consolidation, rescheduling, pausing or retirement. Migrate ownership and checkpoint
identity explicitly, retaining essential capabilities, independent QA and history
without overlapping writers or silently carrying acceptance to a new role.
This source proposal changes none of
those services and does not make their old copies disappear.

Observe a subsequent authorized stage before claiming operational improvement.
Rollback only the affected documents through a reviewed change against current
state; preserve concurrent edits, evidence, safe artifacts and data. No force-push,
hardware rollback or publication action is authorized by this adoption plan.

## Incremental extension and current remote comparison

This local extension reuses the saved reconciliation rather than re-extracting
or rewriting its source history. Read-only connector checks on 2026-10-25 observed
Cursor `fc5de6d388c12aa2d67b5187b9b32dbd8467eb55` (two commits after the pinned base),
firmware `nightly` `93449136baae02c9715ba20edc5c43176786b635` and `main`
`c9718ae833eb64a5191c350922593df8a36017c9`. The isolated proposal retains base
`aa90929547f32eb255ec2cbd88eac919b0257099`; current remote deltas are reconciled
explicitly, not represented as already merged ancestors. Native `ls-remote` failed
DNS; the successful connector reads are the remote evidence. Refresh before any
future integration because these observations are not a permanent current-head gate.

PR #155's resolution stage refreshed the same exact heads and normally merges
Cursor `fc5de6d388c12aa2d67b5187b9b32dbd8467eb55` into the owned proposal lane,
whose first parent is `54f5290505b7023906821429f0619a968b53ec53`. This supersedes
the preceding unmerged-ancestry description for that combined candidate. Cursor's
branch remains untouched. The resolution retains its exact hardware-record
glossary, explicit feature-base distinction and required license package content;
canonical gates keep publication separate from source integration. Existing
helper implementation/test bytes and compiled firmware inputs remain unchanged. Review
and affected link/host checks cover the resolution; earlier accepted evidence
remains scoped to unchanged inputs.

The two newer Cursor commits clarify feature-PR base versus firmware integration,
publication/download/picker versus `nightly`/`main`, LaunchAgent `--ctl`, and exact
hardware package/controller/model plus backup/readback/running identity. Those
clarifications are retained. Blanket license waivers, unattended flash authority
and QA_PENDING bypass are not imported because current user instructions preserve
licensing, action authorization and meaningful independent QA. Existing safety
rules remain binding in `safety-gates.md`, linked from the shortened core.
Read-only checks found PRs #133/#134/#137/#146/#147/#149 open and unmerged; their
links above are history, not mandates to act or stale claims that they merged.
No verified merged-PR active instruction was identified to remove.

`tools/workflow_checkpoint.py` adds conservative local decisions and optional run
receipts without changing the accepted checkpoint, source, schedulers, cadence,
publication or firmware. Host tests cover unchanged exit, changed input, missing
checkpoint, read failure/malformed observation, pending same-SHA work, per-output
reuse, relevant-policy invalidation and changed proof/head/workflow revision.
The fixture executes 22 baseline file reads versus two measured decision-input
opens, avoiding 20 immutable fixture proof reads. This is not a production query
or token saving; real tokens, live scheduler before/after prompt metrics, retries,
completed firmware work and time to owner-installable NIGHTLY remain unmeasured.
Scheduler definitions and proposals require their separate live baseline/review.

The saved reconciliation's canonical core measured 1,101 whitespace-separated words
and 8,374 bytes; task36's incremental core measured 843 words and 6,453 bytes, using
`len(Path(path).read_bytes().split())` and `len(...)` on actual source. This reduces
the core by 258 words (23.4%); technical checkpoint and safety references are loaded
for their relevant stages. This is source size, not token telemetry or an estimate
of production usage. The original Cursor entrypoint at the pinned base measured
1,647 words/10,857 bytes; that different-scope comparison is not a runtime saving.

A separately read live scheduler baseline was relayed and parent-verified from
Work `6adda19e-1628-83eb-9689-fcd1ccc9d6ad`, private Library artifact
`libfile_ba93090efec4819199d7543ccda62912`; this implementation worker did not
independently download those definitions. The supplied packet reports 12 hourly
roles (288 potential runs/day), 16,430 prompt words/134,210 bytes, and 91,527 bytes
of excess exact-paragraph duplication. Proposed prompt changes are 74% smaller;
that is a text-size proposal, not applied changes or measured token/runtime gain.
The last observed runs were eight completed, three in progress and one failed;
four summary results predated their actual run. Actual daily executions, retries,
effective tier/effort and tokens were unavailable. No scheduler or cadence was
changed here. The checkpoint adapter therefore distinguishes actual run state
from stale latest-result summaries. Parent also verified PR #103 and website
PRs #3/#8 merged; no website content is modified by this source implementation.
