# Code stages

1. **Audit.** Inspect the specified revision and intended behavior. Record defects, evidence, affected scope, fixes, existing approvals, and missing decisions.
2. **Implement.** Use the audit and approvals. Hand off checked commits and deferred items. Ask only for decisions still missing.
3. **Test.** Check the complete stage-2 candidate. Use separate test and investigation subagents.
   Record passing checks and each failure's cause and disposition. A source repair needs a fresh stage 2, then fresh stage 3 for affected checks.
4. **Validate.** Use the tested candidate, configurations, and dependencies.
   Record exact build/install-package identities, checks, and logs under [rule 11](../../x0x37-integrate/SKILL.md) and [rule 6](../../x0x37-install/SKILL.md).
5. **Merge.** Wait for all contributors' validated commits. The sole assigned integrator merges the complete set and records revision and validation identity.
6. **Install.** Use the merged revision, matching artifacts, and existing permission under [rule 14](../../x0x37-owner-decisions/SKILL.md).
   The designated installer records installation and data checks under [rules 5–7](../../x0x37-install/SKILL.md). Do not add another approval question.
7. **Push.** Use the completed merge/install evidence and approved repository, ref, and scope. Verify the remote ref matches the intended commit.
   End the workflow. Do not start another audit automatically.

Use [scoped checks and matching evidence](../../x0x37-check/SKILL.md). Stage 4 defaults to development.
Stage order is not release-build permission. Before release work or a full gate, use [rule 11](../../x0x37-integrate/SKILL.md).
A public release package also requires the owner's device PASS for that installed build.
Start its fresh task only when requested and its evidence exists. A Git push does not authorize release packaging or publication.

Before installation, compare merged inputs with stage-4 evidence. A new commit ID alone does not invalidate it.
Changed checked source, configuration, tools, dependencies, or artifacts need a fresh chat for the earliest affected stage, then a fresh install chat.
Do not repeat a completed merge unless another merge is needed. Any new merge needs applicable validation and merge stages.
Stage 5 receives sole integration ownership in its handoff. Check project activity and the target branch before shared changes; do not compete.
Preserve [source protection](../../x0x37-task/SKILL.md) and [existing permission](../../x0x37-owner-decisions/SKILL.md).
A stage change does not permit an unspecified destination, publication, release, or new defect fix.
