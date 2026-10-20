# Independent investigation

Result: FAIL for candidate `bb953dfaa086c44e57382e9d82fe9e0a2714f965`.

## I1: Compiler symlink retargeting permits stale generated output

`tools/asset_cache.py:333` resolves the selected compiler to its target. Lines 338–339 record the resolved target as a dependency. `verify_identity` (110–124) rehashes those saved files and directory inventories. `lookup_directories` (64–75) records entry names, without symlink targets. The original compiler lookup and link are not revalidated on an inventory hit (191–207, 403–409).

The proof copied the candidate tools, assets and firmware source to a disposable Linux directory. It set CC to a disposable symlink to `/usr/bin/gcc`, generated the real drum dependencies, and generated then warmed the real built-in break bank. A second wrapper already existed in the same compiler folder, so retargeting the CC link changed no directory entry names. The wrapper adds `-ffast-math` and records its invocation. After retargeting, the cache reported a hit, skipped that selected wrapper and returned the old header. The identical call with `X0X_ASSET_CACHE=0` invoked the wrapper and generated different bytes.

- Original and stale-hit SHA256: `54bd2932086d911ac12d4fa5b52b1950db09c23eaf6035bf2acc4893ecc99c56`.
- Disabled-cache SHA256 with the new selected compiler: `bfe99a56c3de8e350bdda36584022ee4f811b2908da9b75ad8645b7d5646b1d6`.
- Evidence: `compiler-symlink-proof.py`, `compiler-symlink-proof.log`.
- Impact: Persistent discovery can reuse break-bank output from a different effective compiler. This violates the required compiler dependency and generated-byte identity.
- Disposition: Do not advance this candidate. Use a fresh repair stage2, then fresh affected independent stage3 checks. Keep candidate source unchanged here.

The repair should preserve and revalidate effective command resolution, lookup candidates and link targets, as well as the actual selected bytes. This includes CC and the GCC compiler/assembler/linker pipeline. Unknown or changed resolution must cause rediscovery or conservative fallback. Do not weaken the content checks or depend on mtimes. Add the demonstrated retarget case plus an earlier-PATH candidate case and directly affected helper-resolution cases.

## Related source inferences

These are review leads, not additional proven defects:

- CC lookup uses `shutil.which` (331–333), but the inventory does not retain its ordered PATH resolution. A pre-existing earlier PATH directory can gain a compiler without changing the already selected target's parent. The environment string can remain unchanged. The proof above demonstrates the related symlink case only.
- Compiler/assembler/collect2 executables (354) and the linker (366–367) are likewise resolved before storage. Their selected lookup paths and link chains need the same repair review. Exact helper retarget behavior was not executed.
- ELF libraries are discovered from `/proc/self/maps` (273–276) or `ldd` (306–313); loader configuration, cache and RPATH/RUNPATH lookup directories are included (127–188). Those inventories preserve loaded bytes and names, but native mapping changes through links or nested lookup paths deserve targeted review. No native-loader stale-output claim is made here.
- Python's `sys.executable` is retained without `.resolve()` (234), so its current file bytes are rehashed through its path. It does not directly share the confirmed CC path-loss bug. A repair should preserve this protection and review interpreter lookup identity if changing the shared resolver code.

## Reviewed boundaries

- Font keys include the generator, ordered font search configuration, selected files, all explicit font candidates, Pillow package files, imported Python files and loaded native files (225–299). BDF and variable-font configuration are contained in the generator bytes. Package enumeration uses the PIL folder, rather than the separate validator's incomplete distribution metadata path.
- Drums include all declared source samples, helper bytes and imported runtime/native dependencies. Custom breaks include the selected bank manifest, referenced WAV files and folder-name semantics. X0X_BREAK_BANK is represented in the discovery signature and bank-specific identity despite not being in the general environment filter.
- Built-in breaks use transitive `-M` dependencies, pipeline commands, external specs, a real linker trace, and linked runtime libraries (322–402). The tiny probe avoids a redundant DSP compile during discovery. Unknown pipelines fall back; the confirmed defect is reuse of an obsolete discovery resolution.
- Environment identity covers Python, loader, locale, font, compiler, QEMU and glibc tunables plus explicit configuration variables (103–107). CPU flags/model data and platform data are recorded. The remaining command-resolution defect is not repaired by stable environment strings.
- Discovery records bind schema, signature and a checksum before reuse; malformed data rediscoveries are conservative. Entry restoration checks the key, exact output names and hashes of the exact bytes copied (426–445). Both drum outputs are covered.
- Publication writes a private directory and renames a complete entry (448–462). Generation failures and signals occur before publication or destination replacement. Inputs are rechecked after generation or restoration (520–535). A corrupt existing entry remains unusable and causes regeneration; publication does not silently replace it in place.
- Unsupported discovery and cache I/O errors use original generation. Fallback source snapshots detect source changes, and the fallback shell renderer uses a per-worktree lock (465–534). A fallback is not claimed to have full unknown-runtime provenance or cache reuse.
- tools/build.py waits for both font/drum jobs and raises before breaks on any error. The source change is confined to the specified helper and build call site; no host/reference/browser consumers were routed here.
- The inspected candidate diff adds the helper, focused tests, asset documentation and text evidence. No SDK/toolchain or newly generated binary artifact was introduced. Candidate production and test sources remain unchanged in this stage.

## Limits

This investigation is a source review plus one targeted real-generator proof. It does not duplicate the separate test helper's broad cache execution scope or stage2's matching evidence. It ran no firmware build, full/release gate, device operation, external push or shared package mutation. The compiler proof used the pinned Linux/amd64 image with user 501:20 and network disabled. The entire candidate mount was read-only; all source copies and compiler changes were inside the disposable container.
