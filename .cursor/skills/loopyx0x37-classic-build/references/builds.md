# Builds on this tree

`BUILDING.md` is the command contract. Re-read it before compiling. This file is the decision procedure around those commands. Publication and hardware gates are in [stages.md](stages.md).

## What a Cursor run builds

Default to a development build (`X0X_DEV_PADCAL=1`). `tools/dev_validate.py` accepts `elite` and `pro`. FM-1 source is reference-only. Its target build is not a gate requirement. `MODEL=mke` exists for the MKE-P37 package. Include it when the user requests a release and `tools/build_smk_release.sh` builds it. A development validation run does not substitute that release script.

Run a firmware build when compiled inputs changed and the user asked for implementation or a build. Skip it for docs, skills, and browser-only diffs. Quickcheck treats `.cursor/skills/`, `.agents/skills/`, `skills/`, and `loopyx0x37-classic-build-skill.zip` as non-target scope for this reason.

Run `tools/gate.sh` only after the user explicitly requests a release build for this work. A generic request to build, validate, or install does not authorize the release gate. A `--trial` gate result does not permit install or release. Release packages use `X0X_DEV_PADCAL=0` and the release identity from `BUILDING.md`.

Development install evidence uses `tools/dev_validate.py` with a JSON plan: `base`, `scope`, and `checks` whose entries have `reason` and `argv`. Use `no_tests_reason` only when no host test applies. Review the plan against the diff. The runner cannot infer coverage. `DEV CHECKED` is local development evidence. It is not a release pass or a device pass.

## Before invoking the compiler

Compare the candidate's compiled inputs with the last verified build's inputs:

- Git blob or tree IDs of `firmware/`, `ble/`, and `assets/`
- `build.sh`, `tools/build.py`, and any generator, packager, or config the build reads
- executable modes of those scripts
- `JIELI_TOOLCHAIN` version and the Clang binary hash
- the three AC79 SDK files named in `BUILDING.md`
- DSP reference pins
- model, `X0X_DEV_PADCAL`, and other environment flags that change compiler output

`tools/fm1_install.py` and `tools/x0x_slots.py` are host installer files. Confirm the build's own input closure before treating the rest of `tools/` as non-compiled.

When every compiled input matches a build whose package bytes, SHA-256, CRC, and budgets are still on record, record `UNCHANGED_VALID_SCOPED` with the old and new root trees and the unchanged package hashes. Do not recompile. Do not call that reuse a new binary or full-root QA. A changed compiler input, or missing attested bytes, requires a new native build.

Host and installer checks still run when those files changed, including an executable-bit change.

## Commands

Linux x86-64, or the documented `linux/amd64` container on macOS. Set the paths the session actually has:

```bash
export CC=cc CXX=c++
export JIELI_TOOLCHAIN=/path/to/toolchain
export AC79_SDK=/path/to/fw-AC79_AIoT_SDK
export X0X_REFS=/path/to/validation_refs
export NINEW9_DIR="$X0X_REFS/schwung-9W9"
export EIGHTW8_DIR="$X0X_REFS/schwung-8W8"
export SCHWUNG303="$X0X_REFS/schwung-303"
export TB3PO_REF="$X0X_REFS/schwung-tb3po/src/dsp/tb3po.c"
```

`CC` is a compiler executable. A quoted flag string is not a compiler.

Scoped development checks:

```bash
bash tools/quickcheck.sh "$PWD"
```

Docs and skill-only checks can use the folder form. Commit-mode quickcheck needs `flock`. `X0X_QUICK_ONLY=ste` requests advisory STE plus rule guards. `X0X_STE_STRICT=1` requests the strict review. Either switch is a writing check. It is not a firmware result.

`./build.sh` with no board builds the FM-1 package `build/x0x.fwsc`. SMK development packages are:

| Command | Package | Development identity |
| --- | --- | --- |
| `BOARD=smk37 ./build.sh` | `build/x0x-smk37.fwsc` | `SMK-37 Pro_900` |
| `BOARD=smk37 MODEL=elite ./build.sh` | `build/x0x-smk37-elite.fwsc` | `SMK-37 Elite_900` |
| `BOARD=smk37 MODEL=mke ./build.sh` | `build/x0x-smk37-mke.fwsc` | `MKE-P37_900` |

`--release X.Y` changes those names and identities to the release form in `BUILDING.md`. Direct `build.sh` is a manual development compile. It is not the formal gate and it is not `tools/dev_validate.py`.

Development validation, one model at a time, after a plan exists. On macOS, add the `--worker-image` argument from `BUILDING.md`.

```bash
python3 tools/dev_validate.py run --commit <commit> --model elite \
  --plan /path/plan.json --output /path/new-dev-result
```

Repeat with `--model pro`. Elite writes `x0x-smk37-elite.fwsc`. Pro writes `x0x-smk37.fwsc`. The runner builds that model in development configuration, checks budgets, builds the controller, and records hashes. Failed checks issue no record.

Current SMK packages rewrite the controller block. Install with `--ctl` pointing at that artifact. An empty controller is refused. `--no-controller` and `--no-ctl` apply to a package that already embeds a valid controller, or to an old package whose application owns that flash range. `BUILDING.md` also contains a later paragraph that says to pass `--no-controller` whenever an install needs no controller. Follow this paragraph and `tools/dev_validate.py` for current SMK packages. The later paragraph describes the older exception.

Release gate, only after an explicit release request:

```bash
export X0X_REPO="$PWD"
export X0X_RESULTS="$PWD/build/gate-results"
export X0X_GATE_WORK="$PWD/build/gate-work"
export X0X_GATE_LOCK="$PWD/build/gate.lock"
export X0X_PREPUSH_HOOK="$PWD/tools/pre-push.hook"
bash tools/gate.sh <commit> --purpose release-install \
  --reason "Install release candidate <version/artifact>"
```

Keep gate work outside `/tmp`. Do not apply the 2026-10-23 replay patches onto a newer tree. See [provenance](provenance.md).

## What to record from a real build

For each model: command and exit, source commit and root tree, toolchain and SDK identities, package size and SHA-256, CRC, model metadata, app and loader sizes, code, static RAM, pool, and flash headroom, and the matching `ctl.img` size and hash when one was produced. Keep those binaries off the source branch. The assigned `gh-pages` publisher is the path that puts release packages on the site branch. Store the manifest in the pull request and, when the change already records evidence, under `evidence/` as hashes and logs only.

`tools/locked_settings.json` is a build-stopping regression guard. Change a locked value only together with the source, in a commit that names the setting, and update `BLE_ATTEMPTS.md` when the Bluetooth settings move.

## Labels

Use the glossary in [handoff.md](handoff.md). A compiler exit of 0 is `BUILD_SUCCEEDED_UNREVIEWED` until independent QA accepts that commit. `DEV CHECKED` is the `tools/dev_validate.py` record for those exact artifacts. It is local development evidence. It is not `QA_PASSED`, not a release gate, and not `DEVICE VERIFIED`. A successful compile stays a successful compile when QA or hosting is still pending. Hosting does not turn that exit into `BUILD_FAILED`. Neither label means the package is on the live installer.
