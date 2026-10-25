# Chicago Arp local proof

- Plan: `local_proof_plan.json` (kind=implement; selectors host-build sequencer projects-page projects-web; plus chicago-arp)
- Ran with `X0X_TEST_JOBS=8`: **PASS** chicago-arp, sequencer, projects-page, projects-web
- **host-build**: not-run / env fail on this Mac (SDK `rusage_info_v4` / `u_short` include errors in `host/x0x_host.c`) — same failure on clean stream5 tip; not caused by this PR
- `node web/test_web.mjs`: skipped — `web/editor.html` absent on stream5 tip
- GitHub Actions are not proof

HEAD: d69b5d7f583d22b6484030c057958494830d3b33
