# W3 #901: executed Node and Chromium acceptance

Date: 2026-11-13. This is an **exact-source host/browser receipt**, not a device or firmware qualification.

PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901
Tested branch: `lane/w3-web-dnpr-20261113`.
Exact fetched commit: `fa85b9463a3f88df8bad022043722f375a762aee`.

## Actual Node test command

```sh
node --test web/presets/test_codec.mjs web/presets/test_page.mjs
```

Environment: Node v24.22.1. Result: **18/18 Node subtests PASS, 0 FAIL, 0 SKIP**. The existing 17 codec cases plus 11 DOM-mock scenarios are all retained, i.e. **28/28 focused cases**. The 11 UI scenarios are executed by one Node umbrella test.

```text
ℹ tests 18
ℹ pass 18
ℹ fail 0
ℹ cancelled 0
ℹ skipped 0
ℹ todo 0
ℹ duration_ms 138.625216
```

## Real Chromium file interactions

Browser: Chrome/156.0.8078.79; Puppeteer 25.17.0; Node v24.22.1. The source-local browser runner started an ephemeral **127.0.0.1 HTTP server**, launched an isolated headless Chromium profile and used actual browser file picker, binary downloads, drag/drop, keyboard Tab+Enter, mobile touch emulation and Chrome DevTools accessibility tree.

**15/15 PASS, 0 FAIL, 0 uncaught page errors, 0 external network requests.** No MIDI, USB, firmware, device writes, installer, or deployment.

| Actual browser scenario | Result |
|---|---|
| 1. Chromium desktop: dark-only layout, all controls, no horizontal overflow | PASS |
| 2. Native file picker: preview is non-mutating until explicit confirmation | PASS |
| 3. Keyboard: reachable Keep action and unchanged .dnpr download | PASS |
| 4. Actual Chromium bad CRC rejects without disturbing existing session | PASS |
| 5. Actual Chromium newer schema rejects with NEWER DATA | PASS |
| 6. Target engine mismatch shows WRONG ENGINE without write | PASS |
| 7. Filename-model hint conflict rejected; not a claimed physical identity | PASS |
| 8. Real ZIP picker: preview/keep, ZIP download, both original records intact | PASS |
| 9. Factory-derived preset preview but both export methods refuse | PASS |
| 10. 909 sample references visibly warn when hardware inventory absent | PASS |
| 11. Chromium AX tree: status, label and button accessible names | PASS |
| 12. Real mobile touchscreen: 390px layout and usable tap controls | PASS |
| 13. Small mobile 320px: no clipping, accessible focus and touch targets | PASS |
| 14. Real drag/drop: browser File and DataTransfer call same validation path | PASS |
| 15. Isolated local server: no external requests or uncaught JS errors | PASS |

## Screenshot evidence manifest

Full-page PNG screenshots were generated on the authorized DnBOX cloud scratch device at `/workspace/scratch/w3-901-verified-fa85/evidence/`. This PR keeps **text evidence only** and does not commit PNG binaries. File byte counts and SHA-256 hashes support independent identification of the captured images.

| Screenshot | Bytes | SHA-256 |
|---|---:|---|
| desktop-empty.png | 411989 | `a9db1b44a13cf6d651f14821eb3c57f13c36e59b0ebad8a32ef82d25ebf373ce` |
| desktop-chicago-preview.png | 372583 | `023b1b01f9d5f54dab91070d3221f695cb2c1c30e371511a80269ca1bb1ba9d6` |
| desktop-kept.png | 487416 | `1483e782eaa8137e279789a8f5f0474d21f022ca98393ad2cf90050588ce6915` |
| desktop-909-warning.png | 556958 | `1358b7a0158d2e0ee296e9521cfc69b542536e7f5a469f05ce2e7a52b0ee3f26` |
| mobile-390-empty.png | 549095 | `c6a85228fbe9bc6ca7f468ec5f7e81f0c4c360b65505d3688294d73a35852af4` |
| mobile-390-808-preview.png | 726255 | `6e44972756cbf400c4d8e90ae0c8565610b3ede6e17d84018dd56453b05820a6` |
| mobile-390-kept.png | 718252 | `05c07ce6661289e6edc28506e7871405ad5af0d7b2e963303c931f065092c037` |
| mobile-320-empty.png | 579512 | `38643e2f0e994cc2f387e6b7097bb891f22b7687cedd7fd8ac495290458084bf` |

Visual review of the actual Chromium screenshots: dark-only UI, legible 390px mobile file/preview controls and sample-reference warnings, Keep/Discard with no observed clipping. Automated layout tests cover desktop 1365px, mobile 390px and narrow 320px. The browser accessibility tree includes labelled controls; an actual Tab then Enter activates Keep.

One initial browser FAIL came from the **test** measuring hidden Keep before a preview existed (zero width/height by design). The script was corrected to check Keep after preview and all 15 browser cases reran successfully. No product source fix was needed.

## Exact pinned source Git blob SHA-1s

| File in `web/presets/` | SHA-1 |
|---|---|
| README.md | `f68294fca8642ed268fd093eae20b4e1dd371a78` |
| app.mjs | `955f31f27c2321e605d7be605f118192e0b011f2` |
| codec.mjs | `0d9667d0f9fccb825c27df0e0969f201deed8299` |
| index.html | `30c5e8046e26d4c4eaca529557605af7e3a1ba0f` |
| page-harness.mjs | `8b6ca5647a9b32e8a35aab52ac416b01f892df6b` |
| test_codec.mjs | `eae908c6d8bdbc95e2a3607309566e8c21e66933` |
| test_page.mjs | `56f19421f38482bae1cfe06f0b49140a56a3b6b9` |
| zip.mjs | `d7af5a808c39cfaba609967d27e8374dd2cef34c` |
| test_browser.mjs | `ff2de3aaf62f79ed3bdb738a18788115d48b1452` |

## Reproduce

```sh
node --test web/presets/test_codec.mjs web/presets/test_page.mjs
DNBOX_SOURCE_SHA=fa85b9463a3f88df8bad022043722f375a762aee DNBOX_PUPPETEER_MODULE=/path/to/puppeteer DNBOX_CHROMIUM=/usr/bin/chromium node web/presets/test_browser.mjs
```

Install Puppeteer in the test environment if unavailable. The browser test runner contains its own loopback server. The exact commit was downloaded over read-only pinned HTTPS, with no network Git checkout or Git push.

**NOT_RUN:** physical Elite/Pro, device model handshake or import/export transport, persistent slot/flash changes, full host suite, firmware/emu360 builds, iOS Safari, real phone hardware, production deployment. Mobile was tested by Chromium touch/device emulation, not a physical mobile device.
