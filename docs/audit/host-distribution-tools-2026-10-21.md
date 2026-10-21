# Reusable private DEV distribution tools

## Scope

The owner requested that reusable packaging, recovery tools and source evidence
be committed, not left only in transfer ZIPs. This change adds host tools and
documentation. It does not change firmware, controller sources, flash layout,
model IDs or device behavior. The existing runtime remains
`816e0d3987bcb69be972cff2cd4df4469969f196`, Git tree
`74db8ca92ab208c3a8fd963ad02e5e60a509d3a8`. Later host-tool commits are not a claim
that its retained binaries were rebuilt from those commits.

`tools/distribution/package_dev.py` replaces the one-off r2 assembly scripts
with a reusable exact-record flow. An embedded matching Bluetooth controller
is mandatory in its sole user-facing firmware. Raw artifacts and the original
DEV record remain internal and unchanged. The reviewed r2 CoreMIDI/dependency
logic, native guards, backup/readbacks and capture are retained in the launcher
template. Custom host Git templates, unrelated refs and excluded reachable
history are not copied. Source symlinks are checked before source tools run.
A clean-extraction regression found that omitting empty Git directories broke
the transferred repository; ZIP creation now preserves them.

Original reusable inputs are identified in
[the import index](host-distribution-tools-2026-10-21.json). Those one-off files
are historical inputs, not a second supported packaging implementation.
The separately reviewed collectors and sanitized evidence are documented in
[SMC recovery provenance](smc-recovery-provenance-2026-10-21.md). Collector output
and raw private logs are not committed.

## Checks

Fresh scoped checks against the integration source:

- Six reachable-history/link boundary cases: removed excluded path, renamed
  compiled archive, renamed SDK pin, safe source, escaping symlink and submodule.
- Twelve CoreMIDI selection/enumeration/error cases with fake MIDI modules.
- Ten missing/present dependency and failure cases with fake Python/pip.
- Twenty-three launcher contract cases, including queue veto, exact native
  arguments, slot backup/readback selection, 45-second capture and error codes.
- Actual retained Elite pair: generation, clean ZIP extraction, offline verify,
  unchanged raw record/logs, mandatory combined artifact, artifact/source/log
  tampering, invalid model/configuration/runtime identity and no overwrite.
- Native mocked installer safety, controller readback, sample readback and
  corrected SMC monitor regressions. These cover both supported model guards
  where the native test does so; they do not create a Pro firmware artifact.
- Both collectors: 11 + 8 tests and three shared boundary tests, all temporary
  fixtures. Shell syntax and Python/JSON parsing checks cover new host files.

Commands are in [development kits](../distribution/DEVELOPMENT_KITS.md) and
[recovery tools](../../tools/recovery/README.md). No real pip install, MIDI
access, firmware compilation, device write or real-home collection ran.
Independent final source review is required before the commit handoff.

## Exact reused runtime inputs

| Input | SHA-256 |
| --- | --- |
| Raw Elite package | `d3d01b82b5ea36a7265e8126103f80dc577eeef5ac202e2d848189b595f991f8` |
| Controller | `9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245` |
| Original DEV record | `2f7aabfa5de737b8f2ff1f51672b91d1fec906fbb36e13d25704918d42588557` |
| Derived combined package | `6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a` |

Original validation remains **LEAK CHECK NOT RUN** with ASan/UBSan retained;
packaging checks do not clear that limitation. An absent install queue is
**NOT CHECKED**. Actual macOS/device behavior and an SMC symptom fix are not
established by these host checks. The private-transfer policy does not grant
public redistribution rights to vendor components.
