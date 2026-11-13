# DnBOX dropout observer and sample fixture

This diagnostic lane is separate from FMB firmware changes and Chicago UI PR #888.
It targets the reported Chicago arp + Juno triad + BREAK + 909 BD scene, with no 808.
It is not an exact owner reproduction: installed package/model, tempo, patches,
FX, BREAK sample, arp settings, held notes and velocities remain unconfirmed.

## First checkpoint: sample fixture

`fixture.py` creates an emulator flash fixture with one complete loop copied
unchanged from a hashed X0XBRKPK pack into a user sample slot. It validates the
pack CRC and length, checks slot capacity, preserves every byte outside the
selected slot, and records pack/ADPCM/slot/full-flash SHA256 values. It never
decodes or re-encodes the sample. It has no device transport.
It rejects an FSMP header in either other slot, so another retained sample
cannot silently invalidate the planned single-source runtime proof.

The result is always `PREPARED_NOT_RUN`. A valid slot file does not prove that
the firmware selected, rendered or mixed it. Runtime observation is a separate
step. The old `BREAK=NOT_RUN` timing gate cannot qualify this scene.

Run the focused synthetic format tests from the emulator repository root:

```sh
python3 -m unittest discover -s dnbox/dropout -p 'test_*.py'
```

For Runner, after creating an immutable 1 MiB flash from the pinned firmware with
the existing `fm1-live --fwsc ... --flash ...` interface:

```sh
python3 dnbox/dropout/fixture.py \
  --flash /workspace/btr-dropout-repro/observer-inputs/base.flash \
  --pack /workspace/build-1214/base-94197fc8/assets/factory-breaks/factory-break-pack.bin \
  --pack-sha256 cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e \
  --loop CRAMEN --slot 1 \
  --out /workspace/btr-dropout-repro/observer-inputs/cramen-slot1
```

Those input paths describe the existing Runner workspace. The owner's private
Library handoff has now materialized in the Classic workspace; both model ELF
and package hashes and the factory pack hash match the recorded pins. The
output directory must not exist.
CRAMEN is a chosen representative sample, not the owner's confirmed sample.
The fixture records the pack's bar count separately from the firmware's
user-slot bar inference (`engine.c:2960`), because the slot format omits bars.

## Exact source and artifact pins

| Input | Identity |
|---|---|
| Firmware source | `94197fc839b00aff47b98ca02f3c6396e4a6eabf` |
| Firmware tree | `72d28fbc741244d18fa3d18495da16ab0fcb1d24` |
| Runner source | `31157d25caf7284080cdb5d86ab7e8e438120fd9` |
| Runner source tree | `3e1864b74d91fd0c3864a5bddd9f94fbf89d963e` |
| Emulator lane base | `6b60618c8bf344d65a166905fb746f46ab708fcb` |
| Existing Runner binary SHA256 | `f3e65ccfa39874168b99494055f9a755342c41bc769257883030d15aef7b54f5` |
| Elite ELF SHA256 | `720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea` |
| Elite package SHA256 | `95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6` |
| Pro ELF SHA256 | `7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535` |
| Pro package SHA256 | `ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8` |

The Rust/Cargo sources at the emulator lane base match the exact Runner commit.
Later lane additions are drivers and documentation. An observer-enabled emulator
will have a new binary hash; record it separately rather than claiming it is the
existing Runner binary. Reuse the firmware ELF/package bytes; no firmware rebuild
is required for this instrumentation.

## Source evidence and runtime requirements

The fixture formats come from the pinned firmware's `tools/break_pack.py:63`,
`firmware/src/app/slots.c:23`, `:50`, `:52`, `:84`, and `:104`. Slots 1/2/3 are
respectively `[0xC2000,0xD6000)`, `[0xD6000,0xF4000)`, `[0xB3000,0xBB000)`.

The existing emulator restores all of `--flash` after package loading and writes
it back on exit (`crates/fm1-live/src/main.rs:274`, `:588`; `Soc::restore_flash`).
Use a disposable run copy. A matching package hash is insufficient if the restored
flash replaces app bytes; the observer integration must compare protected flash
against `stock.flash` before restoration.

The next runtime checkpoint must observe actual DMA edges and ALNK IRQ returns,
not poll changes in `last_us`. Identical durations still represent separate
halves. `x0x_dbg.halves` is incremented before `last_us`, and `in_audio=0` precedes
late/xrun updates (`main_fm1.c:90`, `:91`, `:94`, `:109`). Finalize at IRQ return.

The exact emulator derives I2S from 24 MHz / 544 (~44117.647Hz), so 512 frames are
~11,605.333 us. Store its actual scheduled deadline separately from nominal 11,609 us
and the owner's reported ~44,145 Hz (~11,598 us). Keep dual-core slices unchanged.
`FM1_NO_BLOCKS` is a CPU0-only route and cannot qualify this scene.

`eng_prof` remains the firmware's partial attribution: it omits several core 1
jobs and charges shared waiting to 808. Keep raw per-half deltas, smoothed
`eng_job_cost`, `eng_plan`, and actual masks distinct. Do not label their sum as
complete DSP cost or fix the firmware just to improve observability.

BREAK qualification needs both runtime slot acceptance and current-stretch audio
evidence. Nonzero master PCM alone can come from the other engines. Nonzero kbuf
alone can be muted by mixing. Core 0 dry-buffer deltas after `eng_stage=4` provide
the pinned source's BREAK mix contribution (`engine.c:2528`, `:2751`). This is
numerical contribution, not independent proof of human audibility after arbitrary
master processing. A missing or silent BREAK must remain unqualified.

## Validation status

First checkpoint: eight focused Python tests passed after an observed missing-
implementation failure. Their pack and flash bytes are synthetic; no timing,
owner-scene, hardware, bit-exact PCM or never-slower firmware result is claimed.
No firmware files, packages, proprietary binaries or devices are modified here.
Independent source review found that retained samples in other slots could make
the selected sample ambiguous. A failing regression reproduced it; the builder
now rejects that input. Binary32 versus Python bar inference was also checked
over the supported slot-size domain without finding a differing selection.

## Host observer checkpoint

`crates/fm1-soc/src/dnbox_observer.rs` observes existing RAM stores, scheduled
DMA edges, ALNK entry and the matching outer IRQ return. It does not execute a
guest call, write guest memory, add guest cycles or change dual-core slices.
`FM1_DNBOX_MAP` supplies symbol ranges and `FM1_DNBOX_TRACE` names a new JSONL
file. Both variables must be supplied together. The supported route is the
strict 360 MHz dual-core `--shot` route with the pinned package; incompatible
timing, fault-injection and profiling overrides are rejected before loading.
`FM1_DNBOX_PCM` writes the existing DAC PCM stream, including with observation
disabled for a control run.

The map requires all eighteen pinned counters/buffers. Optional `scene_*` raw
byte ranges support target-layout-verified scene state without guest changes.
They are limited to 64 ranges, 512 bytes each and 4096 bytes total; all ranges
must be disjoint and in RAM. Partial and unaligned overlaps with guest writes
produce clipped byte events, so snapshot replay does not silently omit them.

`trace.py` reduces a complete trace into `summary.json`, `per_half.csv` and
`stage_events.jsonl`. It checks every selected half against actual DMA time,
replays watched stores from IRQ entry to return, requires both 256-frame render
increments and exactly one `last_us` write, and preserves repeated equal values.
Missed DMA edges are counted by sequence, not alternating half identity.
Completion has a documented one-emulator-tick bound; core 1 event timestamps
use the shared slice clock. Output states are `OBSERVED` or `INCOMPLETE`, never
an owner-scene PASS. Scene bytes and human audibility remain unverified until
separate checks interpret them.

Focused validation at this checkpoint:

- 19 Rust observer tests passed in the actual `fm1-soc` crate, including the
  strict dual-CPU DMA/IRQ observer-on versus observer-off CPU/RAM/PCM/clock test.
- 19 Python reducer tests passed, including missing-store replay, truncated
  observations, actual DMA deadlines, counter wrap and optional scene bytes.
- The real built executable rejects `FM1_USB_SIE_NS` and `FM1_FLASH_CUT` before
  attempting to load firmware. Both cases failed before that guard fix.
- `cargo build --locked --release -p fm1-live` passed. Only the host emulator was
  built; the firmware artifacts were reused unchanged.

These synthetic neutrality tests establish the tested execution paths, not
target-wide equivalence. A real firmware observer-off/on control remains a
separate runtime check.

### First exact-input short run: scene rejected

The first 30-second Elite run completed with the exact input package and a
CRAMEN user slot. Its 20–29 second window contained 775 coherent halves, but
it **does not qualify the requested scene**: final RAM showed A=Miami,
B=Chicago, an unintended 808 BD pattern, no Juno and zero BREAK steps.
BREAK was loaded but had no observed dry-mix contribution. Therefore its
zero missed deadlines and 8467 us maximum `last_us` are diagnostic results
for that rejected setup only. They are not evidence that the reported owner
scene meets its deadline. The next fixture revision must verify actual scene
state at captured boundaries before any scene timing qualification.

The sanitized receipt in `evidence/2026-11-13-initial-rejected.json` records
the exact artifact, input-script, trace and final-RAM hashes. Raw firmware,
sample data, RAM and PCM remain private and are not committed.
