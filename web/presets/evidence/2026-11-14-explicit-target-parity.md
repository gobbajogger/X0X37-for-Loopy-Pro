# W3 explicit-target DNPR parity correction — 2026-11-14

## Intake and owned scope

Request: one isolated correction of the explicit-target finding on draft [PR 901](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901). The delegated repair owns `wip/w3-explicit-target-parity-20261114`; the old W3 branch, its visual proposal, and other source lanes remain unchanged. No overlapping codec writer was identified in the supplied current ownership handoff.

Live W3 head was read at intake and again before delivery: `7bece9b819790600bb755e2900817b249d875567`, root tree `00d5dff9d50e7855774cca5d64adfdce0b0bafa6`. Accepted F1 head: `d963c3925a5b05f329e24c1a793a6326118fa466`; native `preset_record.c` blob `0075011b4bd9f8cb43982f8a0d53d106a14a5cfe`. Source intake and isolated sparse checkout were authenticated by Git object hashes at 2026-11-14T11:14:52.828438+00:00. A full native checkout/build is not claimed.

Consumed [owner parity receipt 6873423939](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901#issuecomment-6873423939), [W3 independent QA 6874103295](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901#issuecomment-6874103295) and [accepted F1 contract 6869385190](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/900#issuecomment-6869385190). The 548-assertion qualification and independent native/CLI replay are reused historical evidence at those exact inputs, not new passes on this repair.

## Original failure and exact repair

First substantive scoped action: strengthened A4 and added P1/P2 regressions, then ran the actual original W3 codec/app before editing production. At 2026-11-14 11:16:08Z the run exited 1: 24 Node subtests, 16 pass, 8 expected failures. Valid Chicago payload/Miami provenance (P1) and the inverse (P2) rejected their explicit actual-engine target with `WRONG ENGINE: Target differs from record or original engine`. The actual page could not preview that valid explicit-target input. Implicit parsing and wrong-target rejection were already supported. Original source objects and the red log are retained in private scratch. Red-log SHA-256: `0c59eed1b82d899c799b42862f19e6950b33fe8371838fd5100e2a792c38f1d8`.

`parseDnpr` now refuses an explicit target only when `engineId !== expectedId`, matching native `preset_record_import` / `check_loaded`. Recognized `originalEngine` is still validated, displayed and retained. The stale native-behavior comment and wrong-engine detail now describe actual-engine eligibility. The warning states:

> Original engine differs from record engine. It is retained as provenance. Target checks use the actual record engine; no conversion is performed.

Production delta: one eligibility predicate plus its comment/error detail and one warning string. No layout, model, schema, CRC, encoding, ZIP, factory guard or device API changed.

## Executed focused validation

- Node `v24.19.0`: `node --test web/presets/test_codec.mjs web/presets/test_page.mjs` on the final frozen source → **24/24 subtests PASS, 0 fail, 0 skip**, exit 0. Includes 23 codec subtests and all **17 DOM workflows** (15 retained + 2 new).
- P1/P2 explicit token/numeric actual-target and implicit accepts retain provenance, parameter bytes and original export bytes. Opposite actual-engine rejects. Invalid provenance, future schema, invalid parameter count and CRC reject. Mixed-batch failures preserve empty/occupied sessions, existing record bytes and `nextId`. Factory and derived records remain inspectable but cannot export.
- Actual app event checks cover successful target preview/Keep/download and preserved warning, plus wrong-target/invalid provenance/schema/CRC refusal with no accepted pending item or changed existing session/selection.
- Playwright `1.57.0`, actual Chromium `154.0.8037.57`: focused `test_target_browser.mjs` → **6/6 scenarios PASS**, exit 0 (P1/P2 at 1365, 390 and 320 CSS pixels). Each checks explicit acceptance, visible truthful warning, Keep, byte-identical browser download, opposite-target refusal preserving the existing entry, and implicit preview. Zero page exceptions and off-loopback HTTP requests.
- The actual P1 warning screenshots at all three widths were visually inspected for visible wrapping and overflow. These are browser viewport checks, not physical-phone proof or general visual-design approval.
- `git diff --check` → PASS. No separate lint command is configured for this standalone module.

Reproduce the focused browser check with installed Playwright/Chromium:

```sh
DNBOX_PLAYWRIGHT_MODULE=/path/to/playwright DNBOX_CHROMIUM=/usr/bin/chromium node web/presets/test_target_browser.mjs
```

Final Node-log SHA-256: `e0f5b3388212feba2c0104dc4698d28f8ec3f936c84be9af09b4c78278515ef1`. Browser fixture SHA-256: P1 `c61b4531258d777c555cea657b5fd02e761b3a1e662f8d4f06a0571ecf3a8881`; P2 `9b14354e873cffa49e102e1d646c0d58418d0c5920aa4a4b49ac6a16b04aaa0f`. No fixture binaries or screenshots are committed.

## Exact frozen changed closure

Paths are relative to `web/presets/`.

| File | Intake Git blob | Reviewed/executed Git blob | Final SHA-256 |
| --- | --- | --- | --- |
| `codec.mjs` | `0d9667d0f9fccb825c27df0e0969f201deed8299` | `8e77fde3f117c0e430f0d8d4ea60a31a34bda181` | `ef05c7d7eeac5c93cb10975f9b6d0eeac01387b0ce1b2b32188e7c198ad13cd8` |
| `app.mjs` | `2e7e0c507ad773a3a70eea4de9c2bc77cb4b6972` | `895827c62d10bd07c073cdf076cea277d6a34560` | `380ca057cdfa3df620cb14ac10d32d31d2ef694618837a30f53df3eb04033f22` |
| `test_codec.mjs` | `eae908c6d8bdbc95e2a3607309566e8c21e66933` | `dff620af60b8678bad92855b52d119ff2dbd2b27` | `8db80acd5963f42e6c7814e0d64dcc53949129accf79cb7e44e6e5176ef49fb5` |
| `page-harness.mjs` | `de803814b37dc3aa75e47b9a9be5274f2fde6978` | `e7cac9b25f4c2b22cf89025f8c0a92e8702119a9` | `9990679aaf70daaef58a91189d47471af47cc5e55e5796b812bc844a858a56cb` |
| `test_page.mjs` | `b56fcf144a316fe5214db4aa363c22c377e38f6e` | `72dc599be141d5ec227a9a8a90c56b02c610e5bb` | `fa1c919232b3a1aed65a603d5f12da02119619e876e54940038b89ba123168e1` |
| `test_target_browser.mjs` | `new` | `ba4e659ef8db9c686c4588765e994a78864a97ae` | `1af459a6b10225f5dca1e4f23b095a3f7a48b9f6f038c7e23b153825802519b3` |
| `index.html` (unchanged dependency) | `b59a553e597171dc267967a9ea202b45be7bb2ad` | `b59a553e597171dc267967a9ea202b45be7bb2ad` | `9325b755b13421ad3a39300d3fb3d0cdba44b0e094bdb28659fc2d9c6b712682` |
| `zip.mjs` (unchanged dependency) | `d7af5a808c39cfaba609967d27e8374dd2cef34c` | `d7af5a808c39cfaba609967d27e8374dd2cef34c` | `2a1d9da77317281065f53b35b494d83f2e8ddddc25795951ee478dbaaa9138c9` |

## Screenshot evidence (private scratch only)

| Actual preview | SHA-256 |
| --- | --- |
| P1-1365-warning.png | `f0e29313b5ac5661feafedb941ff2e3d8667efe8fbed0d8726156f177117d92d` |
| P2-1365-warning.png | `4239ce4adbcaa0b003f59649b02b224281e6ef2a518f505713f506e567abbd56` |
| P1-390-warning.png | `cad40d7def8405d1ddee7cb488209ef548c6ddac20e686fbd05b17dbe7dc6d94` |
| P2-390-warning.png | `f8f192f5d8d8ea54647cc88fe32c577ddf919869d877ccde47f2576c0d23644f` |
| P1-320-warning.png | `084370a105369acded1a0e4b044e9867cf2e3f0180a1e7f8b28c303b95d2f206` |
| P2-320-warning.png | `cbe4549f8c55be9f513260452a95f0e89a39d92abc52a349a408aabc27d9a8f6` |

## Independent review and delivery

Independent read-only reviewer `w3_frozen_review` returned **SCOPED ACCEPT**, no actionable findings. It independently verified all six frozen Git blob/SHA-256 identities, inspected the actual source/test delta against the pinned W3 base and the accepted native functions, checked the producer logs and all screenshot hashes, and visually inspected P1 at 320px and P2 at 1365px. Its own `git diff --check` passed. It did not rerun Node, Chromium, native compilation or historical qualification. It also inspected this technical receipt before its pending review paragraph was replaced by this outcome. Browser-result manifest SHA-256: `4e540a7ecf61fca4360b0d11fd0bcacbbd3c95a539d4c6835eaae79527567918`. Acceptance applies only to the frozen parity correction; remote Git receipt is verified separately after delivery.

## Website-facing contract and remaining scope

This is browser-file preset validation, preview, temporary-session import and unchanged user-file export. It is not project persistence, sample/audio transfer, firmware save, device acknowledgement or an authenticated model handshake. A preset stores engine parameters and optional sample references; a project is a separate saved state, and referenced samples are separate audio assets. Keep and download do not establish device receipt, save or kit completeness.

Factory and factory-derived export stays disabled; device transfer remains unavailable. No conversion or provenance erasure is introduced. Existing warning adjacency, deliberate focus and mixed eligible-only export UX remain separately pending; this correction does not repair or approve them. The old visual proposal is preserved without redesign or approval. Native compilation, full W3 browser matrix, SDK builds, physical phone/device/transport, deployment, release and merge are **NOT_RUN** in this repair.

Next consumer: existing W3/incumbent independent QA can inspect the exact correction PR and decide this bounded finding separately from the other W3 holds. Source delivery is not integration or website publication.
