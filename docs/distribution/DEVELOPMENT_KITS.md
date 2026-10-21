# Private development install kits

`tools/distribution/package_dev.py` is the reusable packaging path for an
owner-authorized **Elite DEV** transfer. Its only user-facing firmware always
contains the exact matching Bluetooth controller. There is no raw-only mode.
Other models need their own validated packaging checks; this tool does not
provide a Pro artifact or a release result.

```sh
python3 -B tools/distribution/package_dev.py \
  --source-repo /path/to/runtime/repository \
  --validated-result /path/to/exact/dev-result \
  --output /new/path/Elite-DEV-kit \
  --zip /new/path/Elite-DEV-kit.zip
```

The result directory must contain the untouched `record.json`, raw Elite
`.fwsc`, `ctl.img` and every log named by that DEV record. The generator never
compiles firmware, downloads dependencies, opens MIDI, rewrites the record,
or assigns the packaging-tool commit to an older binary. It materializes the
runtime commit named by the record into a separate Git checkout. The manifest
records runtime commit/tree, packaging-tool commit and exact tool hashes
separately. Available source history may be shallow; the manifest says so. All reachable
commit paths and blobs are checked for excluded source/vendor outputs before
transfer. This scan is not general secret redaction: use an approved source
repository. Custom host Git templates and unrelated refs are not copied.

The raw package, controller, original record and logs are internal evidence.
The generated `verify_combined.py` checks their identities and original DEV
validation, model, controller ABI/CRC, reversible composition and equality
with the native installer's logical bytes. `Install.command` verifies the kit,
then passes the original raw inputs and explicit controller to the unchanged
native guard and installer. This preserves the original validation semantics;
it does not pretend an assembled file is raw build output.

The launcher retains present-queue veto, model/write-range guards, hardware
lock, confirmation, every-slot backup, readback and a 45-second connection
capture. Missing queue is reported **NOT CHECKED**; no empty queue is created.
Relative `--head-ref` paths and bypass flags are rejected. Failed installation
skips collection; logging failures cannot replace the installer's error code.

On macOS, dependencies are installed only when imports are missing. The
`python-rtmidi==1.5.8` source-build setting disables JACK. The launcher selects
CoreMIDI explicitly for its child processes and logs read-only port enumeration
without opening ports or changing system settings. First installation can need
network, Git and Python 3.9+ with venv/pip. `--verify-only` is offline.

## Required checks

```sh
python3 -B tests/distribution/test_source_inventory.py
python3 -B tests/distribution/test_backend_selection.py
python3 -B tests/distribution/test_dependency_setup.py
python3 -B tests/distribution/test_package_dev.py \
  --source-repo /path/to/runtime/repository \
  --validated-result /path/to/exact/dev-result
python3 -B tests/distribution/test_launcher_contract.py /path/to/generated-kit
```

The first two tests use fake dependency/MIDI modules. The latter checks use
supplied exact artifacts and isolated launchers. They do not write a device.
Check every selected model artifact; an Elite result does not cover Pro.
Keep vendor/library binaries out of Git. Private controller transfer remains
subject to the vendor terms in `LICENSING.md`; this tool grants no public
redistribution permission.

## Existing development evidence

The 2026-10-21 runtime `816e0d3987bcb69be972cff2cd4df4469969f196` and its
unchanged current controller were built before these packaging tools were
committed. Its combined firmware SHA-256 is
`6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a`.
The original DEV record explicitly used `ASAN_OPTIONS=detect_leaks=0` because
LeakSanitizer could not run under the cloud runtime: **LEAK CHECK NOT RUN**.
ASan/UBSan and target budgets remained enabled. Packaging reuse neither clears
that limitation nor proves a physical SMC symptom fixed. No new runtime build
or device result follows from the host-tool commit.
