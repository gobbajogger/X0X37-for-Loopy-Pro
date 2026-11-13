# SEQ piano roll + chord notation — reply 1
Source: https://chatgpt.com/g/g-p-ad9d6d4ab04c8191af6aa22fd752707e-loopyx0x37/c/6af70865-6c84-83e9-8f20-b831873de391 (Extra High; worked 7m 58s). Tables reformatted to Markdown; text otherwise verbatim.

DnBOX SEQ chord notation: review findings

Source verified: The attached piano-roll.md matches SHA-256 a6ab7d1a15d091645f18bb45bed0057c3a2170e89d2107209dc88583f29e6466.

The core musical rules are sound: exact pitch-class matching, C for major, Cmin7, bass-aware slash notation, and conservative fallback for genuinely ambiguous chords. The C6 versus Amin7 distinction is musically valid.

However, several implementation and edge-case findings need attention before accepting the notation as correct.

## Findings

**F1. HIGH: Drone's sounding-note requirement contradicts its input contract.**

§9.1 defines the namer's input as held_chord_get plus transpose. That provides stored notes, not necessarily the four notes currently sounding from Drone's four voices.

For six stored notes C3 E3 G3 B3 D4 F4:
- Stored set: six notes, not an exact chord template.
- Hypothetical active set G3 B3 D4 F4: G7.
- Another possible active subset can produce another symbol.

The spec correctly identifies voice stealing as state-dependent, but a static stored-note reader cannot determine which notes have been stolen.

Finding: The G7 example and dimmed silent-note marks cannot be guaranteed by a draw-only chord-label function without a verified read-only source of actual voice state. A cache keyed only by step and transpose is insufficient.

Do not assume the highest four notes always survive. N-15 must remain explicitly inferred until separately verified.

**F2. MEDIUM: Ambiguity detection must take precedence over "first match wins."**

§9.2 says to return the first exact match, but also says to return notes when two non-bass roots match.

The implementation must therefore distinguish:
- Bass-root match: return immediately.
- Exactly one non-bass match: return slash notation.
- Multiple non-bass matches: return notes, not the first candidate.

For example, E3 G3 A3 C4 has two valid readings with Tier 2: C6/E and Amin7/E. Returning either would violate the final ambiguity rule.

**F3. MEDIUM: Tier 1 and Tier 2 can legitimately produce different labels.**

C3 E3 G3 A3 correctly becomes C6 with Tier 2 and Amin7/C with Tier 1 only.

The reverse case matters too: E3 G3 A3 C4 should become notes with Tier 2, but Amin7/E with Tier 1 only.

This is not a musical error. It means Tier 2 availability affects the semantics, not merely the number of names recognized. Tests must explicitly cover both configurations.

**F4. LOW: Augmented and diminished-seventh inversions are correct, but the root name changes.**

These chords are symmetric, so bass-first naming is a reasonable convention.
- C3 E3 G#3 → Caug; E3 G#3 C4 → Eaug.
- C3 D#3 F#3 A3 → Cdim7; D#3 F#3 A3 C4 → D#dim7.

Both are musically acceptable. The spec's phrase "stable under inversion" should mean deterministic naming, not that the chord symbol remains identical.

**F5. MEDIUM: Cache invalidation is underspecified.**

§9.5 lists held-step/onset changes, transpose changes, and chord writes. Tests should additionally cover pattern switching, part switching, selecting a TIE step, editing the onset of a selected TIE, and switching between Juno and Drone.

If the Drone label represents actual voices, changes to that voice set also require invalidation. Otherwise the display can continue showing an outdated chord after the sounding pitches have changed.

**F6. MEDIUM: Transpose clamping can collapse distinct pitches.**

For example, stored MIDI 103,107,110 with bp->transpose=48 produces 127,127,127 after clamping.

The pitch-class mask has only G. It must not produce a major-chord symbol merely because the original pitches had major-triad intervals.

The distinction between three stored note events and the number of actual sounding voices needs explicit treatment for both xN notation and Drone.

**F7. HIGH: Flash acceptance is still blocked by missing linked-size evidence.**

The attached spec reports Tier 1 at 285 B HOST MEASURED, Tier 2 at an additional 46 B, against 248 B of shared Elite flash headroom. The figures alone do not establish whether the final firmware fits, especially with the display fix and preset work sharing the budget. Only an exact-source linked build can do that. [piano-roll(1).md]

**F8. LOW: Long slash labels sacrifice musical information in the NOTE cell.**

Dropping /Bass to fit 54 px is a documented, reasonable compromise, provided the held line preserves the complete symbol. The 51 px fallback labels have only 3 px spare horizontally, so real framebuffer clipping checks remain necessary. [piano-roll(1).md]

**F9. LOW: The packed-suffix example is unsafe as literal C syntax.**

The example "\0min\05\07\0maj7\0min7" contains \05 and \07, which C interprets as octal character escapes, not NUL separators followed by the characters 5 and 7. If copied literally into firmware, this could corrupt the 5 and 7 suffixes. The actual host implementation is not included in the attachments, so this is a documentation-level risk rather than a verified firmware defect.

## Proposed test-set additions

The following rows extend §9.6's published table (19 visible rows, despite its statement that the separate host harness contains 23).

Inputs are sounding MIDI after transpose unless stated otherwise.

| New ID | Input notes / condition | Tier | Expected result |
|---|---|---|---|
| T20 | C3 D3 G3 | 2 | Csus2 |
| T21 | G3 C4 D4 | 2 | Gsus4 |
| T22 | G3 C4 F4 | 2 | Notes, NOTE G3x3; two non-bass sus readings |
| T23 | A2 C3 E3 G3 | 2 | Amin7 |
| T24 | E3 G3 A3 C4 | 2 | Notes, NOTE E3x4; C6/Am7 ambiguity |
| T25 | E3 G3 A3 C4 | 1 only | Amin7/E |
| T26 | G3 A3 C4 E4 | 2 | Notes, NOTE G3x4 |
| T27 | C3 D#3 F#3 | 2 | Cdim |
| T28 | D#3 F#3 C4 | 2 | Cdim/D# |
| T29 | C3 D#3 F#3 A3 | 2 | Cdim7 |
| T30 | D#3 F#3 A3 C4 | 2 | D#dim7, no slash |
| T31 | G#2 C3 E3 | 2 | G#aug, no slash |
| T32 | C3 D#3 F#3 A#3 | 2 | Cmin7b5 |
| T33 | D#3 F#3 A#3 C4 | 2 | Cmin7b5/D#; NOTE drops bass suffix if necessary |
| T34 | G2 C3 | 1 | C5/G |
| T35 | D3 C4 E4 G4 | 2 | Notes, NOTE D3x4; foreign bass |
| T36 | C3 E3 G3 onset, followed by two TIE steps | 1+2 | C remains available on both TIE steps; extras read at onset |
| T37 | Stored C3 E3 G3, transpose 23 | 1+2 | B; displayed pitches B2 D#3 F#3, store unchanged |

These expected labels follow an independent enumeration of the specified masks. Across all 13 templates and 12 possible roots, the overlapping-mask families are exactly sus2/sus4, 6/min7, aug, and dim7. The two former families need ambiguity tests; the two symmetric families need bass-root tests.

## Additional non-musical regression cases

The following should also be acceptance cases, but their exact symbol expectations require an explicit sounding-voice contract:

| Test | Required assertion |
|---|---|
| Drone, four stored/active notes C3 E3 G3 B3 | Cmaj7, assuming all four are confirmed active |
| Drone, six stored notes; confirmed active G3 B3 D4 F4 | G7; C3/E3 dimmed only if confirmed silent |
| Drone, identical six stored notes but different active four | Label follows the verified active set, not a hardcoded highest-four assumption |
| High transpose clamp to MIDI 127 | No false chord match; compact count semantics defined |
| Orphan TIE | No invented chord or pitch |
| Pattern switch, same selected step number | No stale symbol |
| Onset chord edit while viewing a TIE | Updated chord shown without reading extras from the TIE |
| Maximum-width slash label | Complete slash in held line; NOTE cell uses specified fallback without clipping |
| Seventh key refused | Existing stored chord and displayed notation remain unchanged |

Overall disposition: The Tier 1/Tier 2 musical matching rules are suitable for implementation, with the edge-case tests above. The outstanding correctness issue is Drone's distinction between stored and actually sounding pitches. The reported standalone flash size also requires exact-link verification before release acceptance.

Read-only review completed. No files, firmware, builds, devices, or playback behaviour changed.
