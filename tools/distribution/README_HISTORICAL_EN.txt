SMK-37 Elite — historical 1.03 / 53d baseline REBUILD (DEV)

This is a fresh rebuild of owner-selected source
53d5654c9bbaec10b64c55fb60d0e7d857ba8156, not a new SMC fix and not a
claim of stability or successful connection on the current keyboard.
Historical mappings and features are intentional. Later features, diagnostic
changes and vendor-port experiments are not merged into this firmware.

The source/time association is historical evidence, not an original installed
checksum. Compared with the retained early raw package, this rebuild differs
by 53 application bytes in generated FONT_XS_DATA plus integrity bytes.
All other decoded application bytes match. The freshly built controller is
byte-identical to the retained historical controller. Do not call the raw
package the original flashed binary.

Start with `bash Install.command --verify-only`. It opens no MIDI port and
installs no dependencies. For an approved installation, run Install.command
in a foreground Mac terminal. Git and Python 3.9+ with venv/pip are required;
missing mido/python-rtmidi may require network. CoreMIDI is selected only for
this launcher's child processes, with JACK disabled for a missing dependency.
Disconnect other compatible keyboards. Keep the Elite connected and powered.

Use the guarded launcher, not a direct flash tool. It checks exact DEV evidence,
model, source, firmware/controller pair, write ranges, queue and head reference,
holds the native device lock, asks for confirmation, backs up every sample/loop
slot, and performs mandatory native loader/controller and sample readback.
Keep the complete logs and backup. Never use force or a no-controller shortcut.
Keep the designated installer's existing queue and head-reference settings.
A present queue is enforced; an absent queue is explicitly NOT CHECKED.
If no genuine unit head reference is supplied, that comparison is also NOT
CHECKED. Supply the designated installer’s genuine unit head with --head-ref
/an/absolute/path when available or required; never fabricate a reference.

NO automatic post-install connection watcher or polling runs. The launcher
ends after the native installation/readback and log checks. Optional diagnostics
must be started separately by hand only when requested; they can change load.
For such a later manual test, from this folder after installation:
MIDO_BACKEND=mido.backends.rtmidi/MACOSX_CORE .venv/bin/python3 -B host-tools/tools/smc_connection_watch.py --seconds 45 --interval 0.25 --output logs/manual-smc.jsonl
Use a fresh output name and --port NAME if needed. This is not run by Install.command.

The one user-facing firmware in firmware/ already includes its exact matching
Bluetooth controller. Original raw package, controller, genuine DEV record and
logs remain unchanged in evidence/validated-build/. The combined package is
an exact reversible derivation, not relabeled raw build output.
Exact runtime Git source is under x0x37/. Separately pinned maintained host tools
are under host-tools/; they were not compiled into the firmware. MANIFEST.json
records all source/tool/artifact identities and any shallow history boundary.
SDK/toolchain binaries, credentials and private device captures are excluded.
Licensing files are retained. Only Elite DEV is included, not Pro or a release.

The genuine fresh DEV checks passed within their recorded scope. No physical
installation, current connection test, full release gate, or new hardware PASS
was performed in the cloud. The owner reported nonconnection with the later
515 diagnostic and 816 rollback; this kit does not establish the cause or cure.
