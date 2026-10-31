# F5 waveform cache and host-suite registration

This is a bounded source delivery for rows 60 and 63/64/66 test registration.
It does not close all F5 rows or establish visual, build or hardware acceptance.

## Base and plan

Owner-authorized pinned base: PR #789 head
`02c5224d12c7a96c0061fea71f13de04de58741b`, root tree
`7568ad94ed45ce62ab3e0cb98526923a50b29b91`. The owner explicitly authorized
working on this head before #789 merges. One owned branch, one draft PR into
`build/0.0.23`; no merge or auto-merge.

1. Carry the archived waveform regressions into the existing original UI test.
2. Port the waveform cache and painter from `9e237cce304dd021792ea80e7426d5f3bbb2a671`.
3. Register the existing navigation, ADSR graph and grouped-settings scripts.
4. Review the exact diff, deliver the draft and defer execution to the authorized
   box stage. No builds or tests run in Codex, including a red/green claim.

## Provenance and exclusions

- Source branch: `archive/box-restore-features-9e237cce`, verified at
  `9e237cce304dd021792ea80e7426d5f3bbb2a671`.
- `firmware/src/app/ui_original_wave.h` is byte-identical to archive blob
  `568bcf906655b7dc4eae7b22e0ae1b72369cf63f`. It restores the native programs
  into temporary storage, applies relevant lock/motion previews and caches
  29-pixel projections for Chicago's oscillator mix and Miami's carrier,
  modulator and wobble. It does not edit the sound or advance a voice.
- `original_wave_view`, `original_wave_slot` and `draw_original_wave` are ported
  from that commit. The existing widget retains its geometry and readout helper;
  the archived waveform painter replaces its fixed shape cue when projection
  succeeds. The old fallback remains for an unavailable projection.
- Only the waveform portion of the archive's `restored_cases` is carried over,
  with cache invalidation, malformed input and held-step nonmutation coverage.
- The archive's PERF uint16 change and touch-only ADSR schematic are excluded:
  the owner requested the waveform cache, and the base already has the newer
  Chicago inline ADSR renderer, stage highlighting and live cursor. No Miami ADSR.
- `f5e1be1b76c0a94e6774df606dec80c417fddfb2` and
  `b51303f681f33d45f06f1aa213550e5b84979201` have equivalent glyph/ADSR behavior
  in the pinned source; they are not cherry-picked again.
- The SELECT handler from `4f039829352d7dc805ebd2865b073d34f397cb6e` is already
  source-present at base `ui.c:4230`; this patch registers its existing test.
- These source histories diverge from #789. GitHub ancestry comparison is not
  patch equivalence, and no whole-commit merge or completed cherry-pick is claimed.
- PR #764's DSP inserts, PR #738's alternate SMC popup, and `d7f156ec`'s
  selected-note storage are excluded from this waveform/runner patch.

## Tests written and registered

`tests/host/original_release_ui_test.c:restored_wave_cases` covers the four
waveform slots, projection bounds, source-cell preservation, preview changes,
held-step locks and restoration, voice-key invalidation, invalid masks/values/
identity, and literal TRI/DOWN/UP modulation geometry. The existing
`original-release-ui` selector runs it in normal and sanitizer modes and compares
the real framebuffer PNGs. No new standalone harness is introduced.

New runner entries in `tests/run_tests.sh`:

| Selector | Existing script | Class |
| --- | --- | --- |
| `ui-page-wrap` | `tests/host/ui_page_wrap_test.py` | P: isolated extracted handler |
| `chicago-adsr-graph` | `tests/host/chicago_adsr_graph_test.sh` | G: shared generated inputs |
| `ui-settings-sections` | `tests/host/ui_settings_sections_test.sh` | G: shared generated inputs |

The ADSR graph regression previously expected its top at row Y + 14, before the
base reserved eight pixels for K5-K8 readouts. It now checks row Y + 22, matching
the existing production painter. Bounds, monotonicity, minimum stage widths,
sustain height, inactive row and live-playhead assertions remain. No check is
deleted or failure count relaxed.

The original UI test also retained a pre-ADSR cache/view size guard of 76/14
bytes. The base already added four native ADSR floats to the view. The guard is
corrected to 92/32 bytes (14 metadata bytes, two alignment bytes and 16 float
bytes; cache prefix 60 bytes). The old guard exits before the new waveform cases;
the new guard accounts for existing fields rather than removing the size check.

## Memory

The restored single shared cache is placed in `X0X_POOL`, like the existing text
cache. Its layout is expected to occupy 156 bytes with 32-bit alignment. Written
reason for that additional pool use: retain exact factory/native dependencies
and lock/motion preview keys plus three small waveform projections so unchanged
draws reuse the projection. There is no per-part array or framebuffer allocation.

Elite and Pro free static RAM: **NOT_MEASURED**. Required comparison is against
2352 B for 0.0.23 and the actual stacked parent. #789 reports 2432 B static free
and 8536 B pool free; these are upstream receipts, not measurements of this patch.
Record any candidate reduction and its reason before acceptance.

## Validation handoff

No build, host test, target compile, framebuffer capture or hardware action ran
in Codex, per the owner's instruction and AGENTS.md. Source/blob/diff inspection
does not establish a test PASS. The review receipt is separate from execution.

On the exact delivered head in the authorized box environment:

```sh
X0X_TEST_JOBS=1 X0X_TESTS='original-release-ui ui-page-wrap chicago-adsr-graph ui-settings-sections ui-waveform-truth ui-theme' sh tests/run_tests.sh
```

Then run the full host suite and both Elite/Pro SDK V1.2.1 builds with
`X0X_DEV_PADCAL=0`, and report static RAM and pool free for each. Use package
identity/model ID, never the USB name. No FM-1 target or `main_fm1.c` edit.

## Framebuffer and owner gate

BEFORE/AFTER 240x240 PNGs are **NOT_CAPTURED**. Do not substitute the archived
mockups or claim generated images already exist. Capture the same four waveform
states on the pinned parent and candidate with the actual host renderer; the
test names are `restored-chicago-wave`, `restored-miami-wave`,
`restored-miami-xmod-wave` and `restored-miami-wobble-wave`. The candidate test
writes them under `build/ui-theme/original-ui/normal-images/` and
`sanitized-images/`; use the same scenario on the parent through a separate
capture-only fixture. Archive the 240x240 before/after files with source pins.

Keep the PR draft until exact-head tests, Elite/Pro builds, RAM measurements,
real framebuffer evidence and owner visual approval are recorded. Do not merge.

No main, .github, Actions, BTIF, USRTRIM, efuse, key_mac, bootloader, installer,
controller or device changes belong to this delivery.
