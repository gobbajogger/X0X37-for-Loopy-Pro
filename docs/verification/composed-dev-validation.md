# Exact composed DEV validation

`tools/dev_validate_composed.py` validates an unversioned, model-specific composed
candidate without changing the installation guard or its schema-2 verifier. It
never installs, publishes, writes the gate index, or labels a private native
receipt as a previous DEV run.

The input plan has the existing `base`, `scope`, `checks` and optional
`dependencies`, plus `composition` containing:

- `app_sha256`, `raw_package_sha256`, `controller_sha256`, `controller_bytes`,
  `bundled_package_sha256`, and final `package_sha256`;
- absolute `composer` script path;
- `dependency_pins`: absolute paths to expected `validation_inputs.content`
  SHA256 values. The composer tools directory and designated stock bank file are
  mandatory; callers may additionally pin SDK/toolchain inputs.

Run on native Linux x86_64 with explicit `JIELI_TOOLCHAIN`, `AC79_SDK` and matching
configuration inputs:

```
python3 tools/dev_validate_composed.py --repo /path/to/source-repo \
  --commit FULL_SHA --model pro --plan /path/to/plan.json \
  --output /path/to/new-result-directory
```

The runner checks base ancestry, extracts the exact Git archive without Git
metadata, verifies tracked blob contents/modes, runs affected checks, freshly
builds the application and controller, and performs pinned controller bundling,
stock composition and final package inspection. Each command is synchronous and
has its own hashed log; composition appears as additional `check-N` entries.
All expected output pins must match. Input provenance fingerprints source,
configuration, tools, Python dependencies, SDK/toolchain, runner/helpers, and
composition dependencies before and after execution. No replacement bytes are
silently accepted. Source, dependency, output or command failure emits no record.

The output record is explicitly produced by the composed validator and uses the
unchanged DEV verification contract. Verify with the existing guard using the
exact package, controller and `--dev-record`; preserve its actual queue and
results settings. A successful record is not a release PASS, hardware result,
queue waiver or permission to install. Missing commit metadata or unproven queue
ancestry remains a refusal. Do not substitute an empty queue.

`tests/host/test_dev_validate_composed.py` uses unflashable fake builds and real
runner/record/guard integration. It covers exact positive execution and refused
app/raw/controller/bundled/final pins, stock/composer/SDK drift, controller size,
failed checks, tracked content/mode mutation, unknown base, output overwrite,
log/package/controller tampering, release misuse and invalid queue entries.
These tests do not replace a real pinned native execution and independent
review of the exact composed outputs.
