# V4 publication handoff — current concurrency boundary

Candidate and review are unchanged. This note supplements the original archive rather than changing the reviewed website.

## Exact input

- Complete runnable Library packet: https://chatgpt.com/api/library/files/libfile_fde3_f-d64e0581f8ac8191a9e08811954e96ca_2021479872048191acbfdca1f870a539/download
- ZIP SHA-256: `0f84d16ec2b2d67c589bd80241979883230035d51da96e7bddc40b6b2209dd3a`.
- Reviewed source/archive commit: `90b9d63b8d7b1d4964b706ef0365cfddf58a4ffa`; tree `e4272545a201717f814c03c63ef03bb2557e9420`, canonical repository gobbajogger/X0X37-for-Loopy-Pro.
- Candidate runtime: packet `DnBOX-v4/site/`, exactly 55 files. Runtime manifest SHA-256: `e188de7871e59099b4254ea99994ca0214e51a58e45ab1325390030ab3a28c90`.
- Independent changed-only requalification: `review/requalification.md`, SHA-256 `0d31f30aa90be28ea9a8891e42784f1613247113e05b786f2b6fe92150d2cf4f`. PASS for bounded NIGHTLY website preview testing. Full initial review and all fixes remain in the packet.

## Current owner and mutation boundary

Coordinator reports bounded importer publisher `6af84797` owns only PR #130 delivery and targeted installer serialization preflight, preserving all preview paths. This product revision owns only its fresh `previews/dnbox-v4-20261114/` subtree. Do not edit installer, catalog, firmware packages, importer source, V3, or unrelated paths. Preserve any other current owner discovered before mutation.

Immediately before any shared NIGHTLY `gh-pages` update, the accountable preview publisher must read the live ref and root tree. Apply only this preview subtree against that current tree, then update using an explicit expected-head lease and no force. If the lease fails, read the changed head and reconcile normally; never reuse an earlier whole-tree snapshot or discard concurrent importer/preview changes.

The same rule applies if importer delivery changes the head between preparation and update. Serialize shared-ref mutation with the existing publication owner; independent source work need not stop.

## Publication readiness and remaining proof

READY: exact V3 consumed and all 53 files matched live; reviewed 55-file V4 runtime; source syntax and exact reconstruction PASS; local Chromium 48 responsive renders/23 interactions PASS; fresh independent full-product review and changed-only fixes PASS; canonical source archive and all 23 original blobs/bytes verified remotely.

PENDING: publisher admission/actual input consumption, isolated NIGHTLY preview PR/deployment, remote/live 55-file byte parity, and live-origin browser journeys. This executor has no supported Work-thread creation/readback tool and has not created a successor or changed NIGHTLY gh-pages.

Live Chromium here failed environment certificate trust; normal verified-TLS HTTP tool reads passed. A persistent NSS trust change was rejected before execution. Do not bypass certificate validation or retry that security change. Use the existing publisher's normally trusted browser.

No public playable emulator, hardware companion, physical keyboard, acoustic or realtime acceptance follows from this website qualification. Emulator and companion limitations are accurately visible in the candidate. No device action is part of preview publication.

First substantive consumer action: verify ZIP and runtime manifest, reconstruct/inspect the exact candidate, execute a scoped phone-width product/install/files/emulator-status/companion-status journey, and record hashes and actual results before publication. Receiving this handoff alone is not consumption.
