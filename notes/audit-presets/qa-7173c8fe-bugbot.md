# BugBot QA: preset format v1, head 7173c8fe (residuals + golden lock)

**Overall: PASS**

Baseline: `/workspace/audit-presets/qa-014e1b67-bugbot.md` (head `014e1b67`, overall PASS, host tests later measured 148 / 132 / ASan clean, CRC `0xd644a5f5`).

- **Repo:** gobbajogger/X0X37-for-Loopy-Pro
- **Branch:** `lane/fmb-presets-format` (GitHub `ls-remote` head matches this SHA)
- **Head:** `7173c8fed9ade6d91b6897960680bddce0a5bdd2`
- **Head tree:** `98ed2617847ddce71c233248d5371516c68b91d2`
- **Parent chain:** `7173c8fe` = two commits on `014e1b67b013dae0e93d8b6d5ec095eef1472c32`
  - `935eec6568eba4a5097e9b33d82535c578b6d063` — size-opt + golden-bytes lock (13 November 2026, 06:06:20 AM America/Chicago; recorded 07:06:20 −05:00)
  - `7173c8fed9ade6d91b6897960680bddce0a5bdd2` — residual fixes (06:06:23 AM America/Chicago; recorded 07:06:23 −05:00)
- **Reviewer:** BugBot, 13 November 2026, America/Chicago
- **Method:** `git fetch` then a fresh detached worktree. Rules checks on the two-commit range. Residual fixes reviewed in code and in tests (file:line). Golden fixtures compared to a dump from the existing `014e1b67` encoder at `/workspace/qa-runs/014e1b67`. Official host script run once. No device writes, no pushes, no GitHub writes, no product-repo source edits.

This head is still a host-only codec. It is not linked into the device image. Device flash, RAM, and JieLi size are out of scope (R6 stays with Firmware Main Builder). Host object size is recorded as a HOST ESTIMATE only.

---

## 1. Checkout

```
git -C /workspace/x0x fetch origin lane/fmb-presets-format --no-tags
git -C /workspace/x0x worktree add --detach /workspace/qa-runs/7173c8fe 7173c8fed9ade6d91b6897960680bddce0a5bdd2
```

Verified in that worktree:

- `git rev-parse HEAD` = `7173c8fed9ade6d91b6897960680bddce0a5bdd2`
- `git rev-parse HEAD^{tree}` = `98ed2617847ddce71c233248d5371516c68b91d2`

The `014e1b67` worktree at `/workspace/qa-runs/014e1b67` is still present (tree `cf846cd3`) and was used only as a read-only encoder for the golden comparison.

---

## 2. Rules checks — **PASS**

**Changed paths** (`git diff --name-status 014e1b67..7173c8fe`), and only these. Both commits touch the same four files:

| Status | Path |
| --- | --- |
| M | `docs/plans/2026-11-13-preset-format-v1.md` |
| M | `firmware/src/app/preset_record.c` |
| M | `firmware/src/app/preset_record.h` |
| M | `tests/host/preset_record_test.c` |

Stat: 4 files, +822 / −245. `tests/host/preset_record_test.sh` and `tests/run_tests.sh` are **unchanged** (same blobs as `014e1b67`). That is within the expected set (preset module, tests, docs; `run_tests.sh` did not need another edit).

**Not changed** (explicitly compared, blobs identical to `014e1b67`): `firmware/src/x0x.c`, `tools/build.py`, `firmware/src/app/factory_banks.c`, `firmware/src/app/original_sound.c`, `firmware/src/app/project.c`, factory assets, `.github/`, anything that would be `main` or `nightly`. No workflow files.

**Authors.** Both commits: author and committer are `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`. No `genkigenki` or other author.

**Still not linked into the device build.** `preset_record` does not appear in `firmware/src/x0x.c`, `tools/build.py`, `build.sh`, or `host/build_host.sh`. `x0x.c` and `tools/build.py` are byte-identical to `014e1b67`. The unity include list is unchanged.

---

## 3. Residual fixes — code and tests

Each is **PASS** on this head: implemented in the codec, covered by a named test, and exercised in the measured host run (script exit 0, 0 failures).

### R1 — failed import leaves the caller slot clean — **PASS**

**Code.** `preset_record_import` (`preset_record.c` 623–646) decodes into a local `tmp` via `load_into` + `check_loaded`, then compares engines, and assigns `*rec = tmp` only when the target matches. On `WRONG ENGINE` (or any earlier failure) it returns without writing `*rec`. The header documents this (`preset_record.h` 229–233). `preset_record_decode` (606–620) already used the same scratch-then-assign shape.

**Tests.** `test_import_failed_leaves_slot_clean` (`preset_record_test.c` 764–813), called from `main` at 965:

- Sentinel slot (`EMPTY`, `PRESET_ENGINE_NONE`) stays unchanged after importing a Chicago file as Miami (`c` 785–794): engine, origin, nparams, and name are the prior values; the slot is not filled as Chicago.
- A successful matching import, then a second `WRONG ENGINE` attempt, leaves the good slot byte-for-byte intact (`c` 796–811).

The old `test_reject` comment that accepted a filled buffer on failed import is gone; that case now points at this test (`c` 343–344).

### R2 — tests for bad magic and older-than-v1 — **PASS**

**Code.** `load_into` returns `PRESET_ERR_MAGIC` on a wrong magic (`c` 531, seen at the `rd_u32 != PRESET_MAGIC` path). Format `< PRESET_FORMAT_V1` is `PRESET_ERR_OLDER` (`c` 543–544). Schema `< PRESET_SCHEMA_V1` is `PRESET_ERR_OLDER` in `check_loaded` (`c` 588–589). Both class as `PRESET_IMPORT_BAD_FILE`.

**Tests.**

- `test_reject_bad_magic` (`c` 816–834): flip byte 0, expect `PRESET_ERR_MAGIC` and BAD FILE, output left clean; restoring the magic decodes.
- `test_reject_older_version` (`c` 836–868): format 0 with a valid CRC → `PRESET_ERR_OLDER`, BAD FILE, output clean; schema 0 with a valid CRC → the same.

### R3 — `*written` set only after the buffer-size check — **PASS**

**Code.** `preset_record_encode` (`c` 474–505): after `check_record`, `if (out && cap < total) return PRESET_ERR_BUFFER;` then, and only then, `*written = total`. A size-only query (`out == 0`) still sets `*written` and returns OK, as documented (`h` 199–202). `preset_record_export` (`c` 508–515) returns through `encode` after the export gate, so it inherits the same rule. `preset_commit_encode` (`c` 690–710): `if (out && cap < PRESET_COMMIT_BYTES) return PRESET_ERR_BUFFER;` then `*written = PRESET_COMMIT_BYTES`. Header note at `h` 245.

**Tests.** `test_encode_written_after_buffer_check` (`c` 871–921):

- Size query (`encode(0, 0, &w, …)`): OK, `w` becomes the Chicago record size.
- Short `encode`, `export`, and `commit_encode` with `w = 99` and a 16-byte `0xAA` guard: each returns `PRESET_ERR_BUFFER`, `w` stays 99, and encode leaves the guard untouched.
- Successful `encode` and `commit_encode` set `w` to the real size.

### R5 — 808 with `nsamples > 0`, round-trip and CRC — **PASS**

**Code.** Unchanged rule: `preset_record_expected_nsamples` for 808 is `-1` (variable, 0–16). Encode/decode already packed explicit 16-byte sample refs with CRC.

**Tests.** `test_808_nsamples_and_crc` (`c` 923–956): 808 record with `nsamples = 2` (ROM refs BD / SD, CRCs `0xaaaaaaaa` / `0xbbbbbbbb`); `roundtrip` OK; wire length is `96 + 4 + 144 + 2×16`; decode recovers `nsamples == 2`; `preset_record_verify_samples` passes, then fails on a mismatched CRC; single-sample verify matches and mismatches as expected.

---

## 4. Golden-bytes lock — **PASS**

Fixtures live in `preset_record_test.c` 610–649 (`GOLDEN_CHI` 122 B, `GOLDEN_909` 296 B, `GOLDEN_COMMIT` 32 B), captured (comment, lines 610–612) from the encoder at tree `cf846cd3` (the `014e1b67` / `9bda58dd` tree) before the size-opt. `test_golden_bytes` (`c` 666–761) encodes the same inputs with the current encoder, compares every byte, and decodes the fixtures back.

That test is in the official host run and **passed** (script exit 0; OFF 224 includes these checks).

**Independent comparison to the `014e1b67` encoder** (not a second run of the 7173c8fe test). A small driver at `/workspace/qa-runs/golden-cmp-014e1b67/dump_v1.c` was compiled against `/workspace/qa-runs/014e1b67/firmware/src/app/preset_record.c` (read-only) and encoded the exact golden inputs (Chicago “Golden Chi” via `export`, 909 “Kit” with four ROM refs via `encode`, commit seq 2 / slot 3 / `record_crc` 0x2020 / copy 1). Result: **ALL_MATCH**.

| Fixture | Length | sha256 (golden = 014e1b67 encoder) |
| --- | ---: | --- |
| CHI | 122 | `9b779fc71662176f9b72c41e40cfcf302f5131fd5e046bda66d727f2a00fad1a` |
| 909 | 296 | `a76f9fc1b015e46bf6290125c118c77c4c34054b0207fdeb63643e32eafaacd5` |
| COMMIT | 32 | `5ba96398c95c384925d48a0981e7a37413b1e64740b8b0f4e89a67a0d909cbc4` |

Dump bins and compare log: `/workspace/qa-runs/golden-cmp-014e1b67/` and `/workspace/qa-runs/7173c8fe/logs/golden-vs-014e1b67.txt`. The `935eec65` size-opt (and the `7173c8fe` residual follow-up) did **not** change v1 encoded bytes versus `014e1b67`.

---

## 5. Measured host runs (13 November 2026, 06:11 AM America/Chicago)

**Compiler:** `/usr/bin/cc` — `cc (Debian 14.2.0-19) 14.2.0`. Environment: `/workspace/qa-runs/7173c8fe/logs/environment.txt`.

**Exact command** (one official-script invocation; OFF, ON, and ASan once each). `PRESET_FACTORY_TABLE_CRC` skips the script’s second OFF re-run. Working directory `/workspace/qa-runs/7173c8fe`. The test script blob is unchanged from `014e1b67`:

```
PRESET_FACTORY_TABLE_CRC=0xd644a5f5 sh tests/host/preset_record_test.sh /workspace/qa-runs/7173c8fe/build
```

**Script exit code: 0.** Elapsed 2.53 s. stderr empty (no compiler warnings, no sanitizer reports). Transcript: `/workspace/qa-runs/7173c8fe/logs/script.stdout.txt` (stderr 0 bytes: `script.stderr.txt`). Exit metadata: `logs/exit.txt`. Command: `logs/command.txt`.

| Config | Claimed | **Measured** | CRC | Counts | Failures | Raw log |
| --- | ---: | ---: | --- | --- | --- | --- |
| Export OFF (default) | 224 PASS | **224 PASS** | **0xd644a5f5** | 81/30/4/4 | 0 | `/workspace/qa-runs/7173c8fe/logs/off.log` |
| Export ON (`-DDNBOX_FACTORY_PRESET_EXPORT=1`) | 208 PASS | **208 PASS** | **0xd644a5f5** | 81/30/4/4 | 0 | `/workspace/qa-runs/7173c8fe/logs/on.log` |
| ASan/UBSan OFF (`-fsanitize=address,undefined -fno-sanitize-recover=all`) | clean | **224 PASS, clean** | **0xd644a5f5** | 81/30/4/4 | 0; no ASan/UBSan diagnostics | `/workspace/qa-runs/7173c8fe/logs/asan.log` |

`cmp` of `off.log` and `asan.log`: identical (sha256 `5e51a4a7b1ec35b22370a87c1505f2af423cdcd914d4b60820a8fa71dc269cde`). The ASan binary is linked to `libasan.so.8` and `libubsan.so.1`. The 16-check gap (224 − 208) is the same P8 “no partial” loop as at `014e1b67`. All FMB numeric claims match the measured logs.

Factory sources were already shown identical to `014e1b67` (and therefore to `94197fc8`). The table CRC was recomputed by `factory_table_crc()` on all three runs and matched the pin.

**Host object size (HOST ESTIMATE only; not a device measurement; R6 remains with FMB).** This run’s `size` on `/workspace/qa-runs/7173c8fe/build/preset_record.o` (`cc -std=c99 -Os`, as the unchanged test script does): text 5309, data 0, bss 0. Sections: `.text` 3682, `.rodata` 288, `.rodata.str1.1` 251, `.eh_frame` 1088. Compared with the measured `014e1b67` host object (text 5128; `.text` 3821, `.rodata` 224, `.eh_frame` 832): `.text` shrank by 139 B; the Debian `size` “text” column grew because `.rodata` and `.eh_frame` grew. Working-record `sizeof(preset_record_t)` is still 536 B (HOST). Do not treat these figures as a JieLi image delta.

---

## 6. Other baseline checks still holding

Factory tables, `project.c`, and held-note paths are untouched (not in the four-file diff). The ten checks from the `014e1b67` report are not re-opened except where this head changed them (R1, R2, R3, R5, golden lock). Plugin parsing and new JUN-60 factories remain N-A. The export gate, versioning, and power-loss commit protocol remain in place. Open design notes from the baseline that this head did not address (R4 P6 check-count tightness, R6 device size, R7 1-based factory index) stay as residuals for later wiring / FMB, not FAILs here.

---

## 7. Summary

**PASS** for `7173c8fe` on `lane/fmb-presets-format`. Two commits on `014e1b67`, both authored and committed as gobbajogger. The diff is four expected files only. The module is still not in the device build. R1, R2, R3, and R5 are fixed in code and covered by tests. Golden v1 bytes are identical to the `014e1b67` encoder (Chicago 122, 909 296, commit 32). Host script exit 0: **224 PASS** (OFF), **208 PASS** (ON), **ASan/UBSan clean** (224, log identical to OFF), factory CRC **0xd644a5f5**, counts 81/30/4/4 — all matching FMB claims.

No device was written. No commit, push, GitHub comment, or product-source edit was made.

**Report path:** `/workspace/audit-presets/qa-7173c8fe-bugbot.md`
**Logs:** `/workspace/qa-runs/7173c8fe/logs/`
