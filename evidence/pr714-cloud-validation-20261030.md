# PR #714 cloud validation evidence (2026-10-30)

**Candidate:** `wip/mixer-ux-one-build@11a7bbe680dc111948e36ae7de4312d6dabc4662`
**Integrated base:** `wip/one-build@cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`
**Host:** Linux x86_64. **Result: PARTIAL, NOT a completed full PR validation.**

## Verified scoped regression results
The companion log `pr714-smc-scoped-host-20261030.log` records **18 successfully executed native test binaries**, three SMC test families under GCC and Clang, each in standard, UBSan, and ASan+UBSan mode:
- SMC mixer edges: 1,281 assertions per configuration.
- FX captions: 148 assertions per configuration.
- Four extracted production SMC action/UI functions: 406 assertions per configuration.
- **Total: 11,010 passing assertions.**

The eight test/header/map files in the log match exact GitHub blob SHA-1 identifiers on the candidate. The action/UI function test used a *scoped projection* of four production functions fetched from GitHub `firmware/src/app/smc_mixer.c` (blob `3981ddf907613f7b3b1ad4c0dd959ef50b264ac2`) and the exact current test harness. This is not a complete firmware translation-unit build.

## Complete source and full-suite gate
GitHub's authenticated API returned the candidate's complete **tree metadata** (4,590 entries, including 4,231 blobs; `truncated=false`). It did **not** transfer all corresponding file bytes into the local build environment. The connected GitHub fetch endpoint rejected the commit zipball route (HTTP 400, unsupported endpoint), and the container download could not resolve that URL through its approved viewed-file route. A recovered complete historical source bundle has commit `6ae0928fe3264ee759c25c4f879eff4dc5dc3b20`, **not the candidate**.

- Full candidate `sh tests/run_tests.sh`: **NOT_RUN**. No complete authenticated local checkout of the candidate.
- Candidate Elite development firmware build: **NOT_RUN** for the same reason.
- Candidate Pro development firmware build: **NOT_RUN** for the same reason.
- Exact musical golden/bit-exact audio and real hardware: **NOT_RUN**.

The historical `6ae0928` full host suite was started as an environment check but exceeded its run window and has **no complete PASS result**. Do not transfer its partial results to this candidate.

## SDK and native compiler preflight (historical source only)
The owner's offline private input cache has SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`; the 11 pinned AC79NN SDK V1.2.1 files passed `kit.py verify`. The JieLi toolchain ZIP has SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`, and its native pi32v2 Clang 4.0.1 executed.

To prove this host can perform real native builds, on **historical commit `6ae0928` only**, using `X0X_ORIGINAL_SYNTH=1 X0X_DEV_PADCAL=1` with the pinned SDK/toolchain environment:
- `./build.sh --board smk37 --model elite`: exit 0, Elite DEV package produced, app 463,448 bytes; RAM 95,408/98,304; pool 324,212/344,064.
- `./build.sh --board smk37 --model pro`: exit 0, Pro DEV package produced, app 463,432 bytes; RAM 95,408/98,304; pool 324,212/344,064.

**Neither historical build qualifies PR #714.** The temporary packages, vendor SDK/toolchain, private headers, and binaries remain outside Git. Only this source-neutral text evidence and the scoped host log are committed.

## Required next acceptance
Recover all candidate file bytes to an authenticated exact-source checkout using the connected API or an equivalent approved source archive, verify the local tree identity against commit `11a7bbe6`, then run the FULL host suite and both native Elite/Pro DEV builds on that exact tree. Fix concrete failures before changing the PR from draft or claiming a complete result. No device action, release gate, or publication is requested.
