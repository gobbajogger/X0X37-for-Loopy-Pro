# W3 #901 multi-record preview warning regression: executed evidence

**Scope:** DnBOX web-only DNPR v1 preset preview. Original open/draft PR head `332477009179ea13e90b444b1ba700e41be24fed`, branch `lane/w3-web-dnpr-20261113`. Independent finding: [#901 issuecomment-6862190898](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901#issuecomment-6862190898). No device transfer, firmware rebuild, deployment, merge, installer, host-codec or protected-branch changes.

## What changed

Previously `renderPending()` rendered only `pending[0].record` metadata and warnings while **Keep** staged the entire validated batch. For selections of 2–32 files and multi-entry ZIPs, the new implementation renders a numbered metadata section for **every** record and shows separately numbered warnings for **every warning-bearing record**, including records after a clean first entry. Warning-free records still have visible metadata. A single-record preview retains the original markup and behavior. Warnings use `textContent`, not untrusted HTML. Validation, atomic staging, unchanged bytes and both factory-export blocks remain as before.

### Exact modified source Git blob SHA-1s

| File | Git blob SHA-1 |
| --- | --- |
| `web/presets/app.mjs` | `2e7e0c507ad773a3a70eea4de9c2bc77cb4b6972` |
| `web/presets/index.html` | `b59a553e597171dc267967a9ea202b45be7bb2ad` |
| `web/presets/page-harness.mjs` | `de803814b37dc3aa75e47b9a9be5274f2fde6978` |
| `web/presets/test_page.mjs` | `b56fcf144a316fe5214db4aa363c22c377e38f6e` |
| `web/presets/test_browser.mjs` | `addb1f1684e877ef8254833773299859488cb8ce` |
| `web/presets/README.md` | `de961624559b6a95ca0d522a7ca9a06a9ea9aa45` |

Reused unchanged head blobs: `codec.mjs@0d9667d0f9fccb825c27df0e0969f201deed8299`, `zip.mjs@d7af5a808c39cfaba609967d27e8374dd2cef34c`, and `test_codec.mjs@eae908c6d8bdbc95e2a3607309566e8c21e66933`. Edited and unchanged blobs were checked with `git hash-object` on the DnBOX cloud scratch copy; unchanged blobs match the GitHub connector's exact source at the original head.

## Actual execution, DnBOX cloud (2026-11-13)

Isolated scratch: `/workspace/scratch/w3-901-warning-3324770`. No in-place changes to predecessor's workspaces, no external file upload, and no network Git operation.

```sh
cd /workspace/scratch/w3-901-warning-3324770
node --test web/presets/test_codec.mjs web/presets/test_page.mjs
DNBOX_PUPPETEER_MODULE=/workspace/shared/dnbox-rdc/node_modules/puppeteer \
DNBOX_CHROMIUM=/usr/bin/chromium \
DNBOX_SOURCE_SHA=332477009179ea13e90b444b1ba700e41be24fed \
DNBOX_BROWSER_EVIDENCE=/workspace/scratch/w3-901-warning-3324770/browser-evidence \
node web/presets/test_browser.mjs
```

- **Node v24.22.1: 18/18 PASS, 0 failed, 0 skipped.** Includes 17 codec checks and a page-harness test with **15/15 DOM workflows**. Existing 11 DOM workflows were preserved and four added: clean-first / sample-warning-second and reverse; clean-first / factory-warning-second and reverse plus single/ZIP export blocked after Keep; three-entry mixed ZIP (clean Chicago, 909 references, factory-derived Miami), both warnings before Keep and export still blocked; original-engine source conflict in second record.
- **Chromium 156.0.8078.79, Puppeteer 25.17.0: 19/19 PASS, 0 failed.** Existing 15 real-browser cases remain, plus (1) actual two-file picker sample warning second and reversed, (2) factory warning second and reversed, (3) three-entry ZIP with two indexed warnings before Keep, no mutation on preview, Keep adds three, single and ZIP factory export still blocked, and (4) 390px emulated touch mixed-ZIP visibility, no horizontal overflow and a Keep target at least 44px tall.
- **0 Chromium uncaught page errors; 0 non-loopback HTTP requests.** Real file picker, downloads, Tab/Enter, actual Chromium Accessibility tree, drag/drop, and 320px/390px emulated responsive checks were rerun. Browser run logged its 19 case statuses and screenshot hashes in `browser-evidence/chromium-w3-901.json` on the authorized cloud, not in Git.
- **No automatic Keep**: each preview's record count stays unchanged until the explicit Keep action. No modified codec/ZIP or device communication modules.

### Negative control, original application with expanded harness

The same **15-check expanded harness** executed against the actual unmodified `app.mjs` and `index.html` from the original head (matching `app.mjs@955f31f27c2321e605d7be605f118192e0b011f2`). Result: **11 baseline passes, four expected regression failures**, proving the tests detect the omitted warnings:

```text
EXPECTED_FAIL multi-record preview shows sample warning on second record and reversed order: Missing indexed sample warning
EXPECTED_FAIL multi-record factory warning is visible in either order before keep and export stays blocked: Factory warning missing before bulk Keep
EXPECTED_FAIL three-record mixed ZIP shows both sample and factory warnings before any bulk Keep: All ZIP metadata not visible
EXPECTED_FAIL cross-engine original-source warning on second record is visible before Keep: Source conflict warning hidden
```

The negative-control command exited 0 **only because all four expected failures were observed**. Logs retained locally as `node.log`, `chromium.log` and `negative.log`; this is not an assertion that the original code passed the new checks.

### New real Chromium screenshot SHA-256s

| Scenario | PNG filename | SHA-256 |
| --- | --- | --- |
| Desktop clean-first, 909 warning second | `desktop-warning-second.png` | `cbe3ac680a740ccf2882b4e13680457ef0fdc082ac44e7ecd10c3d102d129778` |
| Desktop mixed ZIP, warning records 2 and 3 | `desktop-mixed-zip-warnings.png` | `e7a105ba1242e7bfacee099b251fc4c21d175e8c029e64198d17110e6ab774e1` |
| Emulated 390px touch mixed ZIP | `mobile-390-mixed-zip-warnings.png` | `f385c57c91d25ae36c0d496d1ce6832447c5252b005fe57988afe4b4791dd620` |

PNG binaries remain on authorized cloud scratch and are **not** added to Git. Original eight baseline screenshot cases remain documented in [the previous receipt](2026-11-13-node-chromium.md).

## Boundaries and re-review

Re-review **only** W3 multi-record preview and its tests against the exact pushed Git blob hashes above. These checks do not qualify a physical SMK-37 Elite/Pro, sample availability, firmware/device model handshake, native persistent storage, a plugin format, real iOS Safari or a real mobile phone. No firmware rebuild, USB transfer, page deployment, protected-branch update or merge was attempted. The PR remains draft for independent QA.
