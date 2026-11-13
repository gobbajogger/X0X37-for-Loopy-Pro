# Proposed pipeline regression: owner Chicago+arp / Juno / BREAK / 909 scene

This is a PROPOSED new script/config, to live under
/workspace/btr-dropout-repro/regression/. It must NOT be merged into
/workspace/zdprobe/dnbox360/gate.sh or run_perf.sh.

## Uncertain inputs (never treat as known)

- Owner tempo: UNKNOWN (this proposal uses 140 BPM as a chosen probe,
  also 120 and 174 later).
- Owner patches: UNKNOWN (factory patch 0 as a chosen probe; also the
  loudest/most-resonant Chicago factory patch, later).
- Owner FX: UNKNOWN (factory defaults, and a chosen "all-on").
- Owner installed package hash: UNPROVEN. Artifact used must be recorded
  every time (Elite 95b3195f… / ELF 720f851c…, or a later exact match).
- BREAK loop: this 94197fc8 image has X0X_NBREAKS=0 and cannot take the
  131152 B factory pack (cbfe682a…) in an 8440 B tail. A regression that
  claims BREAK is playing must record the pack or slot sha and how it
  was loaded into emu flash (not a substituted app package). Until then
  BREAK=NOT_RUN.

## What it must do

- Exact model, ELF sha, fwsc sha, emu version (fm1-live 31157d25c or
  later recorded), FM1_STRICT=1, 360 MHz, in every log.
- A sustained run (at least 5 emulated minutes playing), then idle
  (stopped), then empty sequence (808 silent, no events) — this last
  state is also the 808-spike segment.
- Drive: 303A Chicago + arp on + held notes (USB MIDI, SMK-37 has no
  TRS IN); 303B Juno held triad; 909 BD four-on-the-floor; 808 unused;
  BREAK only if a loop is really loaded.
- Counters: x0x_dbg.late, audio_xruns (pass = both 0); last_us and
  max_us vs 11609 us and ~11598 us (not 5.805 ms); raw per-half us
  series if a probe exists; P99 from that series otherwise UNMEASURED;
  first late timestamp if stored, else UNMEASURED; per-part PERF UI
  (LCD) vs eng_prof (note: PROF_ADD is core 0 only); audio_cpu_pct is
  an IIR average and is not the spike figure.
- Leave-one-outs and FX all-on as configs. Miami ON/OFF and Juno-only
  as extras, not as a replacement for the core scene.
- No claim that a 0-late emu result is universally dropout-free.
  List fidelity gaps (BLE, cycle counter, core-1 PROF, I2S rate,
  USB-in-render, BREAK pack, no raw series on this image).

## Runner

A concrete runner will be filled in after the Elite first-pass key
script is LCD-validated: scripts/run_elite_first_pass.sh and
scripts/parse_scene.py are the starting points. The live gate stays
the RUN PERF TEST path.
