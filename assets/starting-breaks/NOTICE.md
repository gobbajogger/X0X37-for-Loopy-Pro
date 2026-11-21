# Five owner-designated BREAK assets

`starting-break-pack.bin` contains the five complete ADPCM zones in native
`X0XBRKPK` format, in contract order, at 22,050 Hz with one bar per entry.
It is 109,394 bytes, SHA-256
`8df1316157fc8ce8dcb87cfd267fea83790702ccd1963ae91c0de60ca255b582`.
The individual zone files, sample counts, digests and recovered notices are
in the [source asset directory](https://github.com/gobbajogger/DnBOX-nightly/tree/6ec5f5ba3f78a85b84d46679b302cb8880986bf6/web/product-site/assets/starting-breaks).

## Source notice

These are the five selected encoded zones from the preserved original
`slots-before-install.x0xslots`: BR1 zones 1–3, followed by BR2 zones 1–2.
All five are the requested default selection.

The .adpcm files preserve the complete original encoded zone bytes. The .wav
files are deterministic mono 22,050 Hz, 16-bit PCM previews decoded from those
bytes. They exactly match the source contract's preview and PCM hashes. They
are not original pre-backup WAV files; those filenames and lossless provenance
are unknown. No normalization, resampling, pitch or tempo change was performed.

Current owner direction authorizes public Git and Library distribution of
these owner-selected assets. No CC, GPL, public-domain or other specific audio
licence is asserted. Preserve actual recovered notices and attribution.

The GPL-3.0-only notice in verify_assets.py applies to verification source code,
including inherited Felucca IMA decoder semantics, not to the audio payloads.

No installer, device write, sample erasure, package replacement, deployment,
hardware acceptance or complete historical sample restoration occurred.
