# Firmware-specific proof for the tstack flow

Apply these checks when the changed paths and requested delivery make them relevant. They supplement the selected tstack mode rather than creating a second sequence of threads.

1. Identify the exact source commit/tree, changed validation inputs, intended device behavior, model scope, and assigned owner. Work in an isolated branch; do not edit another integrator's active files.
2. Implement the smallest approved change. Preserve features, sample and project data, calibration, model guards, budgets, and proprietary source boundaries.
3. Run covering host checks on the changed behavior. Use an independent QA review for applicable firmware delivery. Record failures and NOT_RUN checks; do not convert either to PASS.
4. For a development package, build affected Elite and Pro inputs as required by [x0x37-check](../../x0x37-check/SKILL.md) and [x0x37-integrate](../../x0x37-integrate/SKILL.md). Record package/controller hashes, source identity, configuration, budgets, and `DEV CHECKED` only after its actual gate passes. BUILD_ONLY is a separate status.
5. Compare every later merged or proposed tree against the validated inputs. Repeat only affected proof when inputs change. The assigned integrator controls firmware branch order and any authorized merge.
6. A device install or hardware validation is a separate action under [x0x37-install](../../x0x37-install/SKILL.md) and current owner permission. Check current model- and action-specific authority; Elite recovery and flash remain withdrawn, while the assigned integrator has read-only Pro investigation permission. Firmware production `main` requires successful hardware validation on the same bytes and model.
7. A PR, push, nightly page, release package, or public publication has its own scope and permission. Do not infer it from a build, test, install, or tstack mode.

For a documentation-only skill change, review the text and links, verify skill syntax and dependency paths, then perform its authorized source delivery and remote readback. Report the evidence and continue any remaining authorized action under the [continuation contract](handoff.md). Documentation does not start a firmware stage chain or require unrelated device proof.
