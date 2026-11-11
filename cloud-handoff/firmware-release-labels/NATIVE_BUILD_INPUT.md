# Native build input for the Codex coordinator

This packet is source and commands only. It does not include a toolchain, an SDK, firmware bytes, or a device step.

## What to build

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Branch: `lane/firmware-build-version-labels-20261023-1814`
- Build the branch head that contains `tools/release_alloc.py` and `releases/openbeta/ledger.json`. The head is the commit named in the latest pin on draft PR 53. `f6c4c23f6822535cae25625cf083ae4ebed10ae5` is an ancestor, not the candidate.
- Do not build `253b5101a9e3ebcc1e8dcca48764322909820b26`. That commit is the input this lane started from, before the label work.
- Do not build or rewrite `892bf60d912eca93597dcf03be25a9756540c949`. That source stays an unversioned installed artifact.
- Do not merge this branch into `nightly` or `main`.

## Version

The ledger in this pull request has no allocation. The first number for a new source tree is `0.1.0`. Take it only when both models will be built from that exact commit. `source_tree` is `git rev-parse <commit>^{tree}`: a 40-character Git object id, not a 64-character digest.

```
python3 tools/release_alloc.py --repo . allocate \
  --remote origin --branch release-records \
  --source-commit <40-char source SHA> \
  --source-tree <40-char git tree id>
```

The command reads the commit in `--repo` and refuses a missing commit or a tree that is not that commit's `^{tree}`. `--remote origin` is the source repository's remote: `git -C <repo> remote get-url origin` supplies the URL used by the temporary checkout. The name is not a path inside that checkout. It commits only `releases/openbeta/ledger.json` in a temporary checkout of `release-records` and pushes `refs/heads/release-records`. It does not reset `--repo`, does not require a clean source tree, and does not push `nightly` or `main`. A rejected push fetches the record branch and retries. A later source commit allocates the next number from that record; the record does not have to match the firmware tree. The same commit and tree print the same number. The same tree with a different commit is refused.

Bind each model after the package and controller hashes are known. Bind uses the same isolated record checkout:

```
python3 tools/release_alloc.py --repo . bind \
  --remote origin --branch release-records \
  --version <X.Y.Z> --model elite \
  --package-sha256 <64 hex> --controller-sha256 <64 hex> --controller-size <bytes>
```

Repeat with `--model pro`. The products must be `SMK-37 Elite_9XXYYZZ` and `SMK-37 Pro_9XXYYZZ` for that same `X.Y.Z`.

## Firmware commands

Use the pinned JieLi SDK and toolchain already qualified for this project. From a clean checkout of the branch head, read the issued ledger without resetting the source checkout. The copy in this repository stays empty.

```
git fetch origin release-records
git show FETCH_HEAD:releases/openbeta/ledger.json > /tmp/openbeta-ledger.json

python3 tools/dev_validate.py run \
  --commit <source SHA> --model elite \
  --firmware-version <X.Y.Z> \
  --release-ledger /tmp/openbeta-ledger.json \
  --plan <plan.json> --output <new-elite-dir>

python3 tools/dev_validate.py run \
  --commit <source SHA> --model pro \
  --firmware-version <X.Y.Z> \
  --release-ledger /tmp/openbeta-ledger.json \
  --plan <plan.json> --output <new-pro-dir>
```

`--firmware-version` stamps the numeric identity. It does not make the build a release and it does not test hardware. The on-device version string is `X.Y.Z`. The settings row shows `X.Y.Z_openbeta`. A version that is not allocated to this commit and `commit^{tree}` is refused. Bind both models after the package and controller hashes exist, then publish. Publish refuses a missing receipt and refuses to replace an existing catalog version with different source, package, or controller bytes. The same bytes may be published again.

Check both packages with `tools/firmware_build_labels.py --model elite|pro --source-commit <SHA> --expected-version <X.Y.Z>`. Refuse the pair if the models disagree, the loader is missing, or the hash does not match the record.

## Return

Return the source SHA, tree hash, allocated `X.Y.Z`, both product strings, both package SHA-256 values, both controller SHA-256 values, and the dev-validate record paths. Do not publish the site and do not push firmware to `nightly` or `main`.
