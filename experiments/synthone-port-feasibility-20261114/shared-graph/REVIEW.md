# Shared graph source projection review

Reviewed project base: `0258bfd846fdd00d5df8d4e7309b2c5888e218a3`, tree
`8530485c500455d77ca2b5b2a8c303db19d29959`.
Upstream pins: Synth One `6466a3715c96b7ecf1dd255a218cbd571408a314` and
AudioKit `03fecf80a66be88f6ac0356cd532c72dbe09922d`.

The nine-file source allowlist has SHA-256
`b78f31df44c910eae4416dff4fbed7bdd496d15484899f323fbd837dfb8cd6a3`.
All nine lengths, SHA-256 values and Git blob identities were independently
verified. Files are regular text with intended Git mode 100644; no unexpected
payload file or symlink was present before adding this review.

| File | SHA-256 |
| --- | --- |
| `.gitignore` | `c7cd0fc55d4b88bc4ed87b1cdc33670d9046174ab762188a0f2ce10290bb56e0` |
| `DEPENDENCIES.json` | `00aa7392ffc13b59deef1a05315cc86cedd751522c54817744319676effc4500` |
| `README.md` | `f413152267b3eb176bb2d029c6f9d01699b7d22ef186f58e1bd1dba066d5572e` |
| `prepare_tables.py` | `ef0b3a9481ea73db462845b5e6bba187c2a022f26b5c9098d7f462476b3a315a` |
| `run.sh` | `afa288451bb7e5d7bf3d10620f25d615283a03d645135cb3027ba0b64636a7d0` |
| `shared_graph.cpp` | `970a5aadafe8eb17043e6acf7afe02821e95dc6b6e554182f099ae1da0f910f3` |
| `shared_graph.hpp` | `137576968b44d5508f919ad54a49a717a3bcec0482e25cde64bb3c7a26231bfc` |
| `tests/shared_graph_test.cpp` | `cd733d4d55f23bbf7ae3c6e8fc35cec239d185a72382378d4558cb880aca7302` |
| `tests/test_bindings.py` | `e72a0fba7082987109e3e9f69f790cfbdfaa4386e0383960d51af8049a6e4b0e` |

## Closure and source equivalence

All 22 dependencies matched the fixed `DEPENDENCIES.json` closure, whose SHA-256
is `00aa7392ffc13b59deef1a05315cc86cedd751522c54817744319676effc4500`.
Twenty existing module/header/oracle/notice inputs are byte-identical to the
reviewed graph closure. The existing interface changes only its first notice-path
comment; the additional dependency is the pinned public-source manifest.

The graph projection changes include paths and a license-location comment only.
The C++ test additionally includes `<limits>` and requires IEEE-754 float32.
Graph, note, LFO and oscillator/filter arithmetic is unchanged. Existing source
and notices are reused without duplicate module or table payloads.

The preparation helper uses explicit checks that remain active with Python
optimization. Its hard-pinned dependency manifest also pins the public input
manifest, preventing self-consistent manifest substitution. All 54 caller-supplied
waveform/index/frequency inputs validate before payload output or compiler
invocation. The runner resolves caller-relative paths before changing directory,
quotes the input path, creates a fresh build directory and links an explicit
sixteen-object list. Its source requires Linux/POSIX and GNU-compatible tooling,
as documented.

The allowlisted payload contains source, tests, technical hashes and documentation.
Generated table payloads, audio, preset-bank data and parameters, executables,
SDK/toolchain material and unrelated execution records are excluded.

## Validation reviewed

The exact nine payload files matched the checkout used for the retained projected
GCC 14.2 `-O2` execution. Its result matches the reviewed graph result: 15 cases,
42,864 bitwise PCM frames, 200,352 source voice calls, 42,909 complete shared-state
snapshots and zero intercepted heap calls during graph actions.

Retained binding regressions report four positive and 24 negative preparations
across normal Python, `-O` and `PYTHONOPTIMIZE=1`, including stale-output preservation
and manifest-substitution rejection. Retained runner negatives reject changed
source/dependencies before producing object files or replaying results, and reject
truncated/trailing payloads before voice rendering. These execution results were
inspected; no DSP suite or preparation suite was rerun for this projection review.
Independent validation in this review consisted of exact byte/closure checks,
source transformation comparison, runner/helper/test examination and payload-boundary
inspection.

## Disposition

No actionable findings. Accepted as the exact portable host source projection on
the pinned project base. This review introduces no implementation change.

This remains a six-note ownership and explicit prefix-scheduling experiment.
Complete upstream event/mono scheduling, ramps, global FX, tuning, arp and
persistence are absent. The 7,272-byte graph size is a 64-bit host figure; 868,564
bytes of external table data still need a target storage architecture. Target
link/math/resource/CPU measurement, full engine fidelity, device and release
qualification remain open.
