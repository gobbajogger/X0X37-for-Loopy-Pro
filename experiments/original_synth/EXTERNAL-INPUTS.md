# External original-engine build inputs

The authored engine source, interfaces, tests and available generators are in
this repository. Twelve generated coefficient or factory-data headers are
external inputs and are not included in the public source tree.
[EXTERNAL-INPUTS.json](EXTERNAL-INPUTS.json) records each expected path, byte
count, SHA256, source pin and available generator. It distinguishes the ten
headers used by the active replacement profile from two alternate/reference
headers. `state_codec_private.h` and `miami_factory_types.h` are authored
interfaces and remain part of the public source.

An authorized local build supplies the exact headers at their recorded paths
and verifies their byte counts and SHA256 values before compiling. The ten
active inputs are required by `X0X_ORIGINAL_SYNTH=1`. Some host reference tests
also compile the full effect owner or older factory adapter and require the
alternate inputs. An absent or changed input is a build failure; do not replace
it with zero data, approximate coefficients or another factory bank.

From the repository root, verify the active profile before a build:

```sh
python3 tools/check_original_inputs.py --scope active
```

For a reference-test checkout containing all twelve headers, save a metadata-only
receipt with the manifest pin made explicit:

```sh
python3 tools/check_original_inputs.py --root . --scope all \
  --manifest-sha256 9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc \
  --json > original-inputs-receipt.json
```

The checker exits nonzero on a missing header, incorrect byte count or SHA256,
changed manifest, or malformed manifest. It reads and hashes local inputs; it
does not fetch, generate, repair or substitute them. `tools/build.py` and the
normal host build invoke the active check before compiling the replacement.
The CLI defaults to the same pinned manifest hash. To inspect an input overlay
outside this checkout, provide its directory with `--root` and the manifest
from this checkout with an absolute `--manifest` path.

The generator source is useful only with its declared inputs. A generator name
does not establish that the necessary input data is in this repository. The
manifest identifies four current generator gaps: dynamics, modulation, tuning
and the full history-owner tables. This source delivery does not claim that
every external header can be regenerated from public files alone.

The exact twelve paths are ignored by Git. Build inputs, original plugin
binaries, private numeric manifests, captures and SDK/toolchain archives must
remain outside source commits and source-delivery bundles. Hash metadata and
authored generator code do not contain those payloads.

The local native validation route records the public Git tree together with
the external-input manifest hash and verified header hashes. The resulting
build closure is distinct from the public Git tree by itself. Host checks,
native packages and hardware results must identify that complete closure.
