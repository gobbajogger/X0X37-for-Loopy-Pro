# SMC recovery: source, artifact and installation evidence

Snapshot: 2026-10-21, 12:53 UTC. This records the inspected evidence, not a claim
that the original working firmware has been identified conclusively. The
[hash index](smc-recovery-provenance-2026-10-21.json) contains full artifact,
report and imported-tool identities without private logs or firmware payloads.

## Established relationships

The prior read-only inspection of the owner-supplied `project.zip` checked
retained archive members against the collector manifest. It executed no uploaded
code and performed no build, device command or install. Its hashed report and
structured data are listed in the index; this summary reuses that evidence.

- Retained validation `commit.txt` files identify
  `532d91fc69232fdc932d516e9190028fb9a699da`.
- The later formal gate identifies `4346c87`, not `532d91f`. Its corresponding
  dev/rel packages match the earlier retained validation bytes wherever both
  exist. Matching bytes do not erase the distinct source/build records.
- The historical gate ended with a gate-trap host self-test failure. Firmware
  and controller builds succeeded. The gate result does not prove either that
  the firmware was defective or that no separate installation occurred.
- `53d5654` is a contemporaneous source reference associated with the owner's
  successful SMC-control report. No recovered original native install record
  yet binds that response to one of the retained dev/rel packages.

The Elite package identities below are SHA-256 hashes of the complete files.
The product is `SMK-37 Elite_900`; each package is 651396 bytes.

| Configuration | Raw package | Controller embedded |
| --- | --- | --- |
| dev | `a64fffc677d1ac3a2e2c685f35bb80b216d20052b6a4964ef597623e8cdf24d9` | `0a7cff9775aeb74caa84715af4ce36244c6a95249fcfb8743cc24c2577233469` |
| rel | `fca1c6c95f0eca166e6d6137a3cf40f8d675ca48543ff75adb4cbf9f67becc16` | `d80ff2a9e1ccb74beb1f287cbfdec616e0ae7e687a6ccb41ff922e9327d06053` |

Removing the embedded controller reproduces each paired raw package exactly.
The historical controller is 65492 bytes, ABI 2, SHA-256
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
Dev and rel app bytes differ; their controller bytes do not. The separately
named October 20 `installed-elite-with-controller.fwsc` contains a different
app; its name alone is not proof of installation.

The `4e921fe` historical reconstruction is close but not byte-identical to the
retained dev package: 67 whole-package bytes differ, including 53 bytes in the
app code area. The controller is identical. The original evidence did not have
an ELF/map that explains the remaining app differences. Do not label the
reconstruction an exact original binary or a proven working device baseline.

The current `816e0d3` runtime is a separate candidate. Its 65524-byte controller
SHA-256 is `9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245`.
Host-tool and packaging commits must not relabel retained firmware as rebuilt
from those later commits. This recovery record makes no new hardware claim.

## Missing installation link

The inspected archives omitted several native install logs and records because
of the original collector's archive budget. Host-test logs and a build-script
comment about automatic controller selection are not physical installation
records. File dates, source labels and directory names are not sufficient.

The narrower [install-record collector](../../tools/recovery/install_records/README.txt)
prioritizes those missing records and inventories narrowly selected SMC and
integration evidence. Its output remains private until the owner reviews it
and manually chooses to share it. No inferred installed identity is substituted
for the missing record. Later records may clarify the chain, but their dates
alone cannot prove which package produced the earlier approved behavior.

## Committed recovery tools and checks

Both reviewed collectors, their launchers, bilingual instructions and original
tests are preserved byte-for-byte under [tools/recovery](../../tools/recovery/README.md).
The index records their original import hashes. A shared boundary suite adds
symlink-ancestor refusal, existing-output preservation, FIFO exclusion and
source-byte preservation checks for both collectors.

Fresh integration checks passed: 11 artifact-collector tests, 8 focused-record
tests, 3 shared safety tests and shell syntax for both launchers. All collection
used temporary fixtures; neither real home folders nor hardware were accessed.
There was no firmware rebuild, install, network use or macOS execution in these
checks. Prior binary and source-linkage inspection is reused, not relabeled as
fresh device validation.

Only reusable tools and this sanitized index belong in Git. Do not commit raw
captures, collector output, credentials, personal logs, device identifiers,
SDK/toolchain files or firmware binaries. Keep original private evidence intact.
