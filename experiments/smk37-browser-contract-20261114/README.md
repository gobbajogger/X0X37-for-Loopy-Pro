# SMK-37 browser MIDI contract harness

Runnable engineering prototype with a sealed fake MIDI endpoint. It requests no device permissions and cannot attach a real port. It does not emulate firmware, produce audio, or establish device compatibility.

From this directory:

```sh
python3 serve.py 8765
```

Open `http://127.0.0.1:8765`. Connect a fake endpoint, arm, then use the wire probes. Status and outgoing bytes are visible. Channel change, release, disconnect, focus loss, Escape, page exit, pointer cancellation and a ten-second arm lease clean up local obligations. Reconnect requires another explicit arm. An ambiguous send or lost link leaves a persistent unconfirmed-release warning; reconnect never replays an old session.

Reproduce acceptance:

```sh
node --test transport.test.mjs
node --test review/hostile.test.mjs
python3 browser.test.py
python3 review/browser-cancel.py
python3 verify_packet.py
```

Node tests use only Node built-ins. Browser checks use the installed Python Playwright package and `/usr/bin/chromium`; this environment used Node 24.19.0, Python 3.12.14 and Chromium 154.0.8037.57. The browser test creates and shuts down its own loopback server. It blocks MIDI requests and external page requests. It is desktop Chromium plus a 390px viewport check, not physical phone/Safari/OS MIDI qualification.

Read [CAPABILITIES.md](CAPABILITIES.md) for exact source behavior, [REPORT.md](REPORT.md) for acceptance and limits, [review/REVIEW.md](review/REVIEW.md) for independent findings, and [NEXT_STAGE.md](NEXT_STAGE.md) for the bounded handoff. Source hashes and commands are in `evidence/` and `MANIFEST.json`. The Library ZIP includes public source evidence copies; the original full source archive remains in the workspace only.

All fixture/transport source is GPL-3.0-only. Copied project evidence retains its original notices and project license. No deployment, shared-source/firmware/UI change, publication, build or hardware result accompanies this packet.
