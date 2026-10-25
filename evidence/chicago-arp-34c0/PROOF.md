# Chicago Arp local proof

- Plan: `local_proof_plan.json` (kind=implement; base `46ca0451` stream tip; selectors host-build sequencer projects-page projects-web; plus `chicago-arp`)
- Ran with `X0X_TEST_JOBS=8`: **PASS** chicago-arp, sequencer, projects-page, projects-web
- **host-build**: not-run / env fail on this Mac when attempted (SDK include noise in `host/x0x_host.c`) — same class of failure on clean stream5 tip; not caused by this PR
- `node web/test_web.mjs`: skipped — `web/editor.html` absent on stream5 tip
- GitHub Actions are not proof

HEAD: d0a8fdbfc40916fe4a15729a9e49e2dc7bcfbbc2
