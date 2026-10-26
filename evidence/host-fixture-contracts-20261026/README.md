# Current-contract host fixture repairs

The strict OTA identity parser exposed three fake responders still emitting
zero declared length and no payload checksum. Correct them to the existing
27-byte firmware envelope. F02 and slot-install scenarios retain their discovery,
transition ownership, backup/restore and failure assertions. F03's required
probe scenarios now identify sample-aware firmware, rather than the explicitly
admitted stock016 exception. A valid empty80KiB layout replaces invalid zero
count/size metadata. The stock no-response exception remains an explicit
separate positive case; required-probe failures still assert zero upgrades.

The preflight capacity case previously filled raw bytes with invalid pattern
flags and motion targets, hitting unsupported-layout rejection before capacity.
Reuse maintained staging valid musical capacity data and assert its layout;
retain -7, no writes and recovery of the prior saved project. Unsupported-layout
-9/no writes is now explicitly separate, as is the sound-hash collision check.

Only test files changed. Strict parser and all storage/backup guards remain
unchanged. Before, intermediate and final logs are preserved in this directory;
these are author macOS proof, pending independent review and combined-pin checks.
