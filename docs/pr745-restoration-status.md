# PR #745 restoration on one-build: checked checkpoint

## Owner scope

The requested result is all ten Chicago FX inserts, connected Bluetooth device names, native Chicago/Miami idle waveform cues, Chicago touched-envelope schematic and saturating PERF CPU display. Only PAD DIAGNOSTICS is excluded. The sole authorized PR is #745. No merge, FM-1 build/test, hardware flash, bootloader or protected-region operation.

Base: wip/one-build at 5bff28caba24568cd5070acb99aaf37729cd8323, exact root tree 0a6cf3b5668fb9c7bb68ef645cb4a3fcf30af7cc. The older PR branch is divergent; a non-force fast-forward merge-parent commit attaches it to the verified base without rewriting the old history. PR target becomes wip/one-build.

## Implemented

- Bluetooth: independent, width-bounded two-peer names in the real global list row; guarded host names stub. No BLE link behavior changes.
- Idle waveform: cached 29-point Chicago/Miami native controls, presented through the original firmware RGB565 renderer; no synthesis or patch persistence changes.
- Chicago ADSR: temporary selected-stage control-amount schematic and four knob cells, preserving the native inline exponential idle envelope.
- PERF: saturated 16-bit CPU percentages, including values above 255, with no sampling/timing changes.
- [Real framebuffer comparison gallery](mockups/pr745-native/README.md): eleven two-column before/after pairs, preserving the exact 240 x 240 firmware pixels. No image generator was used.

## Remaining Chicago FX work is incomplete

The legacy UI type table lists ten inserts, but the current native audio owner admits only the original types 0,4,5,10,18,22,24 (Off, Chorus, Phaser, Ring, Compressor, Decimator/Crush, Peaking EQ). The five missing audio processors are Ensemble, GateRev, Hall, BPM Delay and Short Delay. This checkpoint does not pretend a menu label activates an effect.

Native target link: pool 335,500 / 344,064 B, leaving 8,564 B including mandatory 8,192 B reserve. Safely available growth is only 372 B. The existing selected compact owner measures 4,288 B on host; the historical chi_bpmdly_t alone is 8,204 B. A naive two-part allocation would violate the memory reserve. The required next stage is a validated, bit-exact reusable-memory design with full DSP state/tail regression tests. Do not reduce the reserve, overwrite live bus histories, restore retired engines, or silently degrade sound.

## Checked evidence for source changes above

- Bluetooth native-font/UI regression: PASS, 37 checks, isolated matched source, baseline RED on missing helper/duplicate stub.
- Released native UI regression: PASS, 21,812 checks, 41 framebuffer captures. Normal and ASan/UBSan outputs and PNG bytes match.
- SMK-37 Pro SDK V1.2.1 DEV: PASS, exit 0, 651,457 B model Pro_900; SHA-256 fbaca4d4d0eb48e3ae4227e619a9f5cad7b08e575d8ac14c9596ad14c155370a.
- SMK-37 Elite SDK V1.2.1 DEV: PASS, exit 0, 651,457 B model Elite_900; SHA-256 9a91d58d92400f73527fa109e959a0b1627286185098ce59184f0010845b6d38.
- Both apps 465,160 B, RAM 95,284 / 98,304 B, pool 335,500 / 344,064 B. Native linker/model/locked-settings checks passed. One preexisting missing-braces warning in miami_decimator_adapter.c.
- The full applicable SMK host-suite qualification is not certified by these scoped tests. FM-1 and install-specific tests are excluded from any SMK-only run. Source build is not proof of hardware/device use or bit-exact audio.

No PAD DIAGNOSTICS, GitHub Actions, third-party SDK, credentials, firmware images, raw framebuffer captures or protected data are in the changes. Source and composed, verified PNG review mockups only.
