EC011 bounded OTA receive queue and EC008 finish acknowledgment

Current source fixes terminal queue overflow, retained byte/frame bounds, concurrent reader ownership, timeout/drain generations and false finish success. The two exact test-only red PR regressions are copied unchanged. Added tests exercise actual production Link, Updater.find/serve/resume and ProjectDevice offset correlation with synthetic data.

No queue trimming permits an update to continue after overflow. Protocol match predicates and existing identity/read-envelope guards are retained. See client-budgets.json for all current reply limits and receipt.json for actual checks, hashes and remaining limits. All source work is isolated; no hardware activity. Raw negative-log whitespace is preserved. Final additional firmware sample install/F03/project integration checks all passed; their shared log was appended without source changes.
