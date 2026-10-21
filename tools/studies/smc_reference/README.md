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

## Current runtime adapters: fully resolved diagnostic link

`link_runtime.py` compiles the **current conditional host sources**, not the
pinned baseline export. It enables `CTL_VENDOR_HOST_TEST=1`, retains real calls
rather than artificial API roots, and allows **no unresolved imports**. It still
uses oversized contiguous diagnostic regions and never emits an image/package;
its ELF is **not installable** and does not implement the segmented update path.

Provide the exact donor SDK `cpu/wl82/liba/system.a` privately. Its pinned SHA is
`a71bdc75e5afbcffaf7d79bc2b27f2b65fec63d94d2261b584744d3e9b9098ea`.
Only its `circular_buf.c.o` member is linked. Linking the whole archive pulls
unrelated RTOS/IRQ definitions before LTO and conflicts with the separate shim.

```sh
python3 "$REPO/tools/studies/smc_reference/link_runtime.py" \
  --repo "$REPO" --toolchain "$TC" --libs "$SCRATCH/libs" \
  --rf-tables "$SCRATCH/baseline/ble/ctl/gen/rf_tables.c" \
  --system-lib /path/to/exact-donor-system.a --out "$SCRATCH/resolved-runtime"
python3 "$REPO/tools/studies/smc_reference/compression.py" --repo "$REPO" \
  "$SCRATCH/resolved-runtime/resolved-NOT-FIRMWARE.elf"
```

The first October 21 assembled checkpoint resolved with **108708 B flash / 28760 B RAM
extent**, including 10112 B heap and 4096 B run/ISR stack reservations. Flash exceeds
the most optimistic 106496 B sector budget by 2212 B **before** manifest, placement
and remaining app-side work. Exact RAM-load compression saves 1763 B gross before
its decoder/metadata—still 449 B short. Do not treat resolution as fit or execution.
Input/source hashes and the failure boundaries are retained in
`docs/audit/smc-vendor-resolved-link-2026-10-21.json`.

For the complete app Bluetooth-removal upper bound only, create another fresh
inert-app export as above, add `"-DX0X_BLE=0"` to the application `flags` list
(the one beginning `[*CFLAGS, *FPU, ...]`) in its disposable `tools/build.py`, and
run the same Elite development build. Do not change the loader flags or guarded
source defaults. Expected image 529492 B, code slack 23180 B. Bluetooth is entirely
absent: the spare room must fund all replacement startup/bridge/parser features.
This is not a supported build profile or an installation candidate.

The resolved diagnostic also accepts `--controller-roles central` (role mask25,
central-only approved temporary profile) and `--avctp-extra-commands 0` (supported
scalar disabling extra AVRCP commands; not a vendor LE-only-library claim).
Defaults preserve role31 and extra-command1 comparison. These are oversized
link measurements, NOT executable placement or firmware admission.

After the authoritative-radio regression fix, all-role comparison is108724 B;
central-only is104536 B, and central-only with AVRCP extra-command0 is103188 B.
The last variant still needs4616 B more contiguous executable primary room
(text plus manifest) under existing PC guards; total-byte arithmetic is not fit.

`--placement data-only-92k` tests physical flash placement only:92 KiB primary
(physical0x84000), vendor constants in8 KiB at0xAD000, initialized RAM load at
0xFE000. It reserves128 B for a placeholder header and assumes the separate
reviewed8 KiB app-data relocation at0xAB000. It does NOT generate a manifest,
implement app-data admission, bound RAM, or produce an installable image.
The result records per-region capacities instead of a misleading contiguous
flash extent; ELF/map/symbol hashes identify retained private diagnostics.
