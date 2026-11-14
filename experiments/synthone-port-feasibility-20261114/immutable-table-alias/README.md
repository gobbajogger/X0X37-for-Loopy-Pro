# Immutable exact-row aliases for the Synth One shared graph

This isolated source experiment packs the 52 public morph rows into 50 immutable
rows while preserving all 52 logical descriptors. It uses the unchanged shared
graph from project PR #934 at `a0c8698f724e28ec6275307525933c195817b3c5`
(tree `e58592f678c804798aa2250da84f815520731ca5`). It changes no firmware,
upstream graph, presets, scheduler, waveform values, codec or precision.

The existing `Graph::init` API already borrows `const float *` payloads while
owning separate `sp_ftbl` descriptors. Its validation does not require distinct
sample pointers. Aliasing therefore needs no graph implementation change.
Soundpipe retains a mutable-pointer ABI; this experiment qualifies only the
exact pinned init/reset/compute closure, with OS read-only protection applied
before graph initialization and retained through teardown.

## Exact inputs and local use

Requirements: Python 3.9+,
Linux/POSIX, GNU-compatible C/C++ compilers, and little-endian IEEE-754 float32.
The test uses `mmap`, `mprotect`, `fork`, and signal-based write probes.

Use an existing #934 experiment checkout as `GRAPH_ROOT`, pointing to its
`experiments/synthone-port-feasibility-20261114/shared-graph` directory. The runner
verifies the exact graph `.cpp`/`.hpp`, its pinned dependency manifest, and all
22 dependencies (source, headers and notices) before invoking a compiler. It
copies only those verified inputs and the new harness into a fresh compiler
tree, verifies the copied closure, and removes ambient `CPATH`, `C_INCLUDE_PATH`,
`CPLUS_INCLUDE_PATH` and `OBJC_INCLUDE_PATH`. Unlisted files in caller-provided
trees cannot shadow the pinned headers; substitute DSP bytes are rejected.

`SOURCE_ROOT` must contain the public
`AudioKitSynthOne/DSP/BandlimitedWavetables/` input files from official Synth One
commit `6466a3715c96b7ecf1dd255a218cbd571408a314`. The included metadata-only
`SOURCE-INPUTS.json` has SHA-256
`c08dbf689ce4295acd565f52a16b5af32835edab4695133c012da9a7dd4f8020`.
All 54 table/index/frequency file bytes, Git blobs and SHA-256 values are checked;
the 52 row names/order and their original float32 conversion are checked too.
The other six manifest records are provenance, outside this cut's table closure.

The packer takes an existing **52 × 4096 unguarded** little-endian binary32
artifact, SHA-256
`874b3167827dbd7e07125afc0cf9cf4b4a642790b572878fdb9bd14544184979`.
If the available #934 artifact already has 4097 samples per row, extract its
checked positive-zero guards locally without rerunning its graph tests:

```sh
python3 pack_tables.py extract /path/to/existing/public-morphs.f32 build/raw-public-morphs.f32
```

Only the existing guarded artifact SHA-256
`0d35b5422cb5d3e0e349a66c9e0a50f2980b6287e68f8d94f55640d3f8168bd5`
is accepted by that convenience command. It checks every guard and the resulting
raw hash. It neither downloads nor generates source tables.

```sh
python3 pack_tables.py pack --source-manifest SOURCE-INPUTS.json \
  --source-root "$SOURCE_ROOT" --input build/raw-public-morphs.f32 \
  --payload build/packed-morphs.f32 --map build/map.json
python3 pack_tables.py verify --source-manifest SOURCE-INPUTS.json \
  --source-root "$SOURCE_ROOT" --input build/raw-public-morphs.f32 \
  --payload build/packed-morphs.f32 --map build/map.json
python3 tests/test_packer.py SOURCE-INPUTS.json "$SOURCE_ROOT" \
  build/raw-public-morphs.f32 "$GRAPH_ROOT"
python3 tests/test_compile_bindings.py "$GRAPH_ROOT"
python3 run_tests.py --graph-root "$GRAPH_ROOT" --source-root "$SOURCE_ROOT" \
  --source-manifest SOURCE-INPUTS.json --input build/raw-public-morphs.f32 --variant o2
python3 run_tests.py --graph-root "$GRAPH_ROOT" --source-root "$SOURCE_ROOT" \
  --source-manifest SOURCE-INPUTS.json --input build/raw-public-morphs.f32 --variant sanitize
```

Paths are explicit and may contain spaces. All supplied paths are resolved from
the caller's current directory. Each compile uses a fresh ignored build directory
and an explicit sixteen-object list. There is no download, SDK retrieval, target
build, network mutation or installation step.

## Packing and binding contract

Rows stay in their original band-major triangle/square/PWM/saw order. Each row
receives its original positive-zero guard. The first byte-identical guarded row
is retained, with a scalar logical-to-unique-row map:

- `triangle_0001` (logical 48) aliases `triangle_0002` (logical 44).
- `square_0001` (logical 49) aliases `square_0002` (logical 45).
- Every other logical row retains its own immutable payload.

The local JSON map binds the source commit/manifest/raw input, payload hash,
encoding, all counts/offsets, first-occurrence order and all 52 guarded row hashes.
Verification reconstructs the expected plan from verified original inputs and
requires exact token-equivalent metadata plus byte-identical packed data. It
rejects duplicate JSON keys, booleans/floats in integer fields, changed metadata,
bad offsets, changed source bindings, missing guards, trailing or truncated data.
All input verification completes before output files are replaced. Invalid-input
rejection preserves existing outputs. A pair of filesystem replacements is not
claimed to be an atomic multi-file transaction under I/O failure.

For each logical index `i`, set `Tables::morph[i].samples` to
`packed_base + map[i].offset_bytes`, keep `count=4097`, `band=i/4`, and
`shape=i%4`. Retain all 52 descriptors. The separate sine row is still produced
by the pinned C `sp_gen_sine` routine, has its original positive-zero guard, and
is never aliased to a morph row. Bind both payloads read-only for their entire
graph lifetime. Do not pass either to table-generation or other mutating APIs.

## Focused evidence

The new cases compare two instances of the exact pinned graph: original
distinct immutable rows versus packed immutable aliases. They compare bitwise
additive stereo PCM and field-wise snapshots of all six note processors, both
LFOs, parameters, shared PRNG/context and descriptor metadata/bindings.

Cases deliberately visit both duplicate bands and their preceding nonduplicate
band, triangle/square interpolation neighbors, just-below/exact/just-above morph
boundaries, final-table phase neighbors, automatic frequency-band boundaries,
six asymmetric simultaneous notes, FM/sub/noise/filter/LFO controls, release,
zero active notes, reset, parameter-program changes, clear and re-initialization
at 44.1/48/96 kHz. No full upstream event allocator or app scheduler is implied.

Seven subprocess writes to both alias pairs, their guards, original distinct
storage and the separate sine data/guard must terminate with SIGSEGV or SIGBUS.
A wrong triangle-to-square alias bypasses the admission validator only inside a
negative-control graph; the functional comparison must detect changed output.
Separate input regressions reject wrong maps, a nonduplicate alias, one-bit
payload changes, missing/negative-zero guards, endian swaps, altered public
source, substituted manifests and input/output collisions, including optimized
Python CLI rejection. Payload SHA-256 and source closure are checked after the
test as well as before it. Existing #934/profile/906-preset suites are not rerun.
Independent review found that verification of intended files alone allowed an
extra header to modify both comparison graphs identically. That defect was
reproduced with a noise-compute substitution. The repaired runner compiles only
the fresh allowlist. New compiler-binding regressions first demonstrate RED
header/environment controls, then check that staging and environment cleanup
exclude both, and that a pre-existing compiler tree is refused.

`RESULTS.json` records the retained focused runs. Generated `.f32` files, scalar
binding maps, objects and executables remain local under ignored `build/` paths.
The public source payload contains code, tests, this documentation, input/hash
manifests and scalar result summaries only. No supplied preset banks, extracted
preset arrays, rendered audio, generated waveform arrays or toolchain binaries
are included.

## Storage result and first uncovered boundary

The guarded morph payload is **819,400 bytes**, down from 852,176. The unchanged
separate sine is 16,388 bytes, so their sum is **835,788 bytes**, down from
868,564: **32,776 raw bytes saved before map/format metadata**. The packed morph
SHA-256 is `5e174ae6d1e4c13726831d505fe4c32609c87ff78d80b6b2b9264a0d3464e595`.

This remains too large to infer current target fit. There is no target storage
layout, read bus/cache path, decode path, target RAM/stack measurement, timing,
CPU saving, application/preset fidelity, device or release acceptance here. The
next storage boundary is an actual bounded immutable target-read representation
and exact linked layout, followed by worst-case musical table-access workload
and source-oracle qualification on that representation. No compression or
precision change is selected by this cut.

Original MIT/module author notices remain in #934's pinned parent experiment
`notices/` and DSP sources. The comparator field lists in the new harness are
adapted from that experiment's existing host test; its old cases are not reused.
