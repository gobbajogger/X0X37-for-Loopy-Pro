# Exact decoder and MS20 storage successor

This local successor continues LX-REQ-032 from saved source `348ae21514e4688874d7e32ececd97fece6a018b`. The full saved Git chain and all 202 projected source files were verified before changes. It remains an inactive, private original-engine candidate. No shared firmware binding or original history representation is changed.

## Chicago coefficient preparation

`lossless_table.c` consumes residual bits through a bounded 32-bit reservoir for widths 0..24. It retains the original decoder for widths 25..32, the existing admission checks and unchanged output on failure. At most 31 bits are live in the reservoir. The authentic packed tables and every DSP expression are unchanged.

The immutable old decoder is separately compiled from a hash-pinned fixture. Normal and ASan/UBSan comparisons pass 32,770 authentic coefficient words, 1,115,136 generated words and 337,926 refused inputs. Both directory widths are covered. All 16,385 MP4 preparation indices, rendered samples and full filter states match the authentic raw-table provider with native/software FMA and normal/sanitized builds.

Eleven alternating host measurement pairs give median paired ratios of 1.621x for sequential authentic lookups and 1.336x for shuffled lookups. This work occurs during coefficient preparation on control ticks and note initialization, not the MP4 sample recurrence. It is not a whole-engine or device speed claim. The controlled pi32v2 object costs 104 additional text bytes and 8 additional saved-register bytes in the decoder's leaf prologue; no data, BSS or readonly bytes are added.

## Miami sample transaction

`miami_ms20.c` computes the same values before committing the nine changed 32-bit fields. It removes the complete 72-byte local state temporary while retaining arithmetic order and failure atomicity. Output is published last, preserving aliases to all 16 float subobjects. Layouts, tables, snapshots, ranges and precision are unchanged.

Separately compiled frozen baseline and candidate pass 260,632 differential cases per variant, including 6,952 rejected calls, under native/software FMA and normal/ASan+UBSan. Status, output bits, complete state and host FP flags match. The selected graph retains hash `d41eea1766a4bdbe` across 276,480 stereo frames, all 30 factories and 18 macro endpoints, 12,960 snapshots and 1,080 nested restarts.

The controlled pi32v2 unit has 1,528 text bytes versus 1,684 before: 156 bytes smaller. Readonly remains 2,052 bytes. Sample-local stack allocation falls from 80 to 28 bytes with the same saved-register set. These are object/assembly measurements, not a whole-stack bound. The two existing four-byte constant-conversion memcpy calls remain. Host median time was about 0.6% slower in this campaign; no host speed improvement is established.

## Combined result and qualification

The combined source passes the Chicago selected graph under all four arithmetic/sanitizer variants: 81 factories, all 22 endpoints, 110,484 stereo frames and unchanged hash `b8cff4af65646852`. Dedicated host histories remain a test projection, not a fitting original shared arena.

GCC 13 reports preexisting misleading-indentation, type-limits and array-parameter diagnostics in the unchanged graph closure. Controlled runs retain those warnings while demoting only their Werror treatment; all assertions and other Werror rules remain enabled. The original strict GCC runner failure is preserved in the evidence. LeakSanitizer cannot inspect process tasks in this sandbox; ASan and UBSan pass with only leak detection disabled.

The same pinned JieLi compiler compiled both changed units before/after with identical conservative arithmetic flags. Together the two object text totals fall by 52 bytes; this is not a prediction of final linked flash. There are no new runtime state allocations. Target FP flags, device cycles, audio deadlines and XRUNs are unmeasured.

## Existing receiver continuation

The saved 4e report already records 52 units / 305 exports passing both model links, with no original runtime instances allocated. Its raw native receipt remains with the same receiver. A new direct object comparison establishes that the saved 348 clocked binder emits its added export and adds 142 text bytes; 13 other function sections are identical. The observed binder/helper chain grows from 40 to 88 bytes before the external key validator, whose provider already belongs to the 52-unit closure.

After these two changes, the conservative affected set is `miami_note_context.c`, `miami_control_owner.c`, `engine_events.c`, `miami_ms20.c` and `lossless_table.c`. The same native receiver should use final depfiles to verify the set, retain the new `original_miami_note_context_bind_clocked` root and reuse unchanged exact objects. Expected 306 retained exports and 5 rebuilt / 47 reused objects per model are planning counts, not an executed full-build result.

Dot remains the sole firmware integrator. R1–R3 are unchanged and exhausted; this source is pending applicable changed-source review with no fourth round or reviewer reset. The exact shared FX history backend, full runtime allocation, complete startup/input/persistence adoption, wider original contexts and matched original musical A/B remain open. Optimizing these two functions does not complete or activate the port.
