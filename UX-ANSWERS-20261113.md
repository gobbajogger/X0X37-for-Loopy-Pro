# UX answers (DnBOX SMK Firmware UX, 2026-11-13 06:08 CST), relayed agent text
- Change map (1)-(6) OK; lock_marks skip chord lanes agreed. Diagnosis sha bf01f5e2 differs from eef82297 only in section 6.
- NOTE knob: NOT 'C3+2' (reads augmented/transpose), no '/'. Use root + TOTAL count: 'C3x3', existing glyphs only. If FONT_M is too wide for 54 px: root FONT_M + 'x3' FONT_S/XS; else drop the octave digit last. Mono stays exactly 'C3'.
- Held/WRITE line: full list, low to high. Font chosen by note count only (stable). If XS does not fit: root + 'x6'. Never cut a name.
- Acceptance frames: (a) 2/3/4/6-note chords differ, every pitch marked; (b) chord vs mono distinguishable at 1x; (c) ties at each pitch, across TIE steps; (d) no false p-lock notch; (e) Chicago/Miami mono byte-identical to before; (f) no draw CPU regression, flash within headroom (shared with presets!).

# UX FINAL chord label spec (06:17 CST, supersedes the note above; relayed agent text)
INPUT: the sounding onset set + effective transpose; Drone uses only the voices its allocation actually sounds (cite the rule). 12-bit pc mask, octave doubles fold. Bass = lowest sounding note. Spelling as put_note does it.
QUALITIES Tier1 (required): maj{0,4,7} '' ; min{0,3,7} 'min' ; 5{0,7} '5' ; 7{0,4,7,10} '7' ; maj7{0,4,7,11} 'maj7' ; min7{0,3,7,10} 'min7'.
Tier2 (in order, only if bytes allow): dim{0,3,6} ; sus4{0,5,7} ; sus2{0,2,7} ; aug{0,4,8} ; 6{0,4,7,9} ; min7b5{0,3,6,10} ; dim7{0,3,6,9}. Table: 2 B mask + suffix, shared suffix strings.
MATCH: try roots bass first, then upward pcs; rotate mask, exact equality, first match wins. Bass-root reading always wins (CEGA = C6). aug/dim7 take the bass as root, no slash. No bass match and >1 other-root matches = uncertain, show notes. No match = notes. Never '?'. Single pc = not a chord.
SLASH: 'Root+suffix/Bass' when root != bass (C/E, Cmin7/A#). No octave in the symbol.
NOTE cell 54px: symbol FONT_M, then S, then XS; if still too wide drop '/Bass'; never cut. Unrecognized or octave-only: bass name+octave+'x'+total count (C3x3). Mono 'C3' unchanged.
HELD line: 'STEP n  <symbol>  <notes low->high>  V###'; font by note count B, S, XS; if still too wide replace the note list with 'C3x4' and keep the symbol. Drone stored-but-not-sounding notes in the existing dim colour.
CPU: name the chord once per held-step/onset change, cached; never per column/band/audio/playback.
BUDGET: measure Tier1 first (UX est 90-140 B); Tier2 only within the headroom shared with presets; if Tier1 does not fit, report to UX+ORC before any cut.
ACCEPTANCE: CEG->C ; EGC->C/E ; C Eb G Bb->Cmin7 ; C3 G3 C4 E4->C ; CG->C5 ; CEGA->C6 (Tier2) else Amin7/C ; C C# D->notes/C3x3 ; C4 C5->C4x2 ; C3->C3. 0 wrong symbols.
