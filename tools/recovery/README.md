# Read-only SMC evidence recovery

These are the reviewed local collectors used for the October 2026 SMC recovery.
They are host tools, not firmware or install commands. Their original source,
launchers, tests and user instructions are preserved byte-for-byte. Original
file hashes are in the [provenance index](../../docs/audit/smc-recovery-provenance-2026-10-21.json).

- `artifact_evidence/`: bounded inventory and copies of selected project/gate
  logs, manifests and retained firmware. It reads only the documented project,
  gate-cache and exact controller-cache locations. It excludes SDK/toolchain,
  source-control/session storage, private config and sample-backup directories;
  the documented exact `ctl.img` exception does not enumerate neighboring config.
- `install_records/`: a smaller text-only pass that prioritizes previously
  omitted install records and narrowly selected SMC/integration evidence. It
  does not collect firmware binaries or repeated source, docs and tests.

Copy a whole subdirectory to the owner's Mac and read its `README.txt` before
running its `.command` launcher. Python 3 must already be installed. Launching
collects existing files; it does not install dependencies, execute recovered
files, invoke Git, contact a device or send anything over the network. Each
collector writes a new local output folder and refuses to overwrite its ZIP.
Do not run either collector on unrelated personal folders. Do not disable
macOS security if a launcher is blocked; preserve the exact error instead.

Collector output is **private evidence, not repository content**. Review the
manifest and selected files locally before manually sharing with an approved
recipient. Filename filters are not secret redaction. Do not commit output
archives, logs, captures, binaries, device identifiers or personal paths. A file
hash identifies bytes; dates, directory names and a successful build do not
prove which firmware was installed. Limits and missing files are recorded;
an incomplete scan is not proof of absence.

## Offline regression checks

From the repository root:

```sh
python3 -B tools/recovery/artifact_evidence/test_collect.py
python3 -B tools/recovery/install_records/test_collect.py
python3 -B tools/recovery/test_recovery_safety.py
bash -n tools/recovery/artifact_evidence/Collect-SMC-Evidence.command
bash -n tools/recovery/install_records/Collect-SMC-Install-Records.command
```

Tests call `collect()` only with temporary fixture directories. They do not run
`main()`, enumerate the real home folder, access hardware or use the network.
The shared boundary tests add root-symlink, FIFO, output-preservation and source
preservation coverage. Linux tests do not establish actual macOS execution.
The tools use the repository's GPL-3.0-only licence; keep `LICENSE` when copying.
