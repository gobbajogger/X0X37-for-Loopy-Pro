# F1 source review

Review scope: five independent Compound Engineering personas inspected the
candidate source against verified `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
Correctness, testing, project standards, maintainability and adversarial passes
returned. No different-provider CLI was installed, so adversarial review used
the in-process fallback. No runtime checks or builds were executed.

## Retained findings and source corrections

| Finding | Disposition | Required external verification |
| --- | --- | --- |
| P1: new cell-suite selector exposed the stale linear pitch-bend expectation | Corrected the independent expected mapping; added neutral-64/endpoints, retained linear damper and historical-cell assertions. | `original-sound-cell`, native/soft arithmetic and normal/sanitized variants. |
| P1: compact arena relocation broke the existing idle-note witness and historical source compilation | Updated the fixture accessor; reference builder adapts exactly two history pointers and records the count. PCM/state comparisons remain intact. | `run_chicago_idle_then_note.py` versus pinned base; normal/sanitized transcripts. |
| P2: symmetric-only stereo assertion could miss mono collapse | Added independent asymmetric L-R expectations for each restored insert, part isolation and invalid-edit refusal. | `chicago-fx2-inserts` normal/sanitized variants. |

The author verified each finding from its quoted source and applied the bounded
corrections after the review wave. No second runtime or independent final-tree
PASS is claimed. The archive's C11 assertion was also replaced by an equivalent
strict-C99 compile-time arena bound; no bound or check was removed.

## Coverage and verdict

Source review found no other retained defects. This is a draft source candidate,
**not ready for merge**: full host execution, Elite/Pro SDK V1.2.1 builds with
`X0X_DEV_PADCAL=0`, final RAM/pool receipts, original/reference fidelity,
click-free switching, real 240x240 framebuffer BEFORE/AFTER PNGs and <=20% device
idle CPU remain pending. Owner approval is required before merge.

Per-row checklists, exact port ledger, external commands and proposed capture
paths are in [the delivery record](f1-chicago-miami.md). No device, main,
`.github/` or Actions write is included.

## Owner picture-first UI follow-up

An independent design review checked the bounded FX schematics and page/arrow
cue before implementation. Four subsequent read-only CE reviews (correctness,
testing, project standards, adversarial) inspected the follow-up diff against
`1b06e3ee364b4bafdcf5ae885b17847007baf39b` and returned no actionable findings.
The adversarial pass used the in-process fallback; no different-provider CLI
route was installed. Existing FX/preset names and all earlier checks remain.

Added checks cover twelve distinct bounded FX pictures, the bypass line, the
page-arrow cue bounds and unchanged sound cells. These are NOT_RUN. No builds,
tests or framebuffer captures ran in Codex. Integrated legibility/recognition
still requires actual 240x240 BEFORE/AFTER from the authorized runner and owner
approval; the PR remains draft and not ready for merge.
