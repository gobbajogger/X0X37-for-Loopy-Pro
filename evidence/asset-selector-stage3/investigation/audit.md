# Independent investigation

Result: FAIL for production candidate `746ece6936a456ef963c758da659c07a801cec37`.
Production helper, build integration and regression sources remain byte-identical to that commit.

## I2: Native entry aliases lose their actual ORIGIN lookup precedence

`probe_python` adds native file paths from `/proc/self/maps` at `tools/asset_cache.py:423–426`.
For a shared library opened through a symlink, maps identifies the canonical target.
`loader_inputs` expands `$ORIGIN` using `path.parent` at line 280. The selected alias can have a different parent, and glibc uses that selected name to resolve the entry's relative RUNPATH. The inventory therefore covers the wrong earlier lookup directories. Recording the currently loaded library bytes alone does not cover a new higher-priority candidate.

`origin_earlier_probe.py` proves this with disposable host libraries and the production helper:

1. `alias/entry.so` is a symlink to `real/entry.so`. The entry has RUNPATH `$ORIGIN/early:$ORIGIN/search`. Both directory sets already exist. `alias/search/libtoken.so` returns 2; `real/search/libtoken.so` returns 1.
2. Cold generation returns `audio2fixture`. A separate warm process forbids dependency discovery and reports a hit. Thus the initial pipeline supports persistent reuse.
3. The inventory records `real/early` and its hardware paths, plus the currently loaded `alias/search` directory. It does not record `alias/early`.
4. The proof adds `alias/early/libtoken.so`, which returns 3. Existing source, selected entry alias, loaded library bytes and inventoried directory entries remain unchanged. Compiler scratch source is outside the inventoried source and lookup folders.
5. The next new process reports a cache hit and returns stale `audio2fixture`. The identical generation with `X0X_ASSET_CACHE=0` returns `audio3fixture`.

The stale header hash is `274ba96a799b1359e329df43a8317e9787accd855c5a62873ddf1c82a9d82382`; original generation produces `ae5a3fac1cbe9e75be3d164068c768b70883d06efebddf55b74228c09eac2828`.
`origin-earlier.log` includes the full observed inventory and actual/direct output. The initial failing observation remains in `origin-earlier-initial.log`. A second run on the same immutable source reproduces the failure with added diagnostics.

The related control `origin_alias_probe.py` replaces the already loaded alias-relative library bytes. That case correctly misses and matches original output (`origin-alias.log`). The new defect concerns an earlier candidate absent from the recorded closure.

Repair must retain the actual loader-selected spelling and its link chain when deriving ORIGIN search paths, including transitive native dependencies, or conservatively disable reuse when that selection cannot be established. Canonical maps paths alone are insufficient. Add the failing alias-relative earlier-candidate regression. Do not edit production in stage3 or advance this candidate.

## Saved mocked warm None observation

The historical `loader-focused.log` records `None` at the first mocked warm call of `test_discovery_reuse_checks_contents_without_restarting_tools`. The saved retry passed without a source change. Its cause remains unestablished.

The independent `focused_probe.py` runs the exact three-case sequence from that historical log, followed by three further first-case repetitions in one interpreter. It wraps successful `generate_asset` returns for diagnostics without changing production files. Six cases pass in 75.810 seconds; all 16 traced returns are booleans, including all mocked warm returns. The source's successful path returns `hit`, which is a boolean from the `entry is not None and restore(...)` expression and the real restore function. The source review found no successful implicit-None path. This does not explain or erase the historical observation. It is retained as nonreproduced, with its source log linked in `result.json`.

## Other reviewed contracts

Compiler requests retain unresolved aliases and are reselected with the execution cwd and ordered PATH. Parent and target symlinks, execute bits, GCC program/library/include paths and recursive include aliases are checked. Unknown compiler/helper scripts fall back. Snapshot memoization is local to one snapshot. Old resolution schema values rediscover.

Python interpreter bytes were already hashed through the unresolved path in the original candidate. That was not the original demonstrated defect. The repair preserves this check and adds resolution identity. Startup `.pth` and `.pkg` files, virtual-environment candidates and import directories are recorded; executable startup hooks and custom import hooks fall back.

Discovery records bind schema, signature and checksum. Malformed records cause rediscovery. Restore checks exact family output names and hashes the exact bytes it copies. Publication uses a private complete directory and rename. Postgeneration/postrestore input guards remain. These contracts and the public API are unchanged by the repair; the independent Test helper owns the final-source execution coverage.

The candidate changes only the host helper and its regression tests relative to the failed base. The reviewed production sources remain exact in this lane. No firmware, proprietary SDK/toolchain, generated binary, device, browser, full/release gate or external push work was performed. All new execution used the pinned Linux/amd64 image, user 501:20, network disabled, source mounted read-only and disposable fixtures. Native provenance repair and checked combination remain separate dependencies.
