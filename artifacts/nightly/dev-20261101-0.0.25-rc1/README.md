# 0.0.25-rc1 QA IN PROGRESS

Unmerged, source-only NIGHTLY handover entry for `0.0.25-rc1`. This entry is separate from the published `0.0.24` (`nightly-d8086cc0-dev`) and makes no change to its manifest, binaries, catalog selection, or download URLs.

## Verified build-only inputs
- Base: `d8086cc09203945919c70d3c805f8ca3e6acbe5a` (`build/0.0.24`)
- PR #813: `a41651e9391012c19f9e09df04226f1f0c438afd`
- PR #814: `dcc9833f24b20ec2baf47beb131f937e32d95e11`
- Local combined candidate SHA (not pushed): `8850a799484b28696abb61eb5ea630d530c170b5`
- SDK AC79NN V1.2.1; `X0X_DEV_PADCAL=0`.
- Native SMK-37 Elite and Pro builds **PASS**.
- `package_nightly --check` **PASS** for both exact model identities.
- Wrong-model and missing-controller packages are rejected for both models.
- Measured pool headroom **8,200 B** for both (minimum 8,192 B).
- Bundled controller: 61,552 B; embedded header CRC32 `564a8f55`; SHA-256 `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`.

## Explicit hold
Independent QA is still pending. This entry has `installable: false`, `published: false`, and no download URLs. **Do not merge, publish or offer an installer candidate** on the basis of this source-only PR. Another owner handles eventual release/publication.

The exact checked application/controller firmware images, all nine file sizes/hashes and the build receipts are preserved on the authorized Mac in `/Users/mac/X0X37-CANDIDATE-0.0.25-RC1-20261101/`. The binaries and proprietary controller are not committed in this PR.
