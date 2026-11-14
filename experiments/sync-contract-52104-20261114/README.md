# Atomic PROJECT contract prototype (#52 / #104)

This isolated proposal demonstrates owner-capability, revision-checked project
publication with immutable snapshots, reconnect fencing and recoverable uncertain
outcomes. It addresses accidental lost work and cross-client interference in a
synthetic model. It contains no device connection, firmware change or executable
wire parser. DeviceSync remains disabled in the real editor.

Read [PROTOCOL.md](PROTOCOL.md) for the exact tentative wire layout, state rules,
compatibility, ownership evidence, resource tradeoffs and proof limits. Read the
[independent protocol analysis](review/protocol-analysis.md) and
[adversarial review](review/adversarial-review.md) for findings and resolutions.

From this directory, with Python3.10+ and no additional dependencies:

```sh
python3 run_checks.py
```

The runner records exact source SHA-256, command exits, test counts and logs in
[evidence/results.json](evidence/results.json). It runs focused contract tests,
independent hostile tests, two separately written bounded interleaving explorers
and three deliberate safety-gate mutations in disposable copies. Original source
is never mutated by a negative control. All bytes used by tests are synthetic.

The independent tests also transfer36,452 opaque bytes, including changed tail
and high-bit data. This demonstrates byte preservation in the model, not support
for any actual revision13 or stream16 project schema.

No live integration is accepted here. The parent coordinator must recover the
incumbent protocol owner's current thread/latest editor.c and stream16 pin, then
obtain allocation and boundary acceptance. The existing FMB pattern owner and #7
persistence work remain untouched. No successor creation or receiver input
consumption is claimed. The exact next packet is in [HANDOFF.md](HANDOFF.md).

Model PASS does not establish firmware atomicity, sample/storage durability,
hardware authentication, actual parser/transport behavior, host application
tests, native Elite/Pro resources/performance, emulator/browser or device results.
