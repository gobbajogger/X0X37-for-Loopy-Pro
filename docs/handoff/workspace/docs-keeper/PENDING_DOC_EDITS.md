# Pending doc edits (Docs Keeper)
Written on the box because the Mac link was down. Apply to the Mac files when the link is back.

## USER_PREFERENCES.md
- Replace "Do not push to any remote." with "Do not push outside the box. The only allowed remote is box (/workspace/x0x37.git)."
- Replace "Commits: local only, author" with "Commits: local or to the box remote only, author".
- Add: - 2026-10-16 02:09: The user approved a shared bare repository on the box, /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. Do not push outside the box.
- Add: - 2026-10-16 02:11: The user gave the team more agency. Generic work is approved automatically: builds, tests, commits, pushes to box, doc updates, merges on the box. Before anything that can change how the device works or feels (installs, new behavior rules, changes outside clip mode, flash layout changes, removed features), notify the user first. Safety rules do not change: backups, CRC checks, never touch the bootloader, never push outside the box.
- Add: - 2026-10-16 02:14: If the Mac does not respond, retry for 5 minutes. Report the Mac as offline only after 5 minutes of no response. This replaces the one-minute rule.

## AGENTS_AND_ROOMS.md
- Add rule: Keep one copy of the project on the box and one on the Mac. Keep the two copies in sync.
- Add rule: Shared bare repository on the box: /workspace/x0x37.git, remote name box. Each agent pushes only its own branch there and fetches from it. No agent pushes outside the box. (User approved 2026-10-16 02:09.)
- Replace "Do not commit or copy the JieLi libraries in ~/x0x_trial." with "Keep the JieLi files local on the box and on the Mac. Do not commit them."
- Replace "Commits are local only. Do not push to a remote." with "Do not push outside the box. The only allowed remote is box (/workspace/x0x37.git)."
- Add rule: Generic work is approved automatically. Notify the user first before anything that changes how the device works or feels. Safety rules stay. (User, 2026-10-16 02:11.)
- Replace the one-minute offline rule with: If the Mac does not respond, retry for 5 minutes. Report it offline only after 5 minutes.
- Add docs-keeper box folder: /workspace/x0x37-docs-keeper.
- Bump Last updated; add one Change log line.
