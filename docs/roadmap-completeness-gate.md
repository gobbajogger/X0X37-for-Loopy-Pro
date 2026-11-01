# ROADMAP source-completeness gate

This gate checks source presence, not behavior, native builds, hardware, review
approval or permission to publish. The checked-in audit still reports **2 PRESENT,
144 MISSING and 62 QUEUED**. It cannot authorize a release.

## Independent inputs

The existing integrator/publisher must independently resolve the intended
firmware commit/tree, accepted ROADMAP commit/blob and accepted
`docs/release-selection.json` commit/blob. Never take an expected pin from an
evidence file or the report being verified. The selection manifest in this repair
is a **proposal pending the existing independent review**, not an approval.

The manifest binds all built/accepted ROADMAP IDs and the six owner-selected
fixes. The gate derives built IDs and explicit `**ALIAS → CANONICAL-ID**`
declarations again from the pinned ROADMAP. It rejects omission, remapping,
duplicate declarations and external-to-firmware reclassification, even if a
reduced manifest is recommitted. Unbuilt canonical rows remain QUEUED; selected
source fixes attached to those rows are checked separately. Each selected alias
needs its own proof entry; a broad canonical row proof cannot satisfy it.

`required_release_rows` in the source-evidence map is only a consistency check.
It must exactly match the independently pinned selection and cannot define scope.
Proofs still require exact Git blobs or uniquely anchored byte spans. Both
generation and verification read Git objects, not mutable worktree files.

```sh
python3 tools/roadmap_completeness.py --repo . \
  --candidate "$FINAL_COMMIT" --candidate-tree "$FINAL_TREE" \
  --roadmap-ref "$ROADMAP_COMMIT" --roadmap-blob "$ROADMAP_BLOB" \
  --selection-ref "$ACCEPTED_SELECTION_COMMIT" --selection-blob "$SELECTION_BLOB" \
  --evidence docs/roadmap-source-evidence.json --report "$REPORT"

python3 tools/roadmap_completeness.py --repo . \
  --candidate "$FINAL_COMMIT" --roadmap-ref "$ROADMAP_COMMIT" \
  --selection-ref "$ACCEPTED_SELECTION_COMMIT" --selection-blob "$SELECTION_BLOB" \
  --evidence docs/roadmap-source-evidence.json --verify-report "$REPORT"
```

Package checks and local nightly previews require the same `--selection-ref` and
`--selection-blob` alongside their existing completeness/candidate/ROADMAP options.
Missing pins, stale reports and changed selection fail before package reads or
preview writes. There is no production test bypass.

## Separate installer prerequisite

`FIX-INSTALLER-BACKUP` is explicitly external. A firmware proof, PR link, local
download copy or caller-supplied PASS cannot satisfy it. Its independently
accepted manifest entry must pin `repository`, `commit`, `tree`, `path`, `blob`,
`live_url` and `live_sha256` under `external`. Only the existing NIGHTLY repository
and its HTTPS installer URL are supported. Missing source/live pins remain
blocking; this proposal deliberately invents none.

Its evidence entry has `kind: "external"` and a `source` object using the existing
blob/span proof schema. Supply the separately materialized Git repository with
`--external-repo gobbajogger/x0x37-for-Loopy-Pro_nightly=/path/to/site-source`
to generation, fresh verification and consuming package/preview calls. The gate
checks the manifest's exact external source objects, then fetches and hashes
live HTTPS bytes again on each evaluation. Redirects, unavailable reads, oversized
responses and mismatched bytes fail closed. This establishes exact source/live
identity, not backup behavior or device safety; those existing tests still apply.

## Publication boundary remains separate

`publish_nightly.py` prepares local previews and still refuses `--push`.
`publish_pages.sh --nightly` delegates to that same checked local path. All legacy
shell publishing modes now refuse before gate/Git/Pages I/O: their old force-push
was not authorized by a gate PASS, so no replacement publishing path was added.

The sole publisher still owns enforcement at the **actual external GitHub API
mutation boundary**. It must execute generation and fresh verification on the
same independently resolved pins before the first external write, retain all
existing package/model/controller/host/native/budget/catalog checks, and prove
zero external writes for negative cases. Local preview or shell tests are not
proof of uploader adoption. No merge, publication or device operation is part
of this repair.
