# Independent QA for F06 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-1325f5d3-dev`.

**Why this receipt applies:** F06 BF-003 is web-harness / site-builder only (`web/test_web.mjs`, `web/make_site.py`, comment updates). Package and controller bytes are **unchanged** from the Stage-4 DEV CHECKED tip locally reviewed on PR #210, and match the prior F05/F04 page digests:

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` |
| Blank Pro (stage-4) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` |
| Page Elite (bundled) | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| Page Pro (bundled) | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-003 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — rebuilt + verified packages):** Project store `internal/pr-local-reviews/pr-210.md` (reviewer `bc-d25c3b43-dadc-57d8-bca3-1cbe6d39b937`). Copy of that review body follows (commands + results).

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #210

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/210 |
| Branch | `cursor/f06-bf003-stage4-fb90` |
| Base (PR) | `cursor/f06-bf003-stage3-fb90` @ `3810530065b42c96c584f201ac104446906bda21` |
| Reviewed SHA | `2a0f2ef69dc0e451d0296f1de2795147f8113592` |
| Prior tip (evidence records) | `c1fcebd5890c97f420757ac4fcd6393aa97d31b7` (DEV CHECKED package evidence; still ancestor) |
| Includes | `c1fcebd5` (Stage 4 CHECKS + Elite/Pro records) + `2a0f2ef6` (owner-visible Stage 5 successor on `HANDOFF.md`); ancestry has `1325f5d3` (BF-003 repair) |
| Package validation commit | `1325f5d34340ea877e65ab86f3663f5dea052c0d` |
| Package content tree (non-`.md`) | `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190` |
| Tip content tree (non-`.md`) | `bf18201290f795c8beebdd785c27b0096f1ef0452ce92caac12a385250d6e353` (evidence tip ≠ package tree; documented in CHECKS — F05 pattern) |
| Audited source SHA | `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` / tree `0ee03318…` |
| Title | Record F06 Stage 4 BF-003 DEV CHECKED packages for Stage 5 |
| Kind | Stage 4 validate evidence: `evidence/f06-stage4/*` only vs Stage 3 tip. Packages validated on repair `1325f5d3` / `b02108f7…`; tip adds records + Stage 5 handoff. |
| Reviewer | local cloud VM only (no GitHub Actions, no Codex-on-GitHub) |
| Merge by this review | **not performed** (assignment: do not merge) |
| Overall | **PASS** — Elite/Pro `DEV CHECKED` reproduced locally; package/controller digests match tip records and F05/F04; tip/package tree split accepted |

## Commands run

```bash
git fetch origin cursor/f06-bf003-stage4-fb90 pull/210/head
git reset --hard origin/cursor/f06-bf003-stage4-fb90
# HEAD == 2a0f2ef69dc0e451d0296f1de2795147f8113592

bash tools/agent_env_install.sh
bash tools/get_toolchain.sh /home/ubuntu/.jieli
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
  https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git /home/ubuntu/fw-AC79_AIoT_SDK
export JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain
export AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK
export CC=cc CXX=c++ X0X_DEV_PADCAL=1 CTL_NO_CACHE=1 CTL_NATIVE=1

git worktree add --detach /workspace/scratch/pr210-review/repo \
  1325f5d34340ea877e65ab86f3663f5dea052c0d

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr210-review/repo \
  --commit 1325f5d34340ea877e65ab86f3663f5dea052c0d \
  --model elite \
  --plan evidence/f06-stage4/dev-plan.json \
  --output /workspace/scratch/pr210-review/elite-out

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr210-review/repo \
  --commit 1325f5d34340ea877e65ab86f3663f5dea052c0d \
  --model pro \
  --plan evidence/f06-stage4/dev-plan.json \
  --output /workspace/scratch/pr210-review/pro-out

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr210-review/repo \
  --commit 1325f5d34340ea877e65ab86f3663f5dea052c0d \
  --record evidence/f06-stage4/elite/record.json \
  --fwsc /workspace/scratch/pr210-review/elite-out/x0x-smk37-elite.fwsc \
  --ctl /workspace/scratch/pr210-review/elite-out/ctl.img

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr210-review/repo \
  --commit 1325f5d34340ea877e65ab86f3663f5dea052c0d \
  --record evidence/f06-stage4/pro/record.json \
  --fwsc /workspace/scratch/pr210-review/pro-out/x0x-smk37.fwsc \
  --ctl /workspace/scratch/pr210-review/pro-out/ctl.img

node web/test_web.mjs
bash evidence/f06-stage4/check-make-site-dry.sh
```

## Check results

| Check | Result |
| --- | --- |
| Package validation commit in ancestry | **PASS** — `1325f5d3` ⊂ HEAD |
| Content tree at `1325f5d3` | **PASS** — `b02108f7…` |
| Firmware / `ble/` trees vs audited tip | **PASS** — identical |
| Elite `dev_validate.py run` + `verify` | **PASS** — `DEV CHECKED`; digests match tip + F05/F04 |
| Pro `dev_validate.py run` + `verify` | **PASS** — `DEV CHECKED`; digests match tip + F05/F04 |
| Budgets | **PASS** — code_free 6676, ram_free 492, pool_free 8324 |
| Host BF-003 plan checks | **PASS** |
| GitHub Actions / Codex-on-GitHub | **NOT_RUN** (policy) |

## Findings

1. Package authenticity is repair commit `1325f5d3` / content tree `b02108f7…`.
2. Fresh local rebuild reproduced Elite/Pro package and controller SHA-256 matching tip records and F05/F04 attested bytes.
3. Page publishes the same bundled installable digests (`8ba23a89…` / `d9093211…`) attributed to validated source `1325f5d3…`.
