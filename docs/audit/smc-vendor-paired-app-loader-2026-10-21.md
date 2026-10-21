# Paired app and update-loader checkpoint

Source-only, **non-installable**. The ordinary TEST activation interlock remains.
No package or hardware installation was produced by this replay.

The committed `tools/studies/smc_bundle_loader/build_combined.py` independently
rebuilds the real app, seals its ELF-bound external UI descriptor, pins that exact
app into the separately staged bundle loader, and rebuilds both together. The
second app is byte-identical to the first. This closes the combined build/pin
coherence check; it does not replace complete host response, cold-start recovery,
or actual SMC tests.

| Resource | Actual | Limit / remaining |
|---|---:|---:|
| TEST app | 521156 B | 524000 / 2844 B |
| App main RAM | 98100 B | 98304 / 204 B |
| App BLERAM | 2936 B | 4096 / 1160 B |
| Musical pool | 335484 B | 344064 / 8580 B, including mandatory 8192 B guard |
| Separate loader image | 11776 B | staged below its record sector |
| Packed loader | 10051 B | separate package entry, not app primary bytes |
| Loader BSS | 11520 B | unchanged; no measured stack high-water |

Controller remains the separately resolved 103976-byte candidate: primary93180,
constants7272, RAM-load3524; low RAM26352/26368, admitted high CPU6444/7168.
No musical/sample/boot/IRQ/PC guard reduction is included. Controller transient
heap/stack sufficiency is still unproven; the scoped1936-byte CPU relocation is
not a live high-water measurement.

The default app was freshly rebuilt after the production OTA gate edits:
548480 B, SHA256
`4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9`,
identical to the retained baseline. Its original separate loader remains7280 B.
Only the explicitly prepared study uses the new paired loader.

## Tests and evidence

- Final helper forward interruption sweep:5374 operation boundaries passed.
- Forward and actual approved816 logical rollback torn-prefix sweeps:317 each.
- Actual `ota_session` positive staging uses the resolved10051-byte loader,
  validates staged bytes and persistent/RAM record equality; gate/legacy-marker/
  uncertain-record negative cases preserve NOR.
- Final-record torn erasure preserves the staged body and does not clear RAM
  ownership on error. This is not a vendor SPL execution test.
- Exact ELF/app pin and external UI/padded-controller record-tag tests remain.
- Paired artifact hashes are in the adjacent JSON. Private ELF/map and fixture
  files remain under `reference-smc-measure/combined-replay`; no vendor binary,
  ELF/map, key, or generated package is committed.

Logical rollback means pinned816 firmware with explicitly cleared unused
external extents, **not** byte-for-byte original flash restoration. Store/owner,
samples, calibration and bootloader must remain untouched.

Replay with the verified SDK/toolchain environment already sourced:

```
python3 tools/studies/smc_bundle_loader/build_combined.py \
  --repo /path/to/source --out /new/outside/source/directory
```

The runner refuses an existing/in-repository output, checks unconditional
packaging suppression, enforces the actual524000-byte app ceiling, and requires
app byte identity after the separately pinned loader is integrated. Final
component response binding is being tested separately. No UFW/FWSC writer,
installer activation, hardware/cold-RF/SPL proof, or firmware admission is implied.
