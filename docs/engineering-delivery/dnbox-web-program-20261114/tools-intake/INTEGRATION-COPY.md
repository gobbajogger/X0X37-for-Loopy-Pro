# Source-backed wording for the integrated music tools page

**Sketch the phrase before you reach for the keys.**

The offline editor source offers Chicago/Miami piano rolls and separate 909, 808 and BREAK grids, MIDI-import preview, local JSON drafts and Undo/Redo. It does not play audio or sync with the keyboard.

CTA: **View offline editor source** → https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer

Compatibility note: Its project adapter accepts only a narrow raw revision-13 format-1 subset. It does not open portable `.x0xproject` bundles or stream-14/15 projects. Use **Projects** for backup and restore. Keep the original files and separate sample backup.

**Inspect and keep your own engine-preset files.**

The delivered DNPR librarian checks preset files before an explicit Keep in a temporary browser session, then preserves eligible user bytes on download. It stores no sample audio and cannot send presets to the keyboard. The browser session closes on refresh; download the user files you want to retain.

CTA: **Preset file source & status** → https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/915

**Take the whole song with you.**

Use Projects to back up and restore supported `.x0xproject` files. Project-only files omit sample audio; portable projects can include it. Keep a separate sample-slot backup and check the actual tool's version and result.

CTA: **Open project tools** → https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/projects/

**Bring a loop of your own.**

Use the sample tools to prepare, preview and transfer supported loops, or keep a separate sample backup. Sample transfer and project save are different operations; follow each tool's checks and confirm the result on the instrument.

CTA: **Open sample tools** → https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/breaks/

These are wording options for the sole website writer. No component implementation or publication is claimed. The tested public preset and editor launch routes returned 404, so their CTAs deliberately identify source/status. The active importer, librarian, editor and publisher owners retain their scopes; do not infer device compatibility from a source link.
