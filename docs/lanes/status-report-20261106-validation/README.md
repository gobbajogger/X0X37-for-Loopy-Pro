# Status report lane: pinned-base validation

The existing status report and nine optimizer notes are recut onto exactly
`wip/one-build@48e92b76c2f476cf40771e00a04e1838136bb55d`. The ten original
lane files retain their exact bytes. This lets the owner review these notes
against the requested firmware baseline; installer and player behavior are unchanged.
Old measurements in those notes remain historical snapshots.

## Checks executed here

The root Codex session ran both native DEV builds, the matching controller build,
and every applicable registered host check. No test execution was delegated.

- Elite native DEV build: PASS, exit 0, SDK V1.2.1.
- Pro native DEV build: PASS, exit 0, SDK V1.2.1.
- Matching controller build and image check: PASS.
- Full host coverage: **336 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**.
- Full strict preflight: **16 PASS, 0 FAIL**.
- All 11 consumed SDK file hashes match commit
  `d179b4484759423312073f5fbb232501aa491047`, tag
  `AC79NN_SDK_V1.2.1_2023-12-13`.

The unrestricted full run was interrupted by an environment reconnect after
88 completed passes. The runner registry was enumerated without running tests;
the other 248 checks then ran on unchanged inputs and completed with exit 0.
There was no single uninterrupted green whole-suite invocation. The full registry,
completed receipts, exact resumed selector list and logs are retained here.
No assertions, expected counts or source tests were changed.

Setup corrections were confined to the execution environment: a writable task-local
generation cache, host Clang 19.1.7, and an approved unrestricted runner for real
LeakSanitizer execution. The initial sandbox attempt is not qualification evidence.
GCC 14.2 and Clang sanitizer matrices passed with their stock settings.

| Measured budget | Elite | Pro | Limit |
| --- | ---: | ---: | ---: |
| App image, bytes | 480568 | 480568 | 552672 maximum |
| App RAM, bytes | 95780 | 95780 | 98304 maximum |
| Sample pool free, bytes | 8600 | 8600 | 8192 minimum |

The controller is 61088 of 65536 bytes; its RAM is 26336 of 26368 bytes.
One inherited initializer-braces compiler warning remains; both builds exit 0.
Artifact sizes and SHA-256 values are in [result.json](result.json). Binaries remain local.

## Input and delivery identity

- Pushed source commit: `2ac1b2ab13f6e50bae0c2c757a859aba9661285c`.
- Checked local commit: `223407f8db912e1ba8be58d23c565ef4836c751e`.
- Identical complete source tree: `85fa45f870d19537261b8984fceaecd91fbda096`.

The connected GitHub API used its default author and retained the original lane
as an additional parent, allowing a normal fast-forward update without rewriting
remote history. The two commit identities have different metadata and ancestry;
their complete source trees match. This evidence addition changes documentation
only. Firmware, tools, tests and other checked input subtree identities are
recorded in result.json and remain unchanged.

Qualification is BUILD_ONLY plus complete host coverage. Device, install, release
and publication checks are NOT_RUN. No main/nightly ref, workflow, protection or
device write was performed, and no binary, SDK, private-header or token payload
was added. Existing PR [#843](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/843)
targets wip/one-build.
