# PR #715 scoped evidence archive (text transport)

Original ZIP length: 15810 bytes
SHA-256: `51a5ac53f82b5294a13ec42e96a52e5cc87763b7b855139db03be7c1345ab0e3`

The accompanying `pr715-sanitized-evidence.zip.b64.txt` contains the exact original ZIP encoded as Base64 (single line). The ZIP has only text sources, logs, manifests and patches; no firmware or proprietary headers.

To reconstruct offline:

```sh
base64 --decode pr715-sanitized-evidence.zip.b64.txt > pr715-sanitized-evidence.zip
sha256sum pr715-sanitized-evidence.zip
```
