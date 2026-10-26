# Open-beta metadata contract

This note is for the owner of `gobbajogger/x0x37-for-Loopy-Pro_nightly` pull request 13. It does not change that site.

The machine version and the readable label are different fields.

| Field | Example | Meaning |
| --- | --- | --- |
| `firmware_version` | `0.1.3` | Numeric identity. The same value is in the firmware, the installer catalog, and `model_9XXYYZZ`. |
| `release_display` | `0.1.3_openbeta` | Text shown beside ABOUT X0X. Not a package identity. |
| `models.elite.product` | `SMK-37 Elite_9000103` | Bytes inside the Elite package. |
| `models.pro.product` | `SMK-37 Pro_9000103` | Bytes inside the Pro package. |
| `models.*.sha256` | 64 hex characters | Package bytes. |
| `models.*.controller_sha256` | 64 hex characters | Controller image, when one was validated. |

Rules for the site:

- Show `release_display` to people. Match installs and downloads on `product` plus `sha256`.
- Do not read a version out of a file name. `0.1.3_openbeta.fwsc` can still be an unversioned `model_900` package.
- A legacy catalog entry has no `firmware_version`. Its product stays `SMK-37 Elite_900` or `SMK-37 Pro_900`. Leave it unversioned.
- Elite and Pro of one release share `firmware_version` and have different products and hashes.
- Do not mark `device_tested` or offer a flash action from this contract. Publication still needs the site owner's own approval.

The allocation record is `releases/openbeta/ledger.json` in the firmware repository. An empty `allocations` list means no open-beta number has been issued.
