# Loose-folder 605-version inventory: metadata-only comparison

The uploaded inventory records **605** source versions / **949** source path aliases across six folders; **556** candidate text-version metadata records satisfy the path exclusion filter, while 49 metadata-only generated/instruction/unsafe-path records are excluded from GitHub publication. This is a **metadata SHA check, not raw content verification**; the 605-version archive containing their source bodies is not available. The separate broad text-source restoration archive does not contain checked sample 605 objects.

Git source pins: #734 `50866a1e43e3d2448daba4cc64a7a112c0a7cf42`, #739 `811d500e26061f6e68b9beb3de03d6cb50938ed5`. Legacy local aliases are normalized to repository paths for the comparison. Every eligible version has original alias(es), reported SHA-1, SHA-256, size, and exact-source status in the two TSV files.

| Tree | Exact active-path I | Blob-only R | Active path differs D | Not at expected path A |
|---|---:|---:|---:|---:|
| #734 | 115 | 11 | 122 | 308 |
| #739 | 115 | 11 | 122 | 308 |

I/R/D/A are **source byte-version statuses**, not full feature acceptance. Until the 605 source bodies are provided, do not claim complete audit or successful source preservation for these entries.
