# Active song workflows: bounded source-stage handoff

Ledger stage only. Parent coordination owns implementation assignment. No firmware
or website edits, project operations, merges, publication or hardware use occurred.

## Inputs and evidence limits

- Ledger predecessor: `69a25f7b7da0e9d88bfe4b3833603cb48a6a231d`,
  `coordination/merge-queue`; retain all 36 earlier requests, histories and queue files.
- Firmware NIGHTLY inspected: `08d3164a265123cb4c84e5104642fd93257d63d7`.
- Website [PR19](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/19):
  `570253d35c84748bba82ddd616b9c17af8d7da58`, open draft, reviewed source,
  unpublished. This is an inspection base, not authority to merge or publish it.
- [PR171](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/171):
  `57beefa7aae0a90e8e2fb57d2410f0bd2b85e8e3`, open draft. Its narrow repair
  rejects malformed/incomplete legacy arrangement Load. Its existing-feature
  reconciliation does not establish complete song UI or current combined delivery.
- Workflow adapter pin: `e2482a5039527195e346925c751ac3e3a333ada9`,
  contract SHA256 `2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.
  Live rules were read; current bounded instructions control this documentation stage.

## LX-REQ-037: installer song import/export — active

Reuse firmware repository `web/x0x_projects.html`, `web/x0x_projects.js`,
`docs/PROJECT_FILES.md`, `tools/make_pages.py` and `firmware/src/app/editor.c`.
The companion already implements Save, Load, Reset and `.x0xproject` export/import;
editor protocol ops 64–74 provide query/export/read/import/write/commit/abort/
save/load/reset/validate. The firmware site generator already emits `projects/`.

Portable export contains the whole project (including song, patterns, sounds,
motion and settings) plus original sample slot images. Project-only mode retains
sample identities and refuses mismatches without sample writes. Preserve format,
CRC/semantic validation, occupied-slot confirmation, downloaded verified recovery
backup, stale-preview rejection, exact readback, rollback and recovery hold. Do
not call a sample-only archive a song/project export.

Exact website gap at PR19: `web/` has installer and sample tools, but no Projects
HTML/JS; `web/x0x_installer.html` and generated `docs/install/index.html` expose
firmware/sample navigation with an embedded sample workspace, but no Projects
entry. `scripts/build-preview.mjs` currently emits only installer, breaks and catalog
outputs. Website generation must include the reused page/modules and route; merely
adding a link to a missing page is insufficient. Adapt the existing workspace
and connection ownership so switching between firmware/sample/project tasks
cannot leave competing MIDI sessions or active transfers. Keep GPL notices.

Next bounded owner: parent assigns one installer source owner. Add discoverable
access to reused Projects UI, with explicit song/project and sample-policy labels;
show a reviewable preview. Current protocol has no established arrangement-only
export/import command. Do not silently reinterpret full-project replacement as
arrangement-only, or invent a duplicate file format. If arrangement-only transfer
is needed, return a narrowly scoped semantic proposal before expanding protocol.

Acceptance: synthetic portable/project-only round trips and sample-byte invariants;
malformed/version/CRC refusal; stale preview, busy/disconnect/recovery negatives;
route/assets/navigation checks; session exclusivity and retained previews; website
native build/tests plus affected companion tests (`web/test_projects.mjs`,
`web/test_projects_page.mjs`). Independent exact-head review follows implementation.
No actual owner project is used. Host tests do not establish device/deployed proof.

## LX-REQ-001: sample-preserving song reset — active existing request

Retain [issue #2](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/issues/2)
and existing LX-REQ-001 history; do not create a competing reset request.
Source inspection establishes two distinct existing operations:

- Arrangement only: SONG list exposes `CLEAR SONG`; confirmation is `CLEAR THE
  SONG?`. `run_action` / `ACT_SONG_CLR` sets `song()->len = 0`, resets selection,
  marks dirty and reports `SONG CLEARED`. That branch does not reset patterns,
  sounds, settings or sample slots. Stored bar bytes are not scrubbed by this branch.
- Whole project: `ui_project_reset` acquires the project, stages defaults and
  adopts them. BEHAVIOR SET-07/08 specifies all sequencers, sounds, song, motion
  and project settings reset, uploaded samples and device role settings retained.
  It stops playback/recording and restores autosave ON; quiet autosave can overwrite
  the saved project. This replaces historical FACTORY RESET terminology and is
  not authorization for sample erase or a destructive device factory reset.

Projects companion `device.reset(true)` sends confirmed op 73, which calls
`ui_project_reset`; it does not invoke arrangement-only CLEAR SONG. Thus a browser
button labeled only “Reset song” would conceal a broader destructive project edit.

Next bounded owner: parent assigns project lifecycle/source owner to retain and
verify these implementations, specify the intended exposed scope accurately, and
identify only demonstrated gaps. Browser arrangement-only clear is not established
by the existing op 73. Reuse the device operation where appropriate; any new
browser arrangement operation needs a scoped protocol/UI proposal and lifecycle
checks rather than replacing the existing reset system.

Acceptance: confirmation/cancel, stopped/playing/recording/external-clock and
motion/undo/save/reboot behavior, with synthetic sample hashes and protected
storage unchanged; prove arrangement-only clear preserves other project content.
Reuse `tests/host/song_actions_test.*`, project runtime/stage/protocol suites and
maintained BEHAVIOR SET-06–13, then independent combined-root checks. Refresh the
source pin with the sole firmware integrator; current combined firmware and
hardware flash workers keep ownership of their stages. PR171 is a separate Load
gap and remains subject to its own integration/validation handoff.

## LX-REQ-038: community songs — owner-deferred idea only

Possible library, sharing and community import is remembered for later thinking.
Status is `OWNER_DEFERRED_IDEA_NOT_INTAKE_READY`, `intake_eligible: false`,
`implementation_authorized: false`. It is not a prerequisite for either active
request. No backend, accounts, upload flow, moderation, external issue or automatic
implementation dispatch is authorized. Only explicit owner reopening may change
that boundary.

## Stop and next owner

This stage finishes at independently reviewed public-safe ledger docs, existing
continuity-validator PASS and normal non-main push/readback. Parent receives the
IDs and source-stage packet, then chooses implementation assignments. No new
implementation worker or successor is launched by this ledger stage.
