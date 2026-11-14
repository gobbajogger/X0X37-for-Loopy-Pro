# Firmware source and architecture reference

This appendix supports [the SMK manual](../SMK37.md). It is a source-reading map,
not a release certificate or a replacement behavior specification.

## Exact inputs and evidence meanings

- Manual branch parent: `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`.
- Inspected behavior: `wip/one-build@48e92b76c2f476cf40771e00a04e1838136bb55d`
  (PR #698 head observed 2026-11-06). All candidate source links below use this pin.
- Coordination read: `4a35a6cdaa0f112db6b3f577542138afd170c20d`.
- GitHub API tarballs were checked against complete recursive Git trees: all
  4,935 candidate and 4,021 documentation-base blobs matched their Git identities,
  including symbolic links. No network Git was used.
- **Source-verified**: inspected implementation, not execution on a keyboard.
- **Host PASS**: only the specified host/extracted-function test and its inputs.
- **Planned/separate candidate**: a request, mockup or another PR; not included merely
  because a title says “fix” or a historical receipt passed.
- **Hardware NOT_RUN**: physical sound, timing, LCD/LEDs, MIDI/USB/Bluetooth, persistence,
  installer and recovery were not exercised by this documentation task.

The mutable PR body includes receipts from several different heads. A receipt from
one of them is not proof of the current combined firmware. The coordinator also
reported unselected combined tree `2e38c67d3dbf1b905085580229e395433aefd92d`, including
panic work; this manual does not replace its explicit inspected commit with that
tree. Reconcile against the selected source before merging or shipping a manual.

## Trace a control before changing its explanation

Paths in this table refer to the pinned candidate. Symbols make the reference
usable even if line numbers move in a later revision.

| Manual topic | Source and test to inspect | Scope or caution |
| --- | --- | --- |
| Printed controls | [panel.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/panel.c), `PANEL_DEFAULT`; [smk_input.h](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/hal/smk_input.h), `SMK_BTNC`, `SMK_PAD_OF_WHITE` | Default mapping; saved calibration can change it. |
| Modes, help, confirmation, sequencing and save | [ui.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/ui.c), `button_tap`, `key_event`, `save_project`, `ui_live_key_owner`, `division_pad_owner` | Trace overlay priority before asserting a button works everywhere. |
| Physical faders, wheels and clip keys | [main_fm1.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/main_fm1.c), `smk_faders`, `smk_clip_mode_poll`, `smk_ck_poll` | SMK code lives in a historically named file; the name does not mean FM-1 instructions apply. |
| Chicago/Miami and compact effects | [original_sound.h](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/original_sound.h), [original_sound.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/original_sound.c) | 32-byte patch, schema 2, 55 admitted Chicago programs and seven effect choices. Performance inputs are not ordinary saved edits. |
| SMC pickup, navigation, lock and effects | [smc_mixer.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/smc_mixer.c), `smc_lock_button`, `smc_lock_ready`, `smc_button_apply`, `smc_fx_publish` | Two application contexts; 900 ms unlock; role/preset and matching controller matter. |
| Project format and save safety | [project.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/project.c), `PROJ_STREAM`, `stream_known`, `read_project`; [x0x.h](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/x0x.h) | Revision 13 in original profile; do not adopt the upstream migration paragraph. |
| BREAK, synth dispatch, effects and output | [engine.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/app/engine.c), [fxbus.c](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/firmware/src/dsp/fxbus.c) | Follow engine dispatch and build flags; do not substitute legacy DSP for Chicago/Miami. |
| Intended behavior and history | [BEHAVIOR.md](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/BEHAVIOR.md), [ROADMAP.md](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/48e92b76c2f476cf40771e00a04e1838136bb55d/docs/ROADMAP.md) | Intent/history can conflict with later source; dated device passes apply only to their original bytes/model. |

## Architecture and documented reasons

Input begins in the SMK HAL. It scans the keybed, pads, encoders and analog controls
and carries events into foreground UI/control routing. The UI decides whether an
input belongs to a list, confirmation, step grid, live synth, clip editor or SMC
context. The audio engine and sequencer consume musical state; display work is a
separate concern. Follow that complete route when an apparently simple button has
different behavior on two pages.

The synth slots retain the historical 303A/303B identifiers while original-engine
dispatch selects Chicago or Miami. `original_sound.h` documents why patch storage
keeps an edit mask: untouched controls retain immutable factory binary32 values
rather than being reconstructed from rounded seven-bit display values. Pitch bend
and damper remain transient performance state. That is a documented representation
choice, not an assertion of newly measured bit-exact audio.

`project.c` explains the storage tradeoff. Settings and sound are separate objects;
patterns, arrangement and motion form a compressed stream across four objects.
The header includes length and CRC so a torn/unpackable stream cannot be treated as
valid musical data. The save measures size before writing and avoids rewriting
unchanged objects. This reduces unnecessary work but does not make all objects an
atomic transaction. The UI defers saves because flash activity can interrupt sound.
A successful source-level format check cannot prove real power-loss recovery.

SMC input keeps peer-specific queues, targets, held state and pickup. The target is
captured so a release after navigation belongs to the original press, not whichever
part is now visible. Pickup prevents a newly assigned fader from jumping the value.
Target lock separates the SMC's captured controls from keyboard navigation; the
brief overlay/peek provides feedback. These are the documented operational reasons
for behavior that can otherwise look like an unresponsive control.

The Bluetooth application and controller are separate inputs to qualification.
Application peer count does not certify radio scheduling or matching controller
capacity. Likewise, host audio, UI renders and source inspection answer different
questions from a native build, an installer check or an exact-model device test.
The existing integration/publication owners retain those decisions.

## Reconciled differences and open work

| Older text or separate work | What this manual says for the pin |
| --- | --- |
| SMK summary: outer keys do nothing; keys have no velocity | Dedicated full-key Free Play route exists, with strict guards. Do not generalize it to every view or to all pad behavior. |
| SMK summary: fader takes over on any movement; old part order | Source uses pickup. `PART_ORDER` is 909, 808, BREAK, 303B, 303A; keyboard and SMC strips follow it. |
| FM-1 manual: old projects migrate | Original profile loads revision 13 only. |
| FM-1/manual install links and “all models same hardware” | Not installation authority; require qualified matching model/package/controller and backups. |
| BEHAVIOR SMC-09 says Pause toggles and describes older feedback | Later SMC-11A and actual handler require a separate 900 ms unlock hold. Physical lamp identity remains unverified. |
| Old SMC guide: four buses outside drum pages, Up/Down changes view | FX view owns all eight SELECT effects; main buses have 180 ms tap/hold distinction. Up/Down changes encoder bank. |
| SMC hold code/comment still refers to low-pass | `smc_fx_publish` sends assigned effect; caption state is stale. Trace the value delivered to the engine, not just the comment. |
| Old SMK summary: K4 always changes patterns | Synth SOUND page overrides K4 for factory-preset browsing. |
| #768 DAW-only; #773 shape pads; #765 all keys through HELP | Separate PR work does not override DAW+USER, rate-pad and overlay guards read at this pin. |
| M1 panic #839/#853; newer combined tree reported by coordinator | Candidate-only. No global panic route found in inspected UI/SMC/engine. |
| #855 longest-pattern chains; #860 question transport/sound controls | Separate candidate changes; recheck if selected for integration. |
| Restored Chicago inserts in #764/#798 | Not the compact seven-choice policy inspected here. |
| ROADMAP “Where we are now (2026-10-19)” and old installed hashes | Historical state, not current installation or qualification. ROADMAP is unchanged by this manual task. |

## Executed verification and remaining acceptance

On the exact API-verified candidate archive, executed on 2026-11-06:

| Check | Result | What it proves |
| --- | --- | --- |
| Git blob comparison, both archives including symlinks | PASS, 4,935 / 4,021 blobs | Read source matches the two immutable inputs. |
| `python3 tests/host/test_freeplay_rate_pad_port.py` | PASS | Compiles production handler excerpts with host fakes; enabled eight rates and opt-out behavior. No firmware link or hardware proof. |
| `python3 tests/host/smc_fx_preset_preserve_test.py` | PASS, 502 assertions | Compiled actual FX handler excerpts against two-peer host fakes; assigned-effect publication, not caption or hardware proof. |
| `python3 tests/pad_placement_doc_test.py` | PASS | Existing PAD_MAP tables agree with source placement/mux/LED tables. No new physical orientation test. |

Source inspection also checked default panel mapping, full-key guards, SMC preset
choices/lock/peer count, compact FX choices, revision-13 admission, and absence of
the later global panic route. Documentation links, generated HTML parity and
independent review are recorded in the [delivery receipt](engineering-delivery/firmware-manual-20261106.md).

Full host suite, native Elite/Pro/controller builds, golden audio, performance,
physical UI/SMC/Loopy behavior, save/import/interruption and installation are
**NOT_RUN for this documentation task**. Firmware inputs are unchanged; those checks
are not a condition for a docs-only PR, and their absence prevents firmware claims.
Before publishing a versioned end-user manual, the integrator must name the selected
firmware commit, reconcile these differences, and attach that version's actual
model-specific qualification. Do not turn this appendix into a release checklist
that silently grants installation or publication authority.
