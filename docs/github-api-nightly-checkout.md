# Exact X0X37 nightly source retrieval using GitHub REST

`tools/github_api_checkout.py` reconstructs a **clean, shallow Git checkout at the actual remote commit SHA and tree SHA**, even when native Git smart-HTTP transport is unavailable. It consumes the official GitHub Git Data REST API (commit, recursive tree, blobs). All blob Git hashes, declared sizes, root tree, original signed/unsigned commit object and final clean HEAD are verified before the checkout is exposed. The output is created atomically and only if its destination does not exist.

Use an ordinary native Linux environment able to reach `https://api.github.com`, with Python 3 and Git. The public repository can be read without an API token, but authenticated `GH_TOKEN`/`GITHUB_TOKEN` provides reliable rate limits; do not put tokens in commands/logs. Use a clean historic Git checkout as a blob cache so only changed remote blobs are downloaded. No GitHub-hosted runner, Codex, Work or remote mutation is involved. In chats, follow rule 1m: no git over the network; use the GitHub plugin or API.

```bash
python3 tools/github_api_checkout.py \
  --repo gobbajogger/X0X37-for-Loopy-Pro \
  --commit c9718ae833eb64a5191c350922593df8a36017c9 \
  --expected-tree 715fbd98d427c8ddd6b64d1b923e9d1fb8ae64ea \
  --verify-ref nightly \
  --seed /mnt/data/x0x37-source-audit/base \
  --output /mnt/data/x0x37-source-audit/nightly-verified
```

The hashes above are a historical example only. Before **every new** build, use fresh GitHub-connected remote SHA/tree reads and pass those full immutable values. `--verify-ref nightly` refuses if the branch moves during preflight; for an already integrated eligible SHA intentionally no longer at tip, omit it and record the current protected `nightly` ref separately. Do not invent or substitute a source commit from a historical bootstrap. A code change in the source requires a new source identity and native re-validation.

On success, independently verify `git -C OUTPUT rev-parse HEAD`, `git -C OUTPUT rev-parse HEAD^{tree}` and clean status against the pinned remote. Use this checkout for applicable native host checks and pinned SMK-37 Elite/Pro DEV firmware/controller builds, then validate actual packages/budgets and independent QA separately. A successful checkout is `EXACT_SOURCE_CHECKOUT`, never `DEV CHECKED`, installable, approved to distribute, HTTP live or hardware-tested. Preserve the existing public NIGHTLY website as browser/status-only. Do not upload SDK, DSP references, `.fwsc`, `ctl.img`, backups or secrets into it.

## Source-transfer boundaries

The current ChatGPT Linux sandbox has been observed to reject DNS for `api.github.com` and `github.com`, while the connected GitHub app can **read** remote Git metadata and blobs. The connected app does not automatically mount those returned bytes into a shell checkout. This CLI needs a network-capable **native** process or a separately approved, verifiable tool-mediated byte transfer; a GitHub API read in chat alone is not a build. A narrow `SOURCE_CHECKOUT_BLOCKED` diagnostic is required when the native API endpoint is unreachable. Don't report compiler/SDK absence when the actual error is DNS or old source.

Safety properties: HTTPS GitHub API host (loopback allowed for tests), zero unverified blob content, no submodules, no `.git`/path traversal, no escaping source symlinks, bounded transient retry, shallow commit with byte-for-byte original SHA, safe temporary output cleanup and no overwritten previous work. No source push, merge, firmware publication or flash is performed by this tool.

Regression: `python3 -m unittest discover -s tests/host -p test_github_api_checkout.py -v`. Tests run actual local Git, a loopback fake GitHub REST server, signed/unsigned exact commits, executable and symlink modes, binary blobs, seed-cache reuse, and source-corruption, traversal, branch-move and incomplete-response rejection. These mock API checks are not a live GitHub pull or native firmware build.
