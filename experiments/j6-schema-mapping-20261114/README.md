# Lossless J6 bank observation proposal

This standalone investigation utility preserves an accepted bank's exact bytes
and produces an integrity-checked index of parameter identifiers and values.
It does not convert presets, assign parameter meanings or emit DNPR files.
Every target in the index remains unresolved.

The synthetic fixture checks unknown-field retention, exact decimals, large
integers, negative zero and out-of-normalised-range values. The source bank bytes
remain authoritative; decoded numeric strings are an analysis view.

Run the suite with Python 3.12 and the separately supplied external corpus:

```sh
cd validator
J6_ARCHIVE='/absolute/path/J6presets(1).zip' python3 -m unittest -v
```

The external fixture must match SHA-256
`ab702e00b4cc425c9406b9b2b44e68ba0f40e1125d591fcaa422cc6a8c6272b8`.
It is not included. A missing or mismatched fixture fails instead of skipping.
The corpus contract is 12 banks, 640 presets, 174 identifiers, 112 common IDs.
The coverage manifest contains identifier counts and unresolved semantic fields;
it contains no supplied preset values or bank payloads.

`map_bank(raw_bytes)` returns an observation envelope. `restore_bank(envelope)`
rechecks its hash and rederives its complete index, returning the original bytes
or rejecting an inconsistency. Altered indexes, fabricated targets, duplicate
envelope keys and type substitutions are rejected.

Generated envelopes contain the source bank payload as Base64. Keep them under
the original content's distribution restrictions; do not publish or commit them.
Preserve the original archive separately for ZIP metadata and sidecars.

Exact byte restoration does not establish app acceptance, installed product
identity, source units/defaults/enums, audible fidelity or device compatibility.
No firmware, UI or device path is involved. This is not a hardened input service:
envelope JSON and Base64 decode before the inherited 8 MiB bank-size check.
