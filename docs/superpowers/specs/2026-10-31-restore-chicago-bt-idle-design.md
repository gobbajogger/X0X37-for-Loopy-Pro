# Restore Chicago FX, Bluetooth names and idle UI

Owner request: restore existing implementations lost during one-build integration.
Source base: `integration/all-in-one@e2c33498d33c425287f1df077b8ac6cc89a6e8aa`.
Source tree: `48246f816a7984871fe39dd92db646fbfeab15de`.
Delivery branch: `feat/restore-chicago-fx-bt-idle-extras`; one draft PR into
`integration/all-in-one`, reused for all subsequent approved work.

## Owner approval boundary

This document records a restoration proposal, not completed functionality.
Production UI code stays unchanged until the owner approves real-framebuffer
BEFORE | AFTER proposals. Renderer experiments run only in an isolated scratch
copy; captures are delivered in chat and are never committed. Approval of those
proposals also confirms the design and preserves the requested subagent method.

## Scope and source provenance

### Chicago inserts

Restore availability of all ten named insert classes: Phaser, Ensemble, GateRev,
Cho/Flg, Hall, BPM Delay, Short Delay, EQ, Crush and Ring Mod. Reuse historical
compact implementations, not newly designed DSP.

Original source PRs: Ring #448, Crush #466, EQ #479, Short Delay #495, BPM Delay
#512, Hall #524, Cho/Flg #552, GateRev #561, Ensemble #569 and Phaser #577.
Retunes and consolidated dispatch in #611/#677 are relevant source references.
The compact insert headers already remain in the integration tree.

Preserve current original-engine choices 0..6 (OFF, CHO/FLG, PHASER, RING MOD,
COMPRESSOR, DECIMATOR and EQ), their algorithms, factory indices and revision-13
meaning. Append the five absent classes: ENSEMBLE, GATE REV, HALL, BPM DELAY and
SHORT DELAY. The working current implementations of the other five classes stay.
DECIMATOR is the current Crush class; do not rename persisted choices.

Reuse only the missing compact algorithms after original Chicago voice output,
through a selected per-part owner with historical reset, control-write, tempo and
tail rules. Preserve current processing for the existing seven choices. No
`bass303.c`, `wobble.c` or retired oscillator graph is restored.

The historical compact union is at most 8,204 bytes per part. This is an
architecture estimate, not a measured candidate budget. Actual Elite/Pro code,
RAM, pool and stack admission remains required, including independent part
histories and the existing reserve. The much larger original-reference bank in
#681 is a different implementation and is outside this compact restoration.

Measured unchanged baseline for both models: pool 334,540/344,064 bytes, leaving
9,524 bytes. Retaining an 8,192-byte reserve leaves only 1,332 bytes of growth.
The simple enlarged-owner estimate exceeds that headroom, so the eventual port
must demonstrate a safe reusable allocation or other existing-source memory
solution; do not silently consume the reserve or claim admission from this plan.

### Connected Bluetooth device names

Port the bounded two-peer renderer from #687, `wip/smc-port` at
`7840c0394967f0e731d905e5832fb6cf7c2127d7`:

- `ui.c`: `bt_peer_text`, `R_BTNAME` hook and adequate local name buffer.
- `plat.h`: declaration of `plat_ble_peer_names(char names[2][21])`.
- `host/x0x_host.c`: original peer stub and `X0X_HOST_BLE_NAME_IMPLEMENTED` guard.

The real BLE aggregation and its existing renderer regression test already
remain in the base. Preserve link configuration; this does not activate #735's
two-link controller changes. Verify individual truncation, identical names,
one/two peers, unnamed peers and disconnection.

### BT SAVE PENDING retry

The exact separately named donor is not yet identified. Search evidence covers
740 PR metadata records, 841 branch refs, 108 distinct UI contents, backup patches
and relevant platform/project/BLE files. Absence from these snapshots is not proof
that an older commit or local-only source never contained the feature.

The physical BT/Save button's generic queued project-save retry from #632,
`1fe3430acffa1854f8206186e37882d3f4c00866`, already remains in this base.
It shows SAVE WHEN QUIET / REC OFF TO SAVE / SAVE AT STOP and has the existing
`save_note_quiet_test`. Do not rename or represent it as a separate Bluetooth
settings retry without resolving the owner's wording. This subtask stays pending
source identification; do not invent its implementation.

### Idle-glyph extras

Donor: #681, `fix/original-chicago-miami-idle-glyphs-20261029` at
`ca8299b6bbf2f4ffaa3f10558b335e30a2c10914`. The donor's whole base was rejected;
port only the relevant helpers and hunks.

- Waveforms: use its cached 29-point native Chicago/Miami waveform projection.
  The current UI already has glyphs derived from the former oscillator equations.
  Keep the current cards and control labels; replace only the relevant cues.
- ADSR: preserve the current inline Chicago exponential-stage envelope graph.
  Restore the donor's temporary touch schematic, explicitly labelled
  SCHEMATIC / CONTROL AMOUNTS (%), with correct touched-stage and held-step identity.
- CPU: preserve existing measured readout and timing APIs. Restore the donor's
  16-bit saturating PERF presentation so 300% is not narrowed to 44%. This does not
  claim lower CPU load or hardware timing validation.

## Approaches

Selected by the owner request: bounded ports of existing source into the current
integration branch. Preserve current behavior outside the restored gaps.

Whole-PR cherry-picks are unsuitable because the donors contain old bases,
retired-engine dependencies or excluded pad-calibration UI. Reimplementing the
features would violate the request. The large exact-reference effect bank is not
the historical compact insert restoration requested here.

## Non-goals and invariants

No PAD DIAGNOSTICS page, PAD COLOURS page or calibration UI additions. No `.github`
or GitHub Actions edits. No `main`/`nightly` updates, merges, force pushes, releases,
device installs or flashing. No BTIF, USRTRIM, efuse, key_mac or bootloader changes.
Never build or test FM-1; only SMK-37 Elite and Pro DEV, SDK V1.2.1.

All new commits must be authored and committed as
`gobbajogger <255244738+gobbajogger@users.noreply.github.com>`, never genkigenki.
Never commit `.env`, bass303, firmware binaries, SDK/toolchain bytes or captures.
Existing `*_private.h` files remain valid source and must not be removed or flagged.

## Acceptance and completion

Owner-approved real framebuffer proposals precede production UI implementation.
Independent source/visual review, meaningful actual-source regression checks and
SDK V1.2.1 Elite/Pro builds must pass on the final candidate. Existing choice and
factory indices retain revision-13 meaning. Restore the exact identified BT retry,
or keep that requirement explicitly blocked rather than silently substituting it.
Post results on the same draft PR and verify its remote head and commit identities.
Baseline builds and proposal renders are labelled separately from final-candidate
qualification. Hardware/device proof is not claimed by this source task.
