# NIGHTLY packages include their matching Bluetooth controller

A qualified NIGHTLY package contains the matching controller in its 65536-byte
Bluetooth block at flash `0x8B000..0x9AFFF`. The installer can use that complete
package without asking the owner to select a second `ctl.img`. A blank block,
unknown controller or mismatched model cannot qualify as a bundled package.

`tools/package_nightly.py` reuses the checked composition and atomic output writer
in `tools/add_ctl.py`. It supports both Elite and Pro. It requires an explicit
reviewed controller SHA256 and exact declared image size; it never selects a
cached controller or produces a Bluetooth-less fallback.

```sh
python3 tools/package_nightly.py APP.fwsc ctl.img \
  --model elite --controller-sha256 REVIEWED_SHA256 --controller-size REVIEWED_SIZE \
  -o COMPLETE.fwsc > COMPLETE.receipt.json

python3 tools/package_nightly.py COMPLETE.fwsc --check \
  --model elite --controller-sha256 REVIEWED_SHA256 --controller-size REVIEWED_SIZE
```

Use `--model pro` for the Pro package. The product marker must be the selected
model's exact DEV identity (`SMK-37 Elite_900` or `SMK-37 Pro_900`). The tool checks
package/file CRCs and bounds, the current SMK app
layout and sample slot map, nested app CRCs, controller header/ABI/RAM/interrupt
rules, controller CRC32, exact image size/SHA256 and erased block padding. It
reopens the final output and checks that composition changed only the controller
and dependent checksums. Its JSON receipt records hashes from the actual bytes.

For a separately reviewed catalog, copy the receipt's `controller_strategy:
"bundled"`, `controller_sha256` and `controller_size` to the model package record.
The independent renderer qualification uses `controller.strategy: "bundled"`
with matching `sha256` and `size`. These fields describe a byte contract; they do
not supply independent QA, source/license approval or immutable HTTP receipts.
All existing catalog and installer checks still apply.

The existing public blank-slot packages remain on the legacy explicit-controller
path until a qualified complete replacement and publication are authorized. A
catalog must not claim that those unchanged bytes bundle Bluetooth. Current
SMK packages with a blank controller slot are rejected when no matching
controller is supplied; installing them does not preserve Bluetooth implicitly.
Historical packages whose application owns that range retain their existing
legacy handling.

The receipt does not establish distribution rights. Before any transfer or
publication, verify the exact controller input provenance and component terms,
preserve required notices/source, and obtain the applicable distribution and
publication decisions. A private transfer is not automatically exempt from
license conditions. No real package or controller bytes belong in the browser
source repository.

`python3 tests/nightly_package_test.py` exercises composition and rejection on
synthetic real-format Elite/Pro packages without SDK files or hardware. Its
`--fixtures DIR` mode supplies the same bytes for browser parser/parity tests;
these synthetic files contain no executable firmware and are not installable.
