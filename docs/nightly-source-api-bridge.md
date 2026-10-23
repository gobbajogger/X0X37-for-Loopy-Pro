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
