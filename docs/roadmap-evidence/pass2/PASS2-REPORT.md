# PASS2 report

Processed all 146 rows against `ce27812951555ff7ed9694bf99d20a65dd5f410b`, tree `a917fb6a2c738bb410601bd784278bf35c142f1c`. ROADMAP commit `5f578fc0642a1200a5163802bcd69277c511bfc7`, blob `a204a54c989d6f323d619e70f799427603756541` unchanged.

PRESENT **2**; UNPROVEN **144**. #66 and #68 now have a quoted, explicitly named green test at the pinned SHA. #80 remains UNPROVEN with a known named-test failure. All 132 coverage-search rows remain UNPROVEN.

| Verdict/category | Rows |
|---|---:|
| PRESENT | 2 |
| UNPROVEN / NEEDS_NAMED_TEST | 132 |
| UNPROVEN / DEVICE_BLOCKED | 7 |
| UNPROVEN / TEST_FAILS | 1 |
| UNPROVEN / NOT_RUN | 0 |
| UNPROVEN / UNCLEAR | 4 |

| Coverage among the original 132 NEEDS_NAMED_TEST rows | Rows |
|---|---:|
| COVERED_CANDIDATE | 64 |
| NOT_COVERED | 68 |

## Seven original UNCLEAR rows

| Row | Test | Status/category | Exact receipt quote or reason |
|---|---|---|---|
| #63 | ui-page-wrap | UNPROVEN / UNCLEAR | No explicit ok for ui-page-wrap / ui_page_wrap_test.py at ce278129 in the searched receipts; aggregate totals and absence from FAIL lists are insufficient. |
| #66 | sound-matrix | PRESENT / PRESENT | /workspace/qa-receipt-m7a-final-ce278129.md:83: **Became ok versus recorded gcc `43de100c` (9):** `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `builtin-break-output`, `sound-matrix`. On this gcc run, `ui-theme` logged `PASS ui_theme checks=14865102` and `smc-boot-saved-roles` is `ok`. Comment 6657404493 says those two still FAIL; that run’s other names look like clang. My clang suite was not finished, and its partial log already has `ui-theme` FAIL, so I do not override that clang result. |
| #68 | sound-matrix | PRESENT / PRESENT | /workspace/qa-receipt-m7a-final-ce278129.md:83: **Became ok versus recorded gcc `43de100c` (9):** `original-build`, `ui-theme`, `smc-select-mode`, `smc-solo-overflow`, `smc-boot-saved-roles`, `smc-multi-peer`, `ui-peer-names`, `builtin-break-output`, `sound-matrix`. On this gcc run, `ui-theme` logged `PASS ui_theme checks=14865102` and `smc-boot-saved-roles` is `ok`. Comment 6657404493 says those two still FAIL; that run’s other names look like clang. My clang suite was not finished, and its partial log already has `ui-theme` FAIL, so I do not override that clang result. |
| #94 | offline-sequencer | UNPROVEN / UNCLEAR | No explicit ok for offline-sequencer at ce278129. QA2 receipt m7a-final:73 names offline-sequencer-browser and offline-touchosc-lua as ok; neither is the offline-sequencer entry named by this row. |
| DONE-017 | scenario-blenotes | UNPROVEN / UNCLEAR | No explicit ok for scenario-blenotes / blenotes.x0x at ce278129 in the searched receipts, and no cited identical-tree/test-blob carry-forward for that test. |
| #109 | tests/host/preflight_full_suite.py | UNPROVEN / UNCLEAR | No explicit ok for tests/host/preflight_full_suite.py at ce278129. It is conditional on X0X_STRICT_PREFLIGHT=1 (tests/run_tests.sh:51-58), outside a named run entry; full-suite totals do not prove it ran. |
| #80 | web/test_install_local_file.mjs | UNPROVEN / TEST_FAILS | /workspace/qa-receipt-m7a-final-ce278129.md:113: `timeout 180 node web/test_*.mjs` exits 0 and only executes the first expanded file. `timeout 180 node --test web/test_*.mjs` on both trees: exit 1, **101 pass / 2 fail**. Failures: `web/test_install_local_file.mjs` (`t is not defined` at `selectStockFile`, test file line 47) and `web/test_workspace_browser.mjs` (playwright missing). No new name versus `43de100c`. |

## Receipt and coverage method

Searched 32 existing QA/QA2/Build/candidate/integrator receipts. The receipt manifest and hashes are in `PASS2-AUDIT.json`; exact matches are in `PASS2-RECEIPT-SEARCH.txt`. QA final receipt lines 11–12 identify ce278129/a917fb6a; line 72 identifies its gcc run; line 83 explicitly says sound-matrix became ok. QA2 m7a-final:82 and Build ce278129-ce278129:16 corroborate it. No pass is inferred by complementing a FAIL list or from aggregate totals. Different-SHA results were not admitted.

Each of the 132 rows now has `pass2_coverage`: extracted identifiers and their row citations, up to three runner-entry candidates, literal test file:line quotes, and individual recorded-result state. The rg batch searches `tree/tests` and runner-mapped web test sources; source paths and firmware identifier hits are also retained. Wrapper mappings reuse pass 1, with checked literal controls/storage construction in run_break_features.py:36-40; the chord-display extraction-only call does not count as running the knob fixture. The search excludes generic reconciliation prose, unrelated uses of feature words, mere imports/includes, stub definitions, docstrings and output-only lines. Hand-reviewed incidental matches are rejected in pass2.py.

`COVERED_CANDIDATE` is a source lead for later binding, not full-feature proof. It does not change status or add a test name to the pinned row. `NOT_COVERED` means this conservative literal/source-to-runner search did not admit a candidate; it does not prove no test could cover the feature. Unmapped tests, indirect calls, renamed identifiers and broader semantic coverage can escape a cheap search.

Candidate recorded_results use only the individual ce278129 Dev run lines already parsed in pass 1: 17 ok, 19 FAIL, 6 NOT_RUN. Missing individual names are `unknown`, never inferred green. These states retain their Dev-run attribution; the QA gcc sound-matrix receipt is separate evidence for step 1. No suite entry, build, network action or Git write was executed.

SCHEMA.md and both checker scripts are unchanged: the checker already accepts optional fields. PASS1-REPORT.md remains the historical first-pass report. This PASS2 report supersedes its current counts.

## All 132 coverage rows

| Row | Coverage | Candidates and recorded Dev states | Evidence or reason |
|---|---|---|---|
| #1 | COVERED_CANDIDATE | control-recompile (unknown), original-stale-release (FAIL), ui-glyph-polish (FAIL) | tests/host/control_recompile_test.c:71: CHECK(original_runtime_init(&part,cell,120,&tuning,&owner)==ORIGINAL_RUNTIME_OK);; tests/host/test_original_runtime_lost_off.py:76: CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 100) == ORIGINAL_RUNTIME_OK);; tests/host/ui_glyph_polish_test.c:47: CHECK(original_runtime_init(&live,cell,125,&tuning,&owner)==1);CHECK(original_runtime_key(&live,ORIGINAL_INPUT_KEYS,48,120)==1); |
| #10 | COVERED_CANDIDATE | chicago-presentation (unknown), chicago-readout (unknown), original-release-integration (FAIL) | tests/host/chicago_presentation_test.c:64: CHECK(engine_bass_preset_pending(v)==6);; tests/host/chicago_readout_test.c:59: CHECK(engine_bass_choice_serial()==serial && engine_bass_preset_pending(v)==pending);; tests/host/original_release_integration_test.c:29: CHECK(engine_bass_preset_pending(v) == 0); |
| #100 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/model.test.mjs:13: test('atomic move/resize collisions, finitebounds, stale plan and oneUndo preserve otherparts',()=>{const s=new DraftStore();s.edit(d=>putNote(d,0,0,{start:2,pitch:60,slots:3,velocity:95}));s.edit(d=>putNote(d,0,0,{start:8,pitch:65,slots:2,velocity:90}));const before=copy(s.doc),other=copy(s.doc.patterns[0].bass[1]),rev=s.revision;assert.throws(()=>s.edit(d=>putNote(d,0,0,{start:7,pitch:64,slots:3,velocity:100},2)),/overlap/);assert.deepEqual(s.doc,before);assert.equal(s.revision,rev);assert.throws(()=>s.edit(d=>durationEdit(d,0,0,2,8,0)),/overlap/);s.edit(d=>putNote(d,0,0,{start:3,pitch:62,slots:4,velocity:110},2));assert.deepEqual(s.doc.patterns[0].bass[1],other);assert.equal(notesOf(getPart(s.doc,0,0))[0].slots,4);s.undo();assert.deepEqual(s.doc,before);const plan=noteEditPlan(s,0,0,{start:20,pitch:70,slots:1,velocity:100});s.edit(d=>d.patterns[1].bass[1].length=32);assert.throws(()=>applyPlan(s,plan),/changed/);assert.throws(()=>putNote(copy(s.doc),0,0,{start:63,pitch:60,slots:2,velocity:100}),/bounds/);}); |
| #102 | COVERED_CANDIDATE | offline-sequencer-browser (FAIL) | web/offline-sequencer/f6b-browser.test.py:77: vp.dispatch_event('pointercancel', {'pointerId': 1}); page.mouse.up() |
| #103 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #108 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #11 | COVERED_CANDIDATE | bass-prob-midi (unknown), seq-random-tie-preview (unknown), seq-long-notes (unknown) | tests/host/bass_probability_midi_test.c:147: CHECK(seq.bass_out_valid[0], "recorded live MIDI note remains tracked for its eventual NoteOff");; tests/host/seq_random_tie_lookahead_test.c:84: EXPECT(m_off==0,"selected TIE does not send premature external MIDI NoteOff");; tests/host/seq_long_note_test.c:142: CHECK(at(ON, 0, slide ? 43 : 36, 8820, slide)); |
| #114 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #115 | COVERED_CANDIDATE | remaining-feature-proof-selftest (unknown) | tests/host/remaining_feature_proof_test.py:11: TOOL = Path(__file__).resolve().parents[2] / 'tools/remaining_feature_proof.py' |
| #116 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #118 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #12 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #121 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #13 | COVERED_CANDIDATE | bass-prob-midi (unknown) | tests/host/bass_probability_midi_test.c:83: CHECK(on_count == 1, "probability callback reached for first step"); |
| #14 | COVERED_CANDIDATE | seq-random-tie-preview (unknown) | tests/host/seq_random_tie_lookahead_test.c:65: EXPECT(seq.t[TRK_BASS0].rng==2u,"RANDOM preview does not draw RNG"); |
| #17 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #18 | COVERED_CANDIDATE | sequencer (unknown), smc-lock-beat (unknown), smc-remainder (unknown) | tests/host/seq_test.c:854: CHECK(seq_midi_rt(&S, 0xF8) == 1 && seq_midi_rt(&S, 0xFA) == 1 && seq_midi_rt(&S, 0xFB) == 1 &&; tests/host/smc_lock_beat_test.c:85: CHECK(seq_clk_take(&seq, 0xF8, 0)); CHECK(seq_midi_rt(&seq, 0xF8));; tests/host/smc_remainder_test.c:186: seq_midi_rt(&seq,0xf8); render_audio(1); CHECK(seq.metro_view==1); |
| #19 | COVERED_CANDIDATE | sequencer (unknown), smc-lock-beat (unknown), smc-remainder (unknown) | tests/host/seq_test.c:854: CHECK(seq_midi_rt(&S, 0xF8) == 1 && seq_midi_rt(&S, 0xFA) == 1 && seq_midi_rt(&S, 0xFB) == 1 &&; tests/host/smc_lock_beat_test.c:85: CHECK(seq_clk_take(&seq, 0xF8, 0)); CHECK(seq_midi_rt(&seq, 0xF8));; tests/host/smc_remainder_test.c:186: seq_midi_rt(&seq,0xf8); render_audio(1); CHECK(seq.metro_view==1); |
| #20 | COVERED_CANDIDATE | smc-feedback (unknown) | tests/host/smc_feedback_test.py:94: "smc_home_target", "smc_drum_mix_context", "smc_drum_mix", "smc_drum_track", "smc_lock_blocks", "smc_beat_lit", "smc_led", "smc_frame_peer", "smc_frame")) |
| #21 | COVERED_CANDIDATE | freeplay-rate-pads (FAIL), original-build (unknown) | tests/host/test_freeplay_rate_pad_port.py:117: assert(division_pad_owner()==1);; tests/host/test_original_build_composition.py:144: self.assertIn("static int division_pad_owner(void)", ui) |
| #22 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #23 | COVERED_CANDIDATE | smc-audit-core (unknown), smc-drum-mutes (unknown), smc-lock (unknown) | tests/host/smc_audit_core_test.c:34: CHECK(smc_target(0).a==T_808 && smc_target(0).b==8);; tests/host/smc_drum_mutes_test.c:71: assert(expected.kind == R_ENG && expected.a == (mode == 1 ? T_909 : T_808));; tests/host/smc_lock_test.c:86: CHECK(smc_same(find_ref(kit ? T_808 : T_909, smc.lock_bank, "Level"), smc_target(0))); |
| #24 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #25 | COVERED_CANDIDATE | adaptive-break-controls (unknown) | tests/host/adaptive_break_controls_test.c:138: count_set(32,0);ui.brk_page=3;key_event(0,1);key_event(black(7),1);CHECK(proj.pat[seq.ppat[TRK_BRK]].brk.slice[0]==32); |
| #26 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #27 | COVERED_CANDIDATE | adaptive-break-controls (unknown) | tests/host/adaptive_break_controls_test.c:188: engine_brk_bend(0);audio();CHECK(engine_brk_bend_value()==0); |
| #28 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #30 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #32 | COVERED_CANDIDATE | part-order (unknown), smc-audit-core (unknown), smc-lock-beat (unknown) | tests/host/part_order_test.c:99: pref_t captured=smc_target(2);CHECK(captured.b==PART_BRK && smc_lock_toggle());; tests/host/smc_audit_core_test.c:9: CHECK(!smc_lock_toggle());; tests/host/smc_lock_beat_test.c:115: CHECK(smc_lock_toggle()); |
| #34 | COVERED_CANDIDATE | smc-multi-peer (unknown), smc-mixer-page (unknown), smc-remainder (unknown) | tests/host/smc_multi_peer_test.c:137: smc_ble_link_slot(0,0);smc_poll();CHECK(fx_mask&2);; tests/host/smc_mixer_page_test.c:15: smc_ble_link_slot(1,1);smc_poll();; tests/host/smc_remainder_test.c:319: setup(V_FX,PART_303B); smc_ble_link_slot(1,1); smc_poll(); |
| #35 | COVERED_CANDIDATE | audio-stress (unknown), smc-mode-page (unknown), smc-multi-peer (unknown) | tests/host/audio_stress_test.c:86: CHECK((uint8_t)(smc_inputs[0].enqueued-enqueued)==8);; tests/host/smc_mode_page_test.c:108: CHECK(smc_inputs[0].dropped == 0);; tests/host/smc_multi_peer_test.c:107: CHECK(smc_inputs[0].qr==smc_inputs[0].qw && smc_inputs[1].qr==smc_inputs[1].qw); |
| #36 | COVERED_CANDIDATE | smc-remainder (unknown) | tests/host/smc_remainder_test.c:175: smc_ui_draw_peer(); CHECK(changed_rows(MAIN_Y,MAIN_Y+2*BAND_H,before_fb,fb)>0); |
| #38 | COVERED_CANDIDATE | smc-audit-core (unknown), smc-mixer-page (unknown), smc-remainder (unknown) | tests/host/smc_audit_core_test.c:36: CHECK(smc_drum_track(PART_808,0)==8);; tests/host/smc_mixer_page_test.c:42: CHECK(smc_drum_track(PART_909, 0)==8 && smc_page_now()==1);; tests/host/smc_remainder_test.c:80: CHECK(smc_drum_mix()==kit && smc_drum_track(kit,7)==7); |
| #39 | COVERED_CANDIDATE | part-order (unknown), smc-audit-core (unknown), smc-drum-mutes (unknown) | tests/host/part_order_test.c:82: CHECK(smc_same(smc_knob_target(pos),PR(R_ENG,T_MIX,p,MX_CUT)));; tests/host/smc_audit_core_test.c:35: CHECK(smc_knob_target(0).a==T_808 && smc_knob_target(0).b==8);; tests/host/smc_drum_mutes_test.c:138: assert(smc_knob_target(strip).kind == R_NONE); |
| #4 | COVERED_CANDIDATE | control-recompile (unknown), perf-wheel-motion (unknown), perf-wheel-noise (unknown) | tests/host/control_recompile_test.c:32: CHECK(widths==((id==bend\|\|id==cutoff)?0:4)); \; tests/host/perf_wheel_motion_test.c:36: CHECK(plane(v,bend)<0 && plane(v,mod)<0);            /* not playing: no lane */; tests/host/perf_wheel_noise_test.c:28: CHECK(plane(0,mod)<0);CHECK(plane(0,bend)<0);        /* nothing recorded */ |
| #42 | COVERED_CANDIDATE | smc-multi-peer (unknown), smc-pickup-acceptance (unknown), smc-remainder (unknown) | tests/host/smc_multi_peer_test.c:79: smc_ui_draw(); CHECK(smc_slot==0); CHECK(smc_ui.peer==1);; tests/host/smc_pickup_acceptance_test.c:172: smc_ui_begin(strip, 0); status_is("ACTIVE");; tests/host/smc_remainder_test.c:483: smc.picked=0; pref_set(smc_target(0),0); smc_ui_begin(0,0); |
| #43 | COVERED_CANDIDATE | smc-audit-core (unknown), smc-mixer-page (unknown), smc-pickup-acceptance (unknown) | tests/host/smc_audit_core_test.c:33: smc_page_set(1);CHECK(smc_lock_toggle());smc_page_set(2);; tests/host/smc_mixer_page_test.c:30: smc_page_set(2);CHECK(smc_page_now()==2 && smc_drum_mix()==PART_808);; tests/host/smc_pickup_acceptance_test.c:228: if (!moved) { smc_ui_cancel(); CHECK(!smc_ui_active()); } |
| #44 | COVERED_CANDIDATE | smc-mixer-edges (unknown) | tests/host/smc_mixer_edges_test.c:16: CHECK(smc_pattern_delta(p, -1, 1, 32) == (p == 31 ? -2 : p + 1)); |
| #47 | COVERED_CANDIDATE | original-release-integration (FAIL), part-off (ok), project-stage (unknown) | tests/host/original_release_integration_test.c:168: CHECK(project_save()==0 && project_load_stage()==0);; tests/host/part_off_test.c:446: CHECK(project_save() == -9 && store_writes == w);; tests/host/project_stage_test.c:309: CHECK(project_save() == 0); |
| #48 | COVERED_CANDIDATE | adaptive-break-storage (unknown), legacy-303-project (unknown), part-off (ok) | tests/host/adaptive_break_storage_test.c:38: project_file_header(h);CHECK(get32(h+20)==PROJ_STREAM);CHECK(project_file_validate(h)==0);; tests/host/legacy_303_project_test.c:33: CHECK(project_file_validate(h)!=0);; tests/host/part_off_test.c:429: CHECK(project_load() == 0); |
| #49 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #5 | COVERED_CANDIDATE | chicago-fx-bridge (unknown) | tests/host/chicago_fx_bridge_test.c:24: CHECK(CHI_FX_NTYPES == 12, "12 Chicago types"); |
| #51 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #53 | NOT_COVERED | — | watchDeviceSession appears only as a mocked no-op in projects-page; that fixture does not exercise stale-session rejection. |
| #57 | COVERED_CANDIDATE | home-pattern-glyph (unknown) | tests/host/home_pattern_glyph_test.c:22: home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a); |
| #58 | COVERED_CANDIDATE | home-pattern-glyph (unknown) | tests/host/home_pattern_glyph_test.c:57: b->step[16].flags = G_NOTE; b->step[16].velocity = 120; |
| #6 | COVERED_CANDIDATE | original-release-integration (FAIL), original-sound-cell (FAIL) | tests/host/original_release_integration_test.c:144: CHECK(engine_bass_preset_index(1)==54 && !strcmp(original_sound_name(0,54),"Strome FX"));; tests/host/original_sound_cell_test.c:29: assert(original_sound_name(kind,factory) && original_sound_engine_id(kind)); |
| #60 | COVERED_CANDIDATE | ui-waveform-truth (unknown) | tests/host/ui_waveform_truth_test.c:19: assert(ui_miami_wave_value(1,1,.05f)>.99f); |
| #61 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #62 | COVERED_CANDIDATE | matrix-knob-caption (unknown), ui-theme (FAIL), chicago-presentation (unknown) | tests/host/matrix_knob_caption_test.c:40: CHECK(7+tw(&FONT_T,UI_CAP_HOME ": CANCEL")+8+nav<=234);; tests/host/ui_theme_test.c:69: CHECK(UI_FIRST_KNOB==5 && !strcmp(UI_CAP_EDIT,"PARA"));; tests/host/chicago_presentation_test.c:77: cv_text(x,y,&FONT_T,UI_CAP_ARROWS,c);x+=tw(&FONT_T,UI_CAP_ARROWS)+4; |
| #64 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #65 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #69 | COVERED_CANDIDATE | chicago-presentation (unknown), chicago-readout (unknown), ext-midi-rec-smk (unknown) | tests/host/chicago_presentation_test.c:146: CHECK(!engine_bass_preset_pending(v) && ui.view==V_PART && cur_page()==page);; tests/host/chicago_readout_test.c:94: CHECK(!engine_bass_preset_pending(v) && ui.view==V_PART);; tests/host/ext_midi_rec_test.c:117: CHECK(ui.rec && seq.playing && ui.view == V_SEQ, "%s: REC on, playing, SEQ", name); |
| #7 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #70 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #71 | COVERED_CANDIDATE | boot-screen (unknown), ui-theme (FAIL) | tests/boot_screen_test.c:218: check("factory words are DnBOX and the engine line", !strcmp(l0, "DnBOX") && !strcmp(l1, BOOT_FACTORY_1));; tests/host/ui_theme_test.c:70: CHECK(!strcmp(caps_of("About DnBOX"),"ABOUT DnBOX")); |
| #72 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #74 | COVERED_CANDIDATE | smk-pad-inputs (unknown), padcal-flag (unknown), smk-padcal-wiring (unknown) | tests/smk_pad_inputs_test.py:39: assert 'arg == 6u' in function(main, 'smk_inlog_reply'); tests/padcal_flag_test.py:83: body = rel[rel.index("static void smk_inlog_reply(uint32_t arg)"):]; tests/smk_padcal_wiring_test.py:79: rp = body(main, "static void smk_inlog_reply(uint32_t arg)") |
| #75 | COVERED_CANDIDATE | installer-project-handoff (unknown) | web/test_installer_project_handoff.mjs:12: assert.equal(links.length, 1, 'installer must have one translated project/song route'); |
| #76 | COVERED_CANDIDATE | part-off-web-e2e (ok), part-off-web-e2e-node-gate (ok), projects-web (unknown) | tests/host/part_off_web_e2e.mjs:17: const parse = b => { try { return { ok: true, r: parseProject(b) }; } catch (e) { return { ok: false, code: e.code, msg: e.message }; } };; tests/host/part_off_web_e2e.mjs:17: const parse = b => { try { return { ok: true, r: parseProject(b) }; } catch (e) { return { ok: false, code: e.code, msg: e.message }; } };; web/test_projects.mjs:197: assert.throws(() => parseProject(raw), /incompatible/); |
| #77 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #78 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #79 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #81 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #82 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #84 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #85 | COVERED_CANDIDATE | installer-backup-snapshot (unknown), installer-slots (unknown), installer-backup-selection (unknown) | tests/fm1_install_backup_snapshot_test.py:85: assert raw == b'verified-synthetic-backup'; tests/install_slots_test.py:167: ok("x0x_slots.py restore before.x0xslots" in err and "not written back" in err,; web/test_install_backup_selection.mjs:21: archiveStamp: () => ({created:'test', name:'backup.x0xslots'}), |
| #87 | COVERED_CANDIDATE | breaks-web (unknown) | web/test_breaks.mjs:80: ? userSlotRaw(`BR${i + 1}`, sl.map((l) => ({ adpcm: imaEncode(l.s), n: l.s.length, rate: l.playback })), c.kb[i] * 1024) |
| #89 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #9 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #90 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/model.test.mjs:14: test('sound drafts/engine ordering/Undo strict scope and singleMiami cap',()=>{const s=new DraftStore();s.edit(d=>d.engines=['Chicago','Miami']);assert.throws(()=>s.edit(d=>d.engines=['Miami','Miami']),/One Miami/);const b=copy(s.doc);s.edit(d=>d.sound['synth1/Miami/9']=90);assert.deepEqual(s.doc.patterns,b.patterns);s.undo();assert.deepEqual(s.doc,b);}); |
| #91 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #92 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/project-adapter.test.mjs:56: test('BREAK gates, pinned slices and settings retain unrelated pattern swing',()=>{ |
| #93 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| #95 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/device-sync.test.mjs:69: test('accepted file edits produce a project export and a single replacement Undo',()=>{ |
| #96 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/model.test.mjs:15: test('seeded local New/Mutate preview is deterministic, bounded and isolated; no preview mutation',()=>{for(let part=0;part<2;part++)for(let seed of [0,1,48879,0xffffffff]){const s=new DraftStore(),before=copy(s.doc);const p=generatePlan(s,0,part,'new',{seed});assert.deepEqual(p,generatePlan(s,0,part,'new',{seed}));assert.deepEqual(s.doc,before);assert.ok(notesOf(getPart(p.candidate,0,part)).length>=1);assert.deepEqual(getPart(p.candidate,0,1-part),getPart(before,0,1-part));applyPlan(s,p);s.undo();assert.deepEqual(s.doc,before);}const s=new DraftStore();s.edit(d=>putNote(d,0,0,{start:63,pitch:80,velocity:20}));const p=generatePlan(s,0,0,'new',{seed:1});assert.equal(getPart(p.candidate,0,0).steps[63].flags,0);const cleared=new DraftStore();assert.equal(notesOf(getPart(generatePlan(cleared,0,0,'mutate',{density:0}).candidate,0,0)).length,0);assert.throws(()=>generatePlan(s,0,2,'new',{}));assert.throws(()=>generatePlan(s,0,0,'new',{density:101}));}); |
| #99 | COVERED_CANDIDATE | offline-sequencer (unknown) | web/offline-sequencer/part-operations.test.mjs:29: test('feature 99 clear resets complete stored part but leaves all other contexts alone',()=>{ |
| DONE-001 | COVERED_CANDIDATE | smk-clip-knob (unknown), smk-clip-wait (unknown), smk-apc40 (unknown) | tests/host/smk_clip_knob_test.c:405: check("K11 clip CC turn starts the quiet interval", save_calls == 1u && ui.act_t == fm1_ms);; tests/host/smk_clip_wait_test.c:67: check(smk_clip_wait() == 0u, "W3 USB clip feedback: grid");; tests/smk_apc40_test.py:19: assert match, "missing APC40 pad table" |
| DONE-002 | COVERED_CANDIDATE | apc-palette (unknown), smk-clip-knob (unknown), smk-clip-wait (unknown) | tests/apc_palette_test.py:69: check(ok, f"hue vel {v:3d} {src[v]['apc']:06X}: LCD {hl:5.1f} (blue {blue_lcd:.1f}), "; tests/host/smk_clip_knob_test.c:405: check("K11 clip CC turn starts the quiet interval", save_calls == 1u && ui.act_t == fm1_ms);; tests/host/smk_clip_wait_test.c:67: check(smk_clip_wait() == 0u, "W3 USB clip feedback: grid"); |
| DONE-003 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| DONE-004 | COVERED_CANDIDATE | apc-palette (unknown) | tests/apc_palette_test.py:40: "source: 128 rows, each cites the Akai APC40 Mk2 protocol v1.2 PDF") |
| DONE-005 | COVERED_CANDIDATE | installer-slots (unknown), slot-begin-cancel (unknown), slot-break-menu-migration (unknown) | tests/install_slots_test.py:137: ok(rc == 0 and imgs[:2] == [a, b] and "slots saved: fresh.x0xslots" in out,; tests/x0x_slots_begin_cancel_test.py:60: check(bytes(d.fl.mem[d.slots[k][0]:d.slots[k][0] + len(b)]) == before,; tests/x0x_slots_break_menu_migration_test.py:98: bad += check(xs.migration_reorders_loop_menu(one, [img(1)], [0], mm[:1], {"slots": [{"nz": 1}, {"nz": 0}]}, mm, [(0, 1)]), |
| DONE-006 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| DONE-007 | COVERED_CANDIDATE | loop-bars (unknown), upload-tools (unknown) | tests/host/loop_bars_test.c:26: check(96218u, 44100u, 1, "one bar at 110 BPM, 44.1 kHz");; tests/upload_tools_test.py:57: loops = [wav(OUT / f"l{k}.wav", 2.0) for k in range(3)]       # one bar at 120 BPM each: 88200 samples at 44.1 kHz |
| DONE-008 | NOT_COVERED | — | A KiB unit in a sample-slot backup assertion does not exercise the stated 64-KiB Bluetooth / three-slot flash layout. |
| DONE-009 | COVERED_CANDIDATE | ble-package (unknown) | tests/ble_pkg_test.py:100: ok(area[lo:lo + len(ctl)] == ctl, "the image in the Bluetooth block") |
| DONE-011 | NOT_COVERED | — | Bluetooth block placement/readback is exercised, but no literal matched assertion exercises refusal to add the image to old firmware. |
| DONE-012 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| DONE-016 | COVERED_CANDIDATE | smk-clip-knob (unknown) | tests/host/smk_clip_knob_test.c:405: check("K11 clip CC turn starts the quiet interval", save_calls == 1u && ui.act_t == fm1_ms); |
| DONE-018 | COVERED_CANDIDATE | apc-palette (unknown) | tests/apc_palette_test.py:123: check(lc == LCD_565_CLASH, f"LCD: different source colours give different RGB565, except the known 5-bit clash {lc}") |
| DONE-019 | COVERED_CANDIDATE | padcal-flag (unknown) | tests/padcal_flag_test.py:55: check(re.search(r"#ifndef X0X_DEV_PADCAL[^\n]*\n#define X0X_DEV_PADCAL 1\b", x0x_h) is not None, |
| DONE-021 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| FIX-809-REVERT | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| FIX-INSTALLER-BACKUP | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| FIX-SMC-ALIGN | NOT_COVERED | — | smc_ui_begin/draw references exercise the popup baseline; no literal alignment identifier/assertion was found for this selected fix. |
| FIX-UI-TILES | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| L1 | COVERED_CANDIDATE | usb-live-update (unknown) | tests/host/usb_live_update_test.py:391: check("OTA-disabled live application ignores the unsupported OTA request", |
| L11 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| L2 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| L3 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| L8 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| M2 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| M3 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-789 | COVERED_CANDIDATE | smc-ui-popup (unknown) | tests/host/smc_ui_popup_test.c:189: CHECK(smc_ui_active()==open);CHECK(!memcmp(popup,&smc_ui,sizeof popup)); |
| PR-790 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-791 | COVERED_CANDIDATE | rec-endpoint-lifecycle (unknown), rec-held-duration (unknown) | tests/host/rec_endpoint_lifecycle_test.c:245: CHECK(!seq.rec_bass[0].active && !(engine_bass_keys_held() & 1), "stop while held finalizes recording and live gate");; tests/host/rec_held_duration_test.c:81: EXPECT(guard<10000,"real sequencer reaches requested recording cell"); |
| PR-792 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-793 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-794 | COVERED_CANDIDATE | site-generation-web (unknown) | web/test_site_generation.py:18: with tempfile.TemporaryDirectory(prefix='site-retention-') as td: |
| PR-795 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-796 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-797 | COVERED_CANDIDATE | usb-audio-descriptors (unknown), usb-product-names (unknown), usb-sxq-sticky-apc (unknown) | tests/host/usb_audio_descriptors_test.py:158: check(u16(link, 4) == 0x0101, P + "linked to a USB streaming terminal"); tests/host/usb_product_names_test.py:73: assert result.returncode != 0 and "USB product name exceeds" in result.stderr, result.stderr; tests/host/usb_sxq_sticky_apc_test.py:12: start = usb.index("/* Send a complete SysEx message through the same USB-MIDI event queue") |
| PR-798 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-799 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-800 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-801 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-802 | COVERED_CANDIDATE | audio-stress (unknown) | tests/host/audio_stress_test.c:108: engine_apply_sound(&proj.sound);audio(); |
| PR-803 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-804 | NOT_COVERED | — | The only scoped retry reference sets a fixture timeout; it does not exercise installer retry behavior. |
| PR-805 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-806 | COVERED_CANDIDATE | installer (unknown), installer-f02-bind (unknown), usb-product-names (unknown) | tests/install_test.py:295: ok(rc == 6 and dev.upgrades == 0, "a '37PRO' USB name never overrides the reported model (FM-1 -> exit 6)"); tests/install_f02_loader_bind_test.py:121: ok(e.code == "model" and "more than one new device" in str(e),; tests/host/usb_product_names_test.py:43: f"-DX0X_USB_37{model.upper()}=1", "-Ifirmware/src"] |
| PR-807 | COVERED_CANDIDATE | ui-waveform-truth (unknown) | tests/host/ui_waveform_truth_test.c:14: float actual=pulse?mod_waveform(p,UI_WAVE_REFERENCE_STEP,selected):waveform(p,UI_WAVE_REFERENCE_STEP,selected); |
| PR-808 | COVERED_CANDIDATE | offline-sequencer (unknown), offline-sequencer-browser (FAIL) | web/offline-sequencer/song-library.test.mjs:42: test('song addresses stay within the editor catalog directory and secure origins',()=>{; web/offline-sequencer/f6b-browser.test.py:127: page.locator('#view-editor').click() |
| PR-809 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-810 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-811 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| PR-820 | COVERED_CANDIDATE | smc-multi-peer (unknown), smc-pickup-acceptance (unknown), smc-remainder (unknown) | tests/host/smc_multi_peer_test.c:79: smc_ui_draw(); CHECK(smc_slot==0); CHECK(smc_ui.peer==1);; tests/host/smc_pickup_acceptance_test.c:172: smc_ui_begin(strip, 0); status_is("ACTIVE");; tests/host/smc_remainder_test.c:483: smc.picked=0; pref_set(smc_target(0),0); smc_ui_begin(0,0); |
| PR-821 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| ROW-80982d592fa7 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| ROW-938cc69314d2 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| ROW-e7ebadf91a40 | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |
| ROW-fcf14c2de61c | NOT_COVERED | — | No qualifying literal identifier reference found in runner-mapped test source. |

## Checker

`sh out/tests/host/roadmap_evidence_check.sh ./tree ./out/docs/roadmap-evidence`

Exit **0**. Full output: `PASS2-CHECKER.txt`.

```text
TOTALS present_ok=2 present_bad=0 UNPROVEN=144 MISSING=144 rows=146
ROADMAP_PIN blob=a204a54c989d6f323d619e70f799427603756541 commit=5f578fc0642a1200a5163802bcd69277c511bfc7
```

## Checker self-test

`TMPDIR="$PWD/out" sh out/tests/host/roadmap_evidence_check_test.sh`

Exit **0**. These are evidence-checker fixtures, not suite tests.

```text
PASS GREEN (exit=0)
#1: PRESENT_OK
#2: UNPROVEN
TOTALS present_ok=1 present_bad=0 UNPROVEN=1 MISSING=1 rows=2
ROADMAP_PIN blob=a204a54c989d6f323d619e70f799427603756541 commit=5f578fc0642a1200a5163802bcd69277c511bfc7
PASS RED_missing_path (exit=1)
MUTANT RED_missing_path: drop the os.path.isfile(code_path) check -> RED would exit 0; this case catches it.
PASS RED_empty_result (exit=1)
MUTANT RED_empty_result: treat empty result as PASS (or skip result validation) -> RED would exit 0; this case catches it.
PASS RED_disagreeing_pin (exit=1)
MUTANT RED_disagreeing_pin: keep only the first file's pin and never compare later files -> RED would exit 0; this case catches it.
PASS RED_malformed_json (exit=1)
MUTANT RED_malformed_json: wrap json.load in a bare except that continues as UNPROVEN/OK -> RED would exit 0; this case catches it.
----
SELFTEST pass=5 fail=0
```

## Data integrity audit

DATA AUDIT: 146 rows; 132 coverage records; 124 literal candidate citations verified; 2 PRESENT rows have named pinned ok proof; pins, receipts, schema, checker scripts and PASS1 report preserved. No suite test was executed by this citation/JSON audit.

## Measured wall time

Timed interval: `2026-11-05T13:36:58.779332793Z` to `2026-11-05T13:47:44.652942+00:00`. Elapsed **645873609207 ns / 60,000,000,000 = 10.764560153 wall minutes**. This interval covers the recorded timing checkpoint through evidence updates, both checks and the final citation/JSON audit; initial task/skill/input orientation preceded that checkpoint and was not timed. No unrecorded start time is estimated.

TOTALS PRESENT=2 UNPROVEN=144 NEEDS_NAMED_TEST=132 DEVICE_BLOCKED=7 TEST_FAILS=1 NOT_RUN=0 UNCLEAR=4 COVERED_CANDIDATE=64 NOT_COVERED=68 checker_exit=0 selftest_exit=0 wall_minutes=10.764560153
