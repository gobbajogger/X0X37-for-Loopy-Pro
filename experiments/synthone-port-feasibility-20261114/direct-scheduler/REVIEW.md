# Independent review: direct-event scheduler source stage

Verdict: **accepted for the documented source prototype and corrected portable-control binding boundary**. No remaining actionable finding was observed in this exact reviewed closure. This is not firmware integration, full-engine acoustic acceptance, target resource/CPU qualification, or release approval.

The reviewed intent is a serialized direct-note, arp-off, empty-pending-sequencer, pre-FX scheduler using six fixed poly states plus a separate mono state. The source references are AudioKit Synth One `6466a3715c96b7ecf1dd255a218cbd571408a314`, AudioKit `03fecf80a66be88f6ac0356cd532c72dbe09922d`, and the existing shared graph `a0c8698f724e28ec6275307525933c195817b3c5`.

## Exact reviewed files

| File | SHA-256 |
| --- | --- |
| `scheduler.hpp` | `d54b37188315d701d6ac3d25ea25772870795d96b66d80a66dd2a749f610a61d` |
| `scheduler.cpp` | `d092837529cc71a167de00f1d7861a73fceda4f03a5e72848a0013aa4c8e068b` |
| `tests/scheduler_test.cpp` | `23af3ec6e093215bf3f582cf193513bf40fe018811c4d5ba9d44cc0fae7d5a6f` |
| `run_tests.py` | `e3b41c3cc62b557a9caf8de6217c37bef35303afea0b5b5675a2cf905da07e4d` |
| `mutation_checks.py` | `5f7101d92d7a897cb96151f8ca76188c0d3d166de5b2844533b7c5392675c642` |
| `DEPENDENCIES.json` | `b553d6273e9ae819fd306a70c8372ad42857365303f9a23a8c01da0f6f18cf8b` |
| `README.md` | `07bb5e2608abfc720aacfb5ee49cdad353423c065df1610a131282c5097f9e05` |
| `oracle/generate.py` | `47ee5ef07510bc52367cda501f143f938a51bc023b71514c8afd745468b95a96` |
| `oracle/oracle.hpp` | `c856ac499d87f24ff6d1d239ddaf333e298c486c61cf4aca9579c7d195c38b64` |
| `oracle/oracle.cpp` | `7526b71d59063dca29c9dbd41a64e01414f46fe31974784e3eb18e1d505e498c` |
| `oracle/MANIFEST.json` | `8f1e66a1165e09aefad5086923f0ef5c41afb66325cbfeb9c5b6ee91d5cad4ce` |
| `notices/SynthOne-MIT.txt` | `71ac2b8af8849096ba2d0f3510972b8384204f17517e773bfa52fc755fb0fc49` |
| `notices/AudioKit-4.9.2-MIT.txt` | `b9aad96b49e7afcadb3f351b733de24577924b3ba48c82a3e07393d8b9f96c5e` |
| `.gitignore` | `6d9dd2863a77d46eb538f9032b69297ef450edf9afcd2b99ed117d80a0cb1a74` |

## Portable control-package integration correction

Actual consumption of the sibling public control package exposed a defect in the previously accepted source/text projection: its manifest still pinned private header notice-comment versions. The public package was rejected before compilation. This review supersedes the prior report for that public binding; the original source-stage acceptance did not establish successful consumption of those actual public headers.

The repaired manifest pins these four actual public compiler inputs. Both C++ files are byte-identical to the original producer inputs. Each header changes only its first notice comment; all following bytes, declarations and include directives are identical. No include rewriting, arithmetic adaptation or relaxed matching is used.

| Public control input | Bytes | SHA-256 |
| --- | ---: | --- |
| `parameter_ports.hpp` | 1836 | `b533cc60b93da7df64aca34be3f382ff0191ffd2c4f6db6275bffcd06c665f61` |
| `parameter_ports.cpp` | 5199 | `bdf3ee420d61211849afbdd5de5cb4fe4b027922275b8d8cd9cf828bd21fc86c` |
| `generated/parameter_map.hpp` | 445 | `c7bc2f49818d4f0c202b6b1c79ea7552f8a70476ff388889636908f7421ca11e` |
| `generated/parameter_map.cpp` | 845 | `71b50fadd233d7092abec38be6ce92353a734c578ce68f2a0e3ac1f23bc67cae` |

The reviewer independently ran the corrected public consumer in O2 with the actual public files: three short cases over 44.1/48/96 kHz, 144 total frames, 189 exact state snapshots and 18 events, with no trapped event/render heap calls or failures. All 19 compiler dependency scans consumed only the staged allowlist, including the exact four public inputs; the generated smoke entry is separately recorded. Its renamed original 48-case entry is not called.

Producer O2 and ASan/UBSan smoke receipts were read and matched to the current public-control pins and result metadata. The old private header root now fails the new manifest check before the build begins. This changed binding qualification does not rerun or relabel the historical 48-case suite, original sanitizer runs, ten mutations or independent randomized proof. Those remain tied to original source/input manifest `5a507c9cac27fb63f58544072dc79df85a9af8dd13eb8f53b56cff0e29c5fa59`; scheduler runtime SHA-256 `d092837529cc71a167de00f1d7861a73fceda4f03a5e72848a0013aa4c8e068b`, original fixture and oracle bytes are unchanged.

The sibling package's separate runner and full supplier-package acceptance are outside this consumer review. This review binds only the four stated compiler files and the scheduler's changed portable consumer path.

## Corrections verified

- The original 8 KiB snapshot could not hold all 128 held pitches. The repaired 16 KiB fixture exercises capacity, deduplication and stop-all; independent normal and sanitizer probes also cover the full boundary.
- An unlisted source-adjacent header could previously bypass the dependency allowlist. The runner now compiles a fresh closure containing only verified dependency bytes and recorded local source bytes. An independent run with a deliberately failing unlisted header still present succeeded because that header was excluded. All 19 compiler dependency scans consumed only the allowed staged inputs. The final runner-only hardening clears all four ambient compiler include-path variables for compiler/link calls. Its source was reviewed, and retained producer evidence records a passing O2 run and ten detected mutations with those variables pointing to a deliberately failing header; that ambient run was not independently repeated.
- Fresh inverse-LFO state now follows the original source's zero initial values. The shared oracle adapter also clamps the current-output half-time through the original parameter range. Both repaired assertions were independently checked with compiled regression mutations that failed at runtime.

## Independent evidence

The actual upstream event/reset methods and pre-FX process prefix regenerate byte-identically from pinned inputs with recorded exact-count adapters. All 42 listed dependency identities match. The event allocator, sparse-slot/PRNG order, block-entry release threshold, mono fallback, immediate poly ADSR step, raw mono-mode reset, held ordering, reset/restore separation and output admission were compared with the source contract.

At the preserved original source/input freeze, the repaired focused O2 runner passed 48 cases, 17,889 frames, 18,939 state snapshots and 630 events. At that same original runtime/input freeze, additional independent normal and ASan/UBSan probes passed nine rate/scenario cases at 44.1/48/96 kHz: 19,953 frames, 28,874 exact state snapshots and 7,687 events, including 7,500 seeded mixed operations. No trapped event/render heap call or sanitizer error was observed. Two independently compiled shared-adapter regression controls were detected at runtime; compiler failure was not counted as detection. The hardened negative harness verifies and stages the table payload, requires its unchanged baseline to pass, and rejects a wrong-table input before build lookup. The source/text projection and derived producer aggregates were read and matched to exact source files and retained positive/negative results; no raw tables, banks, generated audio, executable artifacts or private build paths are included.

Oracle independence is bounded: it executes extracted upstream event/reset/process logic, while both implementations share the documented caller-owned initialization/restore/table adapter. Source inspection, the unchanged actual NoteState initializer comparison, explicit source-initial-state assertions and the regression controls cover those adapter seams. This is not an independent build of the complete upstream application.

## Remaining boundaries

Global effects and their tails, arp/pending-note execution, source rate/dependent-parameter routing, external event slicing, sustain/MIDI wrappers and cross-thread snapshot timing remain outside this cut. The borrowed tables and tuning must remain immutable and valid for the instance lifetime. Fresh `init` is a new logical instance, not a model of upstream live-object reinitialization. `restore` accepts resolved physical-range values; transient smoothed outputs can fall below those ranges and be rejected.

The host state includes oracle-only reference-port storage and is not a production target layout. The target table-read path, exact combined Elite/Pro flash/RAM, worst-case workload, integrated audio, preset fidelity and device behavior remain unqualified. No historical graph/control/preset suites or native/device checks were rerun for this review. No firmware source, protected branch, publication or device action was performed.

Actionable findings: none remaining in the exact reviewed source stage.
