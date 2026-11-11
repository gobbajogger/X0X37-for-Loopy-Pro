# PR #689 cloud build and host-test attempt

Observed on 2026-10-30 in an independent Linux x86_64 cloud container.

## Immutable GitHub identities

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/689
- Starting PR HEAD: `1b1a9c135b1e40d12d3431017f486e3aa21f433e`
- Latest integrated base: `wip/one-build`, `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`
- New regression commit: `339cea4b0a2b611b1a12b85f3c039d50f76a7195`
- Full tree API: root tree `70f1867cd54de153fc179a61f97a9dc007c2382f`, complete, `truncated=false`, 4,584 Git entries at the prior PR head.
- Source reconstruction: compared against historical `6ae0928fe3264ee759c25c4f879eff4dc5dc3b20`; 223 changed/new blobs and 216 removed paths were identified. **These changed blobs have NOT all been transferred to the cloud working checkout.**

## Verified native tools

- JieLi Linux toolchain ZIP SHA-256: `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`
- JieLi pi32v2 Clang: version 4.0.1, executed natively.
- Pinned AC79NN SDK V1.2.1 input ZIP SHA-256: `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`
- SDK kit verification: **PASS, 11/11** pinned files.
- The separately staged `wip/private-headers` (PR #721) has HEAD `e189819d817dd3c59b4a1d219bf93be9f065d8d7` and is not yet part of the integrated base.
- The owner-provided V1.2.0 archive was not substituted for V1.2.1.

## Checks run on a historical / GitHub-blob-matched BREAK projection

The local source starts from an **offline historical Git checkout** at `6ae0928`. BREAK-related source, test and runner files were replaced with bytes verified against current GitHub blob IDs. This mixed projection is **not** a byte-complete checkout of the current PR, so its results cannot qualify the whole PR.

| Test | Command / scope | Actual result |
| --- | --- | --- |
| Adaptive BREAK DSP | `tests/host/run_adaptive_break_dsp.sh` | **PASS**, 51,481 checks, zero failures; sanitizer run also passed |
| BREAK performance | `tests/host/run_break_performance.sh` | **PASS**, 2,413,370 checks, zero failures with GCC and Clang; sanitizer run passed |
| MIDI pulse pairing | `tests/host/run_adaptive_break_midi_pair.py` | **PASS**, 27,348 checks |
| Built-in BREAK output | `tests/host/test_builtin_break_output.py` | **PASS**, seven test methods |
| Adaptive BREAK controls | full-suite selector | **FAIL** compiling historical/mixed dependencies, including absent BLE-peer declarations; not a current-PR verdict |
| Adaptive BREAK storage | full-suite selector | **FAIL** compiling historical/mixed dependencies, including weak-inline warnings under `-Werror`; not a current-PR verdict |
| Six-check BREAK selector | `X0X_TEST_JOBS=4 bash tests/run_tests.sh adaptive-break-dsp adaptive-break-controls adaptive-break-storage adaptive-break-midi-pair break-performance builtin-break-output` | **EXIT 1**: four PASS, two FAIL on this mixed projection |
| Full diagnostic suite | `X0X_TEST_JOBS=8 sh tests/run_tests.sh` in historical/mixed checkout, 540-second bound | **EXIT 124 (TIMEOUT)**. The suite was incomplete, with at least 56 individual `.fail` markers observed before timeout; this is NOT an exact-current full-suite result |
| Elite native development build | `BOARD=smk37 MODEL=elite X0X_DEV_PADCAL=1 bash build.sh --board smk37` | **EXIT 1**, JieLi compiler errors against historical UI/platform mix, no current candidate package |
| Pro native development build | `BOARD=smk37 MODEL=pro X0X_DEV_PADCAL=1 bash build.sh --board smk37` | **EXIT 1**, same historical UI/platform mix, no current candidate package |
| New registration guard | `python3 -B tests/host/test_break_suite_registration.py`; `X0X_TEST_JOBS=2 sh tests/run_tests.sh break-suite-registration` | **PASS** direct and through runner; six registrations, class G |
| Registration guard negative cases | locally delete or duplicate one entry in a temporary runner snapshot | **PASS**: both malformed registrations were rejected |
| Runner shell syntax | `sh -n tests/run_tests.sh` | **PASS** |

The current GitHub `firmware/src/app/ui.c` blob `2e51cc1ffa6026c98eeaba76774b041982f5e7ac` contains neither `plat_ble_peers_t` nor `plat_pad_diag_sample`, confirming that the old-mix compiler errors cannot be called verified current-PR regressions.

## Status and next action

**Exact-current full host suite: NOT_RUN to completion. Exact-current Elite and Pro builds: NOT_RUN. No exact-current package, controller validation, audio golden, or hardware PASS exists.**

The GitHub connector successfully reads commit trees and individual source blobs. Direct GitHub archive access through the connector was rejected as an unsupported fetch endpoint; the cloud shell's direct GitHub API attempt failed DNS. A local source transfer sufficient to reconstruct and verify the full current tree remains unfinished. Continue from the exact remote branch, transfer/verify its entire source closure via the authenticated GitHub API and required original-synth inputs, run all tests and both native builds, and fix only failures reproduced on the exact candidate. This report neither weakens test requirements nor authorizes a release, protected merge, firmware publication or flash.

No third-party SDK bytes, firmware binaries, protected flash contents, secrets or raw captures are included in this evidence file.
