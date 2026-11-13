# Phase 1 findings — owner Chicago+arp / Juno / BREAK / 909 dropout
# Build and Test Runner, 2026-11-13 ~08:19 America/Chicago (CST, UTC-6).
# Read-only analysis. No firmware edits. No device writes. No gate changes.
# Inputs: exact 94197fc8 Elite/Pro from FMB (hashes in PROVENANCE.txt, verified here).
# Owner tempo, patches, FX and installed hash: UNKNOWN. Hardware report kept intact.

-----------------------------------------------------------------------------
(a) Why the earlier "harder" RUN PERF TEST gate missed this scene
-----------------------------------------------------------------------------

Source for the gate path: /workspace/zdprobe/dnbox360/run_perf.sh (sha d5f84382…),
ui.c perf_build / perf_test_tick (94197fc8), and the actual 94197fc8 result at
/workspace/kept-026-94197fc8/gate-20261113-202119 (and /workspace/fm1run/gate-20261113-202119).

The prior gate is a 26 s --shot (6.8 s boot+menu, then RUN PERF TEST). Firmware
says "PERF TEST: 15 SECONDS": three of its own patterns, factory sound, 125 BPM,
1 s settle + 3 s measure each, then it restores the project. It never writes
the project. The LCD ended on "PERF TEST DONE", GIT 94197FC8.

What those three patterns actually are (ui.c perf_build, comment and hits):

  0 FACTORY LOOP: 909 BD 0x1111 (four-on-the-floor), clap 0x1010, open hat 0x4444,
    accent 0x0101; 303 A only, line from tb3po_generate(); break steps 0xFFFF.
  1 ALL FIVE: the above plus 808 BD/CP/CH/OH/CB and a 303 B TB-3PO line.
  2 WORST CASE: denser 909 (SD, full CH, CR, RD, LT) and denser 808 (SD, full CH,
    CY, LT), heavier accents.

Compared with the owner's report, item by item:

  Scene
    Gate: 909 + (in 1 and 2) 808 + 303 TB-3PO lines + break *steps*.
          Default factory engines are 303A = Miami preset 0, 303B = Chicago
          preset 0 (engine_sound_defaults). Not Juno. Chicago arp is not
          turned on (CHI_ARP_SWITCH factory default is off unless a preset
          sets it; PERF TEST does not). 808 IS used in the "harder" two
          scenarios. The owner said 808 was NOT used.
    Owner: track 1 Chicago with arp on; track 2 Juno holding a triad;
           track 3 BREAK playing; track 4 909 BD four-on-the-floor; no 808.
    A higher voice count or "WORST CASE" is not the same load. Chicago arp is
    a live held-note path (chicago_arp.h: "the live keyboard path owns note
    generation when Switch is On. Pattern playback is unchanged"). A held
    Juno triad is polyphonic (up to 6 voices) and is not a 303 TB-3PO line.
    BREAK IMA decode is a different cost from "steps all on" with no loop
    bytes (see BREAK below — the gate's break may have been silent).

  Duration
    Gate: 3 s measured per scenario, ~15 s of test, 1655 halves during
    test+restore (~19 s), 2173 halves in the whole 26 s shot.
    Owner / this job: a sustained run, at least 5 emulated minutes per
    config. A short average can miss a rare spike.

  Tempo
    Gate: 125 BPM, hard-coded in perf_test_tick.
    Owner: UNKNOWN. First pass uses 140 BPM as a chosen probe, labelled
    as a chosen value, not as the owner's.

  Controls / event timing
    Gate: sequenced patterns only, motion and the overload guard off,
    no held MIDI notes, no Chicago arp, no live Juno triad, no UI
    playing after the menu. Event timing is the sequencer at 125 BPM.
    Owner: a played scene (arp on Chicago, held Juno triad). Timing
    of note-on, arp steps and any UI is UNKNOWN.

  FX
    Gate: engine_sound_defaults (tape delay on, some break reverb,
    303 sends 0, master comp/pump on). Not "FX all-on".
    Owner: UNKNOWN.

  Counters and pass line
    The gate reads x0x_dbg.max_us and late, and audio_xruns, from a RAM
    dump at the end, plus a boot-vs-test split from a second 6.8 s dump.
    Pass is "0 late halves and worst block under 11609 us".
    Elite 94197fc8: worst 11561 us, late 0, audio_xruns 0, GATE PASS.
    Pro: worst 11525 us, late 0, xruns 0, PASS.
    It does not record a raw per-half us series, a P99, a first-late
    timestamp, per-part PERF vs real cost, or a playing/idle/empty
    comparison. max_us is since boot (the test did exceed the boot max).
    Core-1 busy was sampled (avg 95.5%, peak 99.9% Elite) — that is PC
    sampling outside .c1_text, not a block-time series.

  BREAK in the gate itself
    perf_build sets brk.steps = 0xFFFF, but this image has X0X_NBREAKS=0
    and no pack in the 8440-byte app tail. So the "factory loop" and
    "worst case" may have had a silent BREAK. The gate can still PASS
    at 99.6% of the 11609 us budget from 909+808+303 alone.

Conclusion: the earlier run missed this because it was a different
scene, a different duration, a different tempo, different engines
(Miami/Chicago 303 + 808, not Chicago-arp + Juno), and coarser
counters. It was not a harder version of the owner's scene.

-----------------------------------------------------------------------------
(b) Emulator fidelity and missing instrumentation
-----------------------------------------------------------------------------

Emulator used: fm1-live 0.4.0 (31157d25c),
/workspace/emu360-target/release/fm1-live,
sha256 f3e65ccfa39874168b99494055f9a755342c41bc769257883030d15aef7b54f5,
FM1_STRICT=1, --mhz 360. Calibration note in parse_perf.py: emu raw
block times read a little high vs the device (projection x0.918..x0.973).

What is modelled, at a high level (from fm1-live help, prior gate logs,
and main_fm1.c / plat_fm1.c):

  Interrupts: ALNK0 (audio half), TIMER5 (UI input 10 kHz). The emu
  reports irq levels and ISR instruction share at the end of a shot.
  Nested-in-audio is counted in x0x_dbg.nested. Fidelity of ISR tail
  and priority vs the JieLi chip is UNMEASURED here.

  I2S / DMA / codec: the firmware's own late/xrun path is what we
  trust as the miss counter (see Deadlines). The emu advances an
  audio DMA and records samples. Exact I2S rate in the emu vs the
  device's "about 44,145 Hz" (usb.c uac_out_half comment) is
  UNMEASURED. The help text mentions 44118 Hz in --record. Do not
  treat those as the same number.

  BLE: the SMK-37 Bluetooth controller is a separate image. The
  prior gate and FM1_STRICT shots do not exercise a BLE peer.
  BLE activity in these runs will be "not driven"; any BLE ISR
  cost on a real unit is a fidelity gap. Default BLUETOOTH is OFF
  and needs a restart to turn on (MANUAL).

  USB: SMK-37 MIDI is USB, not the TRS jack (SMK37.md: "there is
  no TRS MIDI IN. MIDI works over USB"). FM1_BENCH_KEYS `mHEX`
  injects into the DIN/TRS MIDI IN jack — on SMK-37 that path may
  not reach the firmware. Scene MIDI must use --usb and FM1_USB_SEND.
  USB audio (uac_render_start / uac_tap / uac_out_mix) sits inside
  the timed render_block(); a --usb run includes that work, a
  non-USB run may not. The owner's unit may have had USB plugged
  in (UNKNOWN).

  UI: LCD, keys, encoders, pads are emulated. FM1_STRICT times LCD
  SPI (FM1_LCD_TIMED). The prior Elite shot spent 876 ms of 26000 ms
  "on the wire" (3%). UI frames are in x0x_dbg.ui_frames. This is
  not a human-in-the-loop session.

  Cycle counter: the 94197fc8 gate LCD showed
  "CLOCK ? (NO CYCLE COUNTER)". plat_cycles_cpu() is perf_cyc_ok;
  when it is 0, plat_cycles() falls back to fm1_ticks() (24 MHz
  timer). eng_prof / PERF stage % then use that timer and
  ups = 24e6/44100. cps (cycles/sample) stays 0. So on the emu
  the CPU cycle counter is missing; the 24 MHz fallback still
  feeds the meters. Whether that matches the device (owner also
  has a real SMK) is UNMEASURED. The owner's "Miami and BREAK
  PERF show 0" was a device observation, not an emu one.

  eng_prof / PERF per-part (important for the meter vs cost
  comparison): in engine.c run_job(), PROF_ADD runs only
  `if (!core)` — core 1's work is not added. Comment: "core 1:
  not timed by the cycle counter (core 0's)". A Chicago part
  "stays on core 0" (engine.c ~2615). Miami and BREAK may be
  planned onto core 1. This is a measurement fact about the
  existing probe, not a diagnosed root cause of the owner's
  zeros. The first-pass logs will put the UI meter next to
  eng_prof and the raw half time so a mismatch can be flagged.
  808 PROF_ADD on some paths includes core 0's wait for core 1.

Underrun detection (the true miss, FMB correction, main_fm1.c):

  HALF_FRAMES = 512, REND_FRAMES = 256.
  The ALNK0 ISR renders one half as two engine_render(256) calls
  inside render_block(), timed by fm1_ticks() from before
  uac_render_start until after the second render, uac_tap,
  engine_volume and uac_out_mix.
  budget = HALF_FRAMES * 1000000u / 44100 = 11609 us (integer).
  Real half period at ~44145 Hz ≈ 512/44145 s ≈ 11598 us
  (usb.c uac_out_half, "the I2S runs ~44,145 Hz").
  5.805 ms per 256-frame block is NOT a deadline and is not used.
  late and audio_xruns increment together when
  fm1_audio_free_half() changes during the render (DMA moved on).
  Pass means 0 late and 0 xrun.
  Margin is reported against both 11609 us and 11598 us.

PERF meter (main_fm1.c 85-89, ui.c perf_window / draw_perf):

  audio_cpu_pct: IIR average over 16 halves of us/budget (q8).
  audio_peak_pct: max per half; the PERF screen reads and RESETS it.
  engine_load(pct, 512) gets the per-half pct.
  perf.stage[k]: each ENG_PROF_N part's share, x10 percent, from
  eng_prof deltas over a ~1 s window (perf_window). Labels:
  909, 808, BASS A, BASS B, BRK, FX, MASTER, SEQ (part_s / SN_TAIL).
  BASS A/B are part names, not "Chicago/Miami/Juno".
  LOAD / PEAK / DROPOUTS on the PERF page use perf.load (plat_cpu_pct
  = audio_cpu_pct), perf.peak, plat_xruns().
  For the owner's 808 spikes of 0.6% to ~4%, log RAW per-half us
  (not the IIR average).

Missing instrumentation on this exact 94197fc8 image:

  * X0X_LCD_TRACE=0, lcd_trace symbols ABSENT. No 64-sample
    P50/P95/worst AUDIO histogram.
  * X0X_DEBUG=0, x0x_worst ABSENT. No over90/over100/half_no/late_in.
  * x0x_dbg.last_us is only the last half; max_us is the max since
    boot, not a series and not per-state.
  * First-late timestamp: not stored. If late==0, first_late is none.
    If late>0, first_late is UNMEASURED.
  * P99 of block time: UNMEASURED without a series.
  * Raw per-half us series: UNMEASURED from firmware in this image.
    FMB may send probe points; those cannot be compiled into this
    exact ELF without changing the artifact. If a probe requires a
    different build, it will be labelled and will not replace 94197fc8.
    An emulator-side sampler (read last_us each half, no firmware
    edit) is possible later; it is not in place yet. The empty-
    sequence segment will still be run; we will record last_us,
    max_us, late, xruns, eng_prof and the UI meter at the dump,
    and any series we can obtain without a new image.
  * The `perf` UI struct was split by the compiler into many
    `perf.N` objects, so a single-struct RAM read of stage[] is
    not straightforward. UI values will also be taken from the
    LCD (PERF page) at each state.
  * Voice counts, per-engine cycle cost on core 1, BLE, and a
    mid-run RAM dump API are UNMEASURED / absent.
  * If the emu shows 0 late / 0 xrun, that is "the emulator did
    not reproduce a dropout", not "universally dropout-free".
    The gaps above could explain a miss.

-----------------------------------------------------------------------------
(c) How to drive the scene in the emulator
-----------------------------------------------------------------------------

Default after boot (engine_sound_defaults): 303A = Miami preset 0,
303B = Chicago preset 0, BREAK bank empty, 909/808 factory mix,
tempo = project default (not 140), Chicago arp off, no Juno,
Bluetooth off. PERF TEST is not this scene and will not be used
for the first pass.

Intended first-pass drive (Elite, 140 BPM), all inputs that are
not in the owner report labelled chosen / UNKNOWN:

  1. Boot the exact Elite fwsc + ELF above, FM1_STRICT=1, --mhz 360,
     --usb (MIDI is USB on SMK-37).
  2. 303A: SEL SOUND page, ENGINE = CHICAGO, PRESET = 0 (factory
     patch 0; loudest/most-resonant is not in this first pass).
     ARPEGGIATOR SWITCH = ON. Speed = default for that preset
     (densest 1/32 is later, "wherever practical"). Chicago arp
     only generates from live held notes, not from the sequencer
     — hold a note (or a small chord) on 303A via USB MIDI ch 2
     (default 303A channel) for the playing segment.
  3. 303B: ENGINE = JUNO, PRESET = 0. Hold a simple triad
     (C-E-G, MIDI ch 3) for the playing segment. 6-voice PLUCK
     is not in this first pass.
  4. 909: program or play BD on every beat (0x1111 on 16 steps,
     or USB MIDI ch 10 note 36 at 140 BPM). Other 909 voices off.
     808: not used; leave empty / silent (needed for (a)).
  5. Tempo 140 BPM (chosen). Via the tempo control or a USB MIDI
     clock at 140. If clock is used, only one clock source.
  6. FX default = engine_sound_defaults (factory mix). FX all-on
     = a chosen meaning, documented in the run log: all part
     REV and DLY sends to max, reverb and delay audible. Owner's
     actual FX is UNKNOWN.
  7. PLAY for at least 5 emulated minutes (playing state), then
     STOP and sit (idle / stopped), then clear the sequences,
     PLAY the empty sequence (silent 808 + empty 909, no holds).
     That empty-sequence segment is also (a): 808 silent, empty
     sequence, no incoming events, raw per-half us if available.
  8. Open the PERF page (GLOBE > PERFORMANCE, same idea as the
     gate's menu but stopping on the live PERF view, not RUN
     PERF TEST) so the LCD shows per-part stage %, LOAD, PEAK,
     DROPOUTS. Read those from the LCD and from RAM
     (audio_cpu_pct, audio_peak_pct, audio_xruns, eng_prof,
     x0x_dbg). Flag UI vs real-cost mismatches. Note that
     visiting PERF RESETS audio_peak_pct.

Key tokens on SMK-37 (fm1-soc smk.rs): GLOBE, SCALE (=SEL),
FX, PARA (=EDIT), PATCH (=HOME), ARP, SEQ, SEQ PLAY, SEQ REC,
OCTAVE-, OCTAVE+, <, >, KNOB BANK, FADER BANK, BT, CHORD,
NOTE REPEAT. Encoders: e0=K1 MASTER, e1=K2 SELECT, e2=K3
ALGORITHM (part), e3=K4 PRESETS, e4..e7=KNOB 1..4.

The exact key timings are UNVERIFIED until a short LCD
validation shot (not a 5-minute run, not a duplicate of the
first pass). If validation fails, the first pass will not be
started on a wrong scene.

BREAK (NOT_RUN, not "absent"):
  X0X_NBREAKS=0. Factory pack
  cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e
  is 131152 B; Elite free app tail is 8440 B — it cannot be
  installed into this exact package (package_stock_loops.py
  would refuse, and a composed package would be a different
  hash, which we must not substitute). User slots were not
  in the FMB artifacts. I will not change the 94197fc8
  package to fit a pack. Leave-one-out "no BREAK" is therefore
  also not a contrast against a playing break; it will be
  labelled NOT_RUN as a comparison. Full-scene runs still
  happen, without BREAK audio, and are labelled BREAK=NOT_RUN.
  If a slot can later be loaded into emu flash only (no
  package change), the pack/slot sha will be recorded and
  BREAK-active segments can be added — that is not ready now.

-----------------------------------------------------------------------------
Deadlines used from here (FMB correction; live gate not changed)
-----------------------------------------------------------------------------

  Report against:
    budget_us = 11609          (integer, main_fm1.c l.40)
    i2s_half_us ≈ 11598        (512 frames at ~44145 Hz)
  Do not report 5.805 ms as a deadline.
  Miss = late and audio_xruns (same increment). Pass = 0 and 0.
  Worst and last_us margins: budget-us and i2s_half-us (positive
  = under the line). Raw series preferred over audio_cpu_pct.
  Re-parse of existing 94197fc8 gate logs: Elite worst 11561 us
  is 48 us under 11609 and 37 us under 11598; Pro 11525 us is
  84 us and 73 us under; late 0, xruns 0. Those logs have no
  per-half series to re-parse. No gate rerun.

-----------------------------------------------------------------------------
First-pass plan (Elite only, 140 BPM) — core first, then extras
-----------------------------------------------------------------------------

Core (report these before extras if extras would more than
double the time). Each config: playing ≥ 5 emulated min, then
idle, then empty sequence. One emu process at a time. Check
uptime and that no /workspace/web-emu node/chrome bench is
running. Kill only our PIDs.

  1. full scene, FX default, patch 0, default arp rate, BREAK=NOT_RUN
  2. full scene, FX all-on (chosen meaning, logged)
  3. leave-one-out no arp     (Chicago held without arp, or arp off)
  4. leave-one-out no Juno
  5. leave-one-out no BREAK   (NOT_RUN as a contrast; same as 1
     for BREAK audio; will be logged as such, not re-run as a
     duplicate if it is identical — see below)
  6. leave-one-out no 909

"No BREAK" as a separate 5-minute run would duplicate (1) while
BREAK is NOT_RUN. It will be recorded as NOT_RUN / identical to
(1), not executed as a second 5-minute shot.

Extras, only after the core result is reported, and only if
they stay modest:

  (a) already inside each config's empty-sequence segment
      (808 silent, empty sequence, no events).
  (b) Miami ON vs Miami OFF, same scene otherwise: total raw
      per-half us, late/xruns, per-part values. Interpretation:
      the core scene has Miami OFF as a selected engine
      (Chicago+Juno occupy both bass parts). Miami ON is a
      variant (303A=Miami instead of Chicago, or both A and B
      Miami — exact layout to be logged). This is not claimed
      to be the owner's scene.
  (c) Juno only, simple triad (owner's meter ~30%): per-half us.
      Chorus/FX off only if it does not change the sound path
      under test. Juno chorus is inside the Juno DSP
      (JUNO6_CHORUS). Turning it off is a different DSP path,
      so chorus-off is NOT_RUN unless FMB confirms that path
      is acceptable; the reason will be in the log.
  (d) UI params-page probes: NOT NOW. Waiting for FMB points.

Patches other than factory 0, densest arp, 6-voice PLUCK,
Pro, 120 and 174 BPM: later, after the first-pass report.

Every log carries: source 94197fc8, Elite ELF
720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea,
package 95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6,
emu 31157d25c, FM1_STRICT=1, 360 MHz, config name, tempo 140
(chosen), patches/FX UNKNOWN-or-chosen, BREAK=NOT_RUN,
owner installed hash UNPROVEN.

-----------------------------------------------------------------------------
Reparse of existing 94197fc8 gate dumps (no rerun)
-----------------------------------------------------------------------------

Parser: /workspace/btr-dropout-repro/scripts/parse_scene.py
Outputs: /workspace/btr-dropout-repro/logs/reparse-gate-20261113-202119/{elite,pro}/
RAM copied from gate-20261113-202119 (original dirs not modified).

Elite (ELF 720f851c…, package 95b3195f…):
  halves 2173 (518 boot+menu, 1655 during test+restore)
  last_us 1608, max_us 11561
  late 0, audio_xruns 0  → 0 late and 0 xrun
  margin max_us: 48 us under 11609, 37 us under 11598
  audio_cpu_pct (IIR) 12, audio_peak_pct 13, cpu_q8 12.1% (these are
  end-of-shot, after restore — not the heavy-test average)
  perf_cyc_ok 0 (no CPU cycle counter on the emu)
  eng_prof_delta (plat_cycles, core 0 only), frames 1112576:
    PR_909 33772959, PR_808 37112119, PR_303A 0, PR_303B 37801903,
    PR_BRK 3318272, PR_FX 4199259, PR_MST 24447743, PR_SEQ 8633587
  P99 and raw series: UNMEASURED. first_late: none.

Pro (ELF 7e67c3b3…, package ce67d560…):
  last_us 1614, max_us 11525, late 0, xruns 0
  margin max_us: 84 us under 11609, 73 us under 11598
  PR_909 30821349, PR_808 43543654, PR_303A 0, PR_303B 38270207,
  PR_BRK 3316232, PR_FX 4240343, PR_MST 24463944, PR_SEQ 8713991

Observation, not a root cause: PR_303A is 0 on both models, even though
PERF TEST plays a TB-3PO line on 303 A in all three scenarios and 303 A
is Miami. PR_303B (Chicago, core 0 by policy) is large. PR_BRK is
non-zero even with X0X_NBREAKS=0 and no pack (some BREAK job cost, not
proof that a loop played). This is consistent with PROF_ADD skipping
core 1, and with the owner's "Miami PERF shows 0 while playing", but
it is not a diagnosis. Flag for FMB.

