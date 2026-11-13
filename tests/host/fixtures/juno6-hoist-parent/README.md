# Juno exact-arithmetic parent fixture

`juno6.c` is the unmodified GPL-3.0-only source retrieved through the GitHub connector from:

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Commit: `1322293e89c1582eecb3d9b7950e14db01722065`
- Path: `firmware/src/dsp/juno6.c`
- Git blob: `902f96b57cc761f975c32bfbd1a31ebf5326dd9c`
- SHA-256: `b9bd8324ee0c430af6976ccac596c17fbfc80a1efd000930b60b2959bf159404`

This is the gain-approved v2 hoist parent, including `/1.85f`. It is not a reference render or implementation from AudioKit J6.

`tests/host/juno6_exact_test.py` rejects a changed fixture and verifies that the candidate checkout's `juno6.h` and `fastmath.h` still match this parent before compiling the two independent translation units with identical floating-point flags. The test compares PCM bit patterns and the complete initialized state, including chorus history, at each operation/block boundary. Boundary injections cover finite, nonnegative release-envelope state, including signed zero and values around the release floor and retirement threshold. The deterministic event stream exercises the public API.

Run from the repository root:

```sh
python3 tests/host/juno6_exact_test.py
python3 tests/host/juno6_exact_test.py --sanitize
python3 tests/host/juno6_exact_test.py --opt Os --bench
```

The optional benchmark emits raw ABBA nanosecond timings for idle, held and releasing voices, always with chorus. Each run records both monotonic wall time and thread CPU time so scheduling delays remain visible. Neither establishes a target CPU result. Sanitizer timing is rejected. `--cpu` can select an allowed host CPU for a controlled timing run. Timing acceptance requires review of the actual measurements; the harness does not label noisy timings as a performance pass.
