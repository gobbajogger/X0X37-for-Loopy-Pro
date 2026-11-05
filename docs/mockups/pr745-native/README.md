# PR #745 native framebuffer comparison gallery

All eleven pairs are produced from the real 240 × 240 firmware UI/framebuffer renderer and native project font data, built from the verified source at `wip/one-build@5bff28caba24568cd5070acb99aaf37729cd8323`. This is not an image-generation approximation.

Each PNG is exactly two columns: **BEFORE** (unchanged application UI) and **AFTER** (restored application UI). The only additions to the baseline renderer are test-only screen-state fixtures; the baseline production renderer is unchanged. The AFTER screen uses the production renderer from PR #745's source candidate. The compositor adds the upper labels and preserves all 240 × 240 RGB pixels from each original source image. Pixel equality was asserted for every comparison.

| Pair | What it checks |
| --- | --- |
| [Bluetooth, selected](bluetooth-peers-selected.png) | Long connected device names in the selected global list row |
| [Bluetooth, unselected](bluetooth-peers-unselected.png) | The same peer names with the other row selected |
| [Chicago oscillator](chicago-oscillator.png) | Native cached Chicago waveform cue |
| [Miami carrier](miami-carrier.png) | Native cached Miami carrier waveform |
| [Miami cross-modulator](miami-xmod.png) | Native cached Miami cross-modulator waveform |
| [Miami wobble](miami-wobble.png) | Native cached Miami wobble shape |
| [Envelope attack](chicago-touch-attack.png) | A held/touched Chicago attack-stage schematic and knobs |
| [Envelope decay](chicago-touch-decay.png) | The decay-stage selection |
| [Envelope sustain](chicago-touch-sustain.png) | The sustain-stage selection |
| [Envelope release](chicago-touch-release.png) | The release-stage selection |
| [PERF above 255%](perf-high-cpu.png) | CPU percentages of 300 and 511, not byte-truncated 44 and 255 |

[Open the complete two-column gallery](all-before-after.png).

These captures use deterministic host inputs, not a hardware display readback or a CPU performance benchmark. Normal and ASan/UBSan firmware UI outputs compared equal byte for byte. The Bluetooth long-name BEFORE fixture includes only test-host stub isolation to avoid a pre-existing duplicate-host-symbol error; it does not patch baseline production UI.

The all-ten Chicago audio insert restoration is a **separate, unfinished DSP requirement**. No screenshot here claims that the five currently unsupported FX types have been wired to the native audio processor. PAD DIAGNOSTICS is intentionally excluded.
