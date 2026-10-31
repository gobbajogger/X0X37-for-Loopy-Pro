Publish only with current action-specific authority through the assigned publisher under [the active workflow](docs/rules/workflow.md). Reuse applicable standing permission; a build or source push does not grant publication or device access.

# Building X0X

Default to development builds and installs (`X0X_DEV_PADCAL=1`). Build a release
configuration only after the user explicitly requests a release build for the
current work. Until then, always skip the full gate, including trial runs. See
[the active release boundary](docs/rules/workflow.md).

Maintained targets are SMK-37 Elite and Pro. The gate builds and tests both in dev
and release configurations. FM-1 source is reference-only for porting; its target
builds, target tests, and memory budgets are not gate requirements. Shared tools
with `fm1` names still serve SMK-37. Historical FM-1 instructions below are reference material.

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `x0x.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `x0x.fwsc` | the installable package (app + loader) |

## Prerequisites (macOS)

- Python 3 with Pillow: `pip3 install Pillow` (Cloud Agent / host bootstrap: `bash tools/agent_env_install.sh`, also the environment `install` in `.cursor/environment.json`)
- Docker Desktop. The JieLi toolchain is Linux x86-64 only. The build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder that Docker can share, for example under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`). They are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).

On Linux x86-64, the toolchain runs natively and you do not need Docker.

## Agent validation and completion

Build and test on Linux x86-64 or in a local Linux container. Use the same pinned SDK, toolchain and DSP references in either one.
The full gate also needs Bash, Python with Pillow, Node, `flock`, `setsid`, `getent`,
and Linux signal support. macOS can run docs checks and supported host checks.
Run Linux-specific checks in the local container. Docker Desktop and Colima both work.

Install Pillow in the agent environment when you can (`bash tools/agent_env_install.sh`).
Missing optional host tooling (Pillow, ASan, missing reference repos) does not stop
independent ready work. Its dependent checks stay NOT_RUN; a missing required check
blocks only its dependent qualification, merge or publication claim.
A generation step that actually ran and failed still fails the suite.

For docs and rules work, read current `AGENTS.md` and the selected maintained
workflow. Do not require removed preference files. Run the affected checks without
a firmware build:

```
X0X_QUICK_ONLY=ste X0X_QC_BASE=<base-commit> bash tools/quickcheck.sh "$PWD"
```

A worktree-folder docs check does not use the shared quickcheck cache. Commit-mode
quickcheck needs `flock`. Use the Linux container when the host lacks it.

Grammar findings are review hints. `X0X_STE_STRICT=1` requests a strict review.
The old `tools/ste_strict` marker does not force every task to pass grammar
heuristics. Current owner rule pointers and commit evidence still fail closed.

For Linux build checks, set paths in the environment that runs the commands:

```
export CC=cc CXX=c++
export JIELI_TOOLCHAIN=/path/to/toolchain
export AC79_SDK=/path/to/fw-AC79_AIoT_SDK
export X0X_REFS=/path/to/validation_refs
export NINEW9_DIR="$X0X_REFS/schwung-9W9"
export EIGHTW8_DIR="$X0X_REFS/schwung-8W8"
export SCHWUNG303="$X0X_REFS/schwung-303"
export TB3PO_REF="$X0X_REFS/schwung-tb3po/src/dsp/tb3po.c"
bash tools/quickcheck.sh "$PWD"
```

Use the actual reference directory names. The release gate pins are 9W9 `10cbe5c`,
8W8 `94aa271`, 303 `7680578` and TB3PO `f49d81d`. A quoted compiler value such
as `CC='clang -U__APPLE__'` is not a compiler executable for these runners. Put
compiler options in the runner's supported flags.

Use checks scoped to the change and development validation by default.
Only after an explicit user request for a release build may the full gate run
under [the active release boundary](docs/rules/workflow.md). Generic full-validation, build or
install requests do not authorize a release build. Reuse matching evidence.

For a development install, prepare a JSON plan with `base` (the reviewed base
commit), `scope` (what changed and which consumers can be affected), and `checks`
(entries with `reason` and `argv`, an array of command arguments). Use
`no_tests_reason` only when no host test applies. Review the plan against the diff;
the runner cannot infer complete test coverage. Checks run in a private worktree.
For example, a change to install evidence can select
`{"reason":"changed install evidence contract","argv":["sh","tests/host/test_install_gate_guard.sh"]}`.

```
python3 tools/dev_validate.py run --commit <commit> --model elite --plan /path/plan.json --output /path/new-dev-result
python3 tools/fm1_install.py /path/new-dev-result/x0x-smk37-elite.fwsc --commit <commit> --config dev --dev-record /path/new-dev-result/record.json --ctl /path/new-dev-result/ctl.img
```

Use `--model pro` for Pro (package `x0x-smk37.fwsc`). On macOS, use native orchestration with the local Linux/amd64 validation image:

```
export JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain
export AC79_SDK=/Users/mac/fw-AC79_AIoT_SDK
export CC=cc CXX=c++
python3 -B tools/dev_validate.py run --commit COMMIT --model elite \
  --plan dev-plan.json --output build/dev-check-result \
  --worker-image x0x37-validation:local
```

Set reference paths and check flags for the selected plan. Each dependency path
must exist on macOS and resolve to the same bytes in the worker. The runner mounts
source, shared Git metadata, scratch, SDK, toolchain, references, plan dependencies,
and absolute check inputs at their actual paths. Use `--worker-mount PATH` for
another read-only input. Every extra consumed input must also be declared in
`plan.dependencies`; a mount alone does not declare an input. Docker must share
these paths. Keep paths outside system temporary folders when the Docker VM cannot access those folders.

The host hashes every SDK, toolchain, and reference input before and after the run.
The worker hashes its actual tools and Python packages. The normalized plan records
the immutable image ID, Linux platform/kernel, effective environment, mounts, and
host Docker/Python/Git bytes. Image changes invalidate the input identity. Build,
font/asset generation, controller, and selected checks all run in that same worker.
The runner removes only its own container before it removes the scratch checkout.
If container cleanup fails, the runner retains the checkout and refuses a record.

The existing Linux x86-64 path remains available without `--worker-image`.
Verification and installation also work on macOS. Plan commands must be synchronous
processes, without detached jobs or nested containers. The runner builds only the
selected model in development configuration. It checks code/RAM/pool budgets,
builds the controller, and records selected test logs and artifact hashes. Current SMK
packages always rewrite the controller block. Validate a controller and install
with `--ctl` pointing to that exact artifact. `--no-controller` / `--no-ctl` are
not a preservation path: an empty current package is refused. They remain usable
only for a package that already embeds a valid controller or an old package whose
application owns that flash range.
Reuse the record for the same source tree and exact artifacts; no repeat gate or
validation run is needed for another install of them. Failed checks issue no
record. DEV CHECKED is local development evidence, not release or device PASS.
The installer still applies its queue, model, bootloader, lock, and data guards.
Release installs continue to use full gate evidence without `--dev-record`.

A gate needs explicit local state paths when the old `/workspace` layout is absent:

```
export X0X_REPO="$PWD"
export X0X_RESULTS="$PWD/build/gate-results"
export X0X_GATE_WORK="$PWD/build/gate-work"
export X0X_GATE_LOCK="$PWD/build/gate.lock"
export X0X_PREPUSH_HOOK="$PWD/tools/pre-push.hook"
bash tools/gate.sh <commit>
```

Keep the gate work path outside `/tmp`. In a container, mount source, references,
SDK and toolchain where these paths resolve. A Git worktree records the path to
its shared repository. Mount the workspace at the same absolute path, or make an
independent clone inside the container with `git clone --no-local`. The approved project image is
`x0x37-validation:local`; its availability is local setup evidence, not a gate PASS.
A lane commit needs `--trial`; its result does not permit install or release.

For a local Git server, set `x0x37.allowedRemote` to the approved local repository.
Set `x0x37.allowedRoot` to its local root. These settings also apply to an installed
pre-push hook. Set
`x0x37.prepushCheck` to the local `tools/prepush-check.sh` and `x0x37.skipLog`
to a writable local log. `X0X_PREPUSH_CHECK` and `X0X_QUICKCHECK_SKIP_LOG` override
the latter paths for a run. Server hooks also support `x0x37.quickcheck`,
`x0x37.gateLock`, `x0x37.workDir` and `x0x37.skipLog`.
For local status and hook setup, use the same state paths:

```
export X0X_RESULTS_DIR="$X0X_RESULTS"
export X0X_PREFS_LINT="$PWD/tools/prefs_lint.py"
X0X_REPO="$(git rev-parse --absolute-git-dir)" bash tools/status.sh --out "$PWD/build/status"
X0X_GATE_DEST=/path/to/local-gate X0X_GATE_LOCK="$X0X_GATE_LOCK" bash tools/install_gate.sh
bash tools/install_server_hooks.sh /path/to/approved-local-bare.git
X0X_RESULTS_DIR="$X0X_RESULTS" bash tools/require_gate_pass.sh <commit> --config rel --fwsc /path/to/package.fwsc
```

Create the gate destination first. Use one lock path across local gate runners and
hook installation. `status.sh` needs the Linux tools; its old roster and STALL
labels are diagnostic hints, not task completion gates. Configure the actual current
docs branch through `X0X_DOCS_BRANCH`. For a hook in a local bare server, configure
its quickcheck, work, lock and skip-log paths before receiving integration pushes.

Scratch gate and quickcheck clones keep external push URLs disabled. This local
validation guard does not prohibit separately authorized source delivery from an
owned lane. An actual installed hook or policy refusal remains binding; do not
remove it or switch routes to bypass it. See [connected source delivery](docs/connected-git-source-delivery.md).

Report the completed local stage and its affected checks, then continue the next
authorized delivery action under the existing owner. Local proof alone does not
close a requested remote delivery. Record missing dependencies and unrun checks;
continue independent ready work. An unavailable device does not reopen completed
source-only work or block unrelated delivery. Development installs need scoped validation; release installs need
the full gate. Exact model/package checks, flash and RAM budgets, slot backup
and readback, and device evidence still apply at their respective steps. A selected test pass is not a full gate or a hardware PASS.

## Locked settings (regression guard)

`tools/locked_settings.json` lists the hardware-relevant settings. They are the Bluetooth TX level and
crystal trim, advertising, ATT MTU, no connection parameter request and the controller config. They are also
the RAM canary default, the pool limit, the sample slots and the flash map.
`tools/check_locked_settings.py` compares the source with it.
`tools/build.py`, `ble/ctl/build_ctl.sh` and `tests/run_tests.sh` run it, and a difference stops the build.
To change a value, change the source and the JSON in one commit that names the setting. Add a
BLE_ATTEMPTS.md row.

## Agent validation and completion

Build and test on Linux x86-64 or in a local Linux container. Use the same pinned SDK, toolchain and DSP references in either one.
The full gate also needs Bash, Python with Pillow, Node, `flock`, `setsid`, `getent`,
and Linux signal support. macOS can run docs checks and supported host checks.
Run Linux-specific checks in the local container. Docker Desktop and Colima both work.

Gate reuse requires a content fingerprint of the source, effective configuration,
external gate runner, toolchain, SDK, reference contents/pins, host tools and Python
dependencies. The runner checks it again before recording PASS. Older evidence
without this identity cannot be reused. A changed identity
alone does not request a gate: the release/owner-purpose rule still applies.

Development records use schema 2 and the same provenance format. Put external check
inputs in the plan's `dependencies` path list; list additional Python distributions
in `X0X_VALIDATION_PYTHON_PACKAGES` (Pillow and NumPy are captured by default).
Installed distributions use their file manifests. If a distro omits the manifest,
capture hashes the source, extensions and data under its declared `top_level.txt`
modules. Missing, empty or unsafe installed-module coverage stops validation.
Gate logs are retained per run. Keep `validation-inputs.json` with transferred gate
logs; `sync_results_to_mac.sh` includes it. Install consumers verify the recorded
identity and exact artifacts without needing the producer's SDK on the installer.

For docs and rules work, read current `AGENTS.md` and the selected maintained
workflow. Do not require removed preference files. Run the affected checks without
a firmware build:

```
X0X_QUICK_ONLY=ste X0X_QC_BASE=<base-commit> bash tools/quickcheck.sh "$PWD"
```

A worktree-folder docs check does not use the shared quickcheck cache. Commit-mode
quickcheck needs `flock`. Use the Linux container when the host lacks it.

Grammar findings are review hints. `X0X_STE_STRICT=1` requests a strict review.
The old `tools/ste_strict` marker does not force every task to pass grammar
heuristics. Current owner rule pointers and commit evidence still fail closed.

For Linux build checks, set paths in the environment that runs the commands:

```
export CC=cc CXX=c++
export JIELI_TOOLCHAIN=/path/to/toolchain
export AC79_SDK=/path/to/fw-AC79_AIoT_SDK
export X0X_REFS=/path/to/validation_refs
export NINEW9_DIR="$X0X_REFS/schwung-9W9"
export EIGHTW8_DIR="$X0X_REFS/schwung-8W8"
export SCHWUNG303="$X0X_REFS/schwung-303"
export TB3PO_REF="$X0X_REFS/schwung-tb3po/src/dsp/tb3po.c"
bash tools/quickcheck.sh "$PWD"
```

Use the actual reference directory names. The release gate pins are 9W9 `10cbe5c`,
8W8 `94aa271`, 303 `7680578` and TB3PO `f49d81d`. A quoted compiler value such
as `CC='clang -U__APPLE__'` is not a compiler executable for these runners. Put
compiler options in the runner's supported flags.

Use checks scoped to the change and development validation by default.
Only after an explicit user request for a release build may the full gate run
under [the active release boundary](docs/rules/workflow.md). Generic full-validation, build or
install requests do not authorize a release build. Reuse matching evidence.

For a development install, prepare a JSON plan with `base` (the reviewed base
commit), `scope` (what changed and which consumers can be affected), and `checks`
(entries with `reason` and `argv`, an array of command arguments). Use
`no_tests_reason` only when no host test applies. Review the plan against the diff;
the runner cannot infer complete test coverage. Checks run in a private worktree.
For example, a change to install evidence can select
`{"reason":"changed install evidence contract","argv":["sh","tests/host/test_install_gate_guard.sh"]}`.

```
python3 tools/dev_validate.py run --commit <commit> --model elite --plan /path/plan.json --output /path/new-dev-result
python3 tools/fm1_install.py /path/new-dev-result/x0x-smk37-elite.fwsc --commit <commit> --config dev --dev-record /path/new-dev-result/record.json --ctl /path/new-dev-result/ctl.img
```

Use `--model pro` for Pro (package `x0x-smk37.fwsc`). Run validation inside Linux x86-64 (a local container is supported); verification
and installation also work on macOS. Plan commands must be synchronous native
processes, without detached jobs or nested containers. The runner builds only that
model in development configuration, checks code/RAM/pool budgets, builds the
controller, and records selected test logs and artifact hashes. Current SMK
packages rewrite the controller block: validate and supply the exact matching
controller with `--ctl`. `--no-controller` / `--no-ctl` apply only to a package that
already embeds a valid controller or an old package whose application owns that
range. An empty current package or unchecked cached controller is not accepted.
Reuse the record for the same source tree and exact artifacts; no repeat gate or
validation run is needed for another install of them. Failed checks issue no
record. DEV CHECKED is local development evidence, not release or device PASS.
The installer still applies its queue, model, bootloader, lock, and data guards.
Release installs continue to use full gate evidence without `--dev-record`.

A gate needs explicit local state paths when the old `/workspace` layout is absent:

```
export X0X_REPO="$PWD"
export X0X_RESULTS="$PWD/build/gate-results"
export X0X_GATE_WORK="$PWD/build/gate-work"
export X0X_GATE_LOCK="$PWD/build/gate.lock"
export X0X_PREPUSH_HOOK="$PWD/tools/pre-push.hook"
bash tools/gate.sh <commit> --purpose release-install --reason "Install release candidate <version/artifact>"
```

Keep the gate work path outside `/tmp`. In a container, mount source, references,
SDK and toolchain where these paths resolve. A Git worktree records the path to
its shared repository. Mount the workspace at the same absolute path, or make an
independent clone inside the container with `git clone --no-local`. The approved project image is
`x0x37-validation:local`; its availability is local setup evidence, not a gate PASS.
A lane commit needs `--trial`; its result does not permit install or release.

For a local Git server, set `x0x37.allowedRemote` to the approved local repository.
Set `x0x37.allowedRoot` to its local root. These settings also apply to an installed
pre-push hook. Set
`x0x37.prepushCheck` to the local `tools/prepush-check.sh` and `x0x37.skipLog`
to a writable local log. `X0X_PREPUSH_CHECK` and `X0X_QUICKCHECK_SKIP_LOG` override
the latter paths for a run. Server hooks also support `x0x37.quickcheck`,
`x0x37.gateLock`, `x0x37.workDir` and `x0x37.skipLog`.
For local status and hook setup, use the same state paths:

```
export X0X_RESULTS_DIR="$X0X_RESULTS"
export X0X_PREFS_LINT="$PWD/tools/prefs_lint.py"
X0X_REPO="$(git rev-parse --absolute-git-dir)" bash tools/status.sh --out "$PWD/build/status"
X0X_GATE_DEST=/path/to/local-gate X0X_GATE_LOCK="$X0X_GATE_LOCK" bash tools/install_gate.sh
bash tools/install_server_hooks.sh /path/to/approved-local-bare.git
X0X_RESULTS_DIR="$X0X_RESULTS" bash tools/require_gate_pass.sh <commit> --config rel --fwsc /path/to/package.fwsc
```

Create the gate destination first. Use one lock path across local gate runners and
hook installation. `status.sh` needs the Linux tools; its old roster and STALL
labels are diagnostic hints, not task completion gates. Configure the actual current
docs branch through `X0X_DOCS_BRANCH`. For a hook in a local bare server, configure
its quickcheck, work, lock and skip-log paths before receiving integration pushes.

Scratch gate and quickcheck clones keep external push URLs disabled. This local
validation guard does not prohibit separately authorized source delivery from an
owned lane. An actual installed hook or policy refusal remains binding; do not
remove it or switch routes to bypass it. See [connected source delivery](docs/connected-git-source-delivery.md).

Report the completed local stage and its affected checks, then continue the next
authorized delivery action under the existing owner. Local proof alone does not
close a requested remote delivery. Record missing dependencies and unrun checks;
continue independent ready work. An unavailable device does not reopen completed
source-only work or block unrelated delivery. Development installs need scoped validation; release installs need
the full gate. Exact model/package checks, flash and RAM budgets, slot backup
and readback, and device evidence still apply at their respective steps. A selected test pass is not a full gate or a hardware PASS.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` change the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 0.1-beta` makes a release build. The package is `build/x0x-0.1-beta.fwsc`.

On macOS with podman instead of Docker, put a `docker` script first on your PATH. The script runs
`exec podman "$@"`. `X0X_JOBS` (default 4) limits parallel compiles: a podman machine drops
connections when many containers start at once.

Build option: `X0X_CDC=1` adds Felucca's USB serial function (off by default: one plain
MIDI interface).

Build option: `X0X_DEV_PADCAL` (SMK-37). `1` (the default) makes a dev build with the pad calibration
mode (BEHAVIOR.md CLP-79). `0` makes a release build without it: no PAD COLOURS row, no pad calibration
code or RAM, and no INLOG args 2-5 export. The pads use the owner-calibrated table `APC40_PAD_OWNER`
(BEHAVIOR.md CLP-13) in the two builds. To make the release build, use this command:

```
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
```

To get the calibration mode again, build with `X0X_DEV_PADCAL=1` (or with no value). The tag `padcal-v1`
marks the complete calibration mode (CLP-79, orientation page, reopen fix, ok dot).

The build also generates `build/gen/`. It contains the font and the 909's cymbal samples (IMA ADPCM,
44.1 kHz) and tables (`tools/gen_drum_samples.py`). The default app image does not compile in factory
break loops. The designated CRAMEN, TASTY and HOTPANT stock loops are IMA ADPCM at 44.1 kHz,
with retained one-bar metadata and a 131,152-byte pack. Their complete lengths and conversion
provenance are in `assets/factory-breaks/provenance.json`. `tools/export_factory_breaks.sh` exports
the same stock bytes for the installer. `tools/package_stock_loops.py` or the installer can place
them in the free app-code tail without recompiling. `X0X_FACTORY_BREAKS=1` explicitly compiles
these stock bytes into the image. `X0X_BREAK_BANK=DIR` uses another bank instead (`tools/gen_break_bank.py`).
`tools/import_breaks.py` makes such a folder from BB Gen's samples, in `build/breaks/` (README.md, Breaks).
The build fails if it links a soft-double routine (a `double` crept in). It also checks the image, RAM
and pool sizes.

### M-Vave SMK-37 Pro

```
BOARD=smk37 ./build.sh          # or: ./build.sh --board smk37  ->  build/x0x-smk37.fwsc
```

This build uses the same sources for the M-Vave SMK-37 Pro (the same AC791N chip, SPL and update loader,
but a different panel). Each difference is behind `BOARD_SMK37` (C, default 0). Only this build sets it
(`-DBOARD_SMK37=1`, for the app and the update loader). The plain `./build.sh` does not change. Its
`build/x0x.bin`, `build/loader/ota.bin` and `build/x0x.fwsc` stay byte for byte as before the SMK port.
The SMK build keeps its objects, app and loader in `build/smk37/`. It writes the package
`build/x0x-smk37.fwsc` (identity `SMK-37 Pro_900`). `--release X.Y[.Z]` gives `x0x-smk37-X.Y.Z.fwsc`
(`SMK-37 Pro_9XXYYZZ`). The package identity is only that number. An open-beta build also shows
`X.Y.Z_openbeta` beside ABOUT X0X; those words are not part of the identity.

`MODEL=elite` or `MODEL=mke` builds the same firmware under the model name of the SMK-37 Elite or the
MKE-P37. The identity is `SMK-37 Elite_900` / `MKE-P37_900`. The objects go in `build/smk37-elite/` /
`build/smk37-mke/`. The packages are `build/x0x-smk37-elite.fwsc` / `build/x0x-smk37-mke.fwsc`. Thus
M-Vave's updater still takes the factory package of that unit. The Starrykey 37 Play uses the Pro build.

The SMK build differs from the FM-1 build in these items:

- The flash map (`firmware/hal/fm1_flash_map.h`): an explicit allow-list with compile-time checks. No
  window reaches the head (0x0-0x3FFF), BTIF / USRTRIM (0xC0000-0xC1FFF, the factory key and pad
  calibration), the stock resources (0xF4000-0xFDFFF) or key_mac (0xFF000-). The RAM flash driver has a
  second guard.
- The store (`storage.c`): seven objects A/B at 0x9D000-0xAAFFF, boot screen copies at 0xAB000 and 0xAC000, spare 0xAD000-0xAEFFF, the store owner tag at
  0xAF000. The update staging at 0xBB000 (`ota.c`). Two user sample slots (`app/slots.c`): slot 1 80 KiB at
  0xC2000 and slot 2 120 KiB at 0xD6000 (to 0xF3FFF). The 44 KiB window at 0xB0000 is not a sample slot.
- The Bluetooth controller block: 64 KiB at flash 0x8B000-0x9AFFF (XIP 0x02087000), at the top of the
  encrypted app slot. The app code ends below it: `APP_CODE_MAX` 0x86EE0 (552,672 B). `build.py` links the
  app to that length and stops when the image is larger.
- The package layout (`tools/fm1pkg_make.py --board smk37`): PRCT 0x9D000, the stock region table and app
  slot 0x97400 (each board sets the XIP length of the linker script). It also has the slot map record at
  flash.bin 0x9CF00 ("X0X-SLOTMAP-1", per slot offset and size, CRC-32). The installers compare it with the
  slots on the device. They save and restore the slots that an install can lose.
- The input HAL (`firmware/hal/smk_input.h`), the LCD pins and the pad LEDs (`fm1_lcd_hw.h`, `lcd.c`), and
  the control mapping (`app/panel.c`, `app/plat_fm1.c`, `app/main_fm1.c`). X0X's UI (`app/ui.c`) does not
  change.
- No TRS MIDI IN (`X0X_TRS` is 0: PH8 is a key-matrix row on the SMK).
- The LCD's SPI clock is 12 MHz, the rate of the stock firmware. `X0X_SMK_LCD_BAUD=n` tries 60 MHz / (n + 1).
  The FM-1 build runs it at 60 MHz.

Store owner: several FM-1 firmwares have ports to the SMK-37 Pro with the same board layer and the same
store area. The SMK build tags the store with its family (`FELUCCA_DATA_FAMILY`, default `X0X`). At boot,
X0X clears a store with the tag of another firmware, or with no tag (`FELUCCA_DATA_UNTAGGED_OK`, default
`0`), before it reads it. X0X never touches the samples, the calibration, the Bluetooth pairing or key_mac.

The tests (below) include the SMK flash map, the store, the package layout and the update path on
SMK-layout packages. They check `build/x0x-smk37.fwsc` when it exists.

## The Bluetooth controller image (SMK-37)

Qualified NIGHTLY packages include their matching Bluetooth controller. See
[bundled NIGHTLY packaging](docs/bundled-nightly-packages.md) for the checked composition
command and catalog contract. Publication depends on the applicable source, component terms,
QA and publication decisions; this source change does not authorize distribution.

The existing unbundled package path remains available. To build a controller locally, use
the toolchain above and Docker (or Linux x86-64 with `CTL_NATIVE=1`):

```
ble/ctl/build_ctl.sh        # fetches JieLi's files (pinned, SHA-256 checked), builds ble/ctl/out/ctl.img
python3 tools/add_ctl.py build/x0x-smk37.fwsc ble/ctl/out/ctl.img -o build/x0x-smk37-ble.fwsc
python3 tools/add_ctl.py ble/ctl/out/ctl.img --check
```

`build_ctl.sh` also copies the image to `~/.config/x0x/ctl.img`, but the installer
does not select that cache implicitly. Give the exact validated image with
`tools/fm1_install.py --ctl FILE` when installing an unbundled current SMK-37 package.
The installer refuses an empty controller block. The legacy web path has a file field for
the matching image; a qualified bundled NIGHTLY package needs no separate selection.
See ble/README.md for how the image and the app fit together.

## Tests

```
tests/run_tests.sh
```

The script runs these tests on the build machine:

- The maths library against libm.
- The sequencer and TB-3PO, against schwung-tb3po (from `../schwung-tb3po` or `TB3PO_REF`).
- It compares each engine with its original (9W9, 8W8, schwung-303, BB Gen). The scripts in `tests/host/` give
  the location of those sources.
- Flash storage.
- Felucca's update-path tests against `build/x0x.fwsc` (the loader test needs `AC79_SDK`).
- The Bluetooth mailbox, BLE-MIDI host, image check and package insertion (`tests/ble_test.c`,
  `tests/ble_pkg_test.py`).
- It runs the whole app in the simulator (`tests/scenarios/*.x0x`) and writes its screenshots and
  audio to `build/scenarios/`.

`host/build_host.sh` builds the simulator alone. `build/host/x0x_host SCRIPT OUTDIR` runs one
script (the command list is at the top of `host/x0x_host.c`).

## Install

These command examples are not device authorization. Apply current model- and
action-specific permission, validated exact artifacts and the designated installer
under [the active workflow](docs/rules/workflow.md) before any hardware command.

X0X's web installer for this repo is
<https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/> (version + model picker, sample
importer at `/breaks/`). Felucca's own installer is on hugelton.github.io. To install your own
build, use the command line, or a local copy of the site (below).

From the command line (needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/x0x-smk37.fwsc
python3 tools/fm1_install.py --info          # identity of the connected device
```

Or, to install your own build from the web installer, make a local copy of the site. Open it from
`localhost` (Web MIDI needs a secure context):

```
tools/build_smk_release.sh 0.2                # Pro + Elite + MKE packages and build/pages/
cd build/pages && python3 -m http.server 8000
# open http://localhost:8000/install/  and  http://localhost:8000/breaks/
```

A single package still works:

```
./build.sh --release 0.1-beta
python3 tools/make_pages.py build/x0x-0.1-beta.fwsc 0.1-beta build/pages
```

You install firmware at your own risk. Sometimes the FM-1 does not start but still reaches the chip's
update mode (4C4A:8057 on USB). Then `tools/fm1_rescue.sh` puts stock V15 back from a Mac (docs/MANUAL.md,
Troubleshooting). Otherwise, recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Publishing a release

These commands describe a separately authorized release. Inspect the exact current
publisher, preserve prior catalog/assets and use its authorized nonforced route.
Required production hardware, review, rights and action gates remain in effect.
A CI workflow is not local validation or proof of live publication.

1. `tools/build_smk_release.sh X.Y` (or `./build.sh --release X.Y` for a single target). Release
   identities are what the installers check (`SMK-37 Pro_9…`, `SMK-37 Elite_9…`, `MKE-P37_9…`).
2. `tools/publish_pages.sh X.Y`: builds the site (landing, installer with every kept version,
   sample importer, manual, firmware) via `tools/make_pages.py`, merges older packages already on
   `gh-pages`, and pushes the branch. The site is
   <https://gobbajogger.github.io/X0X37-for-Loopy-Pro/>. The current `.github/workflows/pages.yml` only displays manual workflow
   instructions; it neither deploys on source pushes nor publishes Release assets.
   A source push or Release upload is not evidence of updated served Pages bytes.
3. `gh release create vX.Y build/x0x-smk37-X.Y.fwsc build/x0x-smk37-elite-X.Y.fwsc build/x0x-smk37-mke-X.Y.fwsc --prerelease`
   for command-line downloads (the installer can also pick these up from the Releases API).
4. Attach `LICENSE`, `LICENSING.md` and the `LICENSES/` folder to the release. Each package
   contains three Apache-2.0 files from the JieLi SDK (LICENSING.md). Thus
   `LICENSES/Apache-2.0.txt` must go with each package.

### SMK-37 APC40 mkII / Loopy Pro clip launcher

The SMK-37 Pro and Elite builds include an APC40 mkII-compatible MIDI identity handshake. They also include the 40-cell APC clip-note/LED protocol and the APC velocity color palette. Clip feedback on MIDI channels 0–15 carries the same color. The firmware trims empty outer margins but preserves internal empty cells. An 8-wide layout shows one local 8×2 view (two rows) at a time. OCTAVE− and OCTAVE+ move it down and up by two rows within the latched clip area. A layout of 3 or 4 rows and 4 or fewer columns folds onto one 8×2 view. Source rows 0–1 (the lowest APC rows) map to physical pads 5–8 and 13–16. Rows 2–3 map to pads 1–4 and 9–12. The order of the pad rows is upside down (vertical flip), and the columns stay left-to-right. Pressing KNOB BANK toggles clip mode. FADER BANK switches the four faders between banks in clip mode too. Press SCALE, CHORD or ARP to open its settings page. Knob 2 selects a field; knob 3 and `<` / `>` change its value. The ON field controls the stage. The keybed plays while a page is open (BEHAVIOR.md CLP-76). The firmware sends no APC navigation messages. Change the Loopy page to reach clips outside its 8×5 APC box. The firmware stores APC feedback as it arrives (there is no staged feedback). It latches the layout 250 ms after the first burst. Seq PLAY and Seq REC send momentary APC40 Play and Record messages in clip mode. Their LEDs follow Loopy's Play and Record/overdub feedback. Configure the Loopy Record binding and overdub behavior in Loopy Pro. Loopy Pro stays the source of clip state and color.

After you install the JieLi toolchain and AC79 SDK (above), use this command. It builds the Pro,
Elite and MKE packages and one combined web site (install + sample importer):

```
tools/build_smk_release.sh 0.2-apc40
```

This writes the three release packages plus `build/pages/` and
`build/x0x37-installer-VERSION.zip`. The ZIP must also contain `LICENSE`, `LICENSING.md` and the
`LICENSES/` folder before you hand it out (each package embeds Apache-2.0 JieLi SDK files;
LICENSING.md). On the install page, pick your keyboard's model so M-UPGRADE still accepts a later
stock restore. Do not use an FM-1 stock image to recover an SMK keyboard — drop that model's own
V016 `.fwsc` on the install page instead.

If Pillow has no optional `libraqm` support, the font generator puts proportional digits in the center of the widest digit cell. Thus numeric fields stay tabular.
