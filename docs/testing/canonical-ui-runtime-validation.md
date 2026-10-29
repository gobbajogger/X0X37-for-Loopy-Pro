# Restored UI and Chicago/Miami runtime qualification

This continues LX-REQ-032 and the original-panel correction described in
[original-plugin-controls.md](original-plugin-controls.md). The owner rejected
the replacement miniature SOUND widgets and requested the existing designed
interface, its animations, and severe-workload checks. Correct original
control names, centered ENV/TUNE edits, exact untouched factories, and wheel
ownership remain part of this change.

## Local source provenance

The recovery used the owner's saved source before consulting remote history.
The complete saved snapshot
`6ae0928fe3264ee759c25c4f879eff4dc5dc3b20` contains the common semantic glyph
system and the real sequencer/tape observers. Its `firmware/src/app/ui.c` has
SHA256 `af1f4ff175f1998244e1fd7b90fd16670c316c9ba5f2d22e9c1ca4d10b77ac83`.
The saved source bundle has SHA256
`b2d09e9eb4e0f9d921f03110610aa55690232b05885fef09709824a914fa6f1b`.

This is a selective restoration. That historical file's ORIGINAL branch
itself disabled the specialized waveform/envelope display; replacing current
files wholesale would restore those stubs and obsolete parameter meanings.
The common drawing functions and presentation geometry are reused with an
explicit adapter for the current original engine IDs. Removed legacy DSP
engines, effect histories and old WBL control laws are not required by this UI.

The prior control-fix checkpoint is
`1f950312939572409b2119a42b0b4ab109b60ccf`, tree
`c9af62ca5834091ac2489427659feaebd771dc47`. Its V5 presentation is retained as
rejected historical evidence, not described as the accepted restored UI.

## Restored presentation and animation

The ordinary four-control strip uses the saved shared semantic glyph
dispatcher: level/send amounts, pan, filter, drive, compression, delay,
repeat/identity, step rate/direction, swing and tempo. Shallow surfaces,
native font hierarchy and case-preserving font generation are restored with
the minimal graphics dependencies. The old dirty continuous-knob arc raster
is no longer the default ordinary-strip path.

SOUND uses the established matrix widget hierarchy. Waveform cards read the
existing 29-point native projection cache; painting does not run a voice,
oscillator compiler or the obsolete WBL waveform formulas. The cache remains
168 bytes. Parameter changes, held-step locks and motion previews retain their
existing change keys. Tempo/tape phase changes do not invalidate this cache.

Chicago's ENVELOPE touch display retains the large shape, active-stage
highlight, four control readouts and held-step interaction. Its graph is
explicitly labelled **SCHEMATIC / CONTROL AMOUNTS (%)**. Attack, decay and
release amounts allocate schematic horizontal space; sustain sets its height.
It is not a measured audio trace or a millisecond axis. Miami gains no extra
ADSR controls.

| Animation | Actual source | Stopped/quiet behavior |
|---|---|---|
| Tempo pendulum | `seq.metro_view`, accepted sequencer clock/sample phase | Zero while stopped or waiting for external clock; no prediction of missing ticks |
| Tape reels | Actual delay input or computed wet signal, gated by tape type/return/mute | Stops after the existing 80 ms visual release; wet tails can continue after transport stops |
| MIX waveform | Existing post-master scope | Existing bounded latch and unchanged-content redraw suppression remain |
| Synth shapes and parameter glyphs | Exact controls or cached native projection | Respond to edits/motion/locks; do not invent freely running synth oscilloscopes |

Only visible relevant phase data contributes to the ordinary-strip redraw
signature. Tape animation resumes from its last phase without catching up
time spent inactive. The observer adds no buffer or second effect history.

The restored UI passed 392,840 checks in each of the normal and ASan/UBSan
runs; all 110 pairs of actual rendered PNGs were byte-identical. The native
SMK caption branch separately passed the same 392,840 normal checks. Coverage
includes 327 mapped control slots across 93 pages, all 32 tempo phases,
the tape 79/80 ms boundary and 16-phase cycle, wet-tail animation, idle freeze,
hidden-strip stability, held-step ownership and unchanged project state.
The final one-expression PERF knob-caption correction passed 30 focused
normal checks and a pixel comparison against KNOB 5. Compiler dependency and
preprocessed-source comparison ties that correction to the full run: the
other 180 local inputs and all code outside the caption expression match.

These are software-render and behavior checks. The host BREAK fixture has
no sample-slot inputs; its empty-slot images do not prove stock delivery.

## What the CPU display measures

The actual [audio ISR](../../firmware/src/app/main_fm1.c) processes a 512-frame
DMA half at 44.1 kHz, using two 256-frame `engine_render` calls. The integer
budget is **11,609 microseconds**. TIMER4 runs at 24 MHz around rendering,
the enabled USB audio tap and `engine_volume`; nested input-interrupt time is
included in that interval. The nominal exact half duration is about
11.60998 ms.

The displayed average uses the existing 16-block Q8 exponential average.
The peak tracks individual measured halves since its read/reset. A 120% peak
therefore represents approximately 13.93 ms spent against an 11.61 ms
budget; a 120% average represents the corresponding smoothed half-time.
Changing the UI frame rate denominator or rescaling this percentage
would not repair the overload; neither operation is performed here.

An additional confirmed display bug narrowed PERF load, peak and saved test
peak to eight bits: 256 became 0 and 300 became 44. The correction widens
only those display fields to 16 bits, saturating values above 65535. Raw
timer measurements, EMA, denominator, peak collection and audio behavior are
unchanged. This bug could conceal higher overload; it cannot explain a
120% reading.

The DMA dropout counter checks half parity before/after rendering. It does
not detect every possible late entry or an even number of missed half
turnovers. The peak read/reset can also lose a concurrent update. Thus zero
dropouts and one displayed peak alone are insufficient deadline evidence.
The existing boot maximum (`x0x_dbg.max_us`) and, in an explicitly enabled
diagnostic build, `x0x_worst` over90/over100 counters provide additional
evidence. Ordinary DEV does not implicitly enable `X0X_DEBUG`.

## Actual Miami idle-path correction

The whole-application call-count probe found an additional hot path in
`experiments/original_synth/miami_output.c`. The earlier exact-zero shortcut
in `miami_fx_input_rne` is used by Chicago. Miami's actual output wrapper
independently performed cleanup, scaling and float-to-integer conversion
for each input channel, including positive and negative zero.

The corrected wrapper writes the same signed 32-bit zero for exactly those
two floating-point encodings. Every nonzero input, including subnormals and
non-finite neighbors, follows the original conversion chain. Channel access
order and argument validation are unchanged. Both fixed-effect paths,
decimator reset events, output conversion, master ramp and musical state
continue to run, so a zero input does not discard a retained effect tail.
The shortcut adds no persistent state or allocation.

The maintained
[Miami output test](../../tests/host/original_miami_output_test.sh) compares
the actual wrapper with the unchanged independent full-conversion FX chain.
Native and software FMA each passed normal and ASan/UBSan runs with identical
logs. In each variant, the existing 166,410-frame all-program output test is
retained, and the new battery covers 305,280 stereo frames across all 30
factories and both flush-to-zero settings. It exercises 292,500 zero input
channels, all twelve signed neighboring test values, mixed stereo input,
48,660 frames with nonzero retained tails despite zero input, and 1,200
reset events. Dry workspace integers, PCM and complete FX/master state
match. The new battery's PCM FNV64 is `95dbf8aea24d7853`.

The actual whole-application count probe confirms the branch is reached.
For a cold mixed pair, `miami_fx_scaled_rne` and `miami_fx_fcvtzs` each fall
from two entries per stereo frame to zero; cleanup entries fall from six to
four. With two Miami parts the corresponding values are four to zero, four
to zero, and eight to four. That removes six redundant conversion-function
entries per frame for one silent Miami part, or twelve for two. At 44.1 kHz
those rates are 264,600 and 529,200 entries per second. Output conversion and
FX processing still run. These are instrumented C-entry counts, not measured
native cycles. Silent four-byte memcpy calls remain zero in every pairing.

This proves a bounded removal of redundant work in the real Miami path and
preservation of the tested output/state. Target timing remains a separate
measurement; the conversion shortcut alone is not a deadline guarantee.

## Focused observer proof

The six production observer files add publication/state only. Delay sample
arithmetic, buffers, effect histories, sequencer scheduling and outgoing
musical events are unchanged. Stopped/waiting tempo observation returns
before division. The empty-delay fast return remains; active delay adds
comparisons of values already computed in its sample loop.

The maintained
[canonical observer test](../../tests/host/canonical_observers_test.py)
compares the actual sequencer and FX bus against an optional baseline tree.
The initial restoration passed normal and ASan/UBSan runs: 937 emitted
events (428 MIDI), 2,550 normalized scheduler snapshots and 49,154,048 bytes
of stereo float PCM matched across baseline/candidate and both compiler
configurations. PCM SHA256:
`978f2e7189314582d0aca7ef80132969f55a7c11fb357a1d0b138edbbe873417`.

Its 46 engine-hook checks exercise source-extracted getter/publication,
initialization, project-hold and master access code. They are explicitly
separate from the complete application integration test.

Actual pi32v2 compilation verified `seq_t` grows from 392 to 408 bytes,
`fxbus_t` remains 18,316 bytes and its following field offsets stay fixed.
Compiler-merged engine BSS grows by four bytes, for a projected twenty-byte
static observer delta. The three object text totals grow by 508 bytes before
linking and garbage collection. The final linked firmware, including UI and
fonts, is the authority for total flash/static RAM/pool budgets.

## Severe-workload scope and acceptance boundary

The host driver uses the maintained complete application source, the real
`engine_render` and `engine_volume`, and two 256-frame renders per measured
half. The baseline matrix covers 64 configurations: all 49 dual-Chicago
effect pairs, seven Chicago/Miami pairs in each order, and dual Miami. Every
case includes cold idle, held notes and release. All 64 completed without a
runtime fault; cold-idle PCM was zero and MIDI backlog drained in every case.

Four additional combined workloads exercise both engine orders and both
same-engine pairs, all 160 ordinary motion lanes, both drum kits, actual
user-loop decoding, tape/reverb sends, queued edits, MIDI and physical-key
traffic, and actual UI frame calls at a 16 ms cadence. The driver reads back
applied motion and queue occupancy rather than counting submission alone.
It checks stop cleanup, key release, output bounds and source identity.

The final combined source passed all four heavy cases in both normal and
ASan/UBSan configurations. PCM hashes, control hashes, applied motion,
queue occupancy, key ownership and event counts match the baseline in all
four runs (baseline/final, normal/sanitized). All 160 motion lanes are
actually applied on every sampled heavy half. The deliberately saturated
command queue reaches 272 entries before rendering and carries 16 through
the continuous-input phase; the MIDI queue reaches 128 and physical-key
queue 16, both draining within each half. After stop, all three queues and
held-key counts are zero.

The maintained complete-application test separately passed 210,498 checks
in each compiler configuration on the same final executable-source overlay.
Its audio hash remains `79d686b167ffdfef`; normal/sanitized logs and all seven
effect renders match. It covers both independent parts, the 55 admitted
Chicago and all 30 Miami factories, seven Chicago effects, project roundtrip,
physical/MIDI ownership, sustain/bend/Stop, motion/base restoration and
guarded output.

Host wall/thread durations identify costly paths and compare the same input
before/after a source change. They are **not target CPU percentages**. The
host executes the second-core job synchronously and does not reproduce DMA,
nested IRQs, the SMK input hardware or the real USB audio service/tap path.
Sanitizer timings are not performance measurements.

Final UI renders, full-application PCM/state comparison, severe-workload
successor results and actual Elite/Pro native packages must be tied to the
same final source tree. BUILD_ONLY means the selected native builds and
artifact/resource checks completed; it is not hardware deadline acceptance.

Hardware acceptance requires the exact matching-model artifact, cold and
settled idle, held/released notes, dense sequencing/motion, heavy effects,
active UI/input and the actual USB configuration. Record measured maximum
half time and overload/dropout counters, together with clock/core status and
IRQ stack high water. The deadline is 11,609 microseconds; a comfortable
margin and zero over-budget halves in the stated run must be demonstrated
on the target. This change does not claim an unconditional no-lag guarantee
or that the owner's observed idle120% has been eliminated on an unmeasured
device.
