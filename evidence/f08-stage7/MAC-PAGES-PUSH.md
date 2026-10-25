# Mac gh-pages push — retry requested (parent tip drift)

## Attempt 1

- Expected parent: `d6a237c43cb6830d8cf0cb51e567181821630a28`
- Actual tip: `9d2872470f474b8d6b68ce8170692662199661e8` (compact installer presentation)
- Result: **FAIL** parent mismatch; no force-push
- Mac agent: `bc-fd41cdd4-425d-5d0b-8321-04650975ae32`

## Attempt 2 (ready)

- New parent: `9d2872470f474b8d6b68ce8170692662199661e8`
- Prepared commit: `60cc264b2bfb2ce8a0c6004f048efef18e042415`
- Installer fingerprint (nightly main after compact UX merge): `4d29566e14c3f252bf52047676e993367e3d155aa1a94a45157ec07d77e47dc8`
- Cloud 403 log: `evidence/f08-stage7/cloud-gh-pages-403.log`
- Brief: Project store `internal/f08-stage7-mac-pages-push.md`
- Artifacts: `artifacts/f08-stage7-publish/candidate-site.tgz`, `gh-pages-f08.bundle`
- Worker: `83d16cde-07a3-5bce-81a3-2234761197a1`
