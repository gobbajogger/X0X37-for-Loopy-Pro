# Held FX arrow review

Static Compound Engineering review of the owner row 41 change against af1a3a5e6ab9d4442977ed283072ded3a034ddee. Correctness, project standards, testing, maintainability and adversarial lenses completed. The cross-model pass could not run because no different-provider CLI route is installed; the local adversarial fallback completed.

Two P2 defects were found and corrected during review: USB/User holds incorrectly required a central BLE link, and releasing the focused FX key left the remaining glyph inconsistent with its strip/footer. One fresh independent validator batch confirmed both original defects and the source/test corrections.

A later synthesis trace found that an ambiguous SMC arrow chord canceled the held popup. The caller preserved that popup and added an assertion; the review orchestrator checked this correction statically. This follow-up was outside the independent validator batch. The obsolete HOME CYC caption was also removed before the final finding set.

No remaining supported source finding. The source manifest binds the reviewed production/test files after corrections. Exact sparse references for the other registered SMC/arrow extraction and source-contract tests were inspected; no newly stale dependency was found.

No builds, tests, compilers, lint or real framebuffer captures ran. Written tests, full-suite execution, SDK1.2.1 Elite/Pro X0X_DEV_PADCAL=0 builds, RAM/flash measurements and actual 240x240 before/after PNGs remain NOT_RUN. The documented ownership mask adds 4 B per peer; native link maps must measure the final cost.

Verdict: Not ready for merge until external qualification and owner approval of actual visuals. Actionable source findings: none.
