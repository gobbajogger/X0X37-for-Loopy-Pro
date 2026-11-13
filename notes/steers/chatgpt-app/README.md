# Shared ChatGPT app (owner-signed-in, display :14 = Orchestrator desktop)
Decision 2026-11-13 ~22:25 UTC+10 (Dot routine coordination within the owner's classic-thread approval): use the EXISTING signed-in app with serialized turns; no extra logins, no credential copying, no settings changes.
- Other bots' desktops cannot show this app (single-instance; second launch hands off to :14). So the Orchestrator relays: the specialist keeps task ownership, sends its handoff file to the Orchestrator, and gets the thread title/reply back.
- Reservation: ./reserve.sh claim <owner> <task> [min] / release <owner> / status. One holder, bounded timeout, release at each UI handoff, check live use before reclaiming a stale hold (RECLAIM=1). Log: log.txt.
- Never close the app or another session; never start a duplicate thread; reuse an existing matching thread in LoopyX0X37 (Chat tab, not Work/Codex).
- Queue: 1 Firmware UX (piano roll + chord notation) -> 2 Main Builder (SEQ chord fix) -> 3 Runner (web emu latency) -> 4 Optimizer (flash savings 026).

UPDATE 2026-11-13 ~22:27 UTC+10, owner's own answer: "Set up a separate ChatGPT app for each bot and I'll sign in to each one" (supersedes the serialized-turns decision above).
- Per-bot instances: `DISPLAY=:<bot display> chatgpt --user-data-dir=/home/box/.config/Codex-<bot>`. Owner signs in to each; no credential copying.
- Launched: fmb :46 (PID 3791188), optimizer :55 (3791190), fux :61 (3791192), runner :49 (3791194). Orchestrator keeps the original at :14 (~/.config/Codex).
- Never close another bot's instance. Reuse threads (same account, shared sidebar) instead of duplicating.
