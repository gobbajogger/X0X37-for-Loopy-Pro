# PR #734 split Part 2: frozen-head host suite, Pro build and XIP

- Repository: gobbajogger/X0X37-for-Loopy-Pro
- Tested branch: integration/all-in-one
- Full HEAD: 636c1f7013a1cf02e34b8d23031086777d898e7b
- Git root tree: 2a6a68b4c7515a757f07af12f2c6f59335b5c7a0
- Recovered source: reused verified #726 GitHub API tree/blob recovery; no re-fetch
- Source validation: **4,579/4,579 blob SHAs and sizes match** in each of two isolated source copies
- Test runner: tests/run_tests.sh at frozen HEAD; unchanged
- Split: **suite cases 159–316 of 316**, disjoint from Part 1 cases 1–158
- Runner: macOS arm64, X0X_TEST_JOBS=3, X0X_TESTS explicitly selects these 158 tests
- SDK: AC79NN_SDK_V1.2.1_2023-12-13 (local SDK commit d179b448); JieLi 20260730.1 toolchain
- Host invocation: `sh tests/run_tests.sh`, with `X0X_TEST_JOBS=3` and `X0X_TESTS` set to the 158 space-separated names in `tests.txt`
- Host process exit: **1 (FAIL)**
- Host totals: **78 PASS / 80 FAIL / 0 NOT_RUN**
- Pro build command: BOARD=smk37 MODEL=pro sh build.sh; **exit 1 (FAIL)**
- Pro safety failure: firmware/src/app/engine.c:48, 0x10000u flagged as register/window address outside hal/
- Pro XIP image: **451,936 / 552,672 bytes**, 100,736 bytes free; **XIP size PASS**
- Pro RAM: 94,960/98,304; pool: 333,772/344,064
- No firmware binaries, raw captures, private header contents, credentials, .github changes or device operations published
- Full Part 1 results: separately owned by chat/pr734-host-part1-logs, not rerun here

## All 158 selected tests (original suite order)

| Global suite # | Test | Result |
|---:|---|---|
| 159 | `status-lanes` | FAIL |
| 160 | `status-isolation` | FAIL |
| 161 | `loop-bars` | PASS |
| 162 | `installer` | PASS |
| 163 | `installer-f02-bind` | PASS |
| 164 | `installer-f02-bind-web` | PASS |
| 165 | `locked-settings` | PASS |
| 166 | `smk-flash` | PASS |
| 167 | `smk-storage` | PASS |
| 168 | `smk-apc40-map` | PASS |
| 169 | `smk-apc40` | PASS |
| 170 | `smk-clip-ble-clear` | PASS |
| 171 | `smk-clip-keys` | PASS |
| 172 | `smk-chord-recognize` | PASS |
| 173 | `smk-clip-chord-display` | PASS |
| 174 | `smk-ck-page-layout` | PASS |
| 175 | `smk-arrows` | PASS |
| 176 | `smk-arrows-owner` | PASS |
| 177 | `apc40-tables` | PASS |
| 178 | `smk-clip-idle` | PASS |
| 179 | `smk-clip-idle-pad` | PASS |
| 180 | `apc40-pad-owner` | PASS |
| 181 | `smk-padcal` | PASS |
| 182 | `padcal-view` | PASS |
| 183 | `smk-padcal-export` | PASS |
| 184 | `smk-padcal-wiring` | PASS |
| 185 | `pad-placement-doc` | PASS |
| 186 | `padcal-flag` | PASS |
| 187 | `apc40-lcd565-csv` | PASS |
| 188 | `apc-palette` | PASS |
| 189 | `installer-targets` | PASS |
| 190 | `diag-size` | PASS |
| 191 | `smk-inlog` | PASS |
| 192 | `ble-central-diag` | PASS |
| 193 | `ble-role-switch` | FAIL |
| 194 | `inlog-tool` | PASS |
| 195 | `smk-pad-inputs` | PASS |
| 196 | `smk-clip-ble-notify` | PASS |
| 197 | `smk-clip-dark` | PASS |
| 198 | `smk-clip-usb-clear` | PASS |
| 199 | `smk-clip-isolation` | PASS |
| 200 | `smk-clip-knob` | PASS |
| 201 | `smk-fader-pickup` | PASS |
| 202 | `smk-clip-wait` | PASS |
| 203 | `smk-apc40-violet` | PASS |
| 204 | `ble-sysex-bounds` | PASS |
| 205 | `ble` | PASS |
| 206 | `upload-images` | PASS |
| 207 | `upload-tools` | PASS |
| 208 | `builtin-break-output` | FAIL |
| 209 | `break-pack` | PASS |
| 210 | `break-pack-web` | PASS |
| 211 | `breaks-web` | PASS |
| 212 | `breaks-page` | PASS |
| 213 | `project-adoption-identities` | PASS |
| 214 | `projects-web` | PASS |
| 215 | `installer-project-handoff` | PASS |
| 216 | `projects-page` | PASS |
| 217 | `install-slots-web` | PASS |
| 218 | `installer-backup-selection` | PASS |
| 219 | `slot-build-safe` | PASS |
| 220 | `wav-import-integrity` | PASS |
| 221 | `slot-backup` | PASS |
| 222 | `slot-begin-cancel` | PASS |
| 223 | `smp-begin-cancel` | PASS |
| 224 | `slot-backup-durable` | PASS |
| 225 | `slot-archive-zone` | PASS |
| 226 | `slot-verify-order` | PASS |
| 227 | `slot-read-reply` | PASS |
| 228 | `slot-ack-integrity` | PASS |
| 229 | `slot-echo-7bit` | PASS |
| 230 | `slot-migration-matching` | PASS |
| 231 | `slot-break-menu-migration` | PASS |
| 232 | `installer-slots` | PASS |
| 233 | `installer-restore-status` | PASS |
| 234 | `installer-ota-finish-ack` | PASS |
| 235 | `installer-backup-snapshot` | PASS |
| 236 | `installer-ctl` | PASS |
| 237 | `ctl-read-framing` | PASS |
| 238 | `ctl-publish-atomic` | PASS |
| 239 | `rec-held-duration` | FAIL |
| 240 | `rec-endpoint-lifecycle` | FAIL |
| 241 | `rec-endpoint-storage` | FAIL |
| 242 | `rec-nonforward-guard` | FAIL |
| 243 | `rec-live` | FAIL |
| 244 | `rec-live-smk` | FAIL |
| 245 | `copy-semantics` | FAIL |
| 246 | `song-actions` | FAIL |
| 247 | `ui-feedback` | FAIL |
| 248 | `remaining-feature-proof-selftest` | PASS |
| 249 | `home-pattern-glyph` | PASS |
| 250 | `home-overview` | FAIL |
| 251 | `sound-matrix` | FAIL |
| 252 | `sound-layout` | FAIL |
| 253 | `chicago-presentation` | FAIL |
| 254 | `chicago-readout` | FAIL |
| 255 | `matrix-knob-caption` | FAIL |
| 256 | `preset-navigation-noop` | FAIL |
| 257 | `ui-303-entry` | FAIL |
| 258 | `scenario-arrows` | FAIL |
| 259 | `scenario-autosave` | FAIL |
| 260 | `scenario-bleclock` | FAIL |
| 261 | `scenario-blenotes` | FAIL |
| 262 | `scenario-brkslice_mode_toggle` | FAIL |
| 263 | `scenario-brkslice_trs_overlap` | FAIL |
| 264 | `scenario-brkslice` | FAIL |
| 265 | `scenario-clip-arrow-menu` | FAIL |
| 266 | `scenario-clklock` | FAIL |
| 267 | `scenario-clkmove` | FAIL |
| 268 | `scenario-clocksrc` | FAIL |
| 269 | `scenario-drum_bass_ch_consume` | FAIL |
| 270 | `scenario-edit_midi_rec_guard` | FAIL |
| 271 | `scenario-edit_oct_no_hidden` | FAIL |
| 272 | `scenario-extclock` | FAIL |
| 273 | `scenario-freeplay` | FAIL |
| 274 | `scenario-getting_started` | FAIL |
| 275 | `scenario-groove` | FAIL |
| 276 | `scenario-guard` | FAIL |
| 277 | `scenario-help` | FAIL |
| 278 | `scenario-jam` | FAIL |
| 279 | `scenario-keysound` | FAIL |
| 280 | `scenario-knobs` | FAIL |
| 281 | `scenario-midiowner` | FAIL |
| 282 | `scenario-migrate` | FAIL |
| 283 | `scenario-migrate0102` | FAIL |
| 284 | `scenario-patterns` | FAIL |
| 285 | `scenario-perf` | FAIL |
| 286 | `scenario-perfsave` | FAIL |
| 287 | `scenario-plock` | FAIL |
| 288 | `scenario-prob` | FAIL |
| 289 | `scenario-probcond` | FAIL |
| 290 | `scenario-probreset` | FAIL |
| 291 | `scenario-roll_continuation` | FAIL |
| 292 | `scenario-roll_short_tie` | FAIL |
| 293 | `scenario-savefeedback` | FAIL |
| 294 | `scenario-savewait` | FAIL |
| 295 | `scenario-showcase` | FAIL |
| 296 | `scenario-slices` | FAIL |
| 297 | `scenario-song_live_mute_save` | FAIL |
| 298 | `scenario-song` | FAIL |
| 299 | `scenario-songrep` | FAIL |
| 300 | `scenario-sound_matrix_chicago_idle` | FAIL |
| 301 | `scenario-sound_matrix_chicago` | FAIL |
| 302 | `scenario-sound_matrix_drum` | FAIL |
| 303 | `scenario-sound_matrix_lock` | FAIL |
| 304 | `scenario-sound_matrix_miami` | FAIL |
| 305 | `scenario-sound_page_split` | FAIL |
| 306 | `scenario-step303` | FAIL |
| 307 | `scenario-steps64` | FAIL |
| 308 | `scenario-stereo` | FAIL |
| 309 | `scenario-tb3po` | FAIL |
| 310 | `scenario-tour` | FAIL |
| 311 | `scenario-trackmute` | FAIL |
| 312 | `scenario-undo` | FAIL |
| 313 | `scenario-upload` | FAIL |
| 314 | `scenario-upload44` | FAIL |
| 315 | `scenario-usbnotes` | FAIL |
| 316 | `perf-scratch` | FAIL |

## All failed cases (80)

`status-lanes` `status-isolation` `ble-role-switch` `builtin-break-output` `rec-held-duration` `rec-endpoint-lifecycle` `rec-endpoint-storage` `rec-nonforward-guard` `rec-live` `rec-live-smk` `copy-semantics` `song-actions` `ui-feedback` `home-overview` `sound-matrix` `sound-layout` `chicago-presentation` `chicago-readout` `matrix-knob-caption` `preset-navigation-noop` `ui-303-entry` `scenario-arrows` `scenario-autosave` `scenario-bleclock` `scenario-blenotes` `scenario-brkslice_mode_toggle` `scenario-brkslice_trs_overlap` `scenario-brkslice` `scenario-clip-arrow-menu` `scenario-clklock` `scenario-clkmove` `scenario-clocksrc` `scenario-drum_bass_ch_consume` `scenario-edit_midi_rec_guard` `scenario-edit_oct_no_hidden` `scenario-extclock` `scenario-freeplay` `scenario-getting_started` `scenario-groove` `scenario-guard` `scenario-help` `scenario-jam` `scenario-keysound` `scenario-knobs` `scenario-midiowner` `scenario-migrate` `scenario-migrate0102` `scenario-patterns` `scenario-perf` `scenario-perfsave` `scenario-plock` `scenario-prob` `scenario-probcond` `scenario-probreset` `scenario-roll_continuation` `scenario-roll_short_tie` `scenario-savefeedback` `scenario-savewait` `scenario-showcase` `scenario-slices` `scenario-song_live_mute_save` `scenario-song` `scenario-songrep` `scenario-sound_matrix_chicago_idle` `scenario-sound_matrix_chicago` `scenario-sound_matrix_drum` `scenario-sound_matrix_lock` `scenario-sound_matrix_miami` `scenario-sound_page_split` `scenario-step303` `scenario-steps64` `scenario-stereo` `scenario-tb3po` `scenario-tour` `scenario-trackmute` `scenario-undo` `scenario-upload` `scenario-upload44` `scenario-usbnotes` `perf-scratch`

## NOT_RUN / skipped (0)

None.
