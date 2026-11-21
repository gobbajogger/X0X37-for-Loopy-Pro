# Library index (files kept only in the User's ChatGPT Library)
Filled by the backup (`backup/dot-<date>/_manifest`, rows of kind `library`). One row per file. Restore = download from the Library by name, check SHA256, place at the path shown.
| Library name | Size (bytes) | SHA256 | Restore to | Why not in Git |
|---|---|---|---|---|
| SuperJ6-0.999.8.ipa | | | `/workspace/sj6-private/originals/` | third-party app installer (RULES 13) |
| SuperJ6 AU extension zip | | | `/workspace/sj6-private/originals/` | third-party app installer |

## Consolidated firmware baseline evidence

These archives preserve generated test outputs from the consolidated baseline at `76040daa5b52d686a8b7b544296ba5d79610a95d`. [Terminal gate and recovery receipt](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7056756805)

| Private file (version 0) | Bytes | SHA-256 |
|---|---:|---|
| [76040daa5b52-verified-positive-outputs.tar.zst](https://chatgpt.com/library?file_id=libfile_fde3_f-42ae122e9a2881919c3f5f69d3add887_2021479872048191acbfdca1f870a539&account_id=personal) | 113,353,279 | `fa4b55dab947f4e6b9f6d72a0487e52da40f2247c1cdd37b4974581cc212fa93` |
| [76040daa5b52-verified-positive-outputs.json](https://chatgpt.com/library?file_id=libfile_fde3_f-678d4dff57608191b28e02ecc141b4d2_2021479872048191acbfdca1f870a539&account_id=personal) | 630,306 | `0d22e3047bf09aa99575cb5c6e0014a5aae051983ae8d1e79f7fb1e1a8f39c46` |
| [76040daa5b52-stock-replayed-ui-output.tar.zst](https://chatgpt.com/library?file_id=libfile_fde3_f-e31af57818088191bb298829f3e62c06_2021479872048191acbfdca1f870a539&account_id=personal) | 20,384,391 | `e917e39d9ea24a5a8f57ff67752b91010e00b5c86fc18ef4c5d11a2188a88410` |
| [76040daa5b52-stock-replayed-ui-output.json](https://chatgpt.com/library?file_id=libfile_fde3_f-eade8c297b788191849bbabd2d71f14b_2021479872048191acbfdca1f870a539&account_id=personal) | 241,592 | `3a2fbc336bec6d7358a5bfa7a155cf35505fe541799a05e42abd8fc020be047a` |

Download an archive and its matching JSON manifest. Check both byte sizes with `wc -c` and both digests with `sha256sum`. Extract each archive into its own empty scratch directory with `tar --zstd -xf ARCHIVE -C DIRECTORY`. Verify the extracted paths, byte sizes, SHA-256 digests and modes against the manifest.

The first manifest maps archive paths to 1,871 passing-test outputs. The second lists 747 members under `members`, comprising 370 normal/sanitized PNG pairs and seven host executables from the [stock Linux replay](https://github.com/gobbajogger/DnBOX/pull/976#issuecomment-7058333600).

## Combined firmware cloud qualification

| Recovery filename | Bytes | SHA-256 |
|---|---:|---|
| `combined-3c6a5a04.zip` | 3,179,654 | `c5d6af67efa4b26b2ae538314f28ecbfdcc200643d681d1feaf85708d4b82aa1` |

Download the named ZIP into an empty directory. Check its byte count and SHA-256 against the table, then use `unzip -t` to verify its contents. The archive contains the two stock model packages and their application and link artifacts. The latest three-lane rebuild reproduced the application, ELF and package hashes for each model. [Exact model hashes](../evidence/integ-cloud-3c6a5a04/refreshed-build-identity.json)
