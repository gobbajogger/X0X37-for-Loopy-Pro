# DnBOX orchestrator routines (handoff)

Model rule: every new or nudged ChatGPT thread uses GPT-6 at Extra High effort, never GPT-5.6.

## 1. DnBOX threads 15-min pass
Every 15 min (America/Los_Angeles */15 * * * *), flexible ±7 min

15-min DnBOX pass (GitHub-first).

0. GitHub first (no browser): using the GitHub connector or API on gobbajogger/X0X37-for-Loopy-Pro, read every open PR's latest status comment, head SHA, base and merge-base relative to the latest wip/one-build, and the last push time. Classify each PR as moving, done, stale base or stalled (no push or status comment in about 2 hours). Only chats that are stalled, need a nudge, are done but not yet merged, or whose PR has no status comment get opened in the browser.

Merging: for PRs whose status comment reports passing cloud tests on the current one-build base, ask Merge Integrator (SendToAgent) to merge them into wip/one-build through the GitHub API, in the Orchestrator's integration-plan order (never main or nightly, no Cursor cloud), and to report conflicts. A conflict goes back to that PR's chat as a full stage (rebase, fix, test, push, comment).

1. Browser, only for the chats picked above. Open a NEW tab and stay in it; never close tabs the owner opened or other chat tabs. If a chat shows 'Could not load' or 'Unable to load history', actually CLICK Try again every 2 minutes whatever it looks like (in light theme it never looks active); never reload. After about 5 minutes of failures, use the verified handoff: confirm on GitHub what's done and pushed, then start a new classic LoopyX0X37 thread (GPT-6, Extra High effort; Pro only for genuinely hard builder tasks) with a full handoff.

2. Each nudge gives a whole stage: rebase onto the latest wip/one-build and name it as the base, fix, build and run the tests yourself in your cloud environment, push through the GitHub API (never git over the network, rule 1m), push only what isn't already on GitHub, reuse the existing PR, then post a short PR status comment (SHA, base, test results, next step, blockers) and approve your own posts and GitHub actions on the owner's behalf (rules 1b/1k/1l). Artifacts rule (owner, 2026-10-29): every important artifact (source, tests, plans, docs, test logs and results, reference files) must be pushed to GitHub as soon as it exists, so nothing lives only in a chat sandbox or on the Mac. Private headers may be pushed (owner, 2026-10-30); no credentials, binaries or raw captures. Click Allow on any card asking to let GitHub create a blob, tree, commit or ref in gobbajogger/X0X37-for-Loopy-Pro (standing owner approval). Flag any chat in Work or Codex mode.

3. The Mac (machineId f75de5b4-4619-4624-9dcb-f47692ef7ef3) is only for hardware installs and device checks; there are no more local test lanes there. If it's reachable and any important result exists only on the Mac, make sure it gets pushed to GitHub.

Report one plain line per changed PR or thread, leading with what changed and anything only the owner can decide. If nothing changed, send nothing.

## 2. DnBOX chats load retry
Every 5 min (America/Los_Angeles */5 * * * *), flexible ±2 min

Retry check for the DnBOX ChatGPT chats. Only act if the latest 'DnBOX threads 15-min pass' run or this routine's last attempt could not load the LoopyX0X37 chats on chatgpt.com ('Could not load this ChatGPT conversation' or 'Unable to load history'); otherwise end silently. If another browser task is already running, skip this run.

Browser rule (owner, 2026-10-29): open a NEW browser tab for this task and stay in it; don't use or change other tasks' tabs. Never close tabs the owner opened or other chat tabs; leave chat tabs open. Stay on each failing chat for about 2 minutes, because switching chats cancels a pending Try again.

Owner's rule: on a load error do NOT reload. Actually CLICK 'Try again' every 2 minutes, whether or not it looks active. In ChatGPT's light theme the button looks the same when it's disabled and when it's clickable, so never judge by its appearance: just press it and check whether the chat loads. Track the time of the last attempt and the list of failing chats in /workspace/dnbox-retry-state.txt.

Long-thread rule (owner, 2026-10-29): long threads cause repeated load failures and stall progress. If a chat is long or still fails after about 5 minutes of retries, hand it off to a new thread, but only with a PROPER, verified handoff:
1. Before starting the new thread, establish what the old chat has done from GitHub, using the GitHub connector or API: its PR (description, comments and checks), the branch's latest commits and SHAs, the base and merge-base relative to the latest wip/one-build, and any 'next step' or blocker in its last PR comment. Also check my memory and journal notes from that chat's last successful load.
2. Confirm the work is actually on GitHub. If the last known SHA or claimed work from my notes isn't in the branch, the old chat may have unpushed local work: don't hand off yet. Keep retrying the old chat (it must push first), or note the gap clearly in the handoff and tell the user if work could be lost.
3. Only then start a NEW classic Chat thread in the LoopyX0X37 project (Extra High effort) with a handoff covering: the task, the PR, the branch, the verified last SHA, the base (latest wip/one-build), what's done, test status, open blockers, the next step and any unverified gaps. Include the standard reminders (build and test in your own cloud, GitHub API only, no git over the network, private headers may be pushed but no credentials or binaries, don't reinvent existing work, reuse the existing PR). Record the new thread URL and the old chat it replaces in the state file.

When a chat loads, do the missed work in classic Chat threads only (never Work or Codex; if a chat offers Work, reply 'Stay in Chat mode and continue here.'): read each chat, nudge stalled ones (reminding them to build and test in their own cloud environment, to use the GitHub API rather than git over the network, and to push all their local work), and resolve blockers with the easiest fix. If a chat shows a card asking to allow GitHub to create a blob, tree, commit or ref in gobbajogger/X0X37-for-Loopy-Pro, click Allow (standing owner approval). Tell the user only owner-only blockers or a risk of losing work, then clear the state file. If chats still fail after about 30 minutes across runs and no safe handoff was possible, tell the user briefly that it's blocked, and clear the state file.

## 3. PR babysitter hourly (backup; PR Babysitter also runs its own 10-min check)
Hourly around :36 (America/Los_Angeles), flexible ±15 min

Wake PR Babysitter (agent id <PR Babysitter id>) with SendToAgent priority true: ask it to refresh its DnBOX PR stage ledger, verify each item's real state, and ping any owner stalled for more than an hour. If it reports a blocker that needs the user, tell the user in two sentences or fewer. Otherwise stay quiet.

## 4. DnBOX keep moving without Mac
Every hour (@every 1h), flexible ±15 min

The user's standing rule for DnBOX (gobbajogger/X0X37-for-Loopy-Pro) is that work never stops because the user or their Mac (machineId f75de5b4-4619-4624-9dcb-f47692ef7ef3) doesn't answer, and that builds and host tests run in the cloud inside classic ChatGPT threads (the box may also run the host suite as a double-check; the Mac never does). Each run: 1) Through the box browser, open each LoopyX0X37 classic ChatGPT chat (Chat mode, GPT-6, Extra High effort; Pro only for genuinely hard builder tasks; never Work or Codex). Any chat that's idle or waiting on the Mac, on Mac builds or tests, or on the user should be told to run its own builds and host unit tests in its cloud environment now, fix any failures, push, and report pass/fail with the commit SHA visible on GitHub. Tell every chat you message to approve its own posts and GitHub activity on the owner's behalf. 2) If the Mac is reachable, read the RESULT lines from ~/Documents/x0x37/codex-lanes/test-*.log and send failures to the matching chats. Use the Mac only for hardware installs and real-device sound or Bluetooth checks. 3) Never ask the user to approve anything except real security issues. Message the user only for a real result or a real security blocker; otherwise stay quiet.

## 5. Hourly builder check (legacy; only if the room-based builders are still used)
Hourly around :28 (Asia/Vladivostok), flexible ±15 min

Decide whether each x0x37 builder (Firmware Main Builder on ble-stage3, Second Builder on clip-keys) is still mid-task. Read its latest room message in the X0X37 rooms. If it says it is working, waiting on something, or about to do a next step (and has not said done, finished, or idle), treat it as working even if nothing new appeared in the last hour. Also count new commits or pushes in /workspace/x0x37.git or a running install as working. Also check for anything waiting on the user: a question, decision, or hardware test asked of the user (by me or any agent, in the rooms or my DM) that the user has not answered. If one is still unanswered after an hour, remind the user once per run in two sentences or fewer, naming the exact question. If both builders are done or idle and nothing waits on the user, stop: no checks, no posts, no message. For each working builder, review its work (commits, the rooms, and read-only logs on Mac f75de5b4-4619-4624-9dcb-f47692ef7ef3 at /Users/mac/Documents/x0x37: x0x37-progress-log.md, merge-monitor-log.md, flash_review/BLE_ATTEMPTS.md). Judge progress, look for stalls or repeated failed attempts, and check rule compliance (agent-rules.md 1-27, lanes, installs only by Firmware Main Builder with notify/backup/CRC, simple-english docs, brief posts). If a builder is stalled or stuck, ping it for status, or post a rule-26 delegation tagging the owner. If a rule is broken, correct it in the room in two sentences or fewer. Otherwise message the user only for a blocker, a decision, or a serious quality problem, in two sentences or fewer.
