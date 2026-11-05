# Worst-case real-time load (synths + drums together)

Generated: 2026-11-06 01:41:37  (Asia/Vladivostok UTC+10)

## Scenario

Simultaneous worst case on the production host engine path:

- Maximum configured synth polyphony: Chicago + Miami (unison) with 8 MIDI notes/part/half flooding the queue
- Dense sequencer pattern: all drum lanes, bass accents/slides, break steps at 300 BPM
- Live retriggers: every 4th half hits all kit drums; break live gate
- Chicago Hall insert enabled; global send FX near-max; master comp/filter/limiter engaged
- Sequencer running; USB full; SMC BLE traffic + `ui_frame`/`smc_frame` in the foreground
- Geometry: **512 frames / half @ 44.1 kHz = 11.609977 ms budget** (86.13 halves/s); two `engine_render(256)` calls per half (+ `engine_volume`), matching `main_fm1.c`

## Harness

Existing project docs/probes consulted:

- `docs/perf/engine-load-20261031.md` (ns/frame cases; not full simultaneous)
- `docs/perf/engine-peaks-20261031.md` (synth-only peak probe; excludes drums/master/UI)
- `docs/evidence/build-0.0.24.1/performance.json` (UI frame observer; not engine half)
- `tests/host/idle_cpu_probe.c` (silence/idle stages; not worst-case)
- `tests/host/audio_path_stress.c` / `.py` (**closest existing** simultaneous stress)

New focused harness (scratch combined only; **not pushed**):

- `tests/perf/worstcase_half_bench.c`
- `tests/perf/run_worstcase_half_bench.py`
- Extends the stress setup with full `eng_prof` stage JSON, dense drums, Chicago Hall insert, live kit hits

Compiler: `cc -O2 -ffp-contract=off -fno-fast-math` + production `ORIGINAL_FLAGS` / sources (no `X0X_FAST_FMA` on host).

## Trees

| Tree | Tip | Notes |
|---|---|---|
| Base | `e9e6643e612b9fb4e968a77d016f3bb24213dc88` | worktree `/workspace/opt/wt-base-e9` |
| Combined | `49410097b24976ff47ceb773ce188cab9c705a14` | `lane/optimizer-combined-20261106` local only — **do not push** |

Cherry-pick: **33 OK**, **0 CONFLICT**, **1 MISSING** (zl-silence branch absent on remote). Full table: `worstcase/combined-conflicts.tsv`.

## Audio identity

Round-0 `audio.i32` SHA-256 **identical** base vs combined (`2536d3a0…`). For this workload, including idle-presets `73e8f517` (decode-range behavior), PCM stayed bit-identical. No per-commit audio delta observed in this scenario.

## MEASURED host results (x86_64)

**Host:** Intel(R) Xeon(R) Processor, 8 CPUs, Linux box.  
**Label:** MEASURED = host wall nanoseconds for the two 256-frame renders + volume per half.  
**Blocks/round:** 128 halves (~1.49 s audio). **Rounds:** 3 (median peak round used below).

| Metric | Base e9e6643e | Combined | Δ |
|---|---:|---:|---:|
| Median peak / half (ns) | 991916 | 904144 | -8.85% |
| Median peak / half (ms) | 0.992 | 0.904 | |
| Peak vs 11.6 ms budget | 8.55% | 7.79% | |
| Average / half (ns) | 664755 | 666691 | +0.29% |
| Deadline misses (>11.6 ms) | 0 | 0 | |
| Foreground UI+SMC peak (ns) | 308423 | 326484 | (main loop; not inside DMA half) |
| Foreground average (ns) | 229724 | 234026 | |

Min/max peak across 3 rounds: base 903749–1921558 ns; combined 876803–1060251 ns (host scheduling noise).

### Per-component (`eng_prof` host ticks ≈ ns on this plat)

| Stage | Base avg ticks/half | Combined | Δ | Share of base |
|---|---:|---:|---:|---:|
| 909 | 294887 | 294702 | -0.06% | 45.8% |
| 808 | 163528 | 164611 | +0.66% | 25.4% |
| Chicago | 42688 | 43362 | +1.58% | 6.6% |
| Miami | 69048 | 69975 | +1.34% | 10.7% |
| BRK | 13467 | 12848 | -4.60% | 2.1% |
| FX | 29251 | 28721 | -1.81% | 4.5% |
| MASTER | 14627 | 14910 | +1.94% | 2.3% |
| SEQ | 16729 | 17104 | +2.24% | 2.6% |

**Biggest cost:** **909 drum kit (~46% of profiled render work)**, then **808 (~25%)**, then **Miami (~11%)**, Chicago (~7%), FX (~4.5%), SEQ (~2.6%), MASTER (~2.3%), BRK (~2%).

### Interpretation

Most optimizer commits target **idle/settled** skips, coefficient caches, and unity FX volume. This worst-case keeps every stage busy, so **average half time is essentially unchanged** (~+0.3%) while median peak moved ~−9% within host noise. Do **not** claim a device underrun fix from these host numbers.

## ESTIMATE (device / SMK-37)

- Device toolchain **not on the box** → absolute AC79 milliseconds **NOT_RUN**.
- Honest estimate: stage **share ratios** above are the transferable picture; absolute host ms must not be quoted as device ms.
- If device stage mix matches host, the binding hotspot remains **909+808 drum render (~71% combined)**. A hopeful host peak ratio combined/base ≈ **0.91** is **not** a certified device speedup.

## What reduces the hotspot next

1. **909/808 active-voice DSP** (largest share) — new engine/drum opts with host bit-identical tests; not covered by idle-skip lanes.
2. **Miami/Chicago active voice** — zl-coefcache/lut already merged into combined; further peak-path work needs synth_peak-style proof.
3. **Global FX** — engines/zl already touch FX; remaining headroom is small vs drums in this scenario.
4. **UI (Dev only — list, no ui.c edits here):** foreground `ui_frame`/`smc_frame` ~0.23 ms host average while knobs/faders move; Dev LCD/blit opts if main-loop contention with audio is observed on device (`x0x_worst` / `audio_xruns`).

## Combined-branch conflict table

Cherry-pick onto `e9e6643e` (local `lane/optimizer-combined-20261106`, tip `49410097b24976ff47ceb773ce188cab9c705a14`):

| Result | Count |
|---|---:|
| OK | 33 |
| CONFLICT | 0 |
| MISSING | 1 (zl-silence) |

```
lane	commit	subject	result	detail
port-chicago-coef	04212987c1566f25467a44f400870e4b1a4ee1bd	test(chicago): stabilize Linux ASan FX inserts under PIE ASLR	OK	
port-ima-lut	7a6546c567132a735f63314493f9093fd64d8e17	perf(ima): exact 64-entry delta rounding LUT with host regressions and scoped target evidence	OK	
port-ima-lut	44c39e3d22d5194766b7e981877106730b1a29fb	test(ima): define X0X_IMA_TABLES_DEFINE for the delta LUT host check	OK	
port-ima-lut	610285d9e4a72e58e4fc3198aa9beba234974727	test(ima): skip missing clang in the delta LUT host runner	OK	
port-dev-nonui	c19fc634f9f5c3bc0bcfdb1a75741448c91d194d	Skip equal 32-bit words in the undo difference scan	OK	
port-dev-nonui	36fbfd700e86f640a6a3832f6685da19c754edd7	Copy project stream bytes in chunks	OK	
port-dev-nonui	7db5fd8b4cbed7ec1679ff4b44c7d1f89eaa309b	Compute bus FX cutoff and rhythm periods once per call	OK	
storage	c67eb8347bbbe84d3a4c43e5948c754ea28b7a43	Skip redundant LZ run scans after a maximum match	OK	
latency	b724505e523bc14bae9a0fdcd175c60cd1b26361	Skip clip arp chord expansion after the note list fills	OK	
latency	8081c00d2552f0d5ba42e4e27485d54ff25f9066	Return raw clip key notes directly when pitch stages are off	OK	
idle	3a6c9cc0650e5db912bbe383c60f47a243e54bf8	perf: advance settled Chicago inserts without silent DSP	OK	
idle	56eb957fc86c96498cc38c441882470481049b3f	perf: skip the settled zero stereo master loop	OK	
idle	b186be6c849456de13363738f20ebbb65ebe087f	perf: omit settled zero FX glue compressor work	OK	
engines	1a1bc7ec5c2acb4c1055d36c874e737415f8127e	perf(master): avoid discarded limiter reciprocals	OK	
engines	63e33a2fb1a6a62dceb729387457ad380b09c620	perf(808): reuse tail threshold until the peak changes	OK	
engines	abb1b12d5637095f8c305ea0cac30a83101b75bb	perf(fxbus): stop idle scans at the first active sample	OK	
engines	3a47b73edd36f29eada5ab08a629863ddaa53e5d	perf(master): hoist compressor coefficient complements	OK	
engines	758c8e0d3e0bebad0e8113bc4345dee6b6a01d9e	perf(fxbus): skip the mono unity-volume pass	OK	
idle-presets	bc9bab0d014f959c9d99afd2da07610820111813	Skip unread silent BREAK scratch clears	OK	
idle-presets	3f6a7371a35dda64ce026b91c5eae5b314ca4cb3	Avoid resetting Chicago delay coefficients at unchanged tempo	OK	
idle-presets	73e8f517a9a57c94ff380f46f078799b82b5a9de	Accept actual preset descriptor ranges in the decode helper	OK	
idle-presets	ccb368f0d5b6d5259a85af519d70d0e32c14c0cb	Add host bit-identical tests for idle-presets optimizations	OK	
zl-coefcache	eb93c3ba7d217dbf2f69b1455b9126cdb0e4ba30	Cache FX reverb damping without growing state	OK	
zl-coefcache	b5be0b83dd84128a8b43940ae94ef50651735c6f	Cache FX delay tone without growing state	OK	
zl-coefcache	527059a52cb03e0ca811ee7f16a126cf7ce4e2ca	Reuse Chicago EQ coefficients for unchanged parameters	OK	
zl-coefcache	3c2ad0ecbcb42f99e432a3a69eca0207326b528d	Skip zero VCA decay arithmetic at equal sustain	OK	
zl-coefcache	5b158ba26f0ca208f1ef33d5805e0be31771eb07	Hoist Chicago Ensemble voice rates out of the sample loop	OK	
zl-coefcache	c994d659582462b150c157d88003a0db14e7d613	Hoist Chicago modulation activity rates out of sample loops	OK	
zl-lut	7fc7c7c273e22f2b6cbfb097f26d06c40b12b947	Use exact flash values for mixer level motion	OK	
zl-lut	4a6db3bc89be1b4a0ea0648599d61aa96364f5da	Use exact flash values for mixer send motion	OK	
zl-lut	79ec47e7aa0e96fa5c78a7b71fbec1ba40c6b863	Use exact control-byte tables to reduce motion parameter work	OK	
zl-lut	c6086ece072e19df87e6034726a20244c77221df	Use exact span reciprocals for voice gain control ticks	OK	
zl-lut	f1842abfafdbfcd699f0100121a256ea1b6d7c14	Look up exact BREAK retrigger probabilities to reduce trigger work	OK	
zl-silence	MISSING	(branch not on remote)	MISSING	no origin/lane/optimizer-zl-silence-20261106
```

## Artifacts

- Base run: `/workspace/dnbox/optimizer/worstcase/base-e9/`
- Combined run: `/workspace/dnbox/optimizer/worstcase/combined/`
- Conflicts TSV: `/workspace/dnbox/optimizer/worstcase/combined-conflicts.tsv`
