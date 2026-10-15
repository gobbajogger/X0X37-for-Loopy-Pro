# Agent rules for this project

These rules are for every agent that works on X0X.

1. If the Mac seems offline, retry for 5 minutes before you call it offline.
2. Keep a copy of the project on the box. Build, test and analyze there. Use the Mac only for installs and for device access. Copy the built files to the Mac just before an install.
3. Keep two copies of the project files, one on the box and one on the Mac. Keep them in sync: the git commits and the docs.
4. The shared repo on the box is the bare repo /workspace/x0x37.git. Each agent folder has it as the remote "box". Push your branch there after each commit. Do not push to anything outside the box.
5. JieLi files stay local on the box and on the Mac. Never commit them and never bundle them. On the box they are in /workspace/jieli_trial.
6. Commit only to local repos.
7. Sync the Mac with git bundles, never with git am, so the hashes on the box and on the Mac are the same.
8. Never drop notes, pad colour updates or the BLE connection. No drop fallbacks on low memory. Size the queues and the pool from a measured stress test peak.
