# Git Verifier — NIGHTLY publication verification checklist (DnBOX)

Prepared 2026-11-05 (Asia/Vladivostok, UTC+10). Local box file only — nothing pushed or published.

**Role boundary.** The Git Verifier only VERIFIES, read-only (anonymous `git fetch`/`git ls-remote`, HTTPS GET, `sha256sum`). The verifier **never** pushes, merges, writes `gh-pages`, edits a catalog, opens a PR, or publishes. The coordinator (via the sole `website_publisher` role, NPP §1 item 6, §4, §6 Step 6) publishes only after **every** gate below is PASS. Any FAIL = publish blocked; report on #832.

Sources: `/workspace/dnbox/nightly-publication-path.md` (NPP), `publication-blockers.md`, `release-plan.md`, `nightly-prep-490e656c-20261105.md`, `workflow-audit-20261105.md` (P0-6 → stage P5.5), `master-plan.md` (P5.1–P5.4, C0.6), `verify-branches.sh`.

Repos: firmware `gobbajogger/X0X37-for-Loopy-Pro` (tracking #832, integrator PR #698 on `wip/one-build`); installer/catalog `gobbajogger/x0x37-for-Loopy-Pro_nightly`.

## 0. Timing rule — same SHA as QA, verified in parallel with QA
- The verifier checks the **same commit SHA that QA reviews**, **at the same time as QA** — do not wait for QA's verdict/push.
- Current M7a combined SHA to track: **`490e656c01b21c52b5116b109bde8e94be856016`**
  (branch `lane/dev-m7a-option-a-combined-v3-20261105`, tree `7cbc23087ca5109159d0c8cce7a216e4fcb63cdd`, parent `c117ed1f0802a51988b0b468d9479ae1a231dd2b`; tip message starts "UNREVIEWED:" — per nightly-prep).
- If QA's verified SHA ≠ the SHA proposed for publish → FAIL, stop.

## 1. Pre-publish gates (read-only)

Setup (anonymous, throwaway dir):
```bash
REPO_FW=gobbajogger/X0X37-for-Loopy-Pro
REPO_NL=gobbajogger/x0x37-for-Loopy-Pro_nightly
SHA=490e656c01b21c52b5116b109bde8e94be856016   # = QA-verified SHA
d=$(mktemp -d); cd "$d"; git init -q .
git fetch -q "https://github.com/$REPO_FW.git" "<branch-or-sha>"
```

| # | Gate | Check (command pattern) | PASS criterion |
|---|---|---|---|
| G1 | Source SHA = QA-verified SHA | compare `$SHA` to SHA quoted in QA's #832 / #698 acceptance comment and in the integrator freeze (NPP §6 Step 0) | identical 40-hex |
| G2 | Tree | `git rev-parse $SHA^{tree}` | = `7cbc2308…` (or the tree QA quoted) |
| G3 | Parent / lineage | `git rev-parse $SHA^`; `git log --format=%H <base>..$SHA` | parent as reported; linear on stated base (c32c54ac RC1 for M7a) |
| G4 | Identity | `git log --format='%an <%ae>|%cn <%ce>' <base>..$SHA \| sort -u` | only `gobbajogger <255244738+gobbajogger@users.noreply.github.com>` for author AND committer |
| G5 | No trailers | `git log --format=%B <base>..$SHA \| grep -i 'co-authored-by'` | no match |
| G6 | No force-push | previously recorded tips are ancestors: `git merge-base --is-ancestor <old_tip> <new_tip>` for every re-pushed ref; `artifacts/nightly-dev` old tip (`f857ffbb…`) ancestor of new tip; `gh-pages` old tip (`7c7baecb…`) ancestor of new tip (non-forced FF, NPP §6 Step 6) | exit 0 for all |
| G7 | Mains untouched | `git ls-remote https://github.com/$REPO_FW.git refs/heads/main` and same for `$REPO_NL` | fw main `1a6bc3fe…`, nightly main `a8205aff…` (or the value recorded at last verification; changes only via `website_main_merger` for nightly main, NPP §6 Step 5) |
| G8 | No PR opened by lanes | read-only PR list check | no new PR from lane branches unless role-authorized (catalog_sync PR is expected per NPP Step 4) |
| G9 | No forbidden content | `git ls-tree -r --name-only $SHA` / diff of new commits | no `.fwsc`, `ctl.img`, SDK, credentials in nightly repo or source lanes (NPP §1); `artifacts/nightly-dev` adds ONLY new files under `artifacts/nightly/dev-<YYYYMMDD>-<tag>/<sha256>/` (NPP §5A, Step 2), old files unchanged (`git diff --diff-filter=MD <old_tip> <new_tip>` empty) |
| G10 | Artifacts sha256 vs manifest | fetch the hosting commit; `git show <host_commit>:<path> \| sha256sum`; `\| wc -c` | sha256 = digest in path dir name = catalog `models.<m>.sha256` = qualification `packages` key; size = catalog `size` |
| G11 | Controller | catalog `controller_sha256`/`controller_size` vs native_build receipt | match (expected ctl `a334208b…c1d984`, 61552 B if unchanged — NPP §7) |
| G12 | Catalog entry shape | read catalog PR diff | schema 1, `status: dev-checked`, `config: dev`, `hardware_tested: false`, label DEV/DEVICE NOT TESTED, full 40-hex `source_commit`=`$SHA`, `source_tree`, `product` SMK-37 Elite_900 / Pro_900, digest-addressed immutable `pkg` URL, `controller_strategy: bundled`; all prior 28 entries unchanged (append-only) (NPP §5B) |
| G13 | Gates/receipts present | qualification & QA comments | every requiredGate PASS with immutable `evidence_url`: package-integrity, model-controller, memory-flash, host-regressions, backup-restore-readback, interruption-recovery, ota-target-binding, independent-qa; native-build PASS if present (NPP §5B) |
| G14 | HOLDs | publication-blockers.md / #698 / #816 / #822 | NIGHTLY HOLD, #816 HOLD, #822 publication HOLD lifted (or owner explicitly authorized an early QA-IN-PROGRESS publish) |
| G15 | Publisher pre-checks evidence | publisher's output | `npm run verify:pages -- --candidate /ABS/OUT --retained-catalog …` PASS recorded (verifier reads the record; does not run publish) |

Signature: commits carry SSH gpgsig shown as `unknown_key` — informational only, not a stated rule (release-plan). New pushes should be unsigned per the push template.

Helper for lane tips: `/workspace/dnbox/verify-branches.sh <repo> <branch> <expected_parent> <expected_tree>`.

## 2. POST-PUBLISH install check (stage P5.5; NPP §1 item 6 + §6 Step 6, as referenced "NPP §1.6" in workflow-audit P0-6)

Run only after the coordinator/publisher reports the `gh-pages` fast-forward. URLs from the NPP:
```bash
SITE="https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly"   # NPP §1, §3
VER="nightly-490e656c-dev"                                          # entry id (nightly-prep draft)
# 1) Catalog readback over HTTPS (no cache)
curl -fsSL -H 'Cache-Control: no-cache' "$SITE/versions.json?nocache=$(date +%s)" -o versions.json
sha256sum versions.json
jq -r '.latest' versions.json                                   # expect $VER
jq '.versions | length' versions.json                           # expect previous count + 1 (28 → 29)
jq --arg v "$VER" '.versions[] | select(.version==$v)' versions.json > entry.json
jq -r '.source_commit, .source_tree' entry.json                 # expect $SHA and 7cbc2308…
# 2) Each package: download the exact pkg URL (no redirects), hash, size
for m in elite pro; do
  url=$(jq -r ".models.$m.pkg" entry.json)                      # raw.githubusercontent.com/<IMMUTABLE_COMMIT>/artifacts/nightly/dev-<YYYYMMDD>-490e656c/<sha256>/x0x37-$m-dev.fwsc
  exp=$(jq -r ".models.$m.sha256" entry.json); sz=$(jq -r ".models.$m.size" entry.json)
  curl -fsS --max-redirs 0 "$url" -o "$m.fwsc"
  got=$(sha256sum "$m.fwsc" | cut -d' ' -f1); gsz=$(stat -c %s "$m.fwsc")
  [[ $got == $exp && $gsz == $sz && $url == *"/$exp/"* && $exp == "<MANIFEST_SHA256_$m>" ]] && echo "PASS $m" || echo "FAIL $m got=$got size=$gsz"
done
# 3) Installer page/script
curl -fsSL "$SITE/" -o index.html;            sha256sum index.html
curl -fsSL "$SITE/install/" -o install.html;  sha256sum install.html     # path per NPP §5B "install/"
curl -fsSL "$SITE/build-status.json" -o build-status.json
curl -fsSL "$SITE/package-version-assignments.json" -o pva.json
# installer_sha256: compare against qualification `installer_sha256` (NPP §5B) — <INSTALLER_SCRIPT_URL> per open question Q3
curl -fsSL "<INSTALLER_SCRIPT_URL>" | sha256sum                 # expect <QUALIFICATION_INSTALLER_SHA256>
# 4) Old entries kept
jq -r '.versions[].version' versions.json | grep -x 'nightly-8be16486-dev'
git ls-remote https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly.git refs/heads/gh-pages   # new tip; check FF from 7c7baecb…
```
`<MANIFEST_SHA256_*>` = sha256 from native_build manifest / qualification `packages` keys — never typed from memory.

**PASS** only if all: HTTP 200 on every URL; `latest` = new entry; entry `source_commit` = QA-verified SHA; each package sha256 & size = catalog = manifest = digest in URL path; `pkg` URL is pinned to a 40-hex commit (not a branch); installer hash = qualification `installer_sha256`; old entries (incl. `nightly-8be16486-dev`) still present and unchanged; `gh-pages` advanced by fast-forward; label still says DEVICE NOT TESTED. **FAIL** on any mismatch, 404, redirect, branch-relative URL, or missing old entry → report immediately; recommend coordinator rollback decision (verifier does not roll back). Allow Pages propagation: retry up to ~10 min before declaring FAIL (open question Q5).

## 3. What to report on #832 (drafted by verifier, posted by coordinator's authorized path)
```
Git Verifier — <PRE-PUBLISH|POST-PUBLISH P5.5> — <PASS|FAIL>  (<time> UTC+10)
SHA <40-hex> = QA-verified SHA (QA comment <id>); tree <40-hex>; parent <…>
Identity gobbajogger noreply xN; trailers 0; force-push none; mains fw <sha8>/nightly <sha8> unchanged
Artifacts host commit <40-hex>: elite <sha256> <size> / pro <sha256> <size> = manifest
[post] versions.json sha256 <…>, latest=<ver>, entries <n>; elite/pro served sha256 match; installer sha256 <…> = qualification; gh-pages <old8>→<new8> FF
FAILs: <list or none>.  Read-only; nothing pushed/published by verifier.
```

## 4. Open questions (NPP gaps — not guessed)
1. NPP has no numbered "§1.6" nor a "P5.5" stage; both come from workflow-audit-20261105 P0-6 (proposed). master-plan stops at P5.4. Confirm P5.5 is adopted and that NPP §1 item 6 + §6 Step 6 are the intended spec.
2. Who runs P5.5 — audit says "Publisher"; this task assigns verification to Git Verifier. Confirm both (publisher self-check + independent verifier readback).
3. Exact installer script URL/file name and how `installer_sha256` is computed (which file: `index.html`, `install/index.html`, a JS bundle?). NPP only says "installer HTML" and `install/`.
4. Where the authoritative package manifest lives for 490e656c (native_build output path / receipt URL); no artifact exists yet (nightly-prep).
5. Pages propagation/CDN cache window to wait before declaring FAIL.
6. Exact `versions.json` schema keys (`latest`, `versions[]`, `version`) — inferred from NPP/nightly-prep draft; confirm against live file.
7. Whether early QA-IN-PROGRESS publish of 490e656c is authorized while NIGHTLY/#816/#822 HOLDs are open, and whether 490e656c (diverged from 8be16486, 10 commits behind) is intended (nightly-prep).
8. Expected `firmware-version`/display version for 490e656c (ledger allocation) to check in catalog.
9. Is signing (SSH gpgsig unknown_key) a rule or informational? Currently informational.
