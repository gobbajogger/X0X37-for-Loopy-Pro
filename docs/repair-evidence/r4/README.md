# PR #8 R4 — finite known browser regressions repaired; independent review pending

Parent requested the finite F1–F7 and browser model/binding/backup matrix at
[`8357bea49e1b80c1d6b315dc749692c92424b0eb`](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/blob/8357bea49e1b80c1d6b315dc749692c92424b0eb/docs/independent-qa/final-browser-review/regression-matrix.md).
This same owned PR8 branch continues from `068f3cd15dd4698cd6d609dee54628d73bd51686`
(root `c39126aac8fcca9e72e5a7c1157b546ecb5366e3`). Independent review
[5638498157](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638498157)
and exact fixtures at `255e00f5330409eb5bdb834d488ad8ee51602efc` were consumed.
Earlier F1–F4 repairs remain; no new producer/merger/publisher was created.

## Minimal shipping changes and reused source

- **F5:** documented per-level catalog/qualification field allowlists and typed
  public display/boolean fields reject the synthetic private/unknown injections.
  Public installer/catalog files use an explicit projection. Full private
  qualification is an input only, not a public qualification.json output. The
  valid actual CLI control still prepares matching installer/versions metadata.
  Existing qualification gates are retained, without new firmware eligibility.
- **F6:** the bounded reader from firmware PR102
  `a48d745b59683eaaf2283aed5ec1ec2187799cfc` enforces approved byte length during
  reading, cancels oversized/stale/error streams and refuses a missing reader
  before arrayBuffer allocation. Valid native Fetch streams, retries and guarded
  local File reads remain supported. The old stale-body test fixture now uses
  a real ReadableStream rather than an arrayBuffer-only pseudo-response.
- **F7:** exact six-line Catalog R27 guard from
  `876adf33d3a96accac3838c21f6f8681d8c1ab39` was ported without moving history
  data. The validator is byte-identical to its blob
  `5309b6a5d9b9fd34a10b447162306ebee648c570`. Matching immutable evidence accepts;
  mutable, mismatched and object-valued evidence_commit reject.
- **Actual WebMIDI/backup paths:** fm1ota.js now matches existing GPL browser
  reference `c2e63feb11385b91a9dbd3b29b20a368ed948a5a`, blob
  `485a54187e78822847f5cb51088204983caffa71`. Its observable single-port/bind,
  loader-family and post-verification guards are reused. Slot helpers bind
  before backup and recheck before restoration; unknown layout/read failures
  block installation. Existing FM-1_015 and supported SMK-family version16's
  exact unsupported-slot-command response remains allowed. No new UID,
  physical_id or transport architecture is introduced. Browser names/ports
  are not proof of durable physical-unit identity.
- **Existing output boundary:** the same path-check defect reported on PR9
  also applied here. Actual CLI RED accepted inside `..candidate` and an
  outside-to-inside symlink alias. Canonical ancestor/segment checks now enforce
  the already documented outside-repository output contract. Genuine outside
  and nested new outside directories still prepare successfully.

## Real records versus malformed/future input

All117 history rows, two historical builds, source/generated status, history
locks, actual empty catalog, current AGENTS/GPL and fm1pkg.js bytes are unchanged.
No real secret leak or wrong firmware admission was alleged in F5/F6. F5 cases
use explicit synthetic private sentinels; F6 previously denied admission only
**after** buffering; F7 affects malformed future appends, not existing locked
rows. These corrections do not require new firmware compilation or owner device
PASS merely to offer a fully qualified NIGHTLY test candidate. They do not turn
any currently BUILD_ONLY artifact into an installable package.

## One consolidated final-source run

`consolidated-commands.json` records every command, exit and elapsed time on the
unchanged final behavioral source. No source changes followed this run except
textual evidence. Linux x86_64, Node24.19.0/npm11.9.0, Playwright1.62.0,
Chromium154.0.8037.57. All commands exited0:

- Build and full native npm tests: all seven prior suites plus new actual-source
  write-boundary suite; 58 admission,84 qualification negatives,117 history pins,
 228 registered status-negative cases,41 actual WebMIDI/binding/slot cases.
- Default browser: **215/215 installer assertions +34/34 history cases**. The
  prior208 assertions are preserved, with7 stream/read-boundary assertions.
- Supplied dependency boundary expectations: **8/8**; RED on the pinned old
  source reproduced **7 failed expectations/8**, including actual CLI outputs.
- Supplementary expanded **82/82**, real click/keyboard/history **22/22**,
  supplied history-response **8/8**, adversarial history extension **31/31**.
- Existing scoped backup/archive/readback assertions **4/4**. Its deliberately
  mocked Updater received a labeled no-op bind-interface adapter; actual bind
  is separately exercised with observable WebMIDI Maps/identity frames in the
  new default41 suite. No shipping guard was removed to accommodate an old mock.

Independent fixtures are **producer replays, not independent QA acceptance**.
Dependency fixture assertions are unchanged; only its fixed source path was
bound to this checkout's canonical path so the native CLI entry guard really
executes. Other supplied browser fixture assertions remain unchanged. The
local `/workspace/pr8-independent-qa` alias supplies their fixed served path.
Real protocol methods are used in the41 native cases; only discovery/I/O,
servicing/timing boundaries are deterministic fixtures. No hardware was flashed.

A bounded source helper `/root/f7_port` ported F7 and existing browser safeguards
on assigned files; coordinator inspected its diff/source hashes and included
its tests in the consolidated run. Helper scope is completed, not a background
worker or an account-visible successor. Original cross-family/layout tests
reproduced11 failures on the original code; the expanded pre-binding snapshot
reproduced24/41 failures, then the complete existing guard port passed41/41.

## PR9 concurrency and consumer routing

PR9 `47cc6eaaa43655192c9ee88d9e1217473fe4f2f4`, root
`fd5a9f26fe886cf787e6d82825656b7091a8b8f9`, was reread with its comments and
handoff. It is a separate unmerged combined source proposal, not a reassignment
of PR8 ownership or a new sole merger. Its publisher has an explicit NO-GO
review [6384892539](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/9#issuecomment-6384892539).
No PR9 or Catalog/firmware source branch was edited, overwritten or rebased.
Parent/sole merger must choose/reconcile the actual combined tree; PR9 must not
publish using superseded PR8 QA. If it is selected, consume these exact repair
commits plus F1–F4 and port into its schema rather than blindly overwrite its119
rows or approval work. Any different combined root needs fresh full QA.

## Release capability (read-only check, not missing global GitHub access)

Native Git fetch/owned-branch push and connected GitHub source authorization work.
`gh` is2.46.0, has an environment-provided GH_TOKEN and configured HTTP(S) proxy.
Its auth-status diagnostic calls that token invalid, but its supported account
read actually fails with `GET https://api.github.com/user: Forbidden`. Metadata
cannot establish the token's origin/release scope or prove global GitHub access
is missing. No token value was inspected/printed; no credentials/security settings
were changed. No documented release-create/upload proxy route or corresponding
connector action is exposed here. After explicit user approval of the tiny unpublished draft test, the helper
received that approval and ran the supported release-list preflight:
`gh api --hostname github.com repos/gobbajogger/X0X37-for-Loopy-Pro/releases?per_page=3`
returned Forbidden, exit1. No release write/upload was attempted and no uncertain
write outcome exists. The helper stopped without alternate routes or credential
changes. Parent is checking Work separately for the existing sole publisher's route.
`release-capability.json` records the precise pending capability.

## Next bounded stage

**REPAIR HANDOFF — PRIOR WEBSITE QA INCOMPLETE.** Keep PR8 draft/HOLD MERGE.
Parent resumes the existing independent Website QA owner on the new exact SHA/root
in the PR body, reviews the finite8357 matrix and reruns full native/default
browser plus supplied fixtures. Producer GREEN is not whole-site acceptance.
Only after exact-head independent acceptance may producer readiness and the
existing sole website Main Merger proceed. Parent retains task dispatch; no
competing successor exists. Sole publisher remains responsible for actual
qualified bytes, lawfully hosted assets and served-byte verification. Real
Elite/Pro remain NO INSTALLABLE YET. No protected refs, publication, firmware/
controller/vendor upload, schedule change or physical device action occurred.
