# Independent source-stage review

Disposition: **Accepted for the generic adapter, documentation and tests only.** Both identified findings were corrected and independently rechecked. This is not merge authority, actual release-policy approval, package/publisher integration, product completeness or device qualification.

The read-only reviewer was explicitly selected as GPT-6.1 Sol with ultra reasoning. Standard service was requested; effective service tier was not exposed or verified. One independent reviewer performed both rounds, without nested agents, source edits, branch changes or publication.

## Exact accepted closure

Base nightly: `83fc6d60289869116f93352cf845a9a7e271cdba`.

| File | SHA-256 |
| --- | --- |
| tools/m12_acceptance.py | `0afccec757f1a2d94d9719428cc662168919d1dbd25f22ecf8b0ea133b9f25be` |
| tests/host/test_m12_acceptance.py | `c38d9c863fe65c1ada842b952be15a87b1500ec0bdb1d3c33ad9d3bfab2bf823` |
| docs/engineering-delivery/m12-adapter-20261114/README.md | `6f22cce4fee248bb8a42d1cfe3d3e31ce98491b0b80464dbc4229ac1492c2bce` |
| docs/superpowers/plans/2026-11-14-m12-independent-adapter.md | `0e222ce4a0ac4b8075e5447c4da5a18485854d614d851139baf676b18ab48c05` |

The reviewer verified these hashes before and after the final recheck. This review note and VERIFICATION.json summarize those completed observations; they are later-generated receipts, not additional reviewed executable source.

## Retained failed first round

The initial 36 focused tests and syntax check passed, but independent adversarial review found:

- **M12-SOL-001, P2:** table headers and separators were accepted as original source-row identities. An invalid accepted-policy row locator `#` or `---` could pass after the real source data row was removed. Evidence alone could not bypass a correctly pinned policy, but the promised mapping validation was defective.
- **M12-SOL-002, P3:** 20,000-level JSON nesting and an escaped unpaired surrogate repository path exited with tracebacks instead of the promised structured JSON rejection. Both failed closed, not as false accepts.

The author independently reproduced both findings and added failing CLI regressions before changing behavior. The repair parses only actual table data after a header/separator pair, preserves valid explicit aliases, resets table state at structural boundaries, rejects non-UTF-8-encodable paths and reports JSON nesting failures as structured validation errors.

## Independent final validation

- Original four rejection reproductions: **4/4 PASS**, exit 0.
- `python3 tests/host/test_m12_acceptance.py -v`: **40/40 PASS**, exit 0.
- `python3 -m py_compile tools/m12_acceptance.py tests/host/test_m12_acceptance.py`: **PASS**, exit 0.
- Six additional affected boundary checks: expected outcomes, exit 0.
- Authenticated current ROADMAP at `96070f0b328c8375ce4946e07da9fddb1f11c820`: 187 canonical records and all 122 numbered requests. There are 186 table-backed records; the actual CLI accepted 183 permitted selections using synthetic source/policy inputs. Three owner-held records were not selected, and non-table COMP1 stayed visible but unselectable.

Final actionable findings: none. The original review failure remains history; the later green recheck does not erase it or turn synthetic results into product proof.

## Remaining limits

Policy and receipt acceptance/provenance remain the caller's responsibility. COMP1 needs a separately reviewed non-table identity extension if required by a later accepted policy; nonselection does not waive an existing requirement. The original proposal artifact hashes were independently recomputed from recovered files, but the reviewer did not independently refetch that preservation commit's provenance.

No actual release policy, source integration, consumer adoption, package/preview/publisher hooks, firmware suite/build, emulator, publication or device proof was established. Downstream owners retain those separate stages and their current permissions.
