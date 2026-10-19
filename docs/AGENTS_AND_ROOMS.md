# Current agents and work ownership

This file describes the active coordination rules. The [2026-10-17 roster](archive/rules-2026-10-17/AGENTS_AND_ROOMS.md) is a historical record. Its agent IDs, box paths, role limits, branch tips, and proposed checks do not assign work now.

The current task and its owner instruction assign agents and scope. For this rules repair, the rules audit task owns docs, rules, entry points, and their enforcement repairs. The existing "Integrate and review x0x37 work" task owns firmware integration. It uses `flash_review_integration`; UX work uses `ux_combined_review`. Do not change their firmware or create a second firmware coordinator.

Use one responsible agent for each action. Give subagents separate files or isolated worktrees. Coordinate an actual file overlap before editing. Roles describe skills and ownership; they do not prevent an authorized task agent from completing its work when an old role is absent.

Use short lived `lane/<agent>-<topic>` branches. The conventional role names are `mb` for firmware and device work, `sb` for UI work, `opt` for optimization, `btr` for validation tools, and `dk` for docs. The owner can authorize a task across these areas. Rule repair uses `lane/dk-rules-<topic>` and can include its enforcement tools.

Every agent and subagent reads [agent-rules.md](agent-rules.md), [rules/core.md](rules/core.md), [rules/style.md](rules/style.md), and the exact writing skills listed in [SKILLS.md](SKILLS.md). Include those paths, the input commit, allowed files, done criteria, and any evidence limits in agent prompts. Plain, correct English and concise caveman style apply to every output.

Report the commit, checks, remaining dependency, and next action in a handoff. Use bounded waits and one final independent review for rule repairs. Do not restart finished work. Device writes stay with the designated installer and retain all [install guards](rules/install.md).
