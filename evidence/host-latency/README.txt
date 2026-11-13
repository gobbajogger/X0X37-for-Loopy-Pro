HOST evidence — not a device test.

Fast note sequences (1 ms and 2 ms apart: 44 and 88 samples at FS=44100)
across SEQ screen enter (V_SEQ), exit (V_HOME), and a held-chord label
redraw (draw_303 / draw_knobs). The event log is sample-accurate: now_ms
is set from the cumulative sample count before each engine_render, and
each block records engine_midi_held, note-on counts, and an FNV audio
hash. The 94197fc8 baseline is the same driver compiled against the
unfixed ui.c.

Result: 68 BLK/UI/BURST lines are BYTE-IDENTICAL to the 94197fc8
baseline. No dropped notes, no late notes (same sample index, same
held transitions, same audio hash). 32 notes, 64 blocks, 4224 samples.

The namer is not on this path. seq_chord_namer_calls does not increase
inside engine_render. Context (cite): seq_chord_symbol / chord_label_cached
are called only from UI_DRAW draw_303 (held/WRITE line, ui.c) and
UI_DRAW draw_knobs (NOTE cell), from ui_frame() at ~60 frames/s in the
main loop (main_fm1.c). They are not called from engine_render (audio
ISR / render_block), midi_in, or the TIMER5 10 kHz key scan. Cached:
named only on an onset or held-step input change, bounded (at most
HELD_CHORD_NOTES pitches, one fmt).

Worst-case namer cost (HOST CPU, 100000 iters, not a device cycle count):
  longest slash (A#min7b5/G#)  21.6 ns/call
  6-note hex                   27.6 ns/call
  F6 clamp 127,127,127         17.4 ns/call

Draw time, fair back-to-back HOST (80 iters, same machine, same load):
  chicago-mono     after 0.154610 ms  baseline 0.157301 ms  delta -0.002691 ms (faster)
  juno-held-triad  after 0.150734 ms  baseline 0.147091 ms  delta +0.003643 ms
The chicago-mono AFTER frame is SHA-256 identical to BEFORE (same pixels).
juno-held-triad paints every onset pitch (three marks vs the baseline's
root only) plus a cache-hit label; the +3.6 µs is that extra draw, well
under a 16 ms UI frame. Not a device measurement.
