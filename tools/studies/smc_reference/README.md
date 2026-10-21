# NON-INSTALLABLE SMC reference-host size studies

These inputs reproduce the October 21 architecture audit, **not firmware**.
`run.py` exports pinned public baseline source into a new scratch directory. It
never edits that baseline checkout, invokes `mkimage.py`, builds an installer,
or installs anything. It deliberately enlarges scratch linker regions and makes
one relaxed size-only link **after a normal report-all link fails**. That failure,
its missing imports, and `installable: false` are retained in `result.json`.
Never use the relaxed ELF on a device. This is not a runtime implementation.

No vendor archives, proprietary IR, generated vendor C, ELF, image, or package
is committed here. `config-values.json` contains the scalar symbol/type/value
configuration facts used in the study, including log switches; the runner emits
its C definitions privately. `roots.json` holds explicit LTO retention roots.
`input-hashes.json` rejects mismatched libraries, compiler/linker and RF tables.
The app patch changes already-public baseline sources only and retains their
licenses; it must only be applied to a disposable exported baseline.

## Prepare private inputs

Use Linux x86-64 and the verified JieLi toolchain release `20260730.1` (see
`tools/get_toolchain.sh`). Set these absolute or relative paths yourself:

```sh
REPO=/path/to/checkout-containing-this-study
TC=/path/to/jieli-linux-toolchains-20260730.1
SDK=/path/to/verified-baseline-sdk
SCRATCH=/path/to/new-private-study-directory
mkdir "$SCRATCH"
mkdir "$SCRATCH/baseline"
git -C "$REPO" archive 816e0d3987bcb69be972cff2cd4df4469969f196 |
  tar -x -C "$SCRATCH/baseline"
# Does not build or install firmware; private files are hash-checked by fetch_sdk.
(cd "$SCRATCH/baseline" && SDK_LOCAL="$SDK" sh ble/ctl/fetch_sdk.sh &&
  python3 ble/ctl/gen_rf_tables.py)
cp -R "$SCRATCH/baseline/ble/ctl/lib" "$SCRATCH/libs"
```

Replace only `btstack.a`, `btctrler.a`, `wl_rf_common.a` in `$SCRATCH/libs` with
the exact donor SDK `64afb30b805af5f5d3c2e6471e970c3fd0ba40ad` files from
`cpu/wl82/liba/`. They are available in a matching private SDK checkout or from
JieLi's official pinned source, for example:

```sh
DONOR=64afb30b805af5f5d3c2e6471e970c3fd0ba40ad
for name in btstack.a btctrler.a wl_rf_common.a; do
  curl -fSL "https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/raw/$DONOR/cpu/wl82/liba/$name" \
    -o "$SCRATCH/libs/$name"
done
```

The runner verifies all required archive hashes before compiling. Other libraries
are the pinned baseline versions listed in `input-hashes.json`; the study does
**not** link `system.a`. Its absent cbuf/lbuf and operational adapters remain
explicit imports. Do not add empty success stubs to get a clean link.

## Run the six controller studies

```sh
for variant in donor one-link central both central-private both-private; do
  python3 "$REPO/tools/studies/smc_reference/run.py" \
    --repo "$REPO" --toolchain "$TC" --libs "$SCRATCH/libs" \
    --rf-tables "$SCRATCH/baseline/ble/ctl/gen/rf_tables.c" \
    --out "$SCRATCH/$variant" --variant "$variant"
done
```

`--out` must be new and outside the supplied checkout. The report-all link must
fail with an explicit import inventory; a surprising clean link is rejected for
review rather than promoted to firmware. The relaxed link only measures retained
sections. Missing task startup, queues, callbacks and live allocation requirements
are not charged in these static totals. Expected historical measurements:

| Variant | Flash B | RAM B | Configuration |
|---|---:|---:|---|
| donor | 106660 | 27336 | generic startup roots, 2 hardware contexts, server 1 |
| one-link | 105956 | 27336 | generic roots, 1 context, server 0 |
| central | 98020 | 27232 | direct central roots, raw shim retained |
| both | 103932 | 27288 | direct central + server roots, raw shim retained |
| central-private | 97724 | 26928 | central roots, vendor private transport |
| both-private | 103636 | 26984 | both-role roots, vendor private transport |

All one-context variants use five RX and ACL buffers; donor uses ten. Security
and LE role capability configuration remains as in the study. These numerical
variants do not implement approved modes. A central-only product/profile would
require the owner's explicit feature choice and a new functional implementation.

The discarded `split-sections` sensitivity probe can be reproduced by adding
`--plugin-opt=-function-sections --plugin-opt=-data-sections` to the runner's
`common` linker argument list for `one-link`; it measured 106520 B flash, worse
than 105956 B. This is not part of the selected six variants.

## App host-removal ceiling (nonfunctional, no package)

Create another new baseline export. Apply the patch only there:

```sh
mkdir "$SCRATCH/app-ceiling"
git -C "$REPO" archive 816e0d3987bcb69be972cff2cd4df4469969f196 |
  tar -x -C "$SCRATCH/app-ceiling"
(cd "$SCRATCH/app-ceiling" &&
  git apply --unidiff-zero --check "$REPO/tools/studies/smc_reference/app-ceiling.patch" &&
  git apply --unidiff-zero "$REPO/tools/studies/smc_reference/app-ceiling.patch" &&
  JIELI_TOOLCHAIN="$TC" AC79_SDK="$SDK" SDK_LOCAL="$SDK" \
    X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
```

Both host APIs are inert; Bluetooth cannot work. Original guarded configuration
is retained in unreachable implementations. The patch removes package generation,
not guard checks. A fresh export prevents stale packages being mistaken for new
output. Expected code slack is 16896 B versus baseline 4192 B: reclaim 12704 B
before functional replacements. Keep only the result as evidence; never install
its ELF/bin. Main RAM 97956 B and pool 335740 B are unchanged except 16 B main RAM.

## Compression and type-size checks

Compression only examines the old `both` study's RAM load sections:

```sh
python3 "$REPO/tools/studies/smc_reference/compression.py" --repo "$REPO" \
  "$SCRATCH/both/ble/ctl/obj/size-NOT-FIRMWARE.elf"
```

Expected roundtrip savings: `.ramtext` 3524→3141 B, `.data` 1804→429 B. The 1758 B
saving excludes decoder/metadata/layout and is not automatically transferable
to another image. This host-side experiment uses the repository GPL codec; it
does not license linking GPL code into the separate proprietary vendor program.

For exact target type sizes, compile the following temporary source with
`$TC/pi32v2/bin/clang -target pi32v2 -integrated-as -mcpu=r3`, adding
`-I "$SCRATCH/baseline/firmware/src/ble"`, and inspect it with target `objdump -t`:

```c
#include "ble_midi_host.h"
#include "ble_midi_central.h"
char measured_bleh[sizeof(bleh_t)];
char measured_blec[sizeof(blec_t)];
```

Common-symbol sizes are 0x300 (768) and 0x1bc (444). The host union is 768 B, not
all BLERAM. No memory guard, sample region or feature is changed by these probes.
