# Stage 1 audit of validated source 4ee8f377

Audited tree: `4ee8f37714b0892f2c70d157fda9328da393e987` (validated source published on nightly; not the diverged firmware `nightly` tip).
Content tree (Stage-4 DEV CHECKED records): `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Git tree (`source_tree`): `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f03-stage1-fb90`.
This thread: `bc-2e0f6b6e-50b2-506d-92f4-e19a81d6cfac` (FCDC-35 / F03 stage-1 audit).
Prior cycle audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Evidence-bearing base: remote tip of `cursor/f02-stage7-fb90` (docs/evidence only; packages authenticate `4ee8f377`).

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed (still diverged / FF refused at `58e2c52…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live HTML | `nightly-4ee8f377-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb` **MATCH** |
| Elite blank (hosted Stage-4) | size 651396, sha256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b` **MATCH** |
| Pro blank (hosted Stage-4) | size 651392, sha256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9` **MATCH** |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Host tip | `artifacts/nightly-dev` @ `f917b636ba00d7dbef18fd00d1f0af15299a0a24` |
| Pages tip (Stage-7 record) | `364c55092b8a40fcb683c3008074c508a6d75bb4` |

Cited blobs at `4ee8f377`:

| File | Blob |
| --- | --- |
| `tools/fm1_install.py` | `8f871f1685ba39ee6478ea67d93ba8fa12708763` |
| `web/x0x_breaks.js` | `bbc17b263bc447d91563be9fe79dc8fe73e93f36` |
| `web/fm1ota.js` | `d7568620efda62c750117a676c75b97c6478635c` |

## Selected defect

**F03. When the same-keyboard install path cannot read the sample-slot layout, the installer skips the prewrite backup and continues the write.**

Oldest deferred defect from `evidence/f02-stage2/DEFERRED.md` that is **not** already fixed in `4ee8f377`. F02 (fail-closed loader attach) is present in this SHA and is not selected.

## Evidence

Native probe swallows timeout/parse and returns `None`:

```618:623:tools/fm1_install.py
def slot_probe(link):
    """the running firmware's slot layout (SMP_INFO), or None if it does not answer (not X0X)"""
    try:
        return xs.parse_info(xs.req(EditorLink(link), xs.CMD_INFO, [], SLOT_PROBE["timeout"], SLOT_PROBE["tries"]))
    except (TimeoutError, xs.SlotError, ValueError, IndexError):
        return None
```

`plan_slots` then returns `None` unless `--backup` / `require_backup`:

```665:671:tools/fm1_install.py
def plan_slots(dev, image, product, require_backup=False):
    """-> SlotMove for at-risk data, or for an explicitly requested full prewrite backup."""
    lay = slot_probe(dev.link)
    if not lay:
        if require_backup:
            raise xs.SlotError("the running firmware did not provide its slot layout")
        return None
```

Caller `run()` only backs up when `move` is set; a silent `None` proceeds to write (`tools/fm1_install.py` ~892–917). Probe budget is short (`SLOT_PROBE` timeout 1.0s, tries 2).

Web twin: `installKeepingSlots` maps layout failure to `null` and installs anyway:

```584:602:web/x0x_breaks.js
      const layout = await readSlotLayout(dev.link, 1000, 2).catch(() => null);
      if (layout && layout.slots.some((sl) => sl.nz)) {
        ...
      }
    } finally { dev.link.close(); }
  }
  const ran = await up.install(image, product, h.step);
```

## Affected scope

- `tools/fm1_install.py`: `slot_probe`, `plan_slots`, and the same-keyboard path in `run()` that calls them.
- `web/x0x_breaks.js`: `installKeepingSlots` layout probe before `up.install`.
- Host / web regressions that prove probe failure does not silently skip backup on that path.

Out of scope: firmware, controller image, package application bytes, production `main`, production Pages, device flash, BF-001–BF-003, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`.

Escape hatches to keep: `--no-slots` (explicit skip); successful parse with empty / no-risk slots (no backup needed).

## Proposed fix

On the same-keyboard slot-preserving install path only:

1. If SMP_INFO / slot layout probe fails (timeout, SlotError, parse), **fail closed before any write** — same class of error `--backup` already raises when layout is missing.
2. Do not treat probe `None` as “empty / no backup needed.” Empty or no-risk slots remain valid only after a **successful** layout parse.
3. Mirror in `installKeepingSlots`: do not `.catch(() => null)` then install; surface a `slots` / `slotbackup` failure and stop before `up.install`.
4. Add a host regression (no real MIDI, no device write) that fails on today’s silent skip and passes after the repair; cover native and preferably the web harness.

No firmware build required. Development configuration if ever needed: `X0X_DEV_PADCAL=1`.

## Approvals

The full-cycle procedure (`AGENTS.md` + `docs/rules/workflow.md`), the Stage-7 successor prompt, and Project task FCDC-35 select F03 as the next defect without asking. Standing owner pre-approval covers properly tested Project merges of that selected repair. This stage does not merge production `main`, publish production Pages, or flash.

Not approved in this repair: BF-001 through BF-003, withheld asset cache, unmerged data-safety physical-endpoint lanes, HTML initial-loader resume paths, lengthening `SLOT_PROBE` (optional hardening only).

## Missing decisions

None block the fail-closed probe on the same-keyboard path. Optional longer probe retries are not required for the minimal fix. BF-001–003 remain named-only in deferred lists with no in-tree definition — out of this repair and not selectable with enough detail until defined.

## Defects considered and not selected

### Already fixed in `4ee8f377`

| Item | Disposition |
| --- | --- |
| F02 fail-closed loader attach | Present (`wait_for` / `waitFor` newly-appeared ports). Closed on this SHA. |

### Younger deferred (`evidence/f02-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| BF-001 | Named only; no definition in repo. Defer. |
| BF-002 | Named only; no definition in repo. Defer. |
| BF-003 | Named only; no definition in repo. Defer. |
| Withheld asset cache | Excluded / not an open tip source defect (prior audits). |
| Unmerged data-safety identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` | Left as initial loader resume by F02 design. |

### Recorded failing checks

No open **FAIL** on this validated SHA outside GitHub Actions (ignored). Stage-3 F02 regressions ALL_PASS; Stage-4 Elite/Pro DEV CHECKED; Stage-7 published live. Optional Pillow host gaps are not-run, not FAIL.

## Stage 2 limit

Implement only F03, with the fail-closed same-keyboard probe above, on a new stage-2 branch from the pushed tip of `cursor/f03-stage1-fb90`. Do not take BF-001–003 or other deferred items in that same change.
