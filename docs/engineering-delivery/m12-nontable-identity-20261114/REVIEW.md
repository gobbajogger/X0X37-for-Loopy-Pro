# Independent source-stage review

The requested independent reviewer was GPT-6.1 Sol with ultra reasoning. Effective model/effort and service-tier metadata were not exposed for verification. The reviewer changed no source and used only the adapter, its tests/docs, public ROADMAP metadata and disposable synthetic inputs.

## Initial findings

The first exact-closure review required changes. Its report SHA-256 is `1626c307e272eaa42718be9f03ff6da129a65ff91fee369442170d612d48a79a`.

- **F1, P2:** Indented preserved-source markers and orphan tables were ignored by the no-table guard, allowing a non-table provenance downgrade.
- **F2, P2:** An indented visible Request/source field could evade duplicate detection.
- **F3, P2:** Python's broad line splitting could authenticate only the prefix before a Unicode/control separator instead of the complete UTF-8 source value.

The author added real-Git CLI regressions and observed 14 false-acceptance failures in the 57-test suite. One repair pass added a conservative new-selector-only provenance guard, treated indented source fields as malformed, and preserved LF/CRLF logical-line value bytes without Unicode normalization. The full focused suite then passed 57/57; syntax, whitespace and the exact public COMP1 synthetic exercise passed. The deliberate rejection of even unrelated table-shaped content is documented in the usage guide.

## Narrow final check

The same independent reviewer accepted the repaired exact closure, **source stage only**. Final report SHA-256: `24d518d7a0f09bc61b810543fe71829984983999abdeed564747694d957ca51a`.

Reviewed adapter SHA-256: `0b77d068b86812f56426e2fa721a0e7734642d814d1316804a5f60f846388d8a`.
Reviewed test SHA-256: `45b930728ad916c2b07b0b5dd0756303dc3f62467a82ee48e6cb2fe4d037ebd4`.

All 12 original actual-CLI/real-Git probes matched the repaired/control expectations. The exact-digest contrast rejected a truncated prefix and accepted the complete UTF-8 value. Four repair test methods passed, including LF/CRLF handling. Whitespace passed and all frozen hashes matched before/after review. There are no remaining findings or deferred minors. The unchanged whole suite and public parser exercise were not repeated during this narrow check.

This is not actual product-policy acceptance, browser-companion behavior, real MIDI, firmware/native/device validation, package/preview/publisher adoption or publication approval. The initial failure history remains part of the result.
