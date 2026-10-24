# Stage 1 audit of e231702

Audited tree: `e23170227ca4ae4b6cc2700692f4d650fa1b506b`.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f02-loader-stage1-e755`.
This thread: `bc-ad31708d-5548-5785-bb62-75a54918e755`.
Parent thread, not resumed: `bc-01a16f1a-08bd-72eb-b558-6c498238d6ff`.

`origin/cursor/full-cycle-stage-workflow-d6ff` also has `122dd429ec3634ecdd4256cb5c345678cfe26b1a`, which only records this thread's id in the previous handoff. That commit is not the audited tree.

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed.

## Selected defect

**F02. After the installer starts the update loader, the next search can attach to a different keyboard.**

This is the oldest open audit finding that is still present in `e231702`. The 2026-10-21 independent project audit recorded it. The later bugfinder baseline, `docs/audit/bugfinder-watch-2026-10-23.md` on `lane/bugfinder-reports-20261023`, repeats it and says no repair was claimed. That report is not in this SHA. The vulnerable blobs are.

Cited blobs, matched at `e231702`:

| File | Blob |
| --- | --- |
| `tools/fm1_install.py` | `77dbe2cfab79a78c953abf7c273dbf6156a35f7e` |
| `web/fm1ota.js` | `014f5fa3490f476554a17b08e941bde04c7eff74` |

Neither file differs between `c9718ae` (the `main` parent) and `e231702`. `origin/nightly` at `54f2ffbc3124f7449a69067dc8215a2d91a2e675` still uses the same unbound search.

## Evidence

Native search used after the running firmware is told to reboot into the loader:

```406:412:tools/fm1_install.py
    def wait_for(self, want, secs):
        end = time.monotonic() + secs
        while time.monotonic() < end:
            dev = self.find(want, only_port=False)
            if dev:
                return dev
```

`find` returns the first enumerated port whose handshake matches. `only_port=False` widens the search to every name that looks like a supported keyboard. `check` uses that search for `i.loader`, then `write` sends the package to whatever came back. `verify` uses the same search for the first running device after the write.

The web updater has the same shape. `waitFor` calls `find`, and `find` returns the first open input that passes the filter:

```137:144:web/fm1ota.js
  async waitFor(filter, ms) {
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const dev = await this.find(filter);
      if (dev) return dev;
      await sleep(1000);
    }
```

`install` waits that way for the loader and again for the restarted firmware. `resume` and the page-level loader scans call `find(IS_OTA)` directly. `installKeepingSlots` in `web/x0x_breaks.js` waits again for a running device before writing sample slots back.

A host model of the native search is `repro.py`. It does not open MIDI. Two fake ports answer: an FM-1 already in its loader, enumerated first, and the selected SMK-37 still running. `wait_for(lambda ident: ident.loader)` returns `ota-FM-1_906` on `Other OTA`. The log is `repro.log`. That is the search `check` uses. It is not a physical install and not a browser pass.

## Affected scope

- `tools/fm1_install.py`: `find`, `wait_for`, `check`, `verify`, and `install`.
- `web/fm1ota.js`: `find`, `waitFor`, `install`, and `resume`.
- Callers that inherit the unbound wait: `web/x0x_breaks.js` `installKeepingSlots`, and the loader scans in `web/x0x_installer.html` and `web/index_pkg.html`.

The initial choice of the first running keyboard, when the user has not named a port, is a separate behavior. This repair does not turn that scan into a device picker.

Firmware, the controller image, package bytes, sample-slot backup format, production `main`, and production Pages are outside this repair.

## Proposed fix

Before the command that reboots the selected device into the loader, remember the loader ports already answering. After the transition, accept a loader only when exactly one new loader port has appeared. If none appear before the existing timeout, keep the current `noloader` result. If more than one new loader appears, or the only loader is one that was already present, stop and do not write.

Apply the same rule when waiting for the selected keyboard to return after the write, including the slot-restore wait in `installKeepingSlots`: accept exactly one newly appeared running device, and do not restore slots to a different keyboard that was already running.

Add a regression that fails on the current first-match behavior and passes after the repair. Cover the native `wait_for` path and the web `waitFor` path. Keep the model free of real MIDI and of device writes.

## Approvals

The full-cycle procedure approves a development repair of this selected defect only. There is no approved patch, and no approval to merge production `main`, publish production Pages, or flash a device.

These are not approved: F03, BF-001 through BF-003, the withheld asset cache, and the unmerged data-safety lanes that add physical MIDI endpoint identity.

If a firmware file is touched, the development configuration remains `X0X_DEV_PADCAL=1`. This repair should not need a firmware change.

## Missing decisions

None block the fail-closed repair above. A richer physical-port identity, for duplicate MIDI names or a loader reboot that cannot be distinguished by port appearance, is a different change and is not decided here.

## Defects considered and not selected

### Deferred defects

Formal deferred lists were checked from oldest to newest.

| Record | Item | In this SHA |
| --- | --- | --- |
| `c275f89` `evidence/mapping-stage4/plan-review.json` | R/S per-track policy | Superseded and present. `smc_mixer.c` has per-drum Record and Solo. `docs/smc-drum-controls-2026-10-21.md` records the owner choice. |
| same list | `docs/SMC_MIXER.md:47` omitted the drum M exception and performance-test controls | Fixed by `4a219ff`. The current map limits part M/S/R to pages outside drum MIX and SEQ, and performance tests own those controls. |
| same list | Cause of physical all-input silence | Source cause is `e00741e`: the central accepts CID 6 pairing, connection-parameter updates, ATT MTU exchange, and complete ACL PDUs. Older evidence files still say unresolved because they predate that commit. The device was not retested. That is not a new source repair. |
| `75f9d16` cache exclusion | Repair the alias-loader stale hit, or keep unsupported reuse disabled | The failed cache is not in the tree. `b6d6516` records that no new cache repair was requested. Not an open defect in this SHA. |

`BLE_ATTEMPTS.md` still mentions older attempt residuals, including counted `drops_in` and image queue drops. Those notes describe side builds. They are not the stage-cycle deferred list, and they do not outrank F02 once the formal deferred defects above are closed.

### Recorded failing checks

| Check | Disposition in this SHA |
| --- | --- |
| `CENTRAL-ABORT-01` mailbox backpressure, `evidence/session-stage3/checks.json` | `blec_poll` calls `blec_disconnect` again while disconnecting, and a disconnect is marked sent only after the command is accepted. |
| `CENTRAL-ABORT-02` disconnect after the original deadline | `blec_disconnect` returns at the original deadline without sending. |
| `CENTRAL-STOP-01` stop reported OFF after a refused command | `blec_stop_progress` keeps the stop until the control command is accepted, and a refused command does not clear the connection by itself. |
| `smc-stage3` receive exit 139 / capture exit 1 | Disposition in that record is a sanitizer startup failure, with the same checks passed on the supported host. No source repair was indicated. |
| Oct 20 clip-extractor and null `memcpy` failures | `tests/host/smc_clip_visibility_test.sh` extracts `static uint8_t smc_message`. `tests/host/ble_midi_central_test.c` copies the extra bytes only when the length is non-zero. |

`node web/test_web.mjs` still fails because `web/editor.html` is absent. That is BF-003, recorded on 2026-10-22, and it is younger than F02. The retire-or-restore choice is still open. It is not this repair.

### Younger open findings

F03, from the same 2026-10-21 audit, is still present: `slot_probe` returns `None` for a timeout or a parse error, and `plan_slots` then skips the backup unless a backup was explicitly required. It is the second finding in that audit, so it waits.

BF-001, the local `.fwsc` selection race in `web/x0x_installer.html` `acceptStock`, is still present at lines 576–615 and was reproduced by the 2026-10-22 bugfinder against `c9718ae`. It is younger than F02. BF-002 is the quickcheck project-suite gap and is also younger.

## Stage 2 limit

Implement only F02, with the fail-closed transition search above, on a new stage-2 branch from the pushed tip of `cursor/f02-loader-stage1-e755`. Do not take F03 or a bugfinder item in that same change.
