# Independent QA for F07 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-6df0f81f-dev`.

**Why this receipt applies:** F07 FP-001 is host dual-digest tooling / digests reconciliation only (`tools/check_installer_digests.py`, tests, planner route, docs). Package and controller bytes are **unchanged** from the Stage-4 DEV CHECKED tip locally reviewed on PR #222, and match the prior F06/F05/F04 page digests:

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` |
| Blank Pro (stage-4) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` |
| Page Elite (bundled) | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| Page Pro (bundled) | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required FP-001 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — rebuilt + verified packages):** Project store `internal/pr-local-reviews/pr-222.md` (reviewer `bc-11907614-f894-5a41-9063-ab05c85048df`). Copy of that review body follows (commands + results).

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #222

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/222 |
| Branch | `cursor/f07-fp-pin-stage4-fb90` |
| Base (PR) | `cursor/f07-fp-pin-stage3-fb90` @ `6cedd29123f2507996c5363ac4fe717d66eff0fa` |
| Reviewed SHA | `f1bfb483956ff2c4313cc9e2ad7ede430b192876` |
| Prior tip (superseded) | `53ff69d8cde7325eac23776d4206a15df586cd56` (Stage-5 create skipped / no API key) |
| Evidence commit | `6943f51e` (DEV CHECKED package evidence; still ancestor) |
| Includes | `6943f51e` (Stage 4 CHECKS + Elite/Pro records) + `53ff69d8` (API skip) + `f1bfb483` (owner-visible Stage 5 successor on `HANDOFF.md`); ancestry has `6df0f81f` (FP-001 repair) |
| Package validation commit | `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` |
| Package content tree (non-`.md`) | `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad` |
| Tip content tree (non-`.md`) | `a7416da07a91e04c689ad70a2a3a298d76a4b703cb42704b57315230a1be9aa2` (evidence tip ≠ package tree; F05/F06 pattern) |
| Audited source SHA | `1325f5d34340ea877e65ab86f3663f5dea052c0d` / content tree `b02108f7…` / git tree `6b6401f5…` |
| Title | F07 Stage 4: DEV CHECKED packages for FP-001 dual-digest candidate |
| Kind | Stage 4 validate evidence: `evidence/f07-stage4/*` only vs Stage 3 tip. Packages validated on FP-001 repair `6df0f81f` / `69ccb3d7…`; tip adds records + Stage 5 handoff. |
| Repair under test | **FP-001** — dual-digest (`installer_source_fingerprint` `8caf8724…` ≠ `served_install_html_sha256` `66b56ba5…`); firmware/package bytes unchanged vs F06 |
| Reviewer | local cloud VM only (no GitHub Actions, no Codex-on-GitHub) |
| Merge by this review | **not performed** (assignment: review only) |
| Overall | **PASS** — tip `f1bfb483`; Elite/Pro `DEV CHECKED` reproduced locally; package/controller digests match tip records and F06/F05/F04; Stage 5 handoff **complete** (`bc-6fe39901…`) |

## Commands run

```bash
git fetch origin cursor/f07-fp-pin-stage4-fb90 pull/222/head
git checkout -B cursor/f07-fp-pin-stage4-fb90 origin/cursor/f07-fp-pin-stage4-fb90
# HEAD == f1bfb483956ff2c4313cc9e2ad7ede430b192876
# == gh pr view 222 headRefOid

git diff --name-only origin/cursor/f07-fp-pin-stage3-fb90...HEAD
git diff --check origin/cursor/f07-fp-pin-stage3-fb90...HEAD
git diff --name-only 6943f51e..HEAD   # HANDOFF.md + api-create-stage5-attempt*.json
git diff --name-only 53ff69d8..HEAD   # only evidence/f07-stage4/HANDOFF.md

bash tools/agent_env_install.sh
bash tools/get_toolchain.sh /home/ubuntu/.jieli
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
  https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git /home/ubuntu/fw-AC79_AIoT_SDK
export JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain
export AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK
export CC=cc CXX=c++ X0X_DEV_PADCAL=1 CTL_NO_CACHE=1 CTL_NATIVE=1

git worktree add --detach /workspace/scratch/pr222-review/repo \
  6df0f81fbbe0adc3207f67d912d2fe0ab676be04

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr222-review/repo \
  --commit 6df0f81fbbe0adc3207f67d912d2fe0ab676be04 \
  --model elite \
  --plan evidence/f07-stage4/dev-plan.json \
  --output /workspace/scratch/pr222-review/elite-out

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr222-review/repo \
  --commit 6df0f81fbbe0adc3207f67d912d2fe0ab676be04 \
  --model pro \
  --plan evidence/f07-stage4/dev-plan.json \
  --output /workspace/scratch/pr222-review/pro-out

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr222-review/repo \
  --commit 6df0f81fbbe0adc3207f67d912d2fe0ab676be04 \
  --record evidence/f07-stage4/elite/record.json \
  --fwsc /workspace/scratch/pr222-review/elite-out/x0x-smk37-elite.fwsc \
  --ctl /workspace/scratch/pr222-review/elite-out/ctl.img

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr222-review/repo \
  --commit 6df0f81fbbe0adc3207f67d912d2fe0ab676be04 \
  --record evidence/f07-stage4/pro/record.json \
  --fwsc /workspace/scratch/pr222-review/pro-out/x0x-smk37.fwsc \
  --ctl /workspace/scratch/pr222-review/pro-out/ctl.img

python3 tests/host/test_check_installer_digests.py -v
python3 tools/check_installer_digests.py evidence/f07-stage2/digests.json \
  --fetch --expect-source 8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157
bash evidence/f07-stage4/check-conflation-alone.sh
python3 tests/host/test_quickcheck_plan.py -v
X0X_TEST_JOBS=1 X0X_TESTS='installer-digests quickcheck-plan' bash tests/run_tests.sh
node web/test_web.mjs
bash evidence/f06-stage4/check-make-site-dry.sh
# absences: web/editor.html, firmware/src/editor.c
```

## Check results

| Check | Command / source | Result |
| --- | --- | --- |
| Tip sync (final) | fetch + `gh pr view 222 headRefOid` | **PASS** — `f1bfb483…` |
| Tip move from prior | `53ff69d8` → `f1bfb483` | **PASS** (recorded) — only `evidence/f07-stage4/HANDOFF.md` |
| Tip move from evidence | `6943f51e` → `f1bfb483` | **PASS** (recorded) — HANDOFF + api-create skip receipts; Elite/Pro records/CHECKS unchanged |
| Scope (three-dot vs Stage 3 tip) | `git diff --name-only origin/cursor/f07-fp-pin-stage3-fb90…HEAD` | **PASS** — only `evidence/f07-stage4/*` (21 paths); no firmware / `web/` / tools production bytes |
| Whitespace | `git diff --check …` | **PASS** |
| Merge-base vs Stage 3 tip `6cedd291` | ancestry | **PASS** (recorded) — **not** ancestor; merge-base is `75acd6a1`; Stage 3 move after merge-base was HANDOFF-only |
| Package validation commit in ancestry | `6df0f81f` ⊂ HEAD | **PASS** |
| Audited tip in ancestry | `1325f5d3` ⊂ HEAD | **PASS** |
| Content tree at `6df0f81f` | `dev_validate.tree` | **PASS** — `69ccb3d7…` (matches CHECKS + records) |
| Content tree at tip `f1bfb483` | same | **PASS** (recorded) — `a7416da0…` ≠ package tree; evidence tip after DEV CHECKED; not a FAIL |
| Content tree at audited `1325f5d3` | same | **PASS** — `b02108f7…` |
| Firmware / `ble/` trees | tip vs `6df0f81f` vs `1325f5d3` | **PASS** — identical (`19840d87…` / `d0479e10…`) |
| Product paths vs `1325f5d3` | firmware + packaging scripts | **PASS** — empty |
| Tip CHECKS.md claims | Elite/Pro hashes, budgets, pool ≥ 8192 | **PASS** (match records + rebuild) |
| Tip record/log digests | elite+pro `record.json` vs `*.log` SHA-256 | **PASS** (all tip logs match tip claims) |
| Package SHA vs F06/F05/F04 | elite `d3b4cac3…` / pro `0b09605f…` / ctl `956e3e21…` | **PASS** — identical |
| Elite `dev_validate.py run` | `X0X_DEV_PADCAL=1`, controllers on, commit `6df0f81f` | **PASS** — `DEV CHECKED`; pkg/ctl SHA match tip |
| Pro `dev_validate.py run` | same | **PASS** — `DEV CHECKED`; pkg/ctl SHA match tip |
| Budgets (rebuild) | elite+pro | **PASS** — code_free 6676, ram_free 492, pool_free 8324 |
| `dev_validate.py verify` elite | tip `record.json` + rebuilt `.fwsc`/`ctl.img` | **PASS** — exit 0, `DEV CHECKED` |
| `dev_validate.py verify` pro | tip `record.json` + rebuilt `.fwsc`/`ctl.img` | **PASS** — exit 0, `DEV CHECKED` |
| `test_check_installer_digests.py` | 14 tests on tip | **PASS** |
| Live `--fetch` digests | `tools/check_installer_digests.py … --fetch` | **PASS** — fetched HTML `66b56ba5…` == served; source `8caf8724…` ≠ served |
| Conflation reject | `check-conflation-alone.sh` | **PASS** — wrapper exit 0; underlying tool exit 1 as required |
| `test_quickcheck_plan.py` | 11 tests on tip | **PASS** |
| `installer-digests` harness | `X0X_TESTS=installer-digests …` | **PASS** |
| `quickcheck-plan` harness | `X0X_TESTS=quickcheck-plan …` | **PASS** |
| `node web/test_web.mjs` | tip | **PASS** — exit 0; no editor.html open |
| make_site dry | `evidence/f06-stage4/check-make-site-dry.sh` | **PASS** — exit 0; installer only; no editor copy |
| Absences | `web/editor.html` / `firmware/src/editor.c` | **PASS** — absent |
| Stage 5 handoff | tip `HANDOFF.md` @ `f1bfb483` | **PASS** — successor `bc-6fe39901-28af-5ef9-8590-1ba47e4031bd`; URL `https://cursor.com/agents/bc-6fe39901-28af-5ef9-8590-1ba47e4031bd`; status **complete** |
| API skip receipt | `api-create-stage5-attempt*.json` | **PASS** (recorded) — documents pod skip; superseded by coordinator CreateAgent on tip HANDOFF |
| BF-001 / BF-002 / BF-003 | ancestry + DEFERRED | **PASS** — not reopened |
| Firmware / package bytes in PR tip diff | three-dot | **PASS** — none (hashes/logs only) |
| Full planner target suite (encoder/fastmath/…) | — | **NOT_RUN** — FP-001 scoped selectors + package rebuild cover the repair; no firmware touch |
| Full `tests/run_tests.sh` (unscoped) | — | **NOT_RUN** (scoped stage/host selectors run instead) |
| Real MIDI / device write | — | **NOT_RUN** (policy) |
| GitHub Actions | — | **NOT_RUN** (policy) |
| `@codex` / Codex-on-GitHub | — | **NOT_RUN** (policy; ignored; not the review gate) |
| Merge by this review | — | **NOT_RUN** (assignment: review only) |

## Findings

1. Final tip `f1bfb483` keeps Stage 4 DEV CHECKED evidence at `6943f51e` and records owner-visible Stage 5 successor `bc-6fe39901-28af-5ef9-8590-1ba47e4031bd` (handoff complete). Diff vs Stage 3 tip is only `evidence/f07-stage4/*`; move from `53ff69d8` only updates HANDOFF.
2. Package authenticity is FP-001 repair commit `6df0f81f` / content tree `69ccb3d7…` as CHECKS documents. Tip content tree `a7416da0…` differs because evidence artifacts landed after validation — same F05/F06 evidence-tip ≠ package-tree pattern; not a FAIL. Firmware/`ble/` trees match audited `1325f5d3`.
3. Fresh local `dev_validate.py run` + `verify` on this VM (`X0X_DEV_PADCAL=1`, toolchain + AC79 SDK + Pillow) reproduced Elite/Pro package and controller SHA-256 matching tip records and F06/F05/F04 attested bytes; `pool_free` 8324. Plan host checks (digests unit, live `--fetch`, conflation reject, quickcheck planner) all **PASS**.
4. This review did **not** merge and did **not** use `@codex` or GitHub Actions. No FAIL on suites that ran locally.
