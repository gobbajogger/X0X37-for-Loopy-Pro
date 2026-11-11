# Stage 1 audit of 13d32e1

Audited tree: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5` (`origin/main` tip at audit time).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f02-stage1-fb90`.
This thread: `bc-93d068c9-df43-525d-b826-6e57a67efb90` (FCDC-3 / Oldest open defect).
Project store status: `/cursor/stores/bc-b05591bb-062b-4903-a604-51ad1dfc9f05/internal/oldest-defect.md`.

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed.

Prior stage-1 record on `cursor/f02-loader-stage1-e755` audited `e231702` and selected the same defect. That stage-2 handoff never started an owner-visible successor. This audit re-validates the current main tip and does not resume that old thread.

## Selected defect

**F02. After the installer starts the update loader, the next search can attach to a different keyboard.**

Oldest open audit / bugfinder finding still present in this SHA. Formal deferred defects from prior cycles are closed or non-source on this tip (see below). The seven host FAILs fixed by #157 are not FAIL on this tip (five ok, two Pillow not-run).

Cited blobs, matched at `13d32e1`:

| File | Blob |
| --- | --- |
| `tools/fm1_install.py` | `77dbe2cfab79a78c953abf7c273dbf6156a35f7e` |
| `web/fm1ota.js` | `014f5fa3490f476554a17b08e941bde04c7eff74` |

Same blobs as the earlier `e231702` audit. No installer fail-closed transition landed on main.

## Evidence

Native search after the running firmware is told to reboot into the loader:

```406:412:tools/fm1_install.py
    def wait_for(self, want, secs):
        end = time.monotonic() + secs
        while time.monotonic() < end:
            dev = self.find(want, only_port=False)
            if dev:
                return dev
```

`find` returns the first enumerated port whose handshake matches. `only_port=False` widens the search. `check` uses that search for `i.loader`, then `write` sends the package to whatever came back.

Web updater, same shape:

```137:144:web/fm1ota.js
  async waitFor(filter, ms) {
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const dev = await this.find(filter);
      if (dev) return dev;
      await sleep(1000);
    }
```

Host model (no real MIDI): `evidence/f02-stage1/repro.py` / `repro.log`. Two fake ports: an FM-1 already in its loader (enumerated first) and the selected SMK-37 still running. `wait_for(lambda ident: ident.loader)` returns `ota-FM-1_906` on `Other OTA`. Log: `BUG REPRODUCED: loader search accepted a different keyboard`.

## Affected scope

- `tools/fm1_install.py`: `find`, `wait_for`, `check`, `verify`, and `install`.
- `web/fm1ota.js`: `find`, `waitFor`, `install`, and `resume`.
- Callers that inherit the unbound wait: `web/x0x_breaks.js` `installKeepingSlots`, and the loader scans in `web/x0x_installer.html` and `web/index_pkg.html` only where they continue a transition already started.

The initial choice of the first running keyboard, when the user has not named a port, is out of scope.

Firmware, controller image, package bytes, sample-slot backup format, production `main`, and production Pages are outside this repair.

## Proposed fix

Before the command that reboots the selected device into the loader, remember the loader ports already answering. After the transition, accept a loader only when exactly one new loader port has appeared. If none appear before the existing timeout, keep the current `noloader` result. If more than one new loader appears, or the only loader is one that was already present, stop and do not write.

Apply the same rule when waiting for the selected keyboard to return after the write, including the slot-restore wait in `installKeepingSlots`.

Add a regression that fails on the current first-match behavior and passes after the repair, for native `wait_for` and web `waitFor`. No real MIDI; no device write.

## Approvals

The full-cycle procedure (AGENTS.md + docs/rules/workflow.md on main, Project preferences, FCDC-3 assignment) approves a development repair of this selected defect only. Standing owner pre-approval covers properly tested Project merges; this stage still does not merge production `main`, publish production Pages, or flash a device.

Not approved in this repair: F03, BF-001 through BF-003, the withheld asset cache, unmerged data-safety physical-endpoint lanes.

Development configuration if a firmware build is ever required: `X0X_DEV_PADCAL=1`. This repair should not need a firmware change.

## Missing decisions

None block the fail-closed repair. Richer physical-port identity for duplicate MIDI names is a different change and is not decided here.

## Defects considered and not selected

### Deferred defects

| Record | Item | In this SHA |
| --- | --- | --- |
| mapping-stage4 deferred | R/S per-track policy | Superseded; per-drum Record/Solo is in tree. |
| same | `docs/SMC_MIXER.md:47` | Fixed by `4a219ff`; current map matches the drum exception. |
| same | Physical all-input silence cause | Source fixed by `e00741e`; device retest is not a new source repair. |
| cache exclusion | Alias-loader stale hit | Failed cache not in tree; no open tip defect. |

### Recorded failing checks (#157 seven)

Ran locally on this tip: `sh tests/run_tests.sh smc-drum-rs rec-live-smk dev-validate validation-inputs runner-admission smk-apc40 gate-trap` → log `seven-host-checks.log`.

| Check | Result |
| --- | --- |
| smc-drum-rs | not-run (Pillow) — optional gap, not FAIL |
| rec-live-smk | not-run (Pillow) — optional gap, not FAIL |
| gate-trap | ok |
| runner-admission | ok |
| validation-inputs | ok |
| dev-validate | ok |
| smk-apc40 | ok |

No recorded FAIL from that set remains. Host-gap false stops are cleared per assignment.

### Younger open findings (not selected)

F03 (`slot_probe` / `plan_slots` backup skip), BF-001, BF-002, BF-003 remain younger than F02.

## Stage 2 limit

Implement only F02, with the fail-closed transition search above, on a new stage-2 branch from the pushed tip of `cursor/f02-stage1-fb90`. Do not take F03 or a bugfinder item in that same change.
