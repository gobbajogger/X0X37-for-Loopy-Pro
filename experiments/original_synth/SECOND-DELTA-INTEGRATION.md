# Second-order exact table storage: inactive continuation

This changes the private packed MP4 provider only. Existing first-order decoder and raw MP4 interface remain available. No production caller, preset, state persistence, audio waveform, coefficient or stock asset changes.

## Format and correctness

Separate `original_lossless_second_lookup` uses independent 32-entry blocks: LE32 first bits, LE32 first modulo-32 difference, byte residual width, then LSB-first zigzag second differences. A singleton still has a nine-byte header. Unsigned arithmetic reconstructs all bits, including wrap and widths 0..32. Invalid descriptors/truncation/widths leave output untouched. No format guessing or reinterpretation of first-order descriptors.

Generator `tools/gen_chicago_second_delta_private.py` verifies the immutable original-derived 44.1-kHz manifest and both table hashes. The old generator remains historical first-order tooling; it must not overwrite the new-format header. The private payload stays private.

All 32,770 coefficients and all 16,385 joined MP4 preparation/sample trajectories are bit-identical. Arbitrary-bit decoder tests cover 1,115,136 lookups including widths0..32, partial blocks, wrap and malformed boundaries. Four affected arithmetic/sanitizer modes retain unchanged Drive and pitch proof. LeakSanitizer cannot run under this environment's ptrace; ASan and UBSan run with detect_leaks=0. The decoder allocates nothing.

## Actual full-app link accounting, not activation

Exact preserved B Pro and Elite objects were relinked; all four baseline loadable sections match original B bytes. A retained but uncalled MP4 preparation caller roots either raw or packed provider, including the actual pinned r3 libc classification dependency. This is a link-capacity probe, not an activated synth or complete final firmware.

Raw probe complete app: 515,208 B. Second-order packed: Pro416,324 / Elite416,340 B. Actual saving:98,884 /98,868 B against raw. The earlier first-order packed probe was443,604 B: additional27,280 /27,264 B saved after real link/alignment.

Stock bank131,152 B is unchanged and aligned16. Against552,672 B code boundary, second-order probe leaves Pro5,184 / Elite5,168 B; first-order overflowed22,096 B. This does NOT fit the full remaining original engine plus boot. The existing boot app adds about33,008 B over B before any new combined link; additive estimate already exceeds remaining room, and is not a verified combined artifact. No samples/features/quality are removed to fit.

## CPU and bounded work

Each lookup reads one block header and performs at most30 residual reconstructions (entry31), with at most five byte-fragments per residual in the generic width0..32 domain. Authentic tables' max width23 needs at most four. Each MP4 prepare needs A and E: at most60 residual reconstructions/240 byte-fragments for this payload; no decoder in the audio sample function. State is constant-size; no heap, decompression cache or full-table RAM.

At690 preparations/s per voice this is1,380 lookups/s; two active lanes imply2,760 lookups/s and at most82,800 residual reconstructions/s. These are static upper work counts, NOT target timing. Extra note-initialization preparations and any permitted explicit control updates add bounded calls and must be counted by eventual integration. Dense MIDI/control event handling is not yet integrated; no dropping NoteOff, changing event order or claiming an arbitrary queue bound. Parameter edits must retain the original tick/sample semantics, not force table decoding per sample.

Single host run262,160 lookups: second-order0.015317s versus historical first-order0.026267s; not a controlled device benchmark or guaranteed speedup. Device block deadlines, both-synth/effect worst-case occupancy, XRUN and physical key latency remain unmeasured. Memory fit alone cannot activate anything. Native target code is retained and inspectable in the evidence packet; no device CPU benefit is inferred.

## Whole-note gates kept separate

Local original manifests expose postfilter stereo coefficients but not their complete gain/pan preparation, two-lane unison coordinate mapping or every paired oscillator gain assignment. Chicago saved native-state artifact is referenced by hash but not locally present as a complete native109 vector. No original executable/disassembly bytes are present in the bounded original-reference directories. A parameterized joined component reproduction is possible; calling it complete named factory output would invent these fields. Original source tracing cannot be resumed through prohibited cloud Work/Codex.
