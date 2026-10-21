# Withdrawn SMC diagnostic: 5150343

The owner reported on 2026-10-21 at 14:43:59 UTC that the delivered diagnostic
build was not connecting at all and requested a revert. Treat this build as
hardware-regressed and withdrawn from further installation. This is the owner's
reported observation; its cause is unknown. It does not establish which source
change, controller behavior, device state or other condition caused the failure.

## Withdrawn artifact and source

- Compiled runtime: `5150343445d39cb4c91f3f9a7ccc609cf56da800`.
- ZIP: `SMK37-Elite-SMC-Security-Diagnostic-5150343.zip`.
- ZIP SHA-256: `2930662e42a97785ebdd13928683f4c8214e31df393fffb2e655b656a715033d`.
- Published diagnostic commits: `48ade05e32fb79df6198f52fabcf52c31c3129da`
  and `1d63b84872b5ded25af546120d064999347cfa0c`.
- Corresponding local integration commits: `aaf16c2a6cf19f9d74ed0988b920b42eab005274`
  and `4dcbfbfb7b6897edcbe7139bd9e7c17e25243a15`.

Both diagnostic changes are reverted by a new commit. History, prior validation
and the original artifact are retained as evidence, not as approval to install
the withdrawn build. Host tests and DEV validation did not establish hardware
connection behavior.

## Rollback identity

The source rollback restores the exact pre-diagnostic state represented by local
`666894e8e72c04bd24445ceffcb0f1565d1a52ed`, published as
`fc29e32fb183f8a901551149b1060a55af780453`, plus this withdrawal note. Previously
reviewed packaging, CoreMIDI, recovery and study sources remain intact.

The firmware and controller source trees are byte-identical to runtime
`816e0d3987bcb69be972cff2cd4df4469969f196`. The existing rollback artifact is not
rebuilt or relabeled as built from this revert commit. Its controller SHA-256 is
`9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245`, also the
controller identity in the withdrawn diagnostic. Both existing BLE modes remain.

Use only the separately verified previous 816e0d3 r2 guarded installation kit
for the requested rollback. Retain its model, queue, port, controller, backup and
readback guards. This source revert performs no hardware write and makes no new
hardware-success, symptom-fix, release-gate or LeakSanitizer claim.
