# SMC control and light-feedback guide — source-reviewed

- [English usage guide](GUIDE-EN.md)
- [Руководство на русском](GUIDE-RU.md)
- [Exact source references, status semantics and verification](SOURCES.md)
- [Separate clip-visibility test-fixture correction](test-fixture-repair/REVIEW.md)

**Build/source:** `reference-smc-integration` at `4cf8c2f1d641bf91041a39f0d34d77cd51fddbf8`, following `81459573792a68edd02fa1ee5658796b55597ba0`; reviewed on 2026-10-21. The SMC mapping, UI and protocol files are unchanged between these revisions.

**Replaces:** the earlier `SMC-X0X37-Usage-Guide-EN-RU-ab1694e.md` as the source-reviewed usage guide only. It does not replace firmware, installation instructions or the live installer.

**Checked against decisions:** one direct SMC in physical DAW mode; coherent main/909/808 strip layouts; individual drum selection and SMK pad sequencing in both MIX and SEQ; temporary central-only test profile with Loopy over USB. Three simultaneous SMCs and simultaneous Loopy peripheral BLE are not delivered by this profile. Physical User-mode rejection cannot be inferred from overlapping MIDI data.

**Review:** [source and test verdict](SOURCES.md#verification) completed independently on 2026-10-21. Five focused checks passed, including 1,838 feedback expectations. The separate fixture repair passed 138 checks in each of default and TEST profiles with `-Werror`; hidden-edit and no-replay negative controls failed as expected. No firmware source was changed.

This review performed no publication, installation or hardware test. The owner's earlier report that lights look correct is not relabeled as validation of the current vendor-host build. Files here are text/source only; no SDK, toolchain, raw capture or firmware binary is included.
