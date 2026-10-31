# Designated stock loops

The stock bank contains CRAMEN, TASTY and HOTPANT, the three owner-designated
loops. `provenance.json` records their original and encoded hashes, complete
sample lengths and conversion details. Original WAV files remain preserved
separately; this folder includes only the designated encoded stock subset.

The frozen bank is 131152 bytes, SHA256
`cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e`.
IMA ADPCM is lossy. TASTY is upsampled from 32 kHz to 44.1 kHz. All three use
the retained one-bar assumption; this is not a listening or hardware claim.

Run `sh tools/export_factory_breaks.sh` to reproduce the binary and browser
module from these ADPCM files. This no longer re-synthesizes the old 909 loops.
The default build still compiles an empty bank. `X0X_FACTORY_BREAKS=1` explicitly
compiles these same stock bytes; `X0X_BREAK_BANK` still selects a custom bank.

For default empty-bank builds, `tools/package_stock_loops.py` composes the stock
bank into a pinned package's free code tail. It verifies the package and exact
matching controller before and after, preserves all other package regions,
refuses insufficient space and never publishes or accesses a device. This
reuses the native application build, but the resulting package has its own
hash and needs exact-package review and delivery evidence.

The installer default keeps the package's existing loops unchanged. Selecting
stock replaces the package bank with these three; choosing WAV files remains
an explicit alternative. Imported user sample slots are not changed by the
bank composition operation.
