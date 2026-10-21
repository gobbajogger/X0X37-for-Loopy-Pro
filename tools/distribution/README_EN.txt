SMK-37 Elite — guarded development candidate

The one user-facing firmware, firmware/x0x-smk37-elite-with-bt.fwsc, already
contains its matching Bluetooth controller. No user assembly is needed.
Use Install.command, not a direct flash tool: it retains native model, queue,
bootloader-range, hardware-lock, confirmation, backup and readback safeguards.

Run `bash Install.command --verify-only` first. It opens no MIDI device and
installs no dependencies. The unchanged raw package, selected controller,
DEV record and logs are internal inputs under evidence/validated-build/.
The combined file is derived from them; its record is never relabeled as if
it were raw build output. Runtime and packaging-tool identities are separate
in MANIFEST.json. Read BUILD_REPORT.txt for scope and limitations.

For an approved install, use a foreground Mac terminal. Git and Python 3.9+
with venv/pip are required. Missing mido/python-rtmidi may need network access.
The launcher selects CoreMIDI only for its own child processes; it does not
change system settings. Disconnect other update-compatible keyboards.
Every sample slot is backed up and preserved data/controller are read back.
Keep the backup and logs. Never use --force, --no-ctl or a manual flash shortcut.

Keep the designated installer's existing queue and head-reference settings.
A present queue is enforced; an absent queue is NOT CHECKED, not a queue PASS.
Do not fabricate an empty queue or skip queued forward fixes.

After a successful install a 45-second read-only connection capture starts.
Try the intended controls and separately record gesture timing. Counters do
not prove physical movement. Packaging/DEV checks do not prove an SMC fix.
No actual device or macOS runtime result is implied by cloud host tests.

Available runtime Git history is included; it can be shallow as declared in
the manifest. SDK/toolchain inputs and private raw captures are not included.
Only Elite DEV is supported by this distribution tool; no Pro artifact or
release PASS is supplied. Licensing files are retained.
