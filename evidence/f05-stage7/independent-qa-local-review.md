# Independent QA for F05 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-ab90f47c-dev`.

**Why this receipt applies:** F05 BF-002 is planner/host-only. Package and controller bytes are **unchanged** from the Stage-4 DEV CHECKED tip locally reviewed on PR #198:

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` |
| Blank Pro (stage-4) | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` |
| Page Elite (bundled) | `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` |
| Page Pro (bundled) | `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Those page digests match the live nightly installer and the prior F04 publish. Stage-3 `CHECKS.md` covers planner/project host tests only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — rebuilt + verified packages):** Project store `internal/pr-local-reviews/pr-198.md` (reviewer `bc-8b474f2f-aeac-5c15-95a8-ac6cc7be2f37`). Copy below.

**Lineage:** Stage-5 evidence local review `internal/pr-local-reviews/pr-199.md` (Overall PASS; hash spot-check agrees with Stage-4 / F04 digests; no package rebuild in that PR).

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #198

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/198 |
| Branch | `cursor/f05-bf002-stage4-fb90` |
| Base (at open) | `cursor/f05-bf002-stage3-fb90` |
| Reviewed SHA | `c4445ca44d442f01ac44b4e8517e88cc6ee0cd94` |
| Package validation commit | `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` |
| Content tree (validation) | `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c` |
| Plan base | `0d79d63485dc6585800a71ef716d095445f5a1fc` |
| Title | Record F05 Stage 4 BF-002 DEV CHECKED packages for Stage 5 |
| Kind | Stage 4 evidence: `evidence/f05-stage4/*` (records, logs, plan, CHECKS/HANDOFF/SUCCESSOR). No production firmware/web/tools source edits in the tip commit. |
| Reviewer | local cloud VM only (no GitHub Actions, no Codex-on-GitHub) |
| Merge | **not merged by this review** (assignment: do not merge). Note: GitHub already shows MERGED at `2026-10-25T11:14:30Z` before this review ran. |
| Overall | **PASS** — Elite and Pro `DEV CHECKED` claims reproduced locally; package/controller SHA-256 match tip records |

## Commands run

```bash
git fetch origin pull/198/head:refs/remotes/origin/pr-198
git checkout -B cursor/pr-198-review-2f37 origin/pr-198
# HEAD == c4445ca44d442f01ac44b4e8517e88cc6ee0cd94

# Host plan checks (BF-002)
python3 -m unittest tests.host.test_quickcheck_plan -v
env X0X_TESTS='projects-web projects-page' bash tests/run_tests.sh

# Record/log integrity (tip evidence)
# elite/pro record.json result, budgets, log digests vs files

# Tooling for package rebuild (was missing on this VM)
bash tools/get_toolchain.sh /home/ubuntu/.jieli
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
  https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git /home/ubuntu/fw-AC79_AIoT_SDK
python3 -m pip install Pillow   # build.sh required Pillow

export JIELI_TOOLCHAIN=/home/ubuntu/.jieli/toolchain
export AC79_SDK=/home/ubuntu/fw-AC79_AIoT_SDK
export CC=cc CXX=c++

git worktree add --detach /workspace/scratch/pr198-review/repo \
  ab90f47ccad0f213114d6a98c1bc6179be2ebb35

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr198-review/repo \
  --commit ab90f47ccad0f213114d6a98c1bc6179be2ebb35 \
  --model elite \
  --plan evidence/f05-stage4/dev-plan.json \
  --output /workspace/scratch/pr198-review/elite-out

python3 tools/dev_validate.py run \
  --repo /workspace/scratch/pr198-review/repo \
  --commit ab90f47ccad0f213114d6a98c1bc6179be2ebb35 \
  --model pro \
  --plan evidence/f05-stage4/dev-plan.json \
  --output /workspace/scratch/pr198-review/pro-out

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr198-review/repo \
  --commit ab90f47ccad0f213114d6a98c1bc6179be2ebb35 \
  --record evidence/f05-stage4/elite/record.json \
  --fwsc /workspace/scratch/pr198-review/elite-out/x0x-smk37-elite.fwsc \
  --ctl /workspace/scratch/pr198-review/elite-out/ctl.img

python3 tools/dev_validate.py verify \
  --repo /workspace/scratch/pr198-review/repo \
  --commit ab90f47ccad0f213114d6a98c1bc6179be2ebb35 \
  --record evidence/f05-stage4/pro/record.json \
  --fwsc /workspace/scratch/pr198-review/pro-out/x0x-smk37.fwsc \
  --ctl /workspace/scratch/pr198-review/pro-out/ctl.img
```

## Check results

| Check | Command / source | Result |
| --- | --- | --- |
| Scope | `git diff --name-only c4445ca4^..c4445ca4` | **PASS** — only `evidence/f05-stage4/*` |
| Ancestry | `ab90f47c` ⊂ HEAD; content tree at `ab90f47c` = `0ee03318…` | **PASS** |
| Tip CHECKS.md claims | Elite/Pro DEV CHECKED hashes/budgets | **PASS** (match records + rebuild) |
| Tip record/log digests | elite+pro `record.json` vs `*.log` SHA-256 | **PASS** (all logs match; budgets match) |
| Planner host unit | `python3 -m unittest tests.host.test_quickcheck_plan -v` | **PASS** (11/11) |
| projects-web / projects-page | `X0X_TESTS=… bash tests/run_tests.sh` | **PASS** |
| Elite `dev_validate.py run` | `X0X_DEV_PADCAL=1` (via runner), controllers on | **PASS** — printed `DEV CHECKED` |
| Pro `dev_validate.py run` | same | **PASS** — printed `DEV CHECKED` |
| Elite package SHA-256 | rebuilt vs tip record | **PASS** — `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` (651396 B, `SMK-37 Elite_900`) |
| Pro package SHA-256 | rebuilt vs tip record | **PASS** — `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` (651392 B, `SMK-37 Pro_900`) |
| Controller SHA-256 (both) | rebuilt vs tip record | **PASS** — `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` (65492 B) |
| Budgets | elite+pro `pool_free` | **PASS** — 8324 (>= 8192); code_free 6676, ram_free 492 |
| `dev_validate.py verify` elite | tip `record.json` + rebuilt `.fwsc`/`ctl.img` | **PASS** — exit 0, `DEV CHECKED` |
| `dev_validate.py verify` pro | same | **PASS** — exit 0, `DEV CHECKED` |
| Real MIDI / device write | — | **NOT_RUN** (policy) |
| GitHub Actions | — | **NOT_RUN** (policy) |
| `@codex` review | — | **NOT_RUN** (policy) |
| Merge by this review | — | **NOT_RUN** (assignment: do not merge) |

## Findings

1. Tip `c4445ca4` is Stage 4 evidence only under `evidence/f05-stage4/` for BF-002 package validation of commit `ab90f47c`.
2. Tip records claim Elite and Pro `DEV CHECKED` with F04-matching package/controller hashes and `pool_free` 8324; tip log digests and budgets are consistent with those records.
3. Fresh local `dev_validate.py run` on this cloud VM (toolchain + AC79 SDK + Pillow installed here; `X0X_DEV_PADCAL=1`) reproduced identical Elite/Pro package and controller SHA-256 values; both `verify` calls against the tip records exited 0 with `DEV CHECKED`.
4. Host BF-002 plan checks (planner unit + projects-web/projects-page) **PASS** on this VM.
5. This review did **not** merge and did **not** post GitHub/Codex review comments. No FAIL from local Stage 4 package validation.


---

# Appendix: PR #199 local review (lineage)

# Local review — PR #199

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/199 |
| Branch | `cursor/f05-bf002-stage5-fb90` |
| Base (PR) | `cursor/f05-bf002-stage4-fb90` @ `8d8b9016d8db6deaa6172138d7dea7365ed37224` |
| Reviewed SHA | `c9d2dcded1294e739fd15d8546d7bf4047fbd4bf` |
| Validated source SHA | `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` (tree `0ee03318…`) |
| Stage-4 evidence tip (ancestral) | `c4445ca44d442f01ac44b4e8517e88cc6ee0cd94` |
| Title | Record F05 Stage 5 BF-002 merge evidence and Stage 7 handoff |
| Kind | Stage 5 evidence only: `evidence/f05-stage5/*`. Nightly FF attempt recorded as REFUSED; stage-6 prompt + stage-7 successor prompt. No firmware / controller / package bytes. |
| Reviewer | local cloud VM only (no GitHub Actions, no Codex-on-GitHub) |
| Merge by this review | **not performed** (assignment: do not merge). GitHub state at review time: already **MERGED** into base by `app/cursor` @ `e36edf3a…` |
| Overall | **PASS** — CHECKS claims verified locally; ACCEPT vs DEV CHECKED correct; nightly FF refusal recorded without force-push; stage-6 prompt present with matching hashes |

## Commands run

```bash
git fetch origin pull/199/head:pr-199-review
git checkout pr-199-review
# HEAD == c9d2dcded1294e739fd15d8546d7bf4047fbd4bf
# == origin/cursor/f05-bf002-stage5-fb90

git fetch origin nightly cursor/f05-bf002-stage4-fb90 cursor/f05-bf002-stage5-fb90
git diff --stat c4445ca4...HEAD
git diff --check c4445ca4...HEAD

# Recompute tools/dev_validate.tree for validated / audited / stage-4 tip / stage-5 tip
# Spot-check package+controller SHA-256 and pool_free vs evidence/f05-stage4/{elite,pro}/record.json
# Spot-check same package+controller digests vs evidence/f04-stage4 records
# Re-count nightly divergence vs ab90f47c; confirm origin/nightly == 08d3164a…
```

## Check results

| Check | Command / source | Result |
| --- | --- | --- |
| Scope | `git diff --name-only c4445ca4...HEAD` | **PASS** — only `evidence/f05-stage5/*` (10 files) |
| Whitespace | `git diff --check c4445ca4...HEAD` | **PASS** |
| Ancestry | `ab90f47c` ⊂ HEAD; stage-4 tip `c4445ca4` ⊂ HEAD | **PASS** |
| Tree gate — validated `ab90f47c` | `dev_validate.tree` → `0ee03318…` vs package records | **PASS** (**ACCEPT**; not a package `DEV CHECKED` claim) |
| Tree gate — audited `0d79d634` | tree `1b067cad…` | **PASS** (AUDITED_REF_ONLY; matches CHECKS) |
| Tree gate — stage-4 tip `c4445ca4` | tree `d35a6439…` | **PASS** (**REFUSED** vs package-record tree; matches CHECKS) |
| Tree gate — stage-5 tip `c9d2dcde` | tree `ca6a6786…` | **PASS** (**REFUSED**; evidence tip ≠ install/FF target) |
| Package labels | stage-4 `elite/pro/record.json` `result` | **PASS** — both **DEV CHECKED** on `ab90f47c` / `0ee03318…` |
| Hash spot-check Elite | pkg `d3b4cac3…5355f2b`, ctl `956e3e21…1ec2d0a`, pool_free 8324 | **PASS** — CHECKS / stage6 / stage-4 elite record / F04 elite record agree on pkg+ctl |
| Hash spot-check Pro | pkg `0b09605f…5e076b9`, ctl `956e3e21…1ec2d0a`, pool_free 8324 | **PASS** — CHECKS / stage6 / stage-4 pro record / F04 pro record agree on pkg+ctl |
| Nightly FF refusal | `nightly-ff-refusal.log` + live `origin/nightly` | **PASS** — push rejected non-fast-forward; `force_push=false`; `origin/nightly` still `08d3164a…` |
| Nightly divergence counts | `git rev-list --count` | **PASS** — 26 on validated not in nightly; 160 on nightly not in validated; merge-base `c1e83ab9…` |
| Stage-6 prompt | `SUCCESSOR-STAGE6.txt` + `stage6-install-prompt.json` | **PASS** — present; validated SHA, trees, Elite/Pro hashes, `REFUSED_non_fast_forward`, `force_push: false` |
| Stage-7 prompt | `SUCCESSOR-STAGE7.txt` + `HANDOFF.md` | **PASS** — present; successor id none (API 401 ×2 recorded); handoff incomplete until CreateAgent |
| ACCEPT vs DEV CHECKED | CHECKS wording | **PASS** — tree ACCEPT is for validated source vs records; DEV CHECKED is stage-4 package result only |
| Full `require_gate_pass` / package bytes | `.fwsc` / `ctl.img` not in Git | **NOT_RUN** (bytes absent; correctly out of scope) |
| Firmware / Elite–Pro package rebuild | — | **NOT_RUN** (no production source change in this PR) |
| Real MIDI / device write | — | **NOT_RUN** (policy) |
| GitHub Actions | — | **NOT_RUN** (policy) |
| `@codex` review | — | **NOT_RUN** (policy) |

## Findings

1. Tip `c9d2dcde` is Stage 5 merge evidence for BF-002: tree gate, nightly FF refusal, stage-6 install prompt, stage-7 successor prompt. Diff vs Stage 4 evidence tip is evidence-only under `evidence/f05-stage5/`.
2. CHECKS correctly separates **ACCEPT** (validated source tree `ab90f47c` / `0ee03318…` matches DEV CHECKED records) from **DEV CHECKED** (Elite/Pro packages at Stage 4). Evidence tips are **REFUSED** for install/FF — recomputed on this VM.
3. Nightly FF attempt to `ab90f47c` is recorded as **REFUSED** non-fast-forward; `origin/nightly` remains `08d3164a…`; refusal log states `force_push=false`. No force-push performed.
4. Stage-6 prompt files are present and hashes match Stage 4 Elite/Pro records (and F04 attested package/controller digests).
5. This review did **not** merge and did **not** ask the owner. At review time GitHub already showed PR #199 MERGED into the Stage 4 base by `app/cursor` — recorded for awareness only; not an action of this review.
6. No FAIL blocks the Stage 5 evidence claims verified here.
