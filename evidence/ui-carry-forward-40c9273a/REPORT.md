# UI carry-forward patch: source ready for existing QA

Base: `c295af1554f9e80a29864cd013995eccad66cd2c`, tree `d98072ba1d689cb99bd16e4a6e1e682b22b04c70`.
Candidate tree: `40c9273a6bfbf99c4e799ad26abc5b4ecb68013e`. Source retained as an applyable patch on this evidence-only branch; no production-source ref is changed. The candidate tree is reconstructed by applying the patch to the exact base. Patch SHA-256 `2fcecab3530e019bc3618e782acdf2b09f2b5f129b818a401f4dd82d9bf3ac4b`. Apply from the exact base with `git apply --index ui-carry-forward.patch`; `git write-tree` must match the candidate above. Dry-run application on the clean base and reverse application on the candidate passed.

## Reused implementation

Six GLOBAL settings sections with remembered per-section selection/scroll and held-arrow consumption; Bluetooth connected peer names with bounded clipping/marquee; native-program waveform projections for Chicago oscillator mix and Miami carrier/modulator/wobble, including lock/motion previews. Waveform header is byte-identical to both supplied donors (SHA-256 c48cd6b043a13cd8b00191958b09b55f631b430eea47e968c1decf14a87e1910).

Donors: `8be16486e33f65b31c76710e1d825b6aa701c2c9` (tree `f952ab4455723e4a1a68fdb3a8eea22d2da59710`) and `c32c54ac8ec68aaca53adccf7af66f155590f2dc` (tree `ac490f7fae9198748aa54bd914d42f99d45444f9`). Narrow original commits: cc116395, 25933a9b, display/snapshot portions of d7e2e833, waveform portions of de80f7b6. All applied without textual conflicts. Adaptations: peer test enters GLOBAL/Bluetooth through actual UI; waveform test retains base's 76/14-byte control-cache guards, not donor's unrelated ADSR expansion; peer snapshot copies directly into zeroed destination, avoiding donor's uninitialized unused name bytes and temporary 42-byte array. Added donor bounds test plus snapshot checks and four SMC settings-navigation assertions.

No changes to sequencer/mutes, smc_mixer.c, DSP, Chicago effect policy, project persistence, display cadence, or bounded LCD dispatch. main_fm1.c change only consumes held arrows after section navigation. Donor Bluetooth independent-save changes were not included; that is a separate persistence change, not needed by name rendering. No whole donor file replacement.

## Actual validation

- PASS settings sections: 494 checks, including physical held arrows, selection/scroll, reset cancellation, all sections and SMC navigation.
- PASS peer UI: 61 checks with ASan+UBSan; 12 real host framebuffer captures including empty, dual/duplicate/max names, scroll and disconnect.
- PASS production BLE name copying and new snapshot: bounds/canaries, max names, empty unused slot and disconnected state under ASan+UBSan.
- PASS original-release-ui: 23,176 checks in normal and ASan+UBSan runs; all 40 normal/sanitized framebuffer pairs byte-equal. Added four waveform captures and cache/preview/invalid-input tests reuse donor coverage.
- PASS existing ui-perf-identity: 25,876 checks (header/arc/WAVE framebuffer and cadence); play-question-ui, sound-reset-ui, smc-multi-peer, lcd-bounded, smc-ui, smc-ui-bounds and smc-record-feedback.
- PASS lcd-dispatch-reuse and ui-waveform-truth.
- PASS clean-base patch application check; whitespace check. Actual composite screenshot inspected locally; constituent PNG hashes in `manifest.json`. Raw framebuffer captures are excluded from this public evidence bundle.

Original focused run: 10 selected checks PASS, original-release-ui FAIL on an unrelated imported 92/32 size guard. Restoring the unchanged-base 76/14 assertion produced the PASS above. This is repaired-run accounting, not an uninterrupted all-green run. An additional old smc_peer_names_test.sh fails compilation on BOTH unchanged base and candidate: redefined plat_ble_name and missing bt_peer_text. Retained as baseline FAIL, not deleted or marked passed. Full unfiltered host suite NOT_RUN. Package checks skipped for unavailable app/loader/SDK inputs, not PASS. Host fixture results are not hardware results.

## RAM and native limitation

Actual GCC host symbol sizes: ui 568 -> 584 (+16); original_wave +156; peer_scroll +68: **+240 static bytes**, of which +172 are marked X0X_POOL in source and +68 ordinary BSS. No new persistent transport buffer. Donor waveform cache remains shared across voices/widgets. These pointer-free state sizes are a target-layout estimate, NOT a native linker measurement. Native code size, alignment effects, stack high-water, pool free and Elite/Pro budget PASS remain NOT_MEASURED/NOT_RUN. Host -fstack-usage reports original_wave_project 352, original_wave_update 48, original_wave_view 128 bytes; these are individual host frames, not target stack totals.

Pinned JieLi compiler and SDK-input ZIPs were found in personal Library. Current supported Library download helper failed before transfer: hosted apps tools/list HTTP 401. No archive bytes were obtained, so no target compiler/SDK qualification was claimed. Existing Build owner can use the patch with its verified SDK V1.2.1/JieLi inputs, run development Elite/Pro builds per BUILDING.md, and compare exact baseline/candidate linker BSS, pool/free >=8192 and stack evidence. Do not infer remaining headroom from historical 8,600-byte receipts.

## Scope and receiving owners

Current policy read at nightly 66073c68dd80d51a7053432c2c161a8b720a1341 and coordination 4a35a6cdaa0f112db6b3f577542138afd170c20d, plus actual main/base AGENTS/workflow. Existing LX-REQ-016 UI/QA/integrator and related LX-REQ-023 owners retained. Separate sequencer lane remains untouched.

Fetched exact #859 eea971c05ac35e91f7ba9449638221d985446f13 and #860 c97587c8cea40a8f6e534ec13d9690feb555486d. Base c295 already carries #860-shaped question/reset routing and header composition plus subsequent safety/LCD work. This patch keeps the specified base, rather than choosing/replacing either overlapping PR. Latest #860 QA comment verifies 12 #859/#860 render pairs but leaves prior design provenance and formal selection with existing UI/QA/integrator. Historical #807-specific preview waiver is not generalized. No new independent approval is claimed. Existing donor source-review/presentation evidence is reuse provenance, not complete combined-tree qualification.

Next owner/action: parent/existing integrator consumes patch and manifest; existing independent QA checks the combined source and resolves applicable presentation provenance; existing native Build owner measures RAM and qualifies exact Elite/Pro inputs. Original source stage performed no shared ref writes, commits, merges, firmware publication, installs or devices. This follow-up publishes only the patch and nonsecret technical evidence on its isolated evidence branch and links #698.
