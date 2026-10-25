# Native build input for the Codex coordinator

This packet is source and commands only. It does not include a toolchain, an SDK, firmware bytes, or a device step.

## What to build

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Branch: `lane/firmware-build-version-labels-20261023-1814`
- Build the branch head that contains `tools/release_alloc.py` and `releases/openbeta/ledger.json`.
- Do not build `253b5101a9e3ebcc1e8dcca48764322909820b26`. That commit is the input this lane started from, before the label work.
- Do not build or rewrite `892bf60d912eca93597dcf03be25a9756540c949`. That source stays an unversioned installed artifact.
- Do not merge this branch into `nightly` or `main`.

## Version

The ledger in this pull request has no allocation. The first number for a new source tree is `0.1.0`. Take it only when both models will be built from that exact tree, from a clean clone:

```
python3 tools/release_alloc.py --repo . allocate \
  --remote origin --branch release-records \
  --source-commit <40-char source SHA> \
  --source-tree <64-char tree hash>
```

That command commits only `releases/openbeta/ledger.json` and pushes `refs/heads/release-records`. It does not push `nightly` or `main`. A rejected push reloads that branch and retries. The same commit and tree print the same number. The same tree with a different commit is refused. A run without `--remote` only edits the local file and does not serialize across clones.

Bind each model after the package and controller hashes are known. Bind updates the ledger file. Commit only that file afterward:

```
python3 tools/release_alloc.py --repo . bind \
  --version <X.Y.Z> --model elite \
  --package-sha256 <64 hex> --controller-sha256 <64 hex> --controller-size <bytes>
```

Repeat with `--model pro`. The products must be `SMK-37 Elite_9XXYYZZ` and `SMK-37 Pro_9XXYYZZ` for that same `X.Y.Z`.

## Firmware commands

Use the pinned JieLi SDK and toolchain already qualified for this project. From a clean checkout of the branch head:

```
python3 tools/dev_validate.py run \
  --commit <source SHA> --model elite \
  --firmware-version <X.Y.Z> \
  --plan <plan.json> --output <new-elite-dir>

python3 tools/dev_validate.py run \
  --commit <source SHA> --model pro \
  --firmware-version <X.Y.Z> \
  --plan <plan.json> --output <new-pro-dir>
```

`--firmware-version` stamps the numeric identity. It does not make the build a release and it does not test hardware. The on-device version string is `X.Y.Z`. The settings row shows `X.Y.Z_openbeta`.

Check both packages with `tools/firmware_build_labels.py --model elite|pro --source-commit <SHA> --expected-version <X.Y.Z>`. Refuse the pair if the models disagree, the loader is missing, or the hash does not match the record.

## Return

Return the source SHA, tree hash, allocated `X.Y.Z`, both product strings, both package SHA-256 values, both controller SHA-256 values, and the dev-validate record paths. Do not publish the site and do not push firmware to `nightly` or `main`.
