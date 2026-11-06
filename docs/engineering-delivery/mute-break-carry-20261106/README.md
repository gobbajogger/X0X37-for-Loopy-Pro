# Mute/BREAK source handoff

Evidence-only branch: `chat/mute-break-carry-evidence-20261106`. Integration remains with the existing #698 owner. This directory contains text evidence and a source patch, not installed source or firmware.

Read REPORT.md for exact donors, candidate tree, producer test receipts, changed M7a expectations and all unrun gates. Source candidate independent QA is still pending. No claim that parent wording “reviewed patch” supplies an independent review receipt.

Apply carry-forward.patch only to c295af1554f9e80a29864cd013995eccad66cd2c with `git apply --index`; the resulting `git write-tree` must be 3b21f19e1b8dc6d4fa9000c76c368a23379a23d1. This evidence commit's tree is different: it only adds this directory to the base. Do not cherry-pick this evidence commit expecting the patch to be applied.

Shared-path warning: serialize engine.c helper/mixer/render-loop edits and tests/run_tests.sh registration with their existing writers. No UI edits. M7a changes two ON+muted internal-sounding expectations from zero to positive, while MIDI remains zero; OFF assertions remain unchanged. See original failed and adapted passing M7a logs.

Public payload review: only the listed text logs, report, patch and manifest; no firmware, private headers, SDK/toolchains, captures or credentials. Local compiler warnings and original failed tests remain in the logs. MANIFEST.json hashes every other file. The patch bytes are unchanged from the producer handoff.
