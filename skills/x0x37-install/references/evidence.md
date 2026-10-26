# Development and release install evidence

Install the complete requested set only after all queued changes and required checks are complete.
Use the exact validated artifacts and configuration. No required check can remain failed.

Never install a fallback only to make the keyboard playable.

Use development configuration by default: `X0X_DEV_PADCAL=1`.
For a development install:

1. Use `tools/dev_validate.py` with the intended model and a justified plan of affected checks.
2. Use its DEV CHECKED record for the build, budgets, selected checks, package, and controller.
3. Pass `--commit HASH --dev-record FILE --config dev` to `tools/fm1_install.py`.
4. Select the controller explicitly with `--ctl FILE` or `--no-ctl` as required by the validated package.

Never omit a required controller to avoid checks. Reuse matching evidence for repeated installs.
Skip the full gate for development work under [rule 11](../../x0x37-integrate/SKILL.md).

Add checks only for an affected dependency or a concrete risk. State that reason in the plan.

A release install needs an explicit release-build request and the required full-gate result.
`tools/require_gate_pass.sh` accepts development evidence only for development configuration. It cannot create a release PASS.

Preserve configuration, queue, model, artifact, and data guards. Do not use a team web install or bypass a refusal.
Current owner permission, a device, and required validation are all needed for installation. The authorized Pro investigation is read-only, not an install grant; Elite flash remains withdrawn. Recheck current permission before any device action.

A public release or release package needs the formal gate and the owner's device PASS for the same build.
Set `X0X_DEV_PADCAL=0`. Include model-specific packages and licensing files.

Publish only on owner request. Exclude `refs-oscuro` and proprietary SDK and toolchain files.
