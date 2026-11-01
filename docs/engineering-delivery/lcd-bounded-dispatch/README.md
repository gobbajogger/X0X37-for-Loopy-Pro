# Bounded LCD transfers and foreground dispatch

Base: `build/0.0.24.1` at `c20943a75d9e680a64e0384458245db4d7999e05`.
Task branch: `fix/0.0.24.1-lcd-bounded-dispatch`. Latest observed `wip/one-build` before this work: `02cc7d104254b4427f257c310fb366bfc16a8e02` (not used as the task's explicit base).

## Why

At the SMK-37's locked 12 MHz SPI setting, a 240 x 240 RGB565 frame occupies 76.8 ms of wire time. A main-band full transfer is 66,240 bytes (44.16 ms), and the 240 x 82 knob strip is 39,360 bytes (26.24 ms). The old row/column hash took the Cartesian product of changed rows and columns, inflating sparse changes into broad DMA rectangles. Synchronous `ui_frame()` then deferred foreground note/MIDI dispatch.

## Change

The canvas renderer and colour math are unchanged. Main and knob regions now hash independent 16 x 16 pixel tiles and merge only neighbouring dirty tiles, three horizontally per DMA at most. Header and full-strip UI transfers also use the same bounded staging buffer. Each operation copies at most 1,536 bytes into a dedicated, reusable .bss buffer and calls the existing physical-key/MIDI dispatcher (and SMK clip-keybed dispatcher) while the transfer is active. The next operation waits for that buffer before reusing it. LCD control commands, DMA ordering, audio DSP, IRQ priorities, key/note queue logic, and device identity are unchanged.

The previous native evidence records a pool of 335,736 / 344,064 bytes, with just 8,328 bytes free. Therefore the new scratch lives in .bss, **not** .pool. The old row/column hashes used 1,120 bytes of .bss. New tile hashes use 900 bytes and the scratch uses 1,536 bytes: a calculated net increase of 1,316 bytes against the baseline's 2,432 bytes of unallocated .data+.bss. These values are **estimates from source declarations**, not an SDK linker PASS. The pool's base usage should remain unchanged, but still requires measurement.

## Optional hardware trace (disabled by default)

Build a development, non-versioned image using `X0X_LCD_TRACE=1 BOARD=smk37 MODEL=elite ./build.sh` (or `MODEL=pro`). With the flag off, all trace code, counters, and its PERF presentation compile out. When enabled, the second PERF band displays KEY, MIDI, LCD DMA, FRAME interval, TIMER5 and AUDIO callback timings as **P50/P95/worst microseconds**, sampled over the latest 32 measurements, plus LCD pixel bytes per last UI frame. KEY arrival is the TIMER5-observed physical key change; MIDI arrival is the TIMER5-observed ring write counter change. Readouts are approximate under simultaneous events and have diagnostic-mode overhead.

Do not use these development images for publishing, install or performance qualification. No device was flashed in this task.

## Standalone regression

Run `sh tests/host/lcd_bounded_test.sh`. The C test exercises the production tile hashing/coalescing/packing callbacks with full-frame pixel reconstruction, unchanged-frame no-DMA behavior, a narrow playhead, distant dirty pixels, and scattered knob changes. The telemetry test checks P50/P95/worst, key/MIDI dispatch timestamps, frame byte accounting and DMA duration. These are focused tests, **not** a substitute for screenshot comparison or the full host/SDK gates.

## Required qualification still open

- Audio identity: 17 scenarios and 50 files against this exact base.
- Pixel-identical PNGs for all host UI scenarios.
- Full host regression: no additional failures relative to the same base (published base evidence: 33 failures / 2 NOT_RUN at the earlier candidate commit).
- At least 5 alternating base/candidate host-performance runs.
- SDK **V1.2.1**, PADCAL=0, Elite and Pro builds, strict SMK-37 Elite_900/Pro_900 names, pooled headroom >= 8,192 bytes.
- `package_nightly --check` including controller SHA256 `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`.
- Real-device tracing for playhead move, knob turn, SMC popup and idle, with BEFORE/AFTER bytes per frame and worst foreground stall.

The indexed project Library did not expose a complete V1.2.1 archive for this cloud environment at the time of implementation; the mounted V1.2.0 archive is not an approved substitute. All unrun gates remain **NOT_RUN**, not PASS. Do not merge or publish based on these focused tests alone.

## Synthetic transfer comparison

The standalone `lcd_bounded_scenarios_test.c` uses the exact old row/column hash algorithm and the new tile algorithm on four deterministic host pixel arrays. Numbers below are **synthetic transfer bytes**, not a record of physical hardware behavior. The worst blocking interval is the pixel-wire-time **estimate** (12 MHz SPI), excluding commands, ISR preemption, and input work.

| Fixture | Old bytes | New bytes | Old max payload time | New max payload time | Old DMA ops | New DMA ops |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Playhead: two separate moving columns | 8,640 | 3,072 | 5.760 ms | 0.342 ms | 1 | 6 |
| Knob: three separate changing strips | 11,040 | 3,072 | 7.360 ms | 0.342 ms | 1 | 6 |
| SMC popup: entire 240x82 band | 39,360 | 39,360 | 26.240 ms | 1.024 ms | 1 | 30 |
| Idle: no changed pixels | 0 | 0 | 0 | 0 | 0 | 0 |

True foreground key/MIDI stall for each scenario on `0.0.24.1` versus the candidate remains **NOT_RUN**. Extra per-operation LCD command overhead and input work must be captured on target before claiming the <=2 ms dispatch goal.

### Trace allocation gate

A first opt-in target build failed: 64 historical values per telemetry series allocated 1,584 bytes to the already-tight pool, leaving only 6,744 bytes and failing the mandatory 8,192-byte headroom test. Reduced the opt-in history to **32 x uint32_t values per metric** and moved its 816-byte allocation into `.bss`; the normal production build has `X0X_LCD_TRACE=0` and allocates neither. This is an intentional diagnostic-only change, not an audio or control change. Full SDK re-verification must still confirm the trace-on image fits.
