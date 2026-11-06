# Native before/after plan — not executed

Remain in this environment. Do not use another executor, download path, identity,
token or proxy to route around the helper HTTP 401. Resume the blocked transfer
only after a supported authentication repair supplies materially new evidence.

Required dependency: the authenticated hosted-app session used by the official
Library download helper must authorize its tools/list request. The returned 401
does not name an ACL, account, OAuth scope or file permission. Those cannot be
inferred. Direct Library search and the earlier build-archive materialization
succeeded, so this is not proof that the owner lacks Library file access.

The current workspace contains no files named clang, llvm-objdump, uboot.boot,
cfg_tool.bin, eq_cfg_hw.bin, or either resolved input archive. The recursive scan
included ignored files and excluded credential/control directories; it had no
read errors. The archive's recorded /workspace/scratch/build025-inputs directory
does not exist here. Runtime status is current, network enforced/unrestricted,
with no configured secrets or outbound identities. No credentials were inspected.

## Input admission after legitimate authentication repair

1. Materialize the already resolved compiler and SDK archives with the current
   official Library flow. Verify archive SHA-256, CRC, paths, symlinks and executable
   modes. Do not treat filename or compiler version alone as provenance.
2. Compare the compiler's 643-entry manifest with the baseline's recorded manifest
   hash `0068281edc2dd32cf3d5ee4f3a5f2a1650195a2005b827e0889c7a9b54166ca4`.
   If that closure is unavailable, explicitly record it as unverified and require
   exact baseline reproduction before claiming an isolated target delta.
3. Verify every one of the 11 SDK entries against baseline/sdk-verification.json
   using its `path` and `expected` fields. Require V1.2.1; do not substitute V1.2.0.
4. Run `python3 tools/check_original_inputs.py --scope all` on both checkouts and
   compare the admitted manifest and inputs with baseline/original-input-check.json.
5. Use four clean isolated copies: before-elite, after-elite, before-pro, after-pro.
   Each begins at c295af1554f9e80a29864cd013995eccad66cd2c with tree
   d98072ba1d689cb99bd16e4a6e1e682b22b04c70. Apply only ram-recovery.patch to after
   copies using `git apply --check`, then `git apply`. Keep private inputs outside
   any deliverable. No commits or shared ref writes are needed.

## Exact build commands in each copy

Set JIELI_TOOLCHAIN and AC79_SDK to the admitted local directories, SDK_LOCAL to
that same SDK, and X0X_GEN_CACHE to an isolated per-copy directory. Record their
verified manifests without secret values. Then run (MODEL is elite or pro):

```sh
export X0X_DEV_PADCAL=1 BOARD=smk37 X0X_JOBS=4
bash build.sh --board smk37 --nightly-version 0.0.25 \
  --git-sha c295af1554f9e80a29864cd013995eccad66cd2c
```

The base SHA is build provenance only; record the patch SHA and candidate tree
separately so a dirty candidate is never described as the original commit's bytes.
Capture command, environment configuration, exit code and full log for each run.
Do not run a release gate, installer or controller/device operation.

## Measurements and acceptance

Preserve each build's x0x.elf, app.ld, x0x.bin, data.bin, ramtext.bin, c1text.bin and
symbol listing privately. Use actual ELF symbols/section headers, not log prose,
to calculate RAM and pool use:

```
RAM used  = _bss_end - 0x01c08000
RAM free  = 98304 - RAM used
POOL used = _pool_end - _pool_start
POOL free = 344064 - POOL used
```

Before builds must reproduce archive baseline app bytes and budgets: Elite app
494828 B, Pro 494812 B, RAM used 98276 B, pool used 335624 B. Compare exact binary
SHA-256 against the admitted baseline archive (not a retyped guessed digest).
If reproduction differs, resolve that difference before attributing a saving.

For after builds require disappearance of the 1024 B mono BSS object; measure
actual RAM delta including alignment; pool use unchanged and at least 8192 B
free; guard/stack/linker regions unchanged; .ram_text and .c1_text identical;
model/version unchanged; all original private input pins unchanged. Record code
delta and all warnings. Do not accept an expected 1052 B free without measuring.
Controller source and its 32 B free margin are unchanged and are not this patch's
RAM claim. No package publication follows from a successful build.

Independent QA must review lifetime across active/inactive 909, early synth
failure/return, split output windows, CPU2 completion and fallback, and downstream
FX overwrite. Consume the host normal/sanitized differential and unsafe-alias
negative. Host CPU2 scheduling is simulated; neither it nor native compilation
establishes device concurrency, cycles, stack high-water or hardware PASS.
