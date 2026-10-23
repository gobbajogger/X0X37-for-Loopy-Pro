# Pinned NIGHTLY source retrieval through the GitHub REST API

Status: **source-only development tool**, not a firmware build, installable candidate, QA approval, or Pages deployment.

## Why

The hourly native build runner must compile the **exact** firmware `nightly` commit, not a historical offline copy. A GitHub connector can read repository refs and text, but its results are not automatically present in the Linux builder's filesystem. Local `git clone` may fail when DNS to `github.com` is unavailable. The script in `tools/fetch_exact_source_api.py` uses GitHub REST Git Data endpoints on a **network-capable native process** to construct a complete local Git checkout, and verifies its commit SHA, tree SHA, Git blob hashes, symlinks, clean checkout, and unchanged branch ref. GitHub Actions/Codex/Work are not required.

The observed firmware `nightly` ref on 2026-10-23 was `c9718ae833eb64a5191c350922593df8a36017c9` (tree `715fbd98d427c8ddd6b64d1b923e9d1fb8ae64ea`). Against the owner-supplied historical tree `152ca944e9cd3d0913906a1d3fa9d8d2826ea639`, GitHub's **untruncated recursive tree metadata** showed 36 added/changed blob paths (728,419 uncompressed bytes total), and 184 removals. This is metadata, not evidence that the changed blob bytes have already reached a native builder.

## Pinned execution (network-accessible native Linux process only)

Using the owner-supplied historical bundle, whose `refs/heads/source-snapshot` head is `f58d62f0b622b9c5792bda81793e0031d8be8747`:

```bash
python3 tools/fetch_exact_source_api.py \
  --repo gobbajogger/X0X37-for-Loopy-Pro \
  --branch nightly \
  --commit c9718ae833eb64a5191c350922593df8a36017c9 \
  --seed-bundle /mnt/data/x0x37-investigate/bootstrap/history/source-cache.bundle \
  --output /mnt/data/x0x37-nightly-checkout-c9718ae8
```

The `--commit` value must be refreshed from the GitHub branch ref at each new eligible integrated commit; if it differs from the head at the start **or end** of materialization, the script fails without promoting an output. `--output` must be a new path. The optional `GITHUB_TOKEN` environment variable raises REST API limits; never log or commit its value. Without the seed, the script downloads all 1,574 tracked blobs at this observed source tree; with a matching historical seed, it reuses identical blob objects and needs only changed blobs. Optional `--seed-git` accepts a previously verified checkout. A seed **never** supplies identity in place of GitHub tree/commit validation.

Run offline tests:

```bash
python3 -m unittest discover -s tests -p 'test_fetch_exact_source_api.py' -v
```

Passing materialization returns `source_checked: true`, an exact commit and tree, number of files and fetched blobs. It **does not** set `installable`, `qa_approved`, or `device_tested` and does not build a firmware package. A native build worker must independently run its reviewed `DEV` plan, both SMK-37 Elite and Pro JieLi target builds, matching Bluetooth controller, source/SDK/toolchain pin checks, budgets, model+CRC/SHA identity, relevant negative host tests, independent QA, data-preservation/installer checks, and distribution/HTTP gates before calling anything a selectable NIGHTLY candidate. Reuse existing `tools/native_nightly_build_probe.py` from [firmware PR #64](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/64) only after its own review and merge; do not silently replace the firmware build process with this fetch helper.

## Current runtime limitation — do not misreport as fixed

The current ChatGPT Linux container cannot resolve GitHub's native DNS (`curl`/`git` blocked), though its separate connected GitHub tool successfully reads the complete live **tree metadata** and files. That tool has no automatic binary/source-file mounting into the native container. Therefore **this helper is tested against fake HTTP GitHub API fixtures, but has not retrieved or built the live NIGHTLY firmware in this container**. Either an approved native networking route to GitHub REST, or a verified complete source-byte bundle transported via an authorized file/Library bridge, is needed to finish the exact-current checkout. Do not bypass a denied file transfer or claim a current build from historical files.

No production `main`, shared firmware `nightly`, NIGHTLY website repo, `gh-pages`, proprietary binaries, or physical device must be modified by a source-transfer test. Source-only updates go through the isolated review lane, then the existing sole firmware integrator and Hourly Build worker; the sole publisher separately owns checked website publication.

## 2026-10-24 verified second transport: disconnected native runner + GitHub connector

A local native compiler does **not** require DNS access to GitHub. The connected GitHub Git Data API can independently retrieve exact old/new blobs. For this container, those contents were transported as compact line-delta JSON and independently matched to Git blob SHA-1 values in the already-authorized offline Git bundle. The new `tools/apply_api_delta.py` provides a reproducible, fail-closed way to consume **complete** connector-produced deltas without native networking.

1. Fetch and pin `nightly` branch commit and tree through the connected GitHub app. Fetch the **untruncated** recursive target tree and the baseline Git tree. Record *every* changed/added blob (with path, mode, old blob SHA if applicable, target blob SHA) and *every* deletion. Obtain the Git Data API `/git/commits/<sha>` object for commit reconstruction. Never assume a partial diff describes the entire source.
2. For each text change, fetch both blob contents from GitHub and produce zero-based LF-line edit operations `[start_original_line, deleted_original_line_count, inserted_text]`. Add new UTF-8 files as full text and include all deletions. Transport the complete JSON as an authorized file into the native runner. The current ChatGPT connector has **no automatic file-system transfer**, so transport must actually be verified; a list of remote hashes alone is insufficient.
3. The offline materializer requires JSON keys `seed_tree`, `target_tree`, `target_commit`, `github_commit` (the exact GitHub Git Data commit object), `files` (objects with `path`, `old` or null, `sha`, `mode`, and `ops` or `content`), and `deletions` (all removed tracked paths). Its invocation is `python3 tools/apply_api_delta.py --seed-git /mnt/data/x0x37-work/firmware --delta-json /mnt/data/x0x37-work/complete-nightly-delta.json --output /mnt/data/x0x37-work/exact-nightly`. The delta file in that command must **actually exist**; neither this document nor a stale source ZIP supplies it.
4. It clones to a private temporary destination, validates every old and new Git blob, refuses unsafe paths or escaping symlinks, creates/deletes exactly the listed tracked files, checks the final **whole-tree** SHA, reconstructs the exact signed or unsigned Git commit object, verifies the full target commit SHA, then checks out that source with a clean Git status. Missing changed paths/deletions make the tree mismatch and **no** output checkout is promoted. No release/installer implication follows from this step.
5. Reread the `nightly` ref after the operation through the GitHub connector. If it moved, do not claim this result as the new branch head; rebuild using the new pin. The exact older source can still be kept as an explicitly labeled versioned historical candidate if appropriate.

**Executed evidence:** 13/13 offline delta materializer tests PASS, including reconstructed checkout identity, missing deletion/change refusal, wrong old/new hash, malicious path, symlink escape, non-LF byte preservation, overlapping edits and amended commit metadata. The original online REST source helper's 12/12 fake-HTTP tests also PASS. Four firmware source paths (`ui.c`, `smc_mixer.c`, `ble_midi_central.c` and `.h`) and three host test paths were matched byte-for-byte to their actual `nightly` Git blob SHA-1 using the GitHub connector, then checked in native Linux. These are **not a complete source reconstruction**, because other changed site/docs/validation files and deleted history are still unmaterialized.

The same Linux x86-64 runner successfully executed `MODEL=elite/pro BOARD=smk37 X0X_DEV_PADCAL=1 bash build.sh --board smk37`, both exit 0, plus `CTL_NATIVE=1 CTL_NO_CACHE=1 bash ble/ctl/build_ctl.sh`, exit 0 with zero undefined symbols. The JieLi clang 4.0.1 and 11/11 pinned SDK kit verification PASS. Native BLE host tests and SMC clip visibility tests PASS. In this **mixed historical tree with exact current firmware C/H inputs**, SHA-256 outputs were:

| Item | Bytes | SHA-256 |
| --- | ---: | --- |
| Elite exploratory DEV .fwsc | 651396 | `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` |
| Pro exploratory DEV .fwsc | 651392 | `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` |
| Controller `ctl.img` (local-only, NOT redistributable without verified vendor terms) | 65492 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

**Mandatory disposition:** `EXPLORATORY NATIVE DEV BUILT`, `SOURCE TREE NOT FULLY RECONSTRUCTED`, `INSTALLABLE=false`, `QA_APPROVED=false`, `DEVICE_TESTED=false`, **NO firmware packages or controller distributed/published**. This proves native compilation and connector-guided byte transfer are possible; it does not prove exact-tree artifact identity or pass skipped QA, data-preservation, publishing or licensing gates. The next safe stage is to finish full delta transport and pass `target_tree` and `target_commit`, then rebuild/validate both targets from that exact clean checkout. Do not relabel these exploratory hashes as official `nightly` builds.
