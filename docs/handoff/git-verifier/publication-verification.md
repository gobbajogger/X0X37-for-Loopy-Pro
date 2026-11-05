# Git Verifier — NIGHTLY publication verification checklist (DnBOX)

Prepared 2026-11-05 (Asia/Vladivostok, UTC+10). Updated same day with coordinator decisions (P5.5 ADOPTED; verifier runs independent P5.5; publish timing gate; manifest path; cache window; live `versions.json` keys; version ledger; signing informational). Local box file only — nothing pushed or published. `.orig` at `publication-verification.md.orig`.

**Role boundary.** The Git Verifier only VERIFIES, read-only (anonymous `git fetch`/`git ls-remote`, HTTPS GET, `sha256sum`). The verifier **never** pushes, merges, writes `gh-pages`, edits a catalog, opens a PR, or publishes. The coordinator (via the sole `website_publisher` role, NPP §1 item 6, §4, §6 Step 6) publishes only after **every** pre-publish gate below is PASS. **P5.5 post-publish install check is ADOPTED** and is run by the **Git Verifier as the independent read-only readback** (the publisher does **not** self-verify for gate credit). Any FAIL = publish blocked / post-publish incident; report on #832.

Sources: `/workspace/dnbox/nightly-publication-path.md` (NPP), `publication-blockers.md`, `release-plan.md`, `nightly-prep-490e656c-20261105.md`, `workflow-audit-20261105.md` (P0-6 → stage P5.5), `master-plan.md` (P5.1–P5.4, C0.6), `verify-branches.sh`, live Pages + `gobbajogger/x0x37-for-Loopy-Pro_nightly` (`main` / `gh-pages`), `scripts/render-qualified-nightly.mjs`, `docs/qualified-nightly-candidate.md`. Live evidence scratch: `/workspace/git-verifier/live/`.

Repos: firmware `gobbajogger/X0X37-for-Loopy-Pro` (tracking #832, integrator PR #698 on `wip/one-build`); installer/catalog `gobbajogger/x0x37-for-Loopy-Pro_nightly`.

## 0. Timing rule — same SHA as QA, verified in parallel with QA
- The verifier checks the **same commit SHA that QA reviews**, **at the same time as QA** — do not wait for QA's verdict/push.
- Current M7a combined SHA to track: **`490e656c01b21c52b5116b109bde8e94be856016`**
  (branch `lane/dev-m7a-option-a-combined-v3-20261105`, tree `7cbc23087ca5109159d0c8cce7a216e4fcb63cdd`, parent `c117ed1f0802a51988b0b468d9479ae1a231dd2b`; tip message starts "UNREVIEWED:" — per nightly-prep).
- If QA's verified SHA ≠ the SHA proposed for publish → FAIL, stop.
- **Must not publish before:** all gates PASS **and** Dev's carry-forward verdict about the 10 commits from `8be16486` is clean. An early "QA in progress" publish of `490e656c` is **NOT allowed** now; `490e656c` is a **0.0.25** line commit; the live **0.0.24.5** entry (`nightly-8be16486-dev`) **stays as published**. HOLDs are lifted for **preparation only**; live publish only when gates PASS (see **G16**).

## 1. Pre-publish gates (read-only)

Setup (anonymous, throwaway dir):
```bash
REPO_FW=gobbajogger/X0X37-for-Loopy-Pro
REPO_NL=gobbajogger/x0x37-for-Loopy-Pro_nightly
SHA=490e656c01b21c52b5116b109bde8e94be856016   # = QA-verified SHA
SHA8=${SHA:0:8}                                 # → catalog id nightly-${SHA8}-dev
d=$(mktemp -d); cd "$d"; git init -q .
git fetch -q "https://github.com/$REPO_FW.git" "<branch-or-sha>"
```

| # | Gate | Check (command pattern) | PASS criterion |
|---|---|---|---|
| G1 | Source SHA = QA-verified SHA | compare `$SHA` to SHA quoted in QA's #832 / #698 acceptance comment and in the integrator freeze (NPP §6 Step 0) | identical 40-hex |
| G2 | Tree | `git rev-parse $SHA^{tree}` | = `7cbc2308…` (or the tree QA quoted) |
| G3 | Parent / lineage | `git rev-parse $SHA^`; `git log --format=%H <base>..$SHA` | parent as reported; linear on stated base (c32c54ac RC1 for M7a) |
| G4 | Identity | `git log --format='%an <%ae>\|%cn <%ce>' <base>..$SHA \| sort -u` | only `gobbajogger <255244738+gobbajogger@users.noreply.github.com>` for author AND committer |
| G5 | No trailers | `git log --format=%B <base>..$SHA \| grep -i 'co-authored-by'` | no match |
| G6 | No force-push | previously recorded tips are ancestors: `git merge-base --is-ancestor <old_tip> <new_tip>` for every re-pushed ref; `artifacts/nightly-dev` old tip (`f857ffbb…`) ancestor of new tip; `gh-pages` old tip (`7c7baecb…`) ancestor of new tip (non-forced FF, NPP §6 Step 6) | exit 0 for all |
| G7 | Mains untouched | `git ls-remote https://github.com/$REPO_FW.git refs/heads/main` and same for `$REPO_NL` | fw main `1a6bc3fe…`, nightly main `a8205aff…` (or the value recorded at last verification; changes only via `website_main_merger` for nightly main, NPP §6 Step 5) |
| G8 | No PR opened by lanes | read-only PR list check | no new PR from lane branches unless role-authorized (catalog_sync PR is expected per NPP Step 4) |
| G9 | No forbidden content | `git ls-tree -r --name-only $SHA` / diff of new commits | no `.fwsc`, `ctl.img`, SDK, credentials in nightly repo or source lanes (NPP §1); `artifacts/nightly-dev` adds ONLY new files under `artifacts/nightly/dev-<YYYYMMDD>-<tag>/<sha256>/` (NPP §5A, Step 2), old files unchanged (`git diff --diff-filter=MD <old_tip> <new_tip>` empty) |
| G10 | Artifacts sha256 vs Build manifest | fetch hosting commit; `git show <host_commit>:<path> \| sha256sum`; `\| wc -c`. **Expected package hashes** come from Build's package manifest `docs/handoff/build/manifest-<sha8>.json` (hashes only) on a **new non-main** branch, linked on #832. After publish, cross-check the same digests against live `versions.json` entry `models.*.sha256` / URL path digests. | sha256 = digest in path dir name = manifest = (post-publish) catalog `models.<m>.sha256`; size = catalog `size` |
| G11 | Controller | catalog `controller_sha256`/`controller_size` vs native_build receipt | match (expected ctl `a334208b…c1d984`, 61552 B if unchanged — NPP §7) |
| G12 | Catalog entry shape | read catalog PR diff against **live** schema (verified 2026-11-05) | schema 1; top-level keys `schema`,`repo`,`pages`,`latest`,`history`,`versions`; entry keys include `version`,`label`,`status`,`config`,`hardware_tested`,`source_commit`,`source_tree`,`models`,`title`,`description`,`evidence_url` (+ `display_version` when assigned); `status: dev-checked`, `config: dev`, `hardware_tested: false`, label DEVICE NOT TESTED, full 40-hex `source_commit`=`$SHA`, `source_tree`, `product` SMK-37 Elite_900 / Pro_900, digest-addressed immutable `pkg` URL, `controller_strategy: bundled` for new entries; all prior entries unchanged (append-only); `latest` → new `nightly-${SHA8}-dev` |
| G13 | Gates/receipts present | qualification & QA comments | every requiredGate PASS with immutable `evidence_url`: package-integrity, model-controller, memory-flash, host-regressions, backup-restore-readback, interruption-recovery, ota-target-binding, independent-qa; native-build PASS if present (NPP §5B) |
| G14 | HOLDs (prep vs publish) | publication-blockers.md / #698 / #816 / #822 / coordinator | HOLDs may be lifted for **preparation only**. Live publish still requires G16 + all other gates PASS. Existing `nightly-8be16486-dev` (0.0.24.5) must remain unchanged. |
| G15 | Publisher pre-checks evidence | publisher's output | `npm run verify:pages -- --candidate /ABS/OUT --retained-catalog …` PASS recorded (verifier reads the record; does not run publish) |
| G16 | **No early 490e656c publish; carry-forward clean** | coordinator + Dev carry-forward note on #832 | **FAIL** if anyone proposes/executes an early "QA in progress" publish of `490e656c`. `490e656c` is 0.0.25-line; 24.5 stays. Live publish only when **all** gates PASS **and** Dev's carry-forward verdict on the **10 commits from `8be16486`** is clean. |
| G17 | Version ledger consistency | Build proposes `display_version` (expected **0.0.25** line) from source; catalog id stays `nightly-${SHA8}-dev` | see §1.1 commands; catalog `version` / `display_version` / package `--firmware-version` / ledger allocation agree per rules below |

**Commit signing (informational only — not a gate).** Report `gpgsig` / verification status (SSH signatures often show as `unknown_key`). **Do not FAIL** on missing, unknown, or unverified signatures. New pushes should still be unsigned per the push template when that is the lane policy; verifier only reports.

Helper for lane tips: `/workspace/dnbox/verify-branches.sh <repo> <branch> <expected_parent> <expected_tree>`.

### 1.1 Version ledger consistency checks (G17)

Existing catalog scheme (live): **`nightly-<sha8>-dev`** (example: `nightly-8be16486-dev`). Build proposes the **0.0.25** display version from source/ledger; verifier checks consistency — never invents hashes or versions.

```bash
SITE="https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly"
VER="nightly-${SHA8}-dev"
# After catalog PR exists (pre-publish) or after Pages update (post-publish):
curl -fsSL -H 'Cache-Control: no-cache' "$SITE/versions.json?nocache=$(date +%s)" -o versions.json
# Pre-publish: use the catalog PR file instead of live if the entry is not live yet.
DISPLAY=$(jq -r --arg v "$VER" '.versions[] | select(.version==$v) | .display_version' versions.json)
# Expect Build-proposed display (0.0.25 line). Live 24.5 reference:
#   nightly-8be16486-dev → display_version "0.0.24.5" (catalog-only label; embedded firmware may report 0.0.24).
test "$DISPLAY" = "<BUILD_PROPOSED_DISPLAY_VERSION>"   # e.g. 0.0.25 — from Build, not invented here

# Package machine identity (--firmware-version) must be canonical X.Y.Z allocated in
# releases/openbeta/ledger.json (default ledger path in tools/package_nightly.py).
# From firmware checkout + Build's composed packages / --check receipts:
#   tools/package_nightly.py <elite.fwsc> --check --model elite \
#     --controller-sha256 <ctl> --controller-size <sz> --firmware-version <BUILD_PROPOSED>
# Receipt JSON fields: firmware_version, release_display (see package_nightly.inspect).
# Compare:
#   receipt.firmware_version  == Build proposed / ledger allocation
#   catalog.display_version   == Build proposed display (may equal firmware_version, or be a
#                               catalog label such as 0.0.24.5 vs embedded 0.0.24 — document which)
#   catalog.version           == nightly-${SHA8}-dev
#   catalog.source_commit     == $SHA
jq -r --arg v "$VER" --arg sha "$SHA" \
  '.versions[] | select(.version==$v) | "\(.version) display=\(.display_version) src=\(.source_commit)"' \
  versions.json
```

## 2. POST-PUBLISH install check — stage **P5.5** (ADOPTED)

**Who runs it:** Git Verifier (independent read-only readback). Publisher/coordinator publishes only after pre-publish gates PASS; publisher does **not** self-verify for P5.5 gate credit. Spec anchors: NPP §1 item 6 + §6 Step 6 (workflow-audit P0-6 called this "NPP §1.6").

**Cache window:** wait up to **10 minutes** for Pages/CDN cache; **poll every ~60s** before calling FAIL on missing/stale bytes.

Run only after the coordinator/publisher reports the `gh-pages` fast-forward. **Live URL bases** (from live `versions.json` top-level `pages`, verified 2026-11-05):

```bash
SITE="https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly"   # trailing slash in catalog.pages; either form OK
VER="nightly-${SHA8}-dev"   # e.g. nightly-490e656c-dev when that entry is published
MANIFEST_REF="<build-non-main-branch>"   # docs/handoff/build/manifest-${SHA8}.json linked on #832
poll_until() { # usage: poll_until <max_sec> <cmd...>
  local deadline=$((SECONDS + ${1:-600})); shift
  until "$@"; do
    (( SECONDS >= deadline )) && return 1
    sleep 60
  done
}

# 1) Catalog readback over HTTPS (no-cache); allow up to 10 min before FAIL
poll_until 600 curl -fsSL -H 'Cache-Control: no-cache' \
  "$SITE/versions.json?nocache=$(date +%s)" -o versions.json
sha256sum versions.json
jq -r '.schema, .repo, .pages, .latest' versions.json
# Live top-level (verified): schema=1,
#   repo=https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly
#   pages=https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/
#   latest=nightly-8be16486-dev (until a newer entry is published), history=[], versions[28]
jq -r '.latest' versions.json                                   # expect $VER after publish
jq '.versions | length' versions.json                           # expect previous count + 1 (28 → 29)
jq --arg v "$VER" '.versions[] | select(.version==$v)' versions.json > entry.json
jq -r '.source_commit, .source_tree, .display_version // empty' entry.json
# expect $SHA, tree QA quoted, display_version = Build proposal

# 2) Expected package hashes from Build manifest (hashes only); then live packages
git fetch -q "https://github.com/$REPO_FW.git" "$MANIFEST_REF"
git show "FETCH_HEAD:docs/handoff/build/manifest-${SHA8}.json" > manifest.json
# Cross-check each model against manifest + catalog + URL path digest + downloaded bytes:
for m in elite pro; do
  url=$(jq -r ".models.$m.pkg" entry.json)
  # Live URL pattern (verified on nightly-8be16486-dev):
  #   https://raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/<40-hex-host-commit>/artifacts/nightly/dev-<YYYYMMDD>-<sha8>/<sha256>/x0x37-$m-dev.fwsc
  exp=$(jq -r ".models.$m.sha256" entry.json); sz=$(jq -r ".models.$m.size" entry.json)
  man=$(jq -r --arg m "$m" '.[$m].sha256 // .models[$m].sha256 // .[$m] // empty' manifest.json)  # adapt to Build's actual JSON shape when file lands
  curl -fsS --max-redirs 0 "$url" -o "$m.fwsc"
  got=$(sha256sum "$m.fwsc" | cut -d' ' -f1); gsz=$(stat -c %s "$m.fwsc")
  [[ $got == $exp && $got == $man && $gsz == $sz && $url == *"/$exp/"* ]] \
    && echo "PASS $m" || echo "FAIL $m got=$got size=$gsz man=$man"
done

# 3) Installer page + installer_sha256 (see §2.1 — dual-digest; do not conflate)
curl -fsSL "$SITE/" -o index.html;                 sha256sum index.html
curl -fsSL "$SITE/install/" -o install.html;       sha256sum install.html
curl -fsSL "$SITE/install/index.html" -o install-index.html; sha256sum install-index.html
# Live (2026-11-05): /install/ and /install/index.html are the same 596990 B;
#   sha256 226360250a147463736281d06b545012f1216c351e62b7e1e1d9135e475dab05
#   (= served_install_html_sha256; NOT qualification.installer_sha256)
curl -fsSL "$SITE/build-status.json" -o build-status.json
curl -fsSL "$SITE/package-version-assignments.json" -o pva.json
# Source composite fingerprint — recompute from the nightly checkout used for prepare:candidate:
#   (cd <nightly-src> && node -e "import { installerFingerprint } from './scripts/render-qualified-nightly.mjs'; console.log(installerFingerprint());")
# Compare to qualification.installer_sha256 from the publisher's private --qualification input
# (not published as qualification.json). See §2.1 for exact bytes/commands and live recompute.

# 4) Old entries kept; gh-pages FF
jq -r '.versions[].version' versions.json | grep -x 'nightly-8be16486-dev'
# 24.5 entry must still show display_version 0.0.24.5 and unchanged package digests
jq -r '.versions[] | select(.version=="nightly-8be16486-dev") | .display_version' versions.json
# expect 0.0.24.5
git ls-remote https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git refs/heads/gh-pages
# new tip; check FF from 7c7baecb… (recorded pre-publish tip)
```

**PASS** only if all: HTTP 200 on every URL (after ≤10 min / 60s polls); `latest` = new entry; entry `source_commit` = QA-verified SHA; each package sha256 & size = catalog = **Build manifest** = digest in URL path; `pkg` URL pinned to a 40-hex commit (not a branch); `qualification.installer_sha256` = recomputed `installerFingerprint()` from the prepare:candidate source tree; served `install/index.html` sha256 recorded (dual-digest; not required to equal `installer_sha256`); old entries (incl. `nightly-8be16486-dev` / 0.0.24.5) still present and unchanged; `gh-pages` advanced by fast-forward; label still says DEVICE NOT TESTED. **FAIL** on any mismatch, 404, redirect, branch-relative URL, or missing old entry → report immediately; recommend coordinator rollback decision (verifier does not roll back). Do not declare FAIL for CDN staleness until the 10-minute poll window elapses.

### 2.1 `installer_sha256` — exact method (derived live)

**Definition (nightly repo):** `qualification.installer_sha256` must equal `installerFingerprint()` in `scripts/render-qualified-nightly.mjs` (also described in `docs/qualified-nightly-candidate.md`). It is a **source composite fingerprint**, **not** `sha256(install/index.html)`.

**Exact bytes / algorithm:**
1. Read raw bytes of each of these paths under the **nightly repository root** used for `npm run prepare:candidate` (order matters — this is the concatenated list from `inputs` plus the extras in `installerFingerprint()`):
   - `web/x0x_installer.html`
   - `web/fm1pkg.js`
   - `web/fm1ota.js`
   - `web/x0x_breaks.js`
   - `web/x0x_breaks.html`
   - `web/x0x_projects.html`
   - `web/x0x_projects.js`
   - `scripts/build-preview.mjs`
   - `scripts/render-qualified-nightly.mjs`
   - `scripts/legacy-controller-packages.json`
   - `package-version-assignments.json`
2. For each path `p`, compute `hex64_p = sha256(file_bytes(p))` (Node `createHash('sha256').update(buffer).digest('hex')`).
3. Build `payload = JSON.stringify([[p, hex64_p], ...])` for that full list (default `JSON.stringify`; no extra spaces).
4. `installer_sha256 = sha256(utf8_bytes(payload))`.

**Exact commands:**
```bash
# From the nightly checkout that prepare:candidate used (today: main tip a8205aff…):
cd /path/to/x0x37-for-Loopy-Pro_nightly
node -e "import { installerFingerprint } from './scripts/render-qualified-nightly.mjs'; console.log(installerFingerprint());"
# Optional dual-digest of served HTML (informational; must NOT be stuffed into installer_sha256):
curl -fsSL "$SITE/install/index.html" | sha256sum
```

**Recompute vs live (2026-11-05, Asia/Vladivostok UTC+10):**
| Digest | Value | Role |
|---|---|---|
| `installerFingerprint()` at nightly `main` `a8205aff483dce09414093defdc19cf79d57b719` | `235626957e92b70cac29b80b25f6ff859d25d670e9c6dc13376ad531ddc9f0bd` | What `validateCatalog` requires for `qualification.installer_sha256` when preparing from that tree |
| Live served `install/index.html` (gh-pages `7c7baecb…`) | `226360250a147463736281d06b545012f1216c351e62b7e1e1d9135e475dab05` | `served_install_html_sha256` only |
| Live catalog entry `nightly-8be16486-dev` | **no `installer_sha256` field** | Qualification object is private (NPP); not published |

**Reproduce against live entry:** package digests for `nightly-8be16486-dev` **do** reproduce (elite live download sha256 = catalog `models.elite.sha256` = `d0080061064a66b9d203dd9e75956cd59ab9372c2f4fd15769fa9b06b5f2073f`). The historical `qualification.installer_sha256` bound when that entry was published is **not** in public Pages/git and **cannot** be compared without the publisher's private `--qualification` JSON → marked **OPEN** in §4 (single file that would answer it).

Evidence note: `/workspace/git-verifier/live/installer-fingerprint-method.txt`.

## 3. What to report on #832 (drafted by verifier, posted by coordinator's authorized path)
```
Git Verifier — <PRE-PUBLISH|POST-PUBLISH P5.5> — <PASS|FAIL>  (<time> UTC+10)
SHA <40-hex> = QA-verified SHA (QA comment <id>); tree <40-hex>; parent <…>
Identity gobbajogger noreply xN; trailers 0; force-push none; mains fw <sha8>/nightly <sha8> unchanged
Signing: <gpgsig present|absent>; verification <unknown_key|…> — INFORMATIONAL only (not a gate)
Artifacts host commit <40-hex>: elite <sha256> <size> / pro <sha256> <size> = Build manifest-<sha8>.json
Version: catalog nightly-<sha8>-dev display=<…> consistent with Build proposal / package --check
G16 carry-forward (10 commits from 8be16486): <clean|FAIL>; early QA-in-progress publish: forbidden
[post P5.5] versions.json sha256 <…>, latest=<ver>, entries <n>; elite/pro served sha256 match manifest+catalog;
  installerFingerprint <…> = qualification.installer_sha256; served install/index.html sha256 <…> (dual-digest);
  gh-pages <old8>→<new8> FF; cache polls <n>/≤10min
FAILs: <list or none>.  Read-only; nothing pushed/published by verifier.
```

## 4. Open questions — status after coordinator decisions

1. **RESOLVED.** P5.5 is **ADOPTED**. Spec = NPP §1 item 6 + §6 Step 6 (workflow-audit P0-6). Git Verifier runs P5.5 as independent read-only readback.
2. **RESOLVED.** Git Verifier runs P5.5; publisher does **not** self-verify for gate credit. Publisher/coordinator publishes only after all gates PASS.
3. **OPEN (partial).** Method and commands for `installer_sha256` are derived (§2.1): source composite `installerFingerprint()` over 11 nightly-repo paths — **not** served HTML. Recomputed at main `a8205aff` → `23562695…`; live HTML → `22636025…`. **Still OPEN:** the historical `qualification.installer_sha256` value bound for live entry `nightly-8be16486-dev` (field absent from public `versions.json`). **Single file that would answer it:** the publisher's private `--qualification` JSON input from the `prepare:candidate` run that produced gh-pages commits `6427d068` / `7c7baecb` (0.0.24.5). No public path found under the nightly repo or Pages tree.
4. **RESOLVED.** Authoritative package hashes for the candidate: Build pushes `docs/handoff/build/manifest-<sha8>.json` (hashes only) on a **new non-main** branch and links it on #832. Verifier takes expected package hashes from that manifest and cross-checks live `versions.json` after publish. (The absolute URL/host commit for composed `.fwsc` still appears only after artifacts host publish — use manifest + catalog together.)
5. **RESOLVED.** Wait up to **10 minutes**; poll about every **60s** before declaring FAIL for Pages/CDN staleness.
6. **RESOLVED.** Live `versions.json` keys verified by curl 2026-11-05: top-level `schema` (1), `repo`, `pages`, `latest`, `history`, `versions` (array, newest first, length 28). Entry keys: `version`, `label`, `status`, `config`, `hardware_tested`, `source_commit`, `source_tree`, `models` (`elite`/`pro` with `product`,`sha256`,`size`,`controller_sha256`,`pkg`, and for recent entries `controller_strategy`,`controller_size`), `title`, `description`, `evidence_url`; `display_version` present on recent entries (e.g. `nightly-8be16486-dev` → `0.0.24.5`). URL patterns: `$SITE/versions.json`, `$SITE/install/`, `$SITE/install/index.html`, `$SITE/build-status.json`, `$SITE/package-version-assignments.json`; pkg → `raw.githubusercontent.com/gobbajogger/X0X37-for-Loopy-Pro/<40-hex>/artifacts/nightly/dev-<YYYYMMDD>-<sha8>/<sha256>/x0x37-{elite\|pro}-dev.fwsc`.
7. **RESOLVED.** Early "QA in progress" publish of `490e656c` is **NOT allowed**. `490e656c` is 0.0.25-line; 24.5 entry stays. HOLDs lifted for preparation only. Live publish only when all gates PASS and Dev's carry-forward verdict on the 10 commits from `8be16486` is clean. Encoded as **G16**.
8. **RESOLVED (scheme + check).** Ledger/catalog id scheme remains `nightly-<sha8>-dev`. Build proposes 0.0.25 display version from source; verifier runs §1.1 consistency checks. **OPEN:** the concrete Build-proposed `display_version` string / ledger allocation for this candidate is not in public data yet. **Single file that would answer it:** `releases/openbeta/ledger.json` (firmware repo) after Build allocates, or Build's manifest/handoff naming the proposed display version.
9. **RESOLVED.** Commit signing is **informational only**, not a gate. Report `gpgsig`/verification; do **not** FAIL on it.
