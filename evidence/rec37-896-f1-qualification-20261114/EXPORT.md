# REC #896 F1 — export-only preservation

This branch preserves the completed independently approved test-only qualification. Parent source: `8b0b6d311577fcc303212016423dd4f5ea4e8184`; branch: `chat/rec37-f1-evidence-20261114`.

The three `tests/host` files exactly match the approved local candidate. The patch, report, review and recorded execution receipts are preserved byte for byte. The qualification report describes its earlier stage, including the then-unpushed state. This export does not change that historical report, its test semantics or its approval. No tests or development were resumed for export.

Patch SHA-256: `89785c38b7b4884c544b1994df66b5242103274402b3036380bb1b6b90883351`.
Frozen local evidence ZIP SHA-256: `e737237483e06a60cf4e6967a7083905f57a22815fbb457ee7cbc1b0a03b9b9b` (archive is not uploaded to Git).

`EXPORT-MANIFEST.json` maps each preserved file to its local source, SHA-256, size and Git blob identity. `PACKET-MANIFEST.json` is the unchanged full local-packet manifest; it also describes selected public source/policy files omitted from this export because their pinned Git inputs already exist. The five production inputs of the probe remain unchanged from the parent. `runs/*.csv` are deterministic synthetic fixture observations, not device or personal raw captures. The prior logs retain expected baseline conformance failure and the draft compilation failure; they are not relabelled PASS.

The only new metadata is this export note and the export manifest. SDK/toolchain material, binaries, Library helpers, original private reports and unrelated personal data are excluded. Production source, `.github`, main/nightly and the incumbent REC branch are unchanged. No PR, merge, firmware publication or device action is part of this export.

## Refusal scope and ownership check

The previous Library prepared-helper batch failed with HTTP 401 at tool discovery, before preparation. The Project refusal concerns raw-byte materialization of SDK/source archives. Neither operation was retried or routed elsewhere. The public payload here comprises authorized test source, the reviewed patch, report/review and reproducibility receipts; no refusal covering this payload was found in the task history or visible room records. Other visible Git denials concern distinct payloads/PR actions and remain untouched.

At export intake, the local agent registry had only this active root; no test, Git exporter or upload process was present. The canonical `rec37` branch search (fully paginated) returned only the incumbent branch, and the proposed export ref was absent. No exporter was found in visible room records. The legacy `coordination/merge-queue` branch was unavailable (404), so it was not treated as a live global registry. The current delegation assigns this isolated finished-stage export; no claim about unrelated workers' global absence is made.

## Reuse and remaining state

Read the qualification report and review first. On this branch, the equivalent reproduction command is `python3 tests/host/rec37_admission_probe.py . <fresh-output-directory>`. It was not executed during export. The expected prior result is substituted compile 0/run 1 and production compile 0/run 0, with 2,240 fresh and 28 transition differences in the substituted model. Firmware, whole-app, audio, real release ownership and device limits remain as recorded.

**Library delivery remains unresolved.** This separate Git preservation neither completes nor retries the failed Library batch. The existing creator/coordinator retains its delivery responsibility; the incumbent REC writer remains the consumer of the test-only correction. No owner acknowledgement, further test, implementation or approval is invented.
