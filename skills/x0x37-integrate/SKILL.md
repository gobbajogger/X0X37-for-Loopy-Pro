---
name: x0x37-integrate
description: "Integrate x0x37 firmware, validate a candidate, or select development versus release checks and full-gate admission."
---

# Integration and validation

### Rule 11: branch order and formal validation

The assigned integrator owns firmware branch order. Do not create another coordinator or use `git update-ref` to avoid checks.
Use history-preserving merges, fast-forwards, or cherry-picks in the controlled checkout.

Default: development builds and installs with `X0X_DEV_PADCAL=1`.
Use [scoped checks](../x0x37-check/SKILL.md) and `tools/dev_validate.py`.
Only an explicit release-build request permits release configuration or consideration of a full gate.
Generic build, validation, merge, install, push, or full-validation requests do not give that permission.
Old instructions cannot override the current owner correction.
Before release work or any full gate, read [release validation](references/release-gate.md).
Until then, skip the full gate, including trial, force, and owner-validation runs.

Maintain Pro and Elite. Select development targets under [rule 9](../x0x37-check/SKILL.md).
FM-1 is a porting reference; do not require its target builds or target tests.
Keep shared SMK-37 checks, including tools with legacy `fm1` names.

For development installs, follow [rule 6](../x0x37-install/SKILL.md). Record source commit and configuration.
Missing setup blocks only dependent work. Documentation changes give no device evidence.
